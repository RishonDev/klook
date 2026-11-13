import QtQuick
import QtQuick.Controls
import klook

Item {
    id: view

    property string filePath
    property url fileUrl
    property string fileName
    property string mimeName
    property string iconName
    property string details

    TextFile {
        id: file
        filePath: view.filePath
    }

    ScrollView {
        anchors.fill: parent

        TextArea {
            readOnly: true
            // Keep keyboard focus on the window so Space/Esc close the preview.
            activeFocusOnPress: false
            selectByMouse: true
            textFormat: TextEdit.MarkdownText
            wrapMode: TextEdit.Wrap
            // Resolve relative image links against the file's folder.
            baseUrl: view.fileUrl.toString().replace(/[^/]*$/, "")
            padding: 28
            background: null
            text: file.text
            onLinkActivated: link => Qt.openUrlExternally(link)
        }
    }
}
