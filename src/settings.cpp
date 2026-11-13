#include "settings.h"

#include <KConfigGroup>

namespace {
const QString Default = QStringLiteral("default");
const QString MacOS = QStringLiteral("macos");
const QString Custom = QStringLiteral("custom");
}

Settings::Settings(QObject *parent)
    : QObject(parent)
    , m_config(KSharedConfig::openConfig(QStringLiteral("klookrc")))
    , m_custom(presetValues(Default))
{
    load();
    m_watcher = KConfigWatcher::create(m_config);
    connect(m_watcher.data(), &KConfigWatcher::configChanged, this, [this] {
        m_config->reparseConfiguration();
        load();
        Q_EMIT changed();
    });
}

Settings::Values Settings::presetValues(const QString &preset)
{
    if (preset == MacOS)
        return {.openAnimation = true, .crossfade = true, .autoHideControls = true,
                .frosted = true, .polishedText = true, .cornerRadius = 18};
    return {.openAnimation = false, .crossfade = false, .autoHideControls = false,
            .frosted = false, .polishedText = false, .cornerRadius = 14};
}

Settings::Values Settings::values() const
{
    return m_preset == Custom ? m_custom : presetValues(m_preset);
}

void Settings::setPreset(const QString &preset)
{
    const QString valid = (preset == MacOS || preset == Custom) ? preset : Default;
    if (valid == m_preset)
        return;
    // Entering Custom starts from whatever was showing.
    if (valid == Custom)
        m_custom = values();
    m_preset = valid;
    save();
    Q_EMIT changed();
}

template<typename T>
void Settings::edit(T Values::*field, T value)
{
    Values next = values();
    if (next.*field == value)
        return;
    next.*field = value;
    m_custom = next;
    m_preset = Custom;
    save();
    Q_EMIT changed();
}

void Settings::setOpenAnimation(bool on) { edit(&Values::openAnimation, on); }
void Settings::setCrossfade(bool on) { edit(&Values::crossfade, on); }
void Settings::setAutoHideControls(bool on) { edit(&Values::autoHideControls, on); }
void Settings::setFrosted(bool on) { edit(&Values::frosted, on); }
void Settings::setPolishedText(bool on) { edit(&Values::polishedText, on); }
void Settings::setCornerRadius(int radius) { edit(&Values::cornerRadius, qBound(0, radius, 32)); }

void Settings::load()
{
    const KConfigGroup general = m_config->group(QStringLiteral("General"));
    const QString preset = general.readEntry("Preset", Default);
    m_preset = (preset == MacOS || preset == Custom) ? preset : Default;

    const KConfigGroup custom = m_config->group(QStringLiteral("Custom"));
    const Values base = presetValues(Default);
    m_custom = {
        .openAnimation = custom.readEntry("OpenAnimation", base.openAnimation),
        .crossfade = custom.readEntry("Crossfade", base.crossfade),
        .autoHideControls = custom.readEntry("AutoHideControls", base.autoHideControls),
        .frosted = custom.readEntry("Frosted", base.frosted),
        .polishedText = custom.readEntry("PolishedText", base.polishedText),
        .cornerRadius = qBound(0, custom.readEntry("CornerRadius", base.cornerRadius), 32),
    };
}

void Settings::save()
{
    // Notify: the other side (klook or the settings page) picks it up live.
    constexpr auto flags = KConfig::Notify;
    KConfigGroup general = m_config->group(QStringLiteral("General"));
    general.writeEntry("Preset", m_preset, flags);
    KConfigGroup custom = m_config->group(QStringLiteral("Custom"));
    custom.writeEntry("OpenAnimation", m_custom.openAnimation, flags);
    custom.writeEntry("Crossfade", m_custom.crossfade, flags);
    custom.writeEntry("AutoHideControls", m_custom.autoHideControls, flags);
    custom.writeEntry("Frosted", m_custom.frosted, flags);
    custom.writeEntry("PolishedText", m_custom.polishedText, flags);
    custom.writeEntry("CornerRadius", m_custom.cornerRadius, flags);
    m_config->sync();
}
