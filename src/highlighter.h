#pragma once

#include <QObject>
#include <QPointer>
#include <QQuickTextDocument>
#include <QtQmlIntegration>

namespace KSyntaxHighlighting {
class SyntaxHighlighter;
}

// Attaches KSyntaxHighlighting (the engine Kate uses) to a QML text document,
// picking the syntax definition from the file name.
class Highlighter : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QQuickTextDocument *document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(QString filePath READ filePath WRITE setFilePath NOTIFY filePathChanged)

public:
    explicit Highlighter(QObject *parent = nullptr);

    QQuickTextDocument *document() const { return m_document; }
    void setDocument(QQuickTextDocument *document);

    QString filePath() const { return m_filePath; }
    void setFilePath(const QString &path);

Q_SIGNALS:
    void documentChanged();
    void filePathChanged();

private:
    void setup();

    QPointer<QQuickTextDocument> m_document;
    KSyntaxHighlighting::SyntaxHighlighter *m_highlighter = nullptr;
    QString m_filePath;
};
