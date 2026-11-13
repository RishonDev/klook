#include <QCommandLineParser>
#include <QFileInfo>
#include <QGuiApplication>
#include <QIcon>
#include <QLocale>
#include <QMimeDatabase>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QRegion>
#include <QUrl>

#include <KWindowEffects>

#include "iconprovider.h"

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

constexpr int CornerRadius = 14;

} // namespace

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("klook"));
    app.setApplicationVersion(QStringLiteral("0.1.0"));
    app.setDesktopFileName(QStringLiteral("klook"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Quick Look style file previewer for KDE"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QStringLiteral("file"), QStringLiteral("File to preview"));
    parser.process(app);

    if (parser.positionalArguments().isEmpty())
        parser.showHelp(1);

    const QFileInfo info(parser.positionalArguments().constFirst());
    if (!info.exists()) {
        qCritical("klook: no such file: %s", qPrintable(info.filePath()));
        return 1;
    }

    const QMimeType mime = QMimeDatabase().mimeTypeForFile(info);
    const QString details = info.isDir()
        ? mime.comment()
        : QStringLiteral("%1 · %2").arg(mime.comment(), QLocale().formattedDataSize(info.size()));

    QQmlApplicationEngine engine;
    engine.addImageProvider(QStringLiteral("icon"), new IconProvider);
    engine.setInitialProperties({
        {QStringLiteral("filePath"), info.absoluteFilePath()},
        {QStringLiteral("fileUrl"), QUrl::fromLocalFile(info.absoluteFilePath())},
        {QStringLiteral("fileName"), info.fileName()},
        {QStringLiteral("kind"), kindFor(mime)},
        {QStringLiteral("mimeName"), mime.name()},
        // Specific icons (e.g. audio-x-vorbis+ogg) are often missing from themes.
        {QStringLiteral("iconName"), QIcon::hasThemeIcon(mime.iconName()) ? mime.iconName() : mime.genericIconName()},
        {QStringLiteral("details"), details},
        {QStringLiteral("cornerRadius"), CornerRadius},
    });

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.loadFromModule("klook", "Main");
    if (engine.rootObjects().isEmpty())
        return 1;

    if (auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst())) {
        const auto applyBlur = [window] {
            KWindowEffects::enableBlurBehind(window, true, roundedRegion(window->size(), CornerRadius));
        };
        QObject::connect(window, &QWindow::widthChanged, window, applyBlur);
        QObject::connect(window, &QWindow::heightChanged, window, applyBlur);
        applyBlur();
    }

    return app.exec();
}
