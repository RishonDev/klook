#include "markdownpolish.h"

#include <QFontDatabase>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTimer>

MarkdownPolish::MarkdownPolish(QObject *parent)
    : QObject(parent)
{
}

void MarkdownPolish::setDocument(QQuickTextDocument *document)
{
    if (m_document == document)
        return;
    if (m_document)
        m_document->textDocument()->disconnect(this);
    m_document = document;
    // Markdown is (re)imported whenever the text changes; restyle after it.
    if (m_document)
        connect(m_document->textDocument(), &QTextDocument::contentsChanged, this, &MarkdownPolish::schedule);
    Q_EMIT documentChanged();
    schedule();
}

void MarkdownPolish::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;
    m_enabled = enabled;
    Q_EMIT enabledChanged();
    schedule();
}

void MarkdownPolish::setTextColor(const QColor &color)
{
    if (m_textColor == color)
        return;
    m_textColor = color;
    Q_EMIT textColorChanged();
    schedule();
}

void MarkdownPolish::schedule()
{
    // Our own edits fire contentsChanged too; coalesce and skip those.
    if (m_applying || m_scheduled || !m_enabled || !m_document)
        return;
    m_scheduled = true;
    QTimer::singleShot(0, this, [this] {
        m_scheduled = false;
        apply();
    });
}

void MarkdownPolish::apply()
{
    if (!m_enabled || !m_document)
        return;
    QTextDocument *doc = m_document->textDocument();
    m_applying = true;

    QColor wash = m_textColor;
    wash.setAlphaF(0.08f);
    const QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    QTextCursor cursor(doc);
    cursor.beginEditBlock();
    for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
        QTextBlockFormat format = block.blockFormat();
        const bool code = format.hasProperty(QTextFormat::BlockCodeFence)
            || format.hasProperty(QTextFormat::BlockCodeLanguage) || format.nonBreakableLines();

        if (code) {
            format.setBackground(wash);
            format.setLeftMargin(12);
            format.setRightMargin(12);
            format.setTopMargin(2);
            format.setBottomMargin(2);
        } else if (format.headingLevel() > 0) {
            format.setTopMargin(format.headingLevel() <= 2 ? 18 : 12);
            format.setBottomMargin(6);
        } else {
            format.setLineHeight(135, QTextBlockFormat::ProportionalHeight);
            format.setBottomMargin(8);
        }
        cursor.setPosition(block.position());
        cursor.setBlockFormat(format);

        // Qt Quick doesn't paint block backgrounds, so shade the code text itself.
        if (code && block.length() > 1) {
            QTextCharFormat charFormat;
            charFormat.setBackground(wash);
            cursor.setPosition(block.position());
            cursor.setPosition(block.position() + block.length() - 1, QTextCursor::KeepAnchor);
            cursor.mergeCharFormat(charFormat);
        }

        // Inline `code`: monospace runs inside ordinary paragraphs.
        if (!code) {
            for (auto it = block.begin(); !it.atEnd(); ++it) {
                const QTextFragment fragment = it.fragment();
                if (!fragment.charFormat().fontFixedPitch() && fragment.charFormat().fontFamilies() != QStringList{mono.family()})
                    continue;
                QTextCharFormat charFormat;
                charFormat.setBackground(wash);
                cursor.setPosition(fragment.position());
                cursor.setPosition(fragment.position() + fragment.length(), QTextCursor::KeepAnchor);
                cursor.mergeCharFormat(charFormat);
            }
        }
    }
    cursor.endEditBlock();
    m_applying = false;
}
