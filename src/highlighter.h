#pragma once

#include <QObject>
#include <QPointer>
#include <QQuickTextDocument>
#include <QtQmlIntegration>

namespace KSyntaxHighlighting {
class SyntaxHighlighter;
}

// Loads a text file (capped in size) and syntax-highlights it with
// KSyntaxHighlighting, the same engine Kate uses.
class Highlighter : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QQuickTextDocument *document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(QString filePath READ filePath WRITE setFilePath NOTIFY filePathChanged)
    Q_PROPERTY(QString text READ text NOTIFY textChanged)
    Q_PROPERTY(bool truncated READ truncated NOTIFY textChanged)

public:
    explicit Highlighter(QObject *parent = nullptr);

    QQuickTextDocument *document() const { return m_document; }
    void setDocument(QQuickTextDocument *document);

    QString filePath() const { return m_filePath; }
    void setFilePath(const QString &path);

    QString text() const { return m_text; }
    bool truncated() const { return m_truncated; }

Q_SIGNALS:
    void documentChanged();
    void filePathChanged();
    void textChanged();

private:
    void setup();

    QPointer<QQuickTextDocument> m_document;
    KSyntaxHighlighting::SyntaxHighlighter *m_highlighter = nullptr;
    QString m_filePath;
    QString m_text;
    bool m_truncated = false;
};
