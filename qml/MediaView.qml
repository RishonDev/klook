import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia

Item {
    id: view

    property url fileUrl
    property string filePath
    property string fileName
    property string mimeName
    property string iconName
    property string details

    readonly property bool isAudio: mimeName.startsWith("audio/")

    signal failed()

    MediaPlayer {
        id: player
        source: view.fileUrl
        videoOutput: video
        audioOutput: AudioOutput {}
        onErrorOccurred: view.failed()
        Component.onCompleted: play()
    }

    VideoOutput {
        id: video
        anchors.fill: parent
        visible: !view.isAudio
    }

    // Audio files: show the file icon instead of a black rectangle.
    Image {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -30
        visible: view.isAudio
        width: 160
        height: 160
        fillMode: Image.PreserveAspectFit
        source: "image://icon/" + view.iconName
        sourceSize: Qt.size(160, 160)
    }

    MouseArea {
        anchors.fill: parent
        onClicked: player.playbackState === MediaPlayer.PlayingState ? player.pause() : player.play()
    }

    // Minimal transport bar.
    RowLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 12
        spacing: 8

        ToolButton {
            icon.name: player.playbackState === MediaPlayer.PlayingState
                       ? "media-playback-pause" : "media-playback-start"
            onClicked: player.playbackState === MediaPlayer.PlayingState ? player.pause() : player.play()
        }
        Slider {
            Layout.fillWidth: true
            from: 0
            to: Math.max(1, player.duration)
            value: player.position
            enabled: player.seekable
            onMoved: player.position = value
        }
    }
}
