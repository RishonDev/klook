import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: root

    required property string filePath
    required property url fileUrl
    required property string fileName
    required property string kind
    required property string mimeName
    required property string iconName
    required property string details
    required property int cornerRadius

    readonly property int headerHeight: 44
    readonly property int contentInset: Math.ceil(cornerRadius * 0.4)
    readonly property int maxWidth: Math.round(Screen.desktopAvailableWidth * 0.8)
    readonly property int maxHeight: Math.round(Screen.desktopAvailableHeight * 0.8)

    // Starting size per view; images refit once their size is known.
    width: kind === "other" ? 520 : kind === "pdf" ? 820 : kind === "archive" ? 640 : 960
    height: kind === "other" ? 360 : kind === "pdf" ? maxHeight : 680
    minimumWidth: 360
    minimumHeight: 240
    visible: true
    title: fileName
    flags: Qt.Window | Qt.FramelessWindowHint
    color: "transparent"

    // Fit the window to an image's natural size, capped at 80% of the screen.
    function fitTo(w, h) {
        if (w <= 0 || h <= 0)
            return;
        const scale = Math.min(1, (maxWidth - 2 * contentInset) / w,
                               (maxHeight - headerHeight - contentInset) / h);
        width = Math.max(minimumWidth, Math.round(w * scale) + 2 * contentInset);
        height = Math.max(minimumHeight, Math.round(h * scale) + headerHeight + contentInset);
    }

    function viewFor(k) {
        switch (k) {
        case "image": return "ImageView.qml";
        case "text":  return "TextView.qml";
        case "pdf":   return "PdfView.qml";
        case "markdown": return "MarkdownView.qml";
        case "archive":  return "ArchiveView.qml";
        case "media": return "MediaView.qml";
        default:      return "OtherView.qml";
        }
    }

    function showFallback() {
        content.setSource("OtherView.qml", {
            iconName: root.iconName, fileName: root.fileName, details: root.details
        });
    }

    SystemPalette { id: pal }

    Shortcut {
        sequences: ["Escape", "Space"]
        onActivated: root.close()
    }
    Shortcut {
        sequences: ["Return", "Enter"]
        onActivated: { Qt.openUrlExternally(root.fileUrl); root.close(); }
    }

    Rectangle {
        anchors.fill: parent
        radius: root.cornerRadius
        color: Qt.rgba(pal.window.r, pal.window.g, pal.window.b, 0.85)
        border.color: Qt.rgba(pal.text.r, pal.text.g, pal.text.b, 0.15)
        border.width: 1
        clip: true

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 1
            spacing: 0

            // Header: drag to move, file name, open button.
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: root.headerHeight

                DragHandler {
                    target: null
                    onActiveChanged: if (active) root.startSystemMove()
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 8
                    spacing: 8

                    ToolButton {
                        icon.name: "window-close"
                        display: AbstractButton.IconOnly
                        onClicked: root.close()
                        ToolTip.text: qsTr("Close (Esc)")
                        ToolTip.visible: hovered
                    }
                    Label {
                        Layout.fillWidth: true
                        text: root.fileName
                        elide: Text.ElideMiddle
                        horizontalAlignment: Text.AlignHCenter
                        font.bold: true
                    }
                    ToolButton {
                        icon.name: "document-open"
                        text: qsTr("Open")
                        onClicked: { Qt.openUrlExternally(root.fileUrl); root.close(); }
                        ToolTip.text: qsTr("Open with default app (Enter)")
                        ToolTip.visible: hovered
                    }
                }
            }

            Loader {
                id: content
                Layout.fillWidth: true
                Layout.fillHeight: true
                // Inset just enough that content never pokes out of the rounded corners.
                Layout.leftMargin: root.contentInset
                Layout.rightMargin: root.contentInset
                Layout.bottomMargin: root.contentInset

                Component.onCompleted: setSource(root.viewFor(root.kind), {
                    filePath: root.filePath,
                    fileUrl: root.fileUrl,
                    fileName: root.fileName,
                    mimeName: root.mimeName,
                    iconName: root.iconName,
                    details: root.details
                })

                // A view whose QML module is missing (e.g. QtPdf) falls back to file info.
                onStatusChanged: if (status === Loader.Error) root.showFallback()
            }

            Connections {
                target: content.item
                ignoreUnknownSignals: true
                function onNaturalSizeChanged() {
                    root.fitTo(content.item.naturalSize.width, content.item.naturalSize.height);
                }
                function onFailed() { root.showFallback(); }
            }
        }
    }
}
