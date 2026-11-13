import QtQuick

Item {
    id: root

    property real volume: 1
    signal volumeEdited(real value)

    implicitWidth: expanded ? 108 : 40
    implicitHeight: 40

    property bool expanded: mouse.containsMouse

    Behavior on implicitWidth { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }

    Row {
        anchors.fill: parent
        spacing: 2

        IconButton {
            icon: "volume"
            tooltipText: "音量"
            onClicked: root.volumeEdited(root.volume > 0 ? 0 : 1)
        }

        Item {
            id: slider
            width: Math.max(0, root.width - 42)
            height: parent.height
            visible: width > 8
            opacity: width > 20 ? 1 : 0

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                height: 3
                radius: 1.5
                color: Theme.track

                Rectangle {
                    width: parent.width * root.volume
                    height: parent.height
                    radius: parent.radius
                    color: Theme.accent
                }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onPressed: function(event) { root.volumeEdited(Math.min(1, Math.max(0, event.x / Math.max(1, width)))) }
                onPositionChanged: function(event) {
                    if (pressed)
                        root.volumeEdited(Math.min(1, Math.max(0, event.x / Math.max(1, width))))
                }
            }
        }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
    }
}
