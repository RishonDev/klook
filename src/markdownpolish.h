#pragma once

#include <QColor>
#include <QObject>
#include <QPointer>
#include <QQuickTextDocument>
#include <QtQmlIntegration>

// Restyles a Markdown document after Qt's importer has built it, since the
// importer takes no stylesheet: shaded code blocks, highlighted inline code,
// spacing around headings and taller lines, the way Quick Look renders Markdown.
class MarkdownPolish : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QQuickTextDocument *document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    // Text colour; code backgrounds are a faint wash of it, so both themes work.
    Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor NOTIFY textColorChanged)

public:
    explicit MarkdownPolish(QObject *parent = nullptr);

    QQuickTextDocument *document() const { return m_document; }
    void setDocument(QQuickTextDocument *document);
    bool enabled() const { return m_enabled; }
    void setEnabled(bool enabled);
    QColor textColor() const { return m_textColor; }
    void setTextColor(const QColor &color);

Q_SIGNALS:
    void documentChanged();
    void enabledChanged();
    void textColorChanged();

private:
    void schedule();
    void apply();

    QPointer<QQuickTextDocument> m_document;
    bool m_enabled = false;
    bool m_applying = false;
    bool m_scheduled = false;
    QColor m_textColor = Qt::black;
};
