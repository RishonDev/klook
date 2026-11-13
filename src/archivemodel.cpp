#include "archivemodel.h"

#include <QIcon>
#include <QLocale>
#include <QMimeDatabase>

#include <K7Zip>
#include <KArchiveDirectory>
#include <KArchiveFile>
#include <KTar>
#include <KZip>

#include <algorithm>
#include <memory>

namespace {

std::unique_ptr<KArchive> openArchive(const QString &path)
{
    const QMimeType mime = QMimeDatabase().mimeTypeForFile(path);
    if (mime.inherits(QStringLiteral("application/zip")))
        return std::make_unique<KZip>(path);
    if (mime.inherits(QStringLiteral("application/x-7z-compressed")))
        return std::make_unique<K7Zip>(path);
    // KTar picks the decompressor (gz, bz2, xz, zstd, …) from the MIME type itself.
    return std::make_unique<KTar>(path);
}

QString iconForName(const QString &name, bool isDir)
{
    if (isDir)
        return QStringLiteral("folder");
    static QMimeDatabase db;
    const QMimeType mime = db.mimeTypeForFile(name, QMimeDatabase::MatchExtension);
    return QIcon::hasThemeIcon(mime.iconName()) ? mime.iconName() : mime.genericIconName();
}

} // namespace

ArchiveModel::ArchiveModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

bool ArchiveModel::supports(const QString &mimeName)
{
    const QMimeType mime = QMimeDatabase().mimeTypeForName(mimeName);
    // Office documents are zips too, but listing their XML parts isn't a useful preview.
    if (mimeName.startsWith(u"application/vnd.oasis.opendocument")
        || mimeName.startsWith(u"application/vnd.openxmlformats"))
        return false;

    static const QStringList tarTypes = {
        QStringLiteral("application/x-tar"),
        QStringLiteral("application/x-compressed-tar"),
        QStringLiteral("application/x-bzip-compressed-tar"),
        QStringLiteral("application/x-bzip2-compressed-tar"),
        QStringLiteral("application/x-xz-compressed-tar"),
        QStringLiteral("application/x-zstd-compressed-tar"),
        QStringLiteral("application/x-lzma-compressed-tar"),
    };
    return mime.inherits(QStringLiteral("application/zip"))
        || mime.inherits(QStringLiteral("application/x-7z-compressed"))
        || std::any_of(tarTypes.begin(), tarTypes.end(), [&](const QString &t) { return mime.inherits(t); });
}

int ArchiveModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_entries.size());
}

QVariant ArchiveModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid))
        return {};
    const Entry &e = m_entries.at(index.row());
    switch (role) {
    case NameRole: return e.name;
    case DepthRole: return e.depth;
    case IsDirRole: return e.isDir;
    case SizeRole: return e.isDir ? QString() : QLocale().formattedDataSize(e.size);
    case IconRole: return e.icon;
    }
    return {};
}

QHash<int, QByteArray> ArchiveModel::roleNames() const
{
    return {
        {NameRole, "name"},
        {DepthRole, "depth"},
        {IsDirRole, "isDir"},
        {SizeRole, "size"},
        {IconRole, "iconName"},
    };
}

void ArchiveModel::setFilePath(const QString &path)
{
    if (m_filePath == path)
        return;
    m_filePath = path;
    Q_EMIT filePathChanged();
    load();
}

QString ArchiveModel::totalSize() const
{
    return QLocale().formattedDataSize(m_totalSize);
}

void ArchiveModel::load()
{
    beginResetModel();
    m_entries.clear();
    m_error.clear();
    m_fileCount = m_dirCount = 0;
    m_totalSize = 0;
    m_truncated = false;

    auto archive = openArchive(m_filePath);
    if (archive->open(QIODevice::ReadOnly))
        walk(archive->directory(), 0);
    else
        m_error = archive->errorString();

    endResetModel();
    Q_EMIT loaded();
}

void ArchiveModel::walk(const KArchiveDirectory *dir, int depth)
{
    QStringList names = dir->entries();
    // Folders first, then files, each alphabetically — like Dolphin.
    std::sort(names.begin(), names.end(), [dir](const QString &a, const QString &b) {
        const bool da = dir->entry(a)->isDirectory();
        const bool db = dir->entry(b)->isDirectory();
        if (da != db)
            return da;
        return QString::localeAwareCompare(a, b) < 0;
    });

    for (const QString &name : std::as_const(names)) {
        const KArchiveEntry *entry = dir->entry(name);
        const bool isDir = entry->isDirectory();
        const qint64 size = isDir ? 0 : static_cast<const KArchiveFile *>(entry)->size();

        if (isDir)
            ++m_dirCount;
        else {
            ++m_fileCount;
            m_totalSize += size;
        }

        // Keep counting past the cap so the summary stays accurate.
        if (m_entries.size() < MaxEntries)
            m_entries.append({name, depth, isDir, size, iconForName(name, isDir)});
        else
            m_truncated = true;

        if (isDir)
            walk(static_cast<const KArchiveDirectory *>(entry), depth + 1);
    }
}
