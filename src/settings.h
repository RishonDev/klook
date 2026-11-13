#pragma once

#include <KConfigWatcher>
#include <KSharedConfig>

#include <QObject>
#include <QtQmlIntegration>

// Appearance options in ~/.config/klookrc, shared by klook and its System
// Settings page. A preset ("default", "macos") fixes every option; editing any
// single option switches to "custom", starting from the preset's values.
// Writes notify, and both sides watch the file, so changes apply live.
class Settings : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString preset READ preset WRITE setPreset NOTIFY changed)
    // Zoom-and-fade when the preview opens and closes.
    Q_PROPERTY(bool openAnimation READ openAnimation WRITE setOpenAnimation NOTIFY changed)
    // Fade between files when moving through a folder.
    Q_PROPERTY(bool crossfade READ crossfade WRITE setCrossfade NOTIFY changed)
    // Images and video: hide the title bar until the mouse moves.
    Q_PROPERTY(bool autoHideControls READ autoHideControls WRITE setAutoHideControls NOTIFY changed)
    // Lighter, more translucent background with a highlight edge.
    Q_PROPERTY(bool frosted READ frosted WRITE setFrosted NOTIFY changed)
    // Markdown: shaded code blocks, airier typography, readable line length.
    Q_PROPERTY(bool polishedText READ polishedText WRITE setPolishedText NOTIFY changed)
    Q_PROPERTY(int cornerRadius READ cornerRadius WRITE setCornerRadius NOTIFY changed)

public:
    explicit Settings(QObject *parent = nullptr);

    QString preset() const { return m_preset; }
    void setPreset(const QString &preset);

    bool openAnimation() const { return values().openAnimation; }
    void setOpenAnimation(bool on);
    bool crossfade() const { return values().crossfade; }
    void setCrossfade(bool on);
    bool autoHideControls() const { return values().autoHideControls; }
    void setAutoHideControls(bool on);
    bool frosted() const { return values().frosted; }
    void setFrosted(bool on);
    bool polishedText() const { return values().polishedText; }
    void setPolishedText(bool on);
    int cornerRadius() const { return values().cornerRadius; }
    void setCornerRadius(int radius);

Q_SIGNALS:
    void changed();

private:
    struct Values {
        bool openAnimation;
        bool crossfade;
        bool autoHideControls;
        bool frosted;
        bool polishedText;
        int cornerRadius;
        bool operator==(const Values &) const = default;
    };

    static Values presetValues(const QString &preset);
    Values values() const;
    template<typename T> void edit(T Values::*field, T value);
    void load();
    void save();

    KSharedConfig::Ptr m_config;
    KConfigWatcher::Ptr m_watcher;
    QString m_preset = QStringLiteral("default");
    Values m_custom;
};
