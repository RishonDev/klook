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
        Label {
            Layout.fillWidth: true
            text: view.details
            opacity: 0.7
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
        }
    }
}
