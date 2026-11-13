#include "klookkcm.h"

#include <KPluginFactory>

#include <QProcess>
#include <QStandardPaths>

K_PLUGIN_CLASS_WITH_JSON(KlookKcm, "kcm_klook.json")

KlookKcm::KlookKcm(QObject *parent, const KPluginMetaData &data)
    : KQuickConfigModule(parent, data)
    , m_settings(new Settings(this))
{
    setButtons(Default);
    connect(m_settings, &Settings::changed, this, [this] {
        setRepresentsDefaults(m_settings->preset() == u"default");
    });
    setRepresentsDefaults(m_settings->preset() == u"default");
}

void KlookKcm::tryIt()
{
    const QString sample = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("klook/try-me.md"));
    if (!sample.isEmpty())
        QProcess::startDetached(QStringLiteral("klook"), {sample});
}

void KlookKcm::defaults()
{
    m_settings->setPreset(QStringLiteral("default"));
}

#include "klookkcm.moc"
