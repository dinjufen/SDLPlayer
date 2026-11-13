import QtQuick

Item {
    id: root

    property real position: 0
    property real duration: 0
    property bool dragging: false
    signal seekRequested(real seconds)

    implicitHeight: 20
    enabled: duration > 0

    property real dragPosition: 0
    property real visualPosition: dragging ? dragPosition : position
    property real ratio: duration > 0 ? Math.min(1, Math.max(0, visualPosition / duration)) : 0

    Rectangle {
        id: track
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        height: 4
        radius: 2
        color: Theme.track

        Rectangle {
            width: Math.max(0, track.width * root.ratio)
            height: parent.height
            radius: parent.radius
            color: Theme.accent
        }
    }

    Rectangle {
        id: thumb
        width: 12
        height: 12
        radius: 6
        color: Theme.textPrimary
        border.color: Theme.accent
        border.width: mouse.containsMouse || root.dragging ? 2 : 0
        anchors.verticalCenter: parent.verticalCenter
        x: Math.round((track.width - width) * root.ratio)
        scale: root.dragging || mouse.containsMouse ? 1.15 : 1
        Behavior on scale { NumberAnimation { duration: 80 } }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: root.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
        enabled: root.enabled

        function secondsAt(px) {
            return root.ratioAt(px) * root.duration
        }
        function ratioAt(px) {
            return Math.min(1, Math.max(0, px / Math.max(1, track.width)))
        }

        onPressed: function(event) {
            root.dragPosition = secondsAt(event.x)
            root.dragging = true
        }
        onPositionChanged: function(event) {
            if (pressed)
                root.dragPosition = secondsAt(event.x)
        }
        onReleased: {
            root.dragging = false
            root.seekRequested(root.dragPosition)
        }
    }
}
