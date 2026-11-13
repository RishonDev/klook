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
    // "3 / 12" within the folder; set by the Previewer.
    property string position: ""
    // Set by the Previewer alongside the file (see propertiesFor()).
    property string kindText: ""
    property string sizeText: ""
    property string modifiedText: ""
    property string appName: ""
    property string appIcon: ""
    // "1600 × 900 · 1.2 MB · 3 / 12": what the view knows, then size and place in the folder.
    readonly property string subtitle: [content.item && content.item.info ? content.item.info : "",
                                        sizeText, position].filter(s => s.length > 0).join(" · ")
    // Shown on behalf of a file manager that keeps keyboard focus (see Previewer::Show).
    property bool passive: false

    // Asks the Previewer to show the previous (-1) or next (+1) file in the folder.
    signal navigate(int delta)

    readonly property int headerHeight: 44
    readonly property int contentInset: Math.ceil(cornerRadius * 0.4)
    readonly property int maxWidth: Math.round(Screen.desktopAvailableWidth * 0.8)
    readonly property int maxHeight: Math.round(Screen.desktopAvailableHeight * 0.8)

    minimumWidth: 360
    minimumHeight: 240
    // The Previewer shows the window once it has set it up (e.g. as a layer surface).
    visible: false
    title: fileName
    flags: Qt.Window | Qt.FramelessWindowHint | (passive ? Qt.WindowDoesNotAcceptFocus : 0)
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

    // Starting size per view; images refit once their size is known.
    function resetSize() {
        width = kind === "other" ? 520 : kind === "pdf" ? 820 : kind === "archive" ? 640 : 960;
        height = kind === "other" ? 360 : kind === "pdf" ? maxHeight : 680;
    }

    // Called on creation and by the Previewer after it swaps in a new file,
    // so arrow-key navigation reuses this window instead of flashing a new one.
    function reload() {
        resetSize();
        content.source = "";
        content.setSource(viewFor(kind), {
            filePath: filePath,
            fileUrl: fileUrl,
            fileName: fileName,
            mimeName: mimeName,
            iconName: iconName,
            details: details
        });
    }

    Component.onCompleted: reload()

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
        sequence: "Left"
        onActivated: root.navigate(-1)
    }
    Shortcut {
        sequence: "Right"
        onActivated: root.navigate(1)
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
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0
                        Label {
                            Layout.fillWidth: true
                            text: root.fileName
                            elide: Text.ElideMiddle
                            horizontalAlignment: Text.AlignHCenter
                            font.bold: true
                        }
                        Label {
                            Layout.fillWidth: true
                            visible: root.subtitle.length > 0
                            text: root.subtitle
                            horizontalAlignment: Text.AlignHCenter
                            font.pointSize: Qt.application.font.pointSize * 0.8
                            opacity: 0.6
                        }
                    }
                    ToolButton {
                        icon.name: root.appIcon || "document-open"
                        text: root.appName ? qsTr("Open with %1").arg(root.appName) : qsTr("Open")
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
