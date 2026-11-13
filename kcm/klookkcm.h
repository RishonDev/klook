#pragma once

#include <KQuickConfigModule>

#include "settings.h"

// Apps & Windows → Quick Look. Changes apply at once (klook watches its config),
// so there is no Apply step; "Defaults" returns to the Default preset.
class KlookKcm : public KQuickConfigModule
{
    Q_OBJECT
    Q_PROPERTY(Settings *settings READ settings CONSTANT)

public:
    KlookKcm(QObject *parent, const KPluginMetaData &data);

    Settings *settings() const { return m_settings; }

    // Opens a sample preview, to see the current style straight away.
    Q_INVOKABLE void tryIt();

    void defaults() override;

private:
    Settings *m_settings;
};
