import QtQuick

Item {
    id: root
    property bool shown: false

    implicitHeight: label.height + 16
    visible: opacity > 0.01
    opacity: shown && player.hasMedia ? 1 : 0
    Behavior on opacity { NumberAnimation { duration: 180 } }

    Rectangle {
        anchors.fill: parent
        radius: 12
        color: Theme.glass
        border.color: Theme.glassBorder
        border.width: 1

        Text {
            id: label
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 14
            anchors.rightMargin: 14
            text: player.mediaSummary
            color: Theme.textSecondary
            font.families: Theme.uiFontFamilies
            font.pixelSize: 12
            elide: Text.ElideRight
        }
    }
}
