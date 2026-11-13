#pragma once

#include <QDateTime>
#include <QObject>
#include <QPointer>
#include <QQmlEngine>
#include <QQuickWindow>

class QQmlComponent;


// Owns the long-lived QML engine and creates one preview window per request.
// Keeping the engine (and compiled QML) warm is what makes repeat previews instant.
class Previewer : public QObject
{
    Q_OBJECT
    // Exported at /Preview so file managers (our Dolphin patch) can drive the
    // preview without stealing keyboard focus from their own window.
    Q_CLASSINFO("D-Bus Interface", "org.kde.klook.Preview")

public:
    explicit Previewer(QObject *parent = nullptr);

    // Shows a preview of path. Previewing the file that is already showing
    // closes it instead, matching Quick Look's Space-to-toggle behaviour.
    void toggle(const QString &path, bool activate = true);
    void close();
    // Previous (-1) / next (+1) file in the open preview's folder.
    void step(int delta) { navigate(delta); }

    bool isShowing() const { return !m_window.isNull(); }

public Q_SLOTS:
    // D-Bus API. activate=false keeps focus with the caller, so its arrow keys
    // keep moving its selection while the preview follows along.
    Q_SCRIPTABLE void Show(const QString &path, bool activate);
    Q_SCRIPTABLE void Close() { close(); }
    Q_SCRIPTABLE bool IsShowing() const { return isShowing(); }

Q_SIGNALS:
    void closed();
    Q_SCRIPTABLE void ShowingChanged(bool showing);

private Q_SLOTS:
    // Previous (-1) / next (+1) file in the current file's folder.
    void navigate(int delta);

private:
    void show(const QString &path, bool activate = true);
    const QStringList &siblingsOf(const QString &path);
    void releaseMemory();

    QQmlEngine m_engine;
    QQmlComponent *m_component = nullptr;
    QPointer<QQuickWindow> m_window;
    bool m_windowPassive = false;
    QString m_currentPath;

    // Cached, sorted listing of the current folder for arrow-key navigation.
    QString m_siblingDir;
    QDateTime m_siblingStamp;
    bool m_siblingsWithHidden = false;
    QStringList m_siblings;
};
