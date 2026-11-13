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

    signal failed()

    SystemPalette { id: pal }

    ArchiveModel {
        id: archive
        filePath: view.filePath
        onLoaded: if (error.length > 0) view.failed()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Label {
            Layout.fillWidth: true
            Layout.margins: 10
            horizontalAlignment: Text.AlignHCenter
            opacity: 0.7
            text: {
                const files = archive.fileCount === 1 ? qsTr("1 file") : qsTr("%1 files").arg(archive.fileCount);
                const dirs = archive.dirCount === 1 ? qsTr("1 folder") : qsTr("%1 folders").arg(archive.dirCount);
                return qsTr("%1, %2 · %3 uncompressed").arg(files).arg(dirs).arg(archive.totalSize);
            }
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.leftMargin: 6
            Layout.rightMargin: 6
            Layout.fillHeight: true
            clip: true
            model: archive
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {}

            delegate: ItemDelegate {
                required property string name
                required property int depth
                required property bool isDir
                required property string size
                required property string iconName

                width: ListView.view.width
                height: 30
                hoverEnabled: true
                focusPolicy: Qt.NoFocus
                background: Rectangle {
                    color: parent.hovered ? Qt.rgba(pal.highlight.r, pal.highlight.g, pal.highlight.b, 0.15)
                                          : "transparent"
                    radius: 4
                }

                contentItem: RowLayout {
                    spacing: 8
                    Item { Layout.preferredWidth: depth * 18 }
                    Image {
                        Layout.preferredWidth: 18
                        Layout.preferredHeight: 18
                        source: "image://icon/" + iconName
                        sourceSize: Qt.size(18, 18)
                    }
                    Label {
                        Layout.fillWidth: true
                        text: name
                        elide: Text.ElideMiddle
                        font.bold: isDir
                    }
                    Label {
                        text: size
                        opacity: 0.6
                    }
                }
            }
        }

        Label {
            Layout.fillWidth: true
            visible: archive.truncated
            padding: 6
            horizontalAlignment: Text.AlignHCenter
            opacity: 0.7
            text: qsTr("Showing the first %1 entries").arg(archive.maxEntries)
        }
    }
}
