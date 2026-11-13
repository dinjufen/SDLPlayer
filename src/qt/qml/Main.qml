import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import SDLPlayer

ApplicationWindow {
    id: root

    width: 1280
    height: 720
    minimumWidth: 800
    minimumHeight: 480
    visible: true
    color: Theme.background
    title: player.mediaTitle.length > 0 ? (player.mediaTitle + " — SDLPlayer") : "SDLPlayer"

    property bool userInteracting: false
    property bool controlsVisible: !player.hasMedia || player.paused || userInteracting || controlBar.hovered

    function requestOpen() {
        openDialog.open()
    }

    function revealControls() {
        userInteracting = true
        hideTimer.restart()
    }

    Timer {
        id: hideTimer
        interval: 2200
        onTriggered: userInteracting = false
    }

    DropArea {
        id: dropArea
        anchors.fill: parent
        keys: ["text/uri-list"]
        onDropped: function(drop) {
            if (drop.hasUrls && drop.urls.length > 0) {
                player.openPath(drop.urls[0].toString())
                drop.acceptProposedAction()
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.canvas
    }

    VideoItem {
        id: videoView
        anchors.fill: parent
        controller: player
        visible: player.hasMedia && player.hasVideo
    }

    AudioPlaceholder {
        anchors.fill: parent
        visible: player.hasMedia && !player.hasVideo
    }

    EmptyState {
        anchors.fill: parent
        visible: !player.hasMedia
        onOpenRequested: root.requestOpen()
    }

    Rectangle {
        anchors.fill: parent
        color: dropArea.containsDrag ? Qt.rgba(122 / 255, 162 / 255, 255 / 255, 0.10) : "transparent"
        border.color: dropArea.containsDrag ? Theme.accent : "transparent"
        border.width: dropArea.containsDrag ? 1 : 0
        radius: 0
        visible: dropArea.containsDrag
        z: 40
    }

    // HoverHandler does not steal clicks from the control bar, unlike a
    // full-window MouseArea. Keep the click/double-click surface above the
    // video only, so the seek bar can receive presses.
    HoverHandler {
        id: activityHover
        onPointChanged: root.revealControls()
    }

    Item {
        id: playbackSurface
        anchors.fill: parent
        anchors.bottomMargin: controlBar.height + controlBar.anchors.bottomMargin
        z: 1

        TapHandler {
            acceptedButtons: Qt.LeftButton
            onTapped: {
                root.revealControls()
                controlBar.closeMenus()
            }
            onDoubleTapped: {
                if (player.hasMedia)
                    player.togglePause()
            }
        }
    }

    MediaInfoOverlay {
        id: infoOverlay
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: 20
        anchors.rightMargin: 20
        anchors.topMargin: 16
        height: 40
        shown: root.controlsVisible
        z: 10
    }

    Text {
        id: statusLabel
        anchors.left: parent.left
        anchors.bottom: controlBar.top
        anchors.leftMargin: 36
        anchors.bottomMargin: 10
        z: 10
        text: player.statusText
        color: player.statusText.indexOf("Failed") >= 0 || player.statusText.indexOf("失败") >= 0
               ? Theme.error : Theme.textTertiary
        font.family: Theme.uiFontFamily
        font.pixelSize: 12
        opacity: root.controlsVisible && player.statusText.length > 0 ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 160 } }
    }

    ControlBar {
        id: controlBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 24
        anchors.rightMargin: 24
        anchors.bottomMargin: 20
        height: Theme.controlBarHeight
        z: 20
        opacity: root.controlsVisible ? 1 : 0
        enabled: root.controlsVisible
        transform: Translate { y: root.controlsVisible ? 0 : 12 }
        Behavior on opacity { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
        onOpenRequested: root.requestOpen()
    }

    FileDialog {
        id: openDialog
        title: "打开媒体文件"
        fileMode: FileDialog.OpenFile
        nameFilters: [
            "媒体文件 (*.mp4 *.m4v *.mkv *.mov *.avi *.flv *.mp3 *.aac *.wav *.m3u8)",
            "所有文件 (*)"
        ]
        onAccepted: player.openPath(selectedFile)
    }

    Shortcut { sequence: "Space"; onActivated: { if (player.hasMedia) player.togglePause(); root.revealControls() } }
    Shortcut { sequence: "Left"; onActivated: { player.seekRelative(-5); root.revealControls() } }
    Shortcut { sequence: "Right"; onActivated: { player.seekRelative(5); root.revealControls() } }
    Shortcut { sequence: "Up"; onActivated: { player.increaseSpeed(); root.revealControls() } }
    Shortcut { sequence: "Down"; onActivated: { player.decreaseSpeed(); root.revealControls() } }
    Shortcut { sequence: "]"; onActivated: { player.increaseSpeed(); root.revealControls() } }
    Shortcut { sequence: "["; onActivated: { player.decreaseSpeed(); root.revealControls() } }
    Shortcut { sequence: "O"; onActivated: root.requestOpen() }
    Shortcut { sequence: "Ctrl+O"; onActivated: root.requestOpen() }
    Shortcut { sequence: "Meta+O"; onActivated: root.requestOpen() }
    Shortcut { sequence: "Esc"; onActivated: controlBar.closeMenus() }
}
