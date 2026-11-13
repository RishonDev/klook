import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: view

    property string iconName
    property string fileName
    property string details
    property url fileUrl
    property string filePath
    property string mimeName
    // The preview window, for details that arrive after loading (folder size).
    readonly property var host: Window.window

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 40, 420)
        spacing: 12

        Image {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 128
            Layout.preferredHeight: 128
            fillMode: Image.PreserveAspectFit
            source: "image://icon/" + view.iconName
            sourceSize: Qt.size(128, 128)
        }
        Label {
            Layout.fillWidth: true
            text: view.fileName
            font.pointSize: 14
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideMiddle
        }
        // Quick Look's info card: kind, size (or folder contents), modification date.
        Repeater {
            model: [
                view.host ? view.host.kindText : "",
                view.host ? view.host.details : view.details,
                view.host && view.host.modifiedText ? qsTr("Modified: %1").arg(view.host.modifiedText) : ""
            ]
            delegate: Label {
                required property string modelData
                Layout.fillWidth: true
                visible: modelData.length > 0
                text: modelData
                opacity: 0.7
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
            }
        }
    }
}
