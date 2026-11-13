import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM

KCM.SimpleKCM {
    id: page

    readonly property var settings: kcm.settings

    // A checkbox that follows the setting even when a preset changes it.
    component Option: QQC2.CheckBox {
        required property string key
        checked: page.settings[key]
        onToggled: {
            page.settings[key] = checked;
            checked = Qt.binding(() => page.settings[key]);
        }
    }

    Kirigami.FormLayout {
        QQC2.ComboBox {
            Kirigami.FormData.label: i18n("Style:")
            textRole: "text"
            valueRole: "value"
            model: [
                { value: "default", text: i18n("Default") },
                { value: "macos", text: i18n("macOS") },
                { value: "custom", text: i18n("Custom") }
            ]
            currentIndex: indexOfValue(page.settings.preset)
            onActivated: page.settings.preset = currentValue
        }

        QQC2.Label {
            Layout.maximumWidth: Kirigami.Units.gridUnit * 22
            wrapMode: Text.Wrap
            opacity: 0.7
            text: page.settings.preset === "macos"
                ? i18n("Springy zoom, fades between files, auto-hiding controls and a frosted look.")
                : page.settings.preset === "default"
                    ? i18n("Plain and instant, with nothing animated.")
                    : i18n("Your own mix. Changing any option below switches to Custom.")
        }

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18n("Details")
        }

        Option {
            Kirigami.FormData.label: i18n("Motion:")
            key: "openAnimation"
            text: i18n("Zoom in and out when opening and closing")
        }
        Option {
            key: "crossfade"
            text: i18n("Fade between files")
        }
        Option {
            Kirigami.FormData.label: i18n("Controls:")
            key: "autoHideControls"
            text: i18n("Hide the title bar over images and video until the mouse moves")
        }
        Option {
            Kirigami.FormData.label: i18n("Look:")
            key: "frosted"
            text: i18n("Frosted, more translucent background")
        }
        Option {
            key: "polishedText"
            text: i18n("Polished Markdown: shaded code, airier text")
        }
        QQC2.SpinBox {
            Kirigami.FormData.label: i18n("Corner radius:")
            from: 0
            to: 32
            value: page.settings.cornerRadius
            onValueModified: {
                page.settings.cornerRadius = value;
                value = Qt.binding(() => page.settings.cornerRadius);
            }
        }

        Item { Kirigami.FormData.isSection: true }

        QQC2.Button {
            icon.name: "document-preview"
            text: i18n("Try it")
            onClicked: kcm.tryIt()
        }
    }
}
