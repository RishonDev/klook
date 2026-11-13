#pragma once

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

public:
    explicit Previewer(QObject *parent = nullptr);

    // Shows a preview of path. Previewing the file that is already showing
    // closes it instead, matching Quick Look's Space-to-toggle behaviour.
    void toggle(const QString &path);
    void close();

    bool isShowing() const { return !m_window.isNull(); }

Q_SIGNALS:
    void closed();

private:
    void show(const QString &path);

    QQmlEngine m_engine;
    QQmlComponent *m_component = nullptr;
    QPointer<QQuickWindow> m_window;
    QString m_currentPath;
};
