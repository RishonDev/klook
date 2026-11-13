import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import klook

Item {
    id: view

    property string filePath
    property url fileUrl
    property string fileName
    property string mimeName
    property string iconName
    property string details

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            TextArea {
                id: edit
                readOnly: true
                // Don't take keyboard focus, so Space/Esc still close the preview.
                activeFocusOnPress: false
                selectByMouse: true
                textFormat: TextEdit.PlainText
                wrapMode: TextEdit.NoWrap
                font.family: "monospace"
                leftPadding: 14
                rightPadding: 14
                background: null
                text: file.text
            }
        }

        Label {
            Layout.fillWidth: true
            visible: file.truncated
            padding: 6
            horizontalAlignment: Text.AlignHCenter
            opacity: 0.7
            text: qsTr("Preview truncated to the first 2 MiB")
        }
    }

    readonly property string info: file.lineCount === 1 ? qsTr("1 line") : qsTr("%1 lines").arg(file.lineCount)

    TextFile {
        id: file
        filePath: view.filePath
    }

    Highlighter {
        document: edit.textDocument
        filePath: view.filePath
    }
}
