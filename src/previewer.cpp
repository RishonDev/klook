#include "previewer.h"

#include <QCollator>
#include <QDir>
#include <QFileInfo>
#include <QTimer>
#include <QIcon>
#include <QLoggingCategory>
#include <QLocale>
#include <QMimeDatabase>
#include <QQmlComponent>
#include <QQuickWindow>
#include <QRegion>
#include <QUrl>

#include <KWindowEffects>

#include <malloc.h>

#include <algorithm>

#include "archivemodel.h"
#include "iconprovider.h"

Q_LOGGING_CATEGORY(lcKlook, "klook", QtWarningMsg)

namespace {

constexpr int CornerRadius = 14;

// Which QML view renders this file.
QString kindFor(const QMimeType &mime)
{
    const QString name = mime.name();
    if (name == u"application/pdf")
        return QStringLiteral("pdf");
    if (name.startsWith(u"image/"))
        return QStringLiteral("image");
    if (name.startsWith(u"video/") || name.startsWith(u"audio/"))
        return QStringLiteral("media");
    if (ArchiveModel::supports(name))
        return QStringLiteral("archive");
    if (mime.inherits(QStringLiteral("text/markdown")))
        return QStringLiteral("markdown");
    if (mime.inherits(QStringLiteral("text/plain")))
        return QStringLiteral("text");
    return QStringLiteral("other");
}

// Rounded-rect region so KWin's blur matches the QML card's corners.
QRegion roundedRegion(const QSize &size, int r)
{
    const QRect rect(QPoint(), size);
    QRegion region(rect.adjusted(r, 0, -r, 0));
    region += QRegion(rect.adjusted(0, r, 0, -r));
    const int d = 2 * r;
    region += QRegion(QRect(rect.left(), rect.top(), d, d), QRegion::Ellipse);
    region += QRegion(QRect(rect.right() - d + 1, rect.top(), d, d), QRegion::Ellipse);
    region += QRegion(QRect(rect.left(), rect.bottom() - d + 1, d, d), QRegion::Ellipse);
    region += QRegion(QRect(rect.right() - d + 1, rect.bottom() - d + 1, d, d), QRegion::Ellipse);
    return region;
}

QVariantMap propertiesFor(const QFileInfo &info)
{
    const QMimeType mime = QMimeDatabase().mimeTypeForFile(info);
    const QString details = info.isDir()
        ? mime.comment()
        : QStringLiteral("%1 · %2").arg(mime.comment(), QLocale().formattedDataSize(info.size()));

    return {
        {QStringLiteral("filePath"), info.absoluteFilePath()},
        {QStringLiteral("fileUrl"), QUrl::fromLocalFile(info.absoluteFilePath())},
        {QStringLiteral("fileName"), info.fileName()},
        {QStringLiteral("kind"), kindFor(mime)},
        {QStringLiteral("mimeName"), mime.name()},
        // Specific icons (e.g. audio-x-vorbis+ogg) are often missing from themes.
        {QStringLiteral("iconName"), QIcon::hasThemeIcon(mime.iconName()) ? mime.iconName() : mime.genericIconName()},
        {QStringLiteral("details"), details},
        {QStringLiteral("cornerRadius"), CornerRadius},
    };
}

} // namespace

Previewer::Previewer(QObject *parent)
    : QObject(parent)
{
    m_engine.addImageProvider(QStringLiteral("icon"), new IconProvider);
    m_component = new QQmlComponent(&m_engine, this);
    m_component->loadFromModule("klook", "Main");
    if (m_component->isError())
        qCritical().noquote() << "klook: failed to load QML:" << m_component->errorString();
}

void Previewer::toggle(const QString &path)
{
    // Canonical so "dir/../file" or a symlink still matches the open preview.
    const QFileInfo info(path);
    const QString absolute = info.canonicalFilePath().isEmpty() ? info.absoluteFilePath() : info.canonicalFilePath();
    qCDebug(lcKlook) << "toggle" << absolute << "showing:" << m_currentPath;
    if (m_window && m_currentPath == absolute) {
        close();
        return;
    }
    show(absolute);
}

void Previewer::close()
{
    if (m_window)
        m_window->close();
}

void Previewer::navigate(int delta)
{
    if (m_currentPath.isEmpty())
        return;
    const QStringList &siblings = siblingsOf(m_currentPath);
    const qsizetype index = siblings.indexOf(m_currentPath);
    const qsizetype target = index + delta;
    // Stop at the ends of the folder rather than wrapping, like Quick Look.
    if (index < 0 || target < 0 || target >= siblings.size())
        return;
    show(siblings.at(target));
}

