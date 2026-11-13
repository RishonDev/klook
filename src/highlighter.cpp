#include "highlighter.h"

#include <QFile>
#include <QGuiApplication>
#include <QStyleHints>
#include <QTextDocument>

#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/SyntaxHighlighter>
#include <KSyntaxHighlighting/Theme>

namespace {

constexpr qint64 MaxBytes = 2 * 1024 * 1024; // previews, not editors

KSyntaxHighlighting::Repository &repository()
{
    static KSyntaxHighlighting::Repository repo;
    return repo;
}

} // namespace

Highlighter::Highlighter(QObject *parent)
    : QObject(parent)
{
}

void Highlighter::setDocument(QQuickTextDocument *document)
{
    if (m_document == document)
        return;
    m_document = document;
    Q_EMIT documentChanged();
    setup();
}

void Highlighter::setFilePath(const QString &path)
{
    if (m_filePath == path)
        return;
    m_filePath = path;
    Q_EMIT filePathChanged();

    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        const QByteArray data = file.read(MaxBytes);
        m_truncated = file.size() > MaxBytes;
        m_text = QString::fromUtf8(data);
    } else {
        m_truncated = false;
        m_text = tr("Could not open file: %1").arg(file.errorString());
    }
    Q_EMIT textChanged();
    setup();
}

void Highlighter::setup()
{
    if (!m_document || m_filePath.isEmpty())
        return;

    auto &repo = repository();
    if (!m_highlighter)
        m_highlighter = new KSyntaxHighlighting::SyntaxHighlighter(m_document->textDocument());

    const bool dark = QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
    m_highlighter->setTheme(repo.defaultTheme(dark ? KSyntaxHighlighting::Repository::DarkTheme
                                                   : KSyntaxHighlighting::Repository::LightTheme));
    m_highlighter->setDefinition(repo.definitionForFileName(m_filePath));
    m_highlighter->rehighlight();
}
