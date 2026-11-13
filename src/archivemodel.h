#pragma once

#include <QAbstractListModel>
#include <QtQmlIntegration>

class KArchiveDirectory;

// Flattened, indented listing of an archive's contents (zip/jar, tar.*, 7z).
class ArchiveModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString filePath READ filePath WRITE setFilePath NOTIFY filePathChanged)
    Q_PROPERTY(QString error READ error NOTIFY loaded)
    Q_PROPERTY(int fileCount READ fileCount NOTIFY loaded)
    Q_PROPERTY(int dirCount READ dirCount NOTIFY loaded)
    Q_PROPERTY(QString totalSize READ totalSize NOTIFY loaded)
    Q_PROPERTY(bool truncated READ truncated NOTIFY loaded)
    Q_PROPERTY(int maxEntries READ maxEntries CONSTANT)

public:
    enum Role {
        NameRole = Qt::UserRole + 1,
        DepthRole,
        IsDirRole,
        SizeRole,
        IconRole,
    };

    static constexpr int MaxEntries = 5000;

    explicit ArchiveModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString filePath() const { return m_filePath; }
    void setFilePath(const QString &path);

    QString error() const { return m_error; }
    int fileCount() const { return m_fileCount; }
    int dirCount() const { return m_dirCount; }
    QString totalSize() const;
    bool truncated() const { return m_truncated; }
    int maxEntries() const { return MaxEntries; }

    // Whether klook can list archives of this MIME type.
    static bool supports(const QString &mimeName);

Q_SIGNALS:
    void filePathChanged();
    void loaded();

private:
    struct Entry {
        QString name;
        int depth;
        bool isDir;
        qint64 size;
        QString icon;
    };

    void load();
    void walk(const KArchiveDirectory *dir, int depth);

    QString m_filePath;
    QList<Entry> m_entries;
    QString m_error;
    int m_fileCount = 0;
    int m_dirCount = 0;
    qint64 m_totalSize = 0;
    bool m_truncated = false;
};
