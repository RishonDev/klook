import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import klook

Window {
    id: root

    required property string filePath
    required property url fileUrl
    required property string fileName
    required property string kind
    required property string mimeName
    required property string iconName
    required property string details
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

    // --- Appearance (Settings: Default / macOS / Custom presets) ---
    readonly property int cornerRadius: Settings.cornerRadius
    // Images and video: the header floats over the content and hides until the mouse moves.
    readonly property bool overlayControls: Settings.autoHideControls && (kind === "image" || kind === "media")
    property bool controlsShown: true
    readonly property int chromeHeight: overlayControls ? 0 : headerHeight
    // The Previewer blurs behind the window only while this is true, so the
    // blur never outlines an empty rectangle around a card that is still
    // zooming in or out.
    property bool blurActive: !Settings.openAnimation
    property bool closing: false

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
                               (maxHeight - chromeHeight - contentInset) / h);
        width = Math.max(minimumWidth, Math.round(w * scale) + 2 * contentInset);
        height = Math.max(minimumHeight, Math.round(h * scale) + chromeHeight + contentInset);
    }

    // Starting size per view; images refit once their size is known.
    function resetSize() {
        width = kind === "other" ? 520 : kind === "pdf" ? 820 : kind === "archive" ? 640 : 960;
        height = kind === "other" ? 360 : kind === "pdf" ? maxHeight : 680;
    }

    // Called on creation and by the Previewer after it swaps in a new file,
    // so arrow-key navigation reuses this window instead of flashing a new one.
    function reload() {
        if (Settings.crossfade && content.item)
            switchAnimation.restart();
        else
            load();
    }

    function load() {
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
        revealControls();
    }

    // Called by the Previewer right after showing the window.
    function present() {
        if (Settings.openAnimation) {
            openAnimation.restart();
        } else {
            card.scale = 1;
            card.opacity = 1;
            blurActive = true;
        }
    }

    // Every close path goes through here, so it can animate out first.
    function dismiss() {
        if (closing)
            return;
        closing = true;
        if (Settings.openAnimation) {
            blurActive = false;
            closeAnimation.restart();
        } else {
            close();
        }
    }

    function revealControls() {
        controlsShown = true;
        if (overlayControls)
            hideControlsTimer.restart();
    }

    Component.onCompleted: load()

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
        onActivated: root.dismiss()
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
        onActivated: { Qt.openUrlExternally(root.fileUrl); root.dismiss(); }
    }

    // Zoom in with a slight overshoot, like Quick Look springing open.
    ParallelAnimation {
        id: openAnimation
        NumberAnimation { target: card; property: "scale"; from: 0.88; to: 1; duration: 320; easing.type: Easing.OutBack; easing.overshoot: 1.1 }
        NumberAnimation { target: card; property: "opacity"; from: 0; to: 1; duration: 180; easing.type: Easing.OutCubic }
        onFinished: root.blurActive = true
    }

    ParallelAnimation {
        id: closeAnimation
        NumberAnimation { target: card; property: "scale"; to: 0.9; duration: 170; easing.type: Easing.InCubic }
        NumberAnimation { target: card; property: "opacity"; to: 0; duration: 170; easing.type: Easing.InCubic }
        onFinished: root.close()
    }

    // Fade out, swap the file in (the resize happens unseen), fade back in.
    SequentialAnimation {
        id: switchAnimation
        NumberAnimation { target: content; property: "opacity"; to: 0; duration: 90; easing.type: Easing.InQuad }
        ScriptAction { script: root.load() }
        NumberAnimation { target: content; property: "opacity"; to: 1; duration: 170; easing.type: Easing.OutQuad }
    }

    Timer {
        id: hideControlsTimer
        interval: 1800
        onTriggered: if (root.overlayControls && !headerHover.hovered) root.controlsShown = false
    }

    Item {
        id: card
        anchors.fill: parent
        // Starts invisible when animating in; present() takes it from there.
        scale: Settings.openAnimation ? 0.88 : 1
        opacity: Settings.openAnimation ? 0 : 1

        HoverHandler {
            onPointChanged: root.revealControls()
        }

        Rectangle {
            id: background
            anchors.fill: parent
            radius: root.cornerRadius
            // Frosted: a thinner tint lets more of the blurred desktop through.
            color: Qt.rgba(pal.window.r, pal.window.g, pal.window.b, Settings.frosted ? 0.62 : 0.85)
            // Frosted: a bright hairline edge instead of a text-coloured one.
            border.color: Settings.frosted ? Qt.rgba(1, 1, 1, 0.22) : Qt.rgba(pal.text.r, pal.text.g, pal.text.b, 0.15)
            border.width: 1
            Behavior on color { ColorAnimation { duration: 150 } }
        }

        Item {
            anchors.fill: parent
            anchors.margins: 1
            clip: true

            Loader {
                id: content
                anchors.fill: parent
                anchors.topMargin: root.chromeHeight
                // Inset just enough that content never pokes out of the rounded corners.
                anchors.leftMargin: root.contentInset
                anchors.rightMargin: root.contentInset
                anchors.bottomMargin: root.contentInset

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

            // Header: drag to move, file name, open button. Floats over images and
            // video (and fades away) when controls auto-hide.
            Item {
                id: header
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: root.headerHeight
                opacity: root.overlayControls && !root.controlsShown ? 0 : 1
                visible: opacity > 0
                Behavior on opacity { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }

                readonly property color ink: root.overlayControls ? "white" : pal.windowText

                Rectangle {
                    anchors.fill: parent
                    visible: root.overlayControls
                    gradient: Gradient {
                        GradientStop { position: 0; color: Qt.rgba(0, 0, 0, 0.55) }
                        GradientStop { position: 1; color: "transparent" }
                    }
                }

                HoverHandler { id: headerHover }
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
                        icon.color: header.ink
                        display: AbstractButton.IconOnly
                        onClicked: root.dismiss()
                        ToolTip.text: qsTr("Close (Esc)")
                        ToolTip.visible: hovered
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0
                        Label {
                            Layout.fillWidth: true
                            text: root.fileName
                            color: header.ink
                            elide: Text.ElideMiddle
                            horizontalAlignment: Text.AlignHCenter
                            font.bold: true
                        }
                        Label {
                            Layout.fillWidth: true
                            visible: root.subtitle.length > 0
                            text: root.subtitle
                            color: header.ink
                            horizontalAlignment: Text.AlignHCenter
                            font.pointSize: Qt.application.font.pointSize * 0.8
                            opacity: 0.7
                        }
                    }
                    ToolButton {
                        icon.name: root.appIcon || "document-open"
                        icon.color: root.appIcon ? "transparent" : header.ink
                        text: root.appName ? qsTr("Open with %1").arg(root.appName) : qsTr("Open")
                        palette.buttonText: header.ink
                        onClicked: { Qt.openUrlExternally(root.fileUrl); root.dismiss(); }
                        ToolTip.text: qsTr("Open with default app (Enter)")
                        ToolTip.visible: hovered
                    }
                }
            }
        }
    }
}
