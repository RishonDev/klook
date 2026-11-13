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

public:
    explicit Previewer(QObject *parent = nullptr);

    // Shows a preview of path. Previewing the file that is already showing
    // closes it instead, matching Quick Look's Space-to-toggle behaviour.
    void toggle(const QString &path);
    void close();
    // Previous (-1) / next (+1) file in the open preview's folder.
    void step(int delta) { navigate(delta); }

    bool isShowing() const { return !m_window.isNull(); }

Q_SIGNALS:
    void closed();

private Q_SLOTS:
    // Previous (-1) / next (+1) file in the current file's folder.
    void navigate(int delta);

private:
    void show(const QString &path);
    const QStringList &siblingsOf(const QString &path);
    void releaseMemory();

    QQmlEngine m_engine;
    QQmlComponent *m_component = nullptr;
    QPointer<QQuickWindow> m_window;
    QString m_currentPath;

    // Cached, sorted listing of the current folder for arrow-key navigation.
    QString m_siblingDir;
    QDateTime m_siblingStamp;
    bool m_siblingsWithHidden = false;
    QStringList m_siblings;
};
