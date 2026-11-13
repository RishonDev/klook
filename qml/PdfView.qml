import QtQuick
import QtQuick.Controls
import QtQuick.Pdf

// Pages stacked vertically and fitted to the window width, like Quick Look.
// (PdfMultiPageView's own fit-to-width left pages offset, so we lay them out ourselves.)
Item {
    id: view

    property url fileUrl
    property string filePath
    property string fileName
    property string mimeName
    property string iconName
    property string details

    readonly property int pageMargin: 16

    signal failed()

    PdfDocument {
        id: doc
        source: view.fileUrl
        onStatusChanged: if (doc.status === PdfDocument.Error) view.failed()
    }

    ListView {
        id: pages
        anchors.fill: parent
        clip: true
        spacing: view.pageMargin
        topMargin: view.pageMargin
        bottomMargin: view.pageMargin
        boundsBehavior: Flickable.StopAtBounds
        model: doc.status === PdfDocument.Ready ? doc.pageCount : 0
        ScrollBar.vertical: ScrollBar {}

        // indexAt() is only valid once delegates are laid out, so recompute lazily.
        property int currentPage: 0
        function updateCurrentPage() {
            currentPage = Math.max(0, indexAt(width / 2, contentY + height / 2));
        }
        onContentYChanged: Qt.callLater(updateCurrentPage)
        onHeightChanged: Qt.callLater(updateCurrentPage)
        onCountChanged: Qt.callLater(updateCurrentPage)

        delegate: Item {
            id: page
            required property int index
            readonly property size points: doc.pagePointSize(index)
            readonly property real pageWidth: ListView.view.width - 2 * view.pageMargin

            width: ListView.view.width
            height: points.width > 0 ? pageWidth * points.height / points.width : 0

            Rectangle {
                x: view.pageMargin
                width: page.pageWidth
                height: page.height
                color: "white"

                PdfPageImage {
                    anchors.fill: parent
                    document: doc
                    currentFrame: page.index
                    asynchronous: true
                    fillMode: Image.PreserveAspectFit
                    sourceSize.width: width * Screen.devicePixelRatio
                }
            }
        }
    }

    Label {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 20
        visible: doc.pageCount > 1
        padding: 6
        text: qsTr("%1 / %2").arg(pages.currentPage + 1).arg(doc.pageCount)
        background: Rectangle { radius: 6; color: Qt.rgba(0, 0, 0, 0.6) }
        color: "white"
    }
}
