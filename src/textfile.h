#pragma once

#include <QObject>
#include <QtQmlIntegration>

// Reads a text file for preview, capped at MaxBytes so huge logs stay instant.
class TextFile : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString filePath READ filePath WRITE setFilePath NOTIFY filePathChanged)
    Q_PROPERTY(QString text READ text NOTIFY textChanged)
    Q_PROPERTY(bool truncated READ truncated NOTIFY textChanged)
    Q_PROPERTY(int lineCount READ lineCount NOTIFY textChanged)

public:
    static constexpr qint64 MaxBytes = 2 * 1024 * 1024;

    explicit TextFile(QObject *parent = nullptr);

    QString filePath() const { return m_filePath; }
    void setFilePath(const QString &path);

    QString text() const { return m_text; }
    bool truncated() const { return m_truncated; }
    int lineCount() const { return m_lineCount; }

Q_SIGNALS:
    void filePathChanged();
    void textChanged();

private:
    QString m_filePath;
    QString m_text;
    bool m_truncated = false;
    int m_lineCount = 0;
};
