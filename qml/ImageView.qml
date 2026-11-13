import QtQuick

Item {
    id: view

    property url fileUrl
    property string mimeName
    property string filePath
    property string fileName
    property string iconName
    property string details

    readonly property bool animated: mimeName === "image/gif" || mimeName === "image/webp"
                                     || mimeName === "image/apng"
    readonly property size naturalSize: loader.item && loader.item.status === Image.Ready
        ? Qt.size(loader.item.sourceSize.width, loader.item.sourceSize.height)
        : Qt.size(0, 0)

    signal failed()

    Loader {
        id: loader
        anchors.fill: parent
        sourceComponent: view.animated ? animatedImage : stillImage
    }

    Component {
        id: stillImage
        Image {
            source: view.fileUrl
            fillMode: Image.PreserveAspectFit
            asynchronous: true
            smooth: true
            mipmap: true
            onStatusChanged: if (status === Image.Error) view.failed()
        }
    }

    Component {
        id: animatedImage
        AnimatedImage {
            source: view.fileUrl
            fillMode: Image.PreserveAspectFit
            onStatusChanged: if (status === Image.Error) view.failed()
        }
    }
}
