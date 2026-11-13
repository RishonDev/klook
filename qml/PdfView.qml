import QtQuick
import QtQuick.Pdf

Item {
    id: view

    property url fileUrl
    property string filePath
    property string fileName
    property string mimeName
    property string iconName
    property string details

    signal failed()

    PdfDocument {
        id: doc
        source: view.fileUrl
        onStatusChanged: {
            if (doc.status === PdfDocument.Error)
                view.failed();
            else if (doc.status === PdfDocument.Ready)
                Qt.callLater(() => {
                    pdfView.scaleToWidth(view.width, view.height);
                    pdfView.goToLocation(0, Qt.point(0, 0), pdfView.renderScale);
                });
        }
    }

    PdfMultiPageView {
        id: pdfView
        anchors.fill: parent
        clip: true
        document: doc
    }
}
