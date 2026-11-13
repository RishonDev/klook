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

    MarkdownPolish {
        document: markdown.textDocument
        enabled: Settings.polishedText
        textColor: markdown.color
    }

    // Polished: keep lines to a readable length on wide windows, like a page.
    readonly property int sidePadding: Settings.polishedText ? Math.max(36, (width - 760) / 2) : 28

    ScrollView {
        anchors.fill: parent

        TextArea {
            id: markdown
            readOnly: true
            // Keep keyboard focus on the window so Space/Esc close the preview.
            activeFocusOnPress: false
            selectByMouse: true
            textFormat: TextEdit.MarkdownText
            wrapMode: TextEdit.Wrap
            // Resolve relative image links against the file's folder.
            baseUrl: view.fileUrl.toString().replace(/[^/]*$/, "")
            topPadding: Settings.polishedText ? 32 : 28
            bottomPadding: topPadding
            leftPadding: view.sidePadding
            rightPadding: view.sidePadding
            background: null
            text: file.text
            onLinkActivated: link => Qt.openUrlExternally(link)
        }
    }
}
