import QtQuick

Item {
    id: root

    property var speeds: []
    property real current: 1
    property bool opened: false
    signal speedChosen(real value)

    implicitWidth: 64
    implicitHeight: 32
    z: 20

    function formatSpeed(value) {
        return Number(value).toFixed(2) + "x"
    }

    Rectangle {
        id: chip
        anchors.fill: parent
        radius: 10
        color: chipMouse.containsMouse || root.opened ? Qt.rgba(1, 1, 1, 0.10) : Qt.rgba(1, 1, 1, 0.05)
        border.color: root.opened ? Theme.accent : Theme.glassBorder
        border.width: 1

        Text {
            anchors.centerIn: parent
            text: root.formatSpeed(root.current)
            color: Theme.textPrimary
            font.family: Theme.monoFontFamily
            font.pixelSize: 12
        }

        MouseArea {
            id: chipMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.opened = !root.opened
        }
    }

    Rectangle {
        id: menu
        visible: root.opened
        width: parent.width
        height: column.height + 12
        radius: 12
        color: Theme.glass
        border.color: Theme.glassBorder
        border.width: 1
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.top
        anchors.bottomMargin: 8

        Column {
            id: column
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            spacing: 2

            Repeater {
                model: root.speeds
                delegate: Rectangle {
                    required property var modelData
                    width: column.width
                    height: 28
                    radius: 8
                    color: itemMouse.containsMouse ? Qt.rgba(122 / 255, 162 / 255, 255 / 255, 0.18)
                                                   : (Math.abs(modelData - root.current) < 0.01 ? Qt.rgba(122 / 255, 162 / 255, 255 / 255, 0.22) : "transparent")

                    Text {
                        anchors.centerIn: parent
                        text: root.formatSpeed(modelData)
                        color: Math.abs(modelData - root.current) < 0.01 ? Theme.accent : Theme.textPrimary
                        font.family: Theme.monoFontFamily
                        font.pixelSize: 12
                    }

                    MouseArea {
                        id: itemMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            root.speedChosen(modelData)
                            root.opened = false
                        }
                    }
                }
            }
        }
    }
}
