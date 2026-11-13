// Settings: presets, the switch to Custom, and persistence across instances.
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

#include "settings.h"

class SettingsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase() { QStandardPaths::setTestModeEnabled(true); }
    void init() { QFile::remove(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + u"/klookrc"); }

    void defaultsToPlain()
    {
        Settings settings;
        QCOMPARE(settings.preset(), u"default");
        QVERIFY(!settings.openAnimation());
        QVERIFY(!settings.frosted());
        QCOMPARE(settings.cornerRadius(), 14);
    }

    void macosPresetTurnsEverythingOn()
    {
        Settings settings;
        QSignalSpy changed(&settings, &Settings::changed);
        settings.setPreset(QStringLiteral("macos"));
        QCOMPARE(changed.count(), 1);
        QVERIFY(settings.openAnimation());
        QVERIFY(settings.crossfade());
        QVERIFY(settings.autoHideControls());
        QVERIFY(settings.frosted());
        QVERIFY(settings.polishedText());
        QCOMPARE(settings.cornerRadius(), 18);
    }

    void editingSwitchesToCustomFromThePreset()
    {
        Settings settings;
        settings.setPreset(QStringLiteral("macos"));
        settings.setCrossfade(false);
        QCOMPARE(settings.preset(), u"custom");
        QVERIFY(!settings.crossfade());
        // Everything else keeps the macOS values it started from.
        QVERIFY(settings.openAnimation());
        QVERIFY(settings.frosted());
        QCOMPARE(settings.cornerRadius(), 18);
    }

    void unchangedValueIsNotAnEdit()
    {
        Settings settings;
        settings.setPreset(QStringLiteral("macos"));
        settings.setFrosted(true);
        QCOMPARE(settings.preset(), u"macos");
    }

    void persistsAcrossInstances()
    {
        {
            Settings settings;
            settings.setPreset(QStringLiteral("macos"));
            settings.setCornerRadius(24);
        }
        Settings reloaded;
        QCOMPARE(reloaded.preset(), u"custom");
        QCOMPARE(reloaded.cornerRadius(), 24);
        QVERIFY(reloaded.openAnimation());
    }

    void unknownPresetFallsBackToDefault()
    {
        Settings settings;
        settings.setPreset(QStringLiteral("bogus"));
        QCOMPARE(settings.preset(), u"default");
    }

    void radiusIsClamped()
    {
        Settings settings;
        settings.setCornerRadius(500);
        QCOMPARE(settings.cornerRadius(), 32);
    }
};

QTEST_GUILESS_MAIN(SettingsTest)
#include "settingstest.moc"