const QStringList &Previewer::siblingsOf(const QString &path)
{
    const QFileInfo info(path);
    const QString dirPath = info.absolutePath();
    const QDateTime stamp = QFileInfo(dirPath).lastModified();
    // Hidden files only join the list when we're already looking at one.
    const bool withHidden = info.isHidden();
    if (dirPath == m_siblingDir && stamp == m_siblingStamp && withHidden == m_siblingsWithHidden)
        return m_siblings;

    QDir::Filters filters = QDir::AllEntries | QDir::NoDotAndDotDot;
    if (withHidden)
        filters |= QDir::Hidden;
    QFileInfoList entries = QDir(dirPath).entryInfoList(filters, QDir::NoSort);

    // Dolphin's default order: folders first, then natural, case-insensitive names.
    QCollator collator;
    collator.setNumericMode(true);
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    std::sort(entries.begin(), entries.end(), [&collator](const QFileInfo &a, const QFileInfo &b) {
        if (a.isDir() != b.isDir())
            return a.isDir();
        return collator.compare(a.fileName(), b.fileName()) < 0;
    });

    m_siblings.clear();
    m_siblings.reserve(entries.size());
    for (const QFileInfo &entry : std::as_const(entries))
        m_siblings.append(entry.absoluteFilePath());
    m_siblingDir = dirPath;
    m_siblingStamp = stamp;
    m_siblingsWithHidden = withHidden;
    return m_siblings;
}

void Previewer::show(const QString &path)
{
    const QFileInfo info(path);
    if (!info.exists()) {
        qWarning("klook: no such file: %s", qPrintable(path));
        return;
    }
    if (m_component->isError())
        return;

    QVariantMap properties = propertiesFor(info);
    const QStringList &siblings = siblingsOf(info.absoluteFilePath());
    const qsizetype index = siblings.indexOf(info.absoluteFilePath());
    properties.insert(QStringLiteral("position"),
                      siblings.size() > 1 && index >= 0 ? QStringLiteral("%1 / %2").arg(index + 1).arg(siblings.size())
                                                        : QString());
    m_currentPath = info.canonicalFilePath().isEmpty() ? info.absoluteFilePath() : info.canonicalFilePath();

    // Already showing something: swap the file into the same window, so arrow
    // keys and repeated Dolphin requests don't flash a new window each time.
    if (m_window) {
        for (auto it = properties.cbegin(); it != properties.cend(); ++it)
            m_window->setProperty(it.key().toUtf8().constData(), it.value());
        QMetaObject::invokeMethod(m_window, "reload");
        m_window->requestActivate();
        qCDebug(lcKlook) << "swapped" << m_currentPath;
        return;
    }

    QObject *object = m_component->createWithInitialProperties(properties);
    auto *window = qobject_cast<QQuickWindow *>(object);
    if (!window) {
        qCritical().noquote() << "klook: failed to create window:" << m_component->errorString();
        delete object;
        return;
    }
    m_window = window;

    const auto applyBlur = [window] {
        KWindowEffects::enableBlurBehind(window, true, roundedRegion(window->size(), CornerRadius));
    };
    connect(window, &QWindow::widthChanged, window, applyBlur);
    connect(window, &QWindow::heightChanged, window, applyBlur);
    connect(window, SIGNAL(navigate(int)), this, SLOT(navigate(int)));

    // QML calls close(); tear the window down once it is hidden.
    connect(window, &QWindow::visibleChanged, this, [this, window](bool visible) {
        if (visible)
            return;
        qCDebug(lcKlook) << "hidden" << window->title();
        window->deleteLater();
        if (m_window == window) {
            m_currentPath.clear();
            Q_EMIT closed();
        }
        releaseMemory();
    });

    window->show();
    applyBlur();
    window->requestActivate();
    qCDebug(lcKlook) << "shown" << m_currentPath;
}

void Previewer::releaseMemory()
{
    // Once the window is really gone, drop JS garbage and hand freed heap pages
    // back to the OS, so the idle resident instance stays small.
    QTimer::singleShot(200, this, [this] {
        if (m_window)
            return;
        m_engine.collectGarbage();
        malloc_trim(0);
        qCDebug(lcKlook) << "trimmed memory";
    });
}
