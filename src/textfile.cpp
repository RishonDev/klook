#include "textfile.h"

#include <QFile>

TextFile::TextFile(QObject *parent)
    : QObject(parent)
{
}

void TextFile::setFilePath(const QString &path)
{
    if (m_filePath == path)
        return;
    m_filePath = path;
    Q_EMIT filePathChanged();

    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        m_text = QString::fromUtf8(file.read(MaxBytes));
        m_truncated = file.size() > MaxBytes;
    } else {
        m_text = tr("Could not open file: %1").arg(file.errorString());
        m_truncated = false;
    }
    // A trailing newline ends the last line rather than starting a new one.
    m_lineCount = m_text.isEmpty() ? 0 : int(m_text.count(u'\n')) + (m_text.endsWith(u'\n') ? 0 : 1);
    Q_EMIT textChanged();
}
