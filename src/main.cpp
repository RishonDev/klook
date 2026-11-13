#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDir>
#include <QGuiApplication>

#include <unistd.h>

#include <KDBusService>

#include "previewer.h"

namespace {

void addOptions(QCommandLineParser &parser)
{
    parser.addOption({QStringLiteral("daemon"), QStringLiteral("Start in the background without showing a preview.")});
    parser.addOption({QStringLiteral("quit"), QStringLiteral("Stop the running klook instance.")});
    parser.addOption({QStringLiteral("no-activate"), QStringLiteral("Show the preview without taking keyboard focus.")});
    parser.addOption({QStringLiteral("next"), QStringLiteral("Show the next file in the folder of the open preview.")});
    parser.addOption({QStringLiteral("previous"), QStringLiteral("Show the previous file in the folder of the open preview.")});
    parser.addPositionalArgument(QStringLiteral("file"), QStringLiteral("File to preview"));
}

// First positional argument, made absolute against the caller's directory.
// Used for our own argv and for argv forwarded by a second `klook` over D-Bus.
QString fileArgument(const QCommandLineParser &parser, const QString &workingDirectory)
{
    const QStringList positional = parser.positionalArguments();
    return positional.isEmpty() ? QString() : QDir(workingDirectory).absoluteFilePath(positional.constFirst());
}

const QString ServiceName = QStringLiteral("org.kde.klook");

// Fast path for the common case: klook is already running. A bare
// QCoreApplication skips the Wayland connection and Plasma theme loading that
// a QGuiApplication pays for, so handing the file over costs only a D-Bus call.
// Returns false when no instance is running and we must become it.
bool forwardToRunningInstance(int argc, char *argv[], int *exitCode)
{
    QCoreApplication app(argc, argv);
    const QStringList arguments = app.arguments();
    if (arguments.contains(u"--help") || arguments.contains(u"-h") || arguments.contains(u"--version")
        || arguments.contains(u"-v"))
        return false;

    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected() || !bus.interface()->isServiceRegistered(ServiceName))
        return false;

    // Same call KDBusService::Unique makes, including the Wayland activation
    // token so the running instance is allowed to focus its window.
    QVariantMap platformData;
    const QByteArray token = qgetenv("XDG_ACTIVATION_TOKEN");
    if (!token.isEmpty())
        platformData.insert(QStringLiteral("activation-token"), QString::fromUtf8(token));

    QDBusMessage call = QDBusMessage::createMethodCall(ServiceName, QStringLiteral("/org/kde/klook"),
                                                       QStringLiteral("org.kde.KDBusService"),
                                                       QStringLiteral("CommandLine"));
    call << arguments << QDir::currentPath() << platformData;
    const QDBusMessage reply = bus.call(call);
    if (reply.type() != QDBusMessage::ReplyMessage)
        return false; // instance vanished or misbehaved; start a fresh one

    *exitCode = reply.arguments().value(0).toInt();
    return true;
}

} // namespace

int main(int argc, char *argv[])
{
    // The fast path leaves QtDBus global state bound to its short-lived
    // QCoreApplication, and a later QGuiApplication can't reuse it (the bus
    // name registers but calls are never dispatched). So when no instance is
    // running, re-exec ourselves as a clean server process.
    constexpr const char *ServerEnv = "KLOOK_SERVER";
    if (qEnvironmentVariableIsEmpty(ServerEnv)) {
        int exitCode = 0;
        if (forwardToRunningInstance(argc, argv, &exitCode))
            return exitCode;
        qputenv(ServerEnv, "1");
        execv("/proc/self/exe", argv);
        qWarning("klook: re-exec failed, continuing in-process");
    }
    qunsetenv(ServerEnv); // don't leak into apps we launch via "Open"

    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("klook"));
    app.setApplicationVersion(QStringLiteral("0.4.0"));
    app.setOrganizationDomain(QStringLiteral("kde.org")); // D-Bus name: org.kde.klook
    app.setDesktopFileName(QStringLiteral("klook"));
    // Stay resident after the preview closes so the next one opens instantly.
    app.setQuitOnLastWindowClosed(false);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Quick Look style file previewer for KDE"));
    parser.addHelpOption();
    parser.addVersionOption();
    addOptions(parser);
    parser.process(app);

    const bool quit = parser.isSet(QStringLiteral("quit"));
    const bool step = parser.isSet(QStringLiteral("next")) || parser.isSet(QStringLiteral("previous"));
    const QString file = fileArgument(parser, QDir::currentPath());
    if (file.isEmpty() && !quit && !step && !parser.isSet(QStringLiteral("daemon")))
        parser.showHelp(1);

    // If klook is already running, this forwards our argv to it and exits 0,
    // so a second `klook` never loads any QML.
    KDBusService service(KDBusService::Unique);

    // Reaching here means we are the only instance: --quit has nothing to stop
    // and --next/--previous have no preview to step through.
    if (quit || step)
        return 0;

    Previewer previewer;
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/Preview"), &previewer,
                                                 QDBusConnection::ExportScriptableSlots
                                                     | QDBusConnection::ExportScriptableSignals);

    QObject::connect(&service, &KDBusService::activateRequested, &previewer,
                     [&previewer](const QStringList &arguments, const QString &workingDirectory) {
                         QCommandLineParser forwarded;
                         addOptions(forwarded);
                         forwarded.parse(arguments);
                         if (forwarded.isSet(QStringLiteral("quit"))) {
                             QCoreApplication::quit();
                             return;
                         }
                         if (forwarded.isSet(QStringLiteral("next")) || forwarded.isSet(QStringLiteral("previous"))) {
                             previewer.step(forwarded.isSet(QStringLiteral("next")) ? 1 : -1);
                             return;
                         }
                         const QString path = fileArgument(forwarded, workingDirectory);
                         if (!path.isEmpty())
                             previewer.toggle(path, !forwarded.isSet(QStringLiteral("no-activate")));
                     });

    if (!file.isEmpty())
        previewer.toggle(file, !parser.isSet(QStringLiteral("no-activate")));

    return app.exec();
}
