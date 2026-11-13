#include "previewer.h"

#include <QCollator>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QFutureWatcher>
#include <QTimer>
#include <QtConcurrentRun>
#include <QIcon>
#include <QLoggingCategory>
#include <QLocale>
#include <QMimeDatabase>
#include <QQmlComponent>
#include <QQmlProperty>
#include <QQuickWindow>
#include <QRegion>
#include <QUrl>

#include <KApplicationTrader>
#include <KService>
#include <KWindowEffects>
#include <LayerShellQt/Window>

#include <malloc.h>

#include <algorithm>

#include "archivemodel.h"
#include "iconprovider.h"

Q_LOGGING_CATEGORY(lcKlook, "klook", QtWarningMsg)

namespace {

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

// "13 November 2026 at 23:40", the way Quick Look's info card reads.
QString formatModified(const QDateTime &when)
{
    const QLocale locale;
    return QObject::tr("%1 at %2").arg(locale.toString(when.date(), QStringLiteral("d MMMM yyyy")),
                                       locale.toString(when.time(), QLocale::ShortFormat));
}

struct FolderSummary {
    qint64 items = 0; // direct children
    qint64 bytes = 0; // everything below, recursively
    bool partial = false; // stopped early: huge tree
};

// Runs on a worker thread. Capped in time so a home directory or a network
// mount can't keep a worker busy; the card then says "more than".
FolderSummary summarise(const QString &path)
{
    FolderSummary summary;
    const auto filters = QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System;
    summary.items = QDir(path).entryList(filters).size();

    QElapsedTimer timer;
    timer.start();
    QDirIterator it(path, QDir::Files | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        summary.bytes += it.nextFileInfo().size();
        if (timer.elapsed() > 1500) {
            summary.partial = true;
            break;
        }
    }
    return summary;
}

QString folderDetails(const FolderSummary &summary)
{
    const QString items = summary.items == 1 ? QObject::tr("1 item") : QObject::tr("%1 items").arg(summary.items);
    const QString size = QLocale().formattedDataSize(summary.bytes);
    return summary.partial ? QObject::tr("%1 · more than %2").arg(items, size) : QObject::tr("%1 · %2").arg(items, size);
}

QVariantMap propertiesFor(const QFileInfo &info)
{
    const QMimeType mime = QMimeDatabase().mimeTypeForFile(info);
    const QString sizeText = info.isDir() ? QString() : QLocale().formattedDataSize(info.size());
    const QString details = info.isDir() ? QObject::tr("Calculating size…") : sizeText;

    // The app "Open" launches (xdg's default for this type), named like Quick Look's
    // "Open with Preview" button.
    const KService::Ptr app = KApplicationTrader::preferredService(mime.name());

    return {
        {QStringLiteral("filePath"), info.absoluteFilePath()},
        {QStringLiteral("fileUrl"), QUrl::fromLocalFile(info.absoluteFilePath())},
        {QStringLiteral("fileName"), info.fileName()},
        {QStringLiteral("kind"), kindFor(mime)},
        {QStringLiteral("mimeName"), mime.name()},
        // Specific icons (e.g. audio-x-vorbis+ogg) are often missing from themes.
        {QStringLiteral("iconName"), QIcon::hasThemeIcon(mime.iconName()) ? mime.iconName() : mime.genericIconName()},
        {QStringLiteral("kindText"), mime.comment()},
        {QStringLiteral("sizeText"), sizeText},
        {QStringLiteral("details"), details},
        {QStringLiteral("modifiedText"), formatModified(info.lastModified())},
        {QStringLiteral("appName"), app ? app->name() : QString()},
        {QStringLiteral("appIcon"), app ? app->icon() : QString()},
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

void Previewer::toggle(const QString &path, bool activate)
{
    // Canonical so "dir/../file" or a symlink still matches the open preview.
    const QFileInfo info(path);
    const QString absolute = info.canonicalFilePath().isEmpty() ? info.absoluteFilePath() : info.canonicalFilePath();
    qCDebug(lcKlook) << "toggle" << absolute << "showing:" << m_currentPath;
    if (m_window && m_currentPath == absolute) {
        close();
        return;
    }
    show(absolute, activate);
}

void Previewer::Show(const QString &path, bool activate)
{
    show(path, activate);
}

void Previewer::close()
{
    // dismiss() animates out first when the open/close animation is on.
    if (m_window)
        QMetaObject::invokeMethod(m_window, "dismiss");
}

void Previewer::updateBlur()
{
    QQuickWindow *window = m_window;
    if (!window)
        return;
    const bool active = window->property("blurActive").toBool();
    const int radius = window->property("cornerRadius").toInt();
    KWindowEffects::enableBlurBehind(window, active, active ? roundedRegion(window->size(), radius) : QRegion());
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
    // Keep the window's mode: stepping through a passive (file-manager driven)
    // preview must not suddenly take focus.
    show(siblings.at(target), !m_windowPassive);
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

void Previewer::show(const QString &path, bool activate)
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
    if (info.isDir())
        summariseFolder(m_currentPath);

    // A layer-shell (passive) window can't become a normal one or vice versa,
    // so a change of mode means a fresh window.
    if (m_window && m_windowPassive == activate) {
        m_window->disconnect(this);
        m_window->close();
        m_window->deleteLater();
        m_window = nullptr;
    }

    // Already showing something: swap the file into the same window, so arrow
    // keys and repeated Dolphin requests don't flash a new window each time.
    if (m_window) {
        for (auto it = properties.cbegin(); it != properties.cend(); ++it)
            m_window->setProperty(it.key().toUtf8().constData(), it.value());
        QMetaObject::invokeMethod(m_window, "reload");
        if (activate)
            m_window->requestActivate();
        qCDebug(lcKlook) << "swapped" << m_currentPath;
        return;
    }

    properties.insert(QStringLiteral("passive"), !activate);
    QObject *object = m_component->createWithInitialProperties(properties);
    auto *window = qobject_cast<QQuickWindow *>(object);
    if (!window) {
        qCritical().noquote() << "klook: failed to create window:" << m_component->errorString();
        delete object;
        return;
    }
    m_window = window;
    m_windowPassive = !activate;

    if (!activate && QGuiApplication::platformName() == u"wayland") {
        // On Wayland the only reliable way to show a window without taking
        // keyboard focus is a layer-shell surface with no keyboard interactivity
        // (KWin ignores Qt::WindowDoesNotAcceptFocus for normal toplevels).
        // No anchors = centred on the screen the user is working on.
        if (auto *layer = LayerShellQt::Window::get(window)) {
            layer->setLayer(LayerShellQt::Window::LayerTop);
            layer->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityNone);
            layer->setAnchors(LayerShellQt::Window::AnchorNone);
            layer->setScope(QStringLiteral("klook-preview"));
            layer->setWantsToBeOnActiveScreen(true);
        }
    }

    connect(window, &QWindow::widthChanged, this, &Previewer::updateBlur);
    connect(window, &QWindow::heightChanged, this, &Previewer::updateBlur);
    QQmlProperty(window, QStringLiteral("cornerRadius")).connectNotifySignal(this, SLOT(updateBlur()));
    QQmlProperty(window, QStringLiteral("blurActive")).connectNotifySignal(this, SLOT(updateBlur()));
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
            Q_EMIT ShowingChanged(false);
        }
        releaseMemory();
    });

    window->show();
    updateBlur();
    QMetaObject::invokeMethod(window, "present");

    // Test hook: KLOOK_SNAPSHOT=<png> renders the window to a file after
    // KLOOK_SNAPSHOT_DELAY ms (default 1500), so tests can check how a preview
    // looks without screenshots of the user's desktop (works offscreen too).
    if (const QString snapshot = qEnvironmentVariable("KLOOK_SNAPSHOT"); !snapshot.isEmpty()) {
        const int delay = qEnvironmentVariableIntegerValue("KLOOK_SNAPSHOT_DELAY").value_or(1500);
        QTimer::singleShot(delay, window, [window, snapshot] {
            const bool saved = window->grabWindow().save(snapshot);
            qCDebug(lcKlook) << "snapshot" << snapshot << saved;
        });
    }
    // Without a request (and without an activation token) KWin leaves focus
    // where it is, which is what a file manager driving us wants.
    if (activate)
        window->requestActivate();
    Q_EMIT ShowingChanged(true);
    qCDebug(lcKlook) << "shown" << m_currentPath;
}

void Previewer::summariseFolder(const QString &path)
{
    auto *watcher = new QFutureWatcher<FolderSummary>(this);
    connect(watcher, &QFutureWatcher<FolderSummary>::finished, this, [this, watcher, path] {
        watcher->deleteLater();
        // The user may have moved on to another file meanwhile.
        if (m_window && m_currentPath == path)
            m_window->setProperty("details", folderDetails(watcher->result()));
    });
    watcher->setFuture(QtConcurrent::run(summarise, path));
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
