#include "previewer.h"

#include <QFileInfo>
#include <QIcon>
#include <QLoggingCategory>
#include <QLocale>
#include <QMimeDatabase>
#include <QQmlComponent>
#include <QQuickWindow>
#include <QRegion>
#include <QUrl>

#include <KWindowEffects>

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
    const QString absolute = QFileInfo(path).absoluteFilePath();
    qCDebug(lcKlook) << "toggle" << absolute << "showing:" << m_currentPath << "window:" << m_window.data();
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

void Previewer::show(const QString &path)
{
    const QFileInfo info(path);
    if (!info.exists()) {
        qWarning("klook: no such file: %s", qPrintable(path));
        return;
    }
    if (m_component->isError())
        return;

    // One window per preview keeps every view's state fresh; the engine and
    // compiled QML are reused, so this stays fast.
    if (m_window) {
        m_window->disconnect(this);
        m_window->close();
        m_window->deleteLater();
    }

    QObject *object = m_component->createWithInitialProperties(propertiesFor(info));
    auto *window = qobject_cast<QQuickWindow *>(object);
    if (!window) {
        qCritical().noquote() << "klook: failed to create window:" << m_component->errorString();
        delete object;
        return;
    }
    m_window = window;
    m_currentPath = info.absoluteFilePath();

    const auto applyBlur = [window] {
        KWindowEffects::enableBlurBehind(window, true, roundedRegion(window->size(), CornerRadius));
    };
    connect(window, &QWindow::widthChanged, window, applyBlur);
    connect(window, &QWindow::heightChanged, window, applyBlur);

    // QML calls close(); tear the window down once it is hidden.
    connect(window, &QWindow::visibleChanged, this, [this, window](bool visible) {
        if (visible)
            return;
        qCDebug(lcKlook) << "hidden" << window << "current:" << (m_window == window);
        window->deleteLater();
        if (m_window == window) {
            m_currentPath.clear();
            Q_EMIT closed();
        }
    });

    window->show();
    applyBlur();
    window->requestActivate();
    qCDebug(lcKlook) << "shown" << m_currentPath << "token:" << qgetenv("XDG_ACTIVATION_TOKEN");
}
