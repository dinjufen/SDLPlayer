import QtQuick

Item {
    id: root

    property bool hovered: barMouse.containsMouse
    signal openRequested()

    implicitHeight: Theme.controlBarHeight + 16

    function formatTime(seconds) {
        const total = Math.max(0, Math.floor(seconds))
        const h = Math.floor(total / 3600)
        const m = Math.floor((total % 3600) / 60)
        const s = total % 60
        const mm = String(m).padStart(2, "0")
        const ss = String(s).padStart(2, "0")
        return h > 0 ? (String(h) + ":" + mm + ":" + ss) : (mm + ":" + ss)
    }

    Rectangle {
        id: bar
        anchors.fill: parent
        radius: Theme.radius
        color: Theme.glass
        border.color: Theme.glassBorder
        border.width: 1

        MouseArea {
            id: barMouse
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.NoButton
        }

        Row {
            id: leftRow
            anchors.left: parent.left
            anchors.leftMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8

            IconButton {
                icon: player.paused || !player.hasMedia ? "play" : "pause"
                tooltipText: player.paused || !player.hasMedia ? "播放" : "暂停"
                onClicked: {
                    if (player.hasMedia)
                        player.togglePause()
                    else
                        root.openRequested()
                }
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                width: 98
                text: root.formatTime(seekBar.visualPosition) + " / " + root.formatTime(player.duration)
                color: Theme.textSecondary
                font.families: Theme.monoFontFamilies
                font.pixelSize: 12
                horizontalAlignment: Text.AlignLeft
            }
        }

        SeekBar {
            id: seekBar
            anchors.left: leftRow.right
            anchors.right: rightRow.left
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            position: player.position
            duration: player.duration
            onSeekRequested: function(seconds) { player.seek(seconds) }
        }

        Row {
            id: rightRow
            anchors.right: parent.right
            anchors.rightMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8

            VolumeControl {
                volume: player.volume
                onVolumeEdited: function(value) { player.volume = value }
            }

            SpeedMenu {
                id: speedMenu
                anchors.verticalCenter: parent.verticalCenter
                speeds: player.supportedSpeeds
                current: player.speed
                onSpeedChosen: function(value) { player.speed = value }
            }

            IconButton {
                icon: "open"
                tooltipText: "打开文件"
                onClicked: root.openRequested()
            }
        }
    }

    function closeMenus() {
        speedMenu.opened = false
    }
}
