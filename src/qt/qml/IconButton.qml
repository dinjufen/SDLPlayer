import QtQuick
import QtQuick.Controls

Item {
    id: root

    property string icon: "open"
    property bool active: false
    property string tooltipText: ""
    signal clicked()

    implicitWidth: 40
    implicitHeight: 40

    Rectangle {
        id: background
        anchors.fill: parent
        radius: width / 2
        color: mouse.pressed ? Qt.rgba(1, 1, 1, 0.10) : (mouse.containsMouse ? Qt.rgba(1, 1, 1, 0.07) : "transparent")
    }

    Canvas {
        id: canvas
        anchors.centerIn: parent
        width: 18
        height: 18
        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            ctx.clearRect(0, 0, width, height)
            ctx.strokeStyle = root.active ? Theme.accent : Theme.textPrimary
            ctx.fillStyle = root.active ? Theme.accent : Theme.textPrimary
            ctx.lineWidth = 1.6
            ctx.lineCap = "round"
            ctx.lineJoin = "round"

            if (root.icon === "play") {
                ctx.beginPath()
                ctx.moveTo(4, 2)
                ctx.lineTo(16, 9)
                ctx.lineTo(4, 16)
                ctx.closePath()
                ctx.fill()
            } else if (root.icon === "pause") {
                ctx.fillRect(4, 3, 4, 12)
                ctx.fillRect(10, 3, 4, 12)
            } else if (root.icon === "open") {
                ctx.beginPath()
                ctx.moveTo(2, 14)
                ctx.lineTo(2, 5)
                ctx.lineTo(7, 5)
                ctx.lineTo(9, 3)
                ctx.lineTo(16, 3)
                ctx.lineTo(16, 14)
                ctx.closePath()
                ctx.stroke()
                ctx.beginPath()
                ctx.moveTo(2, 14)
                ctx.lineTo(6, 8)
                ctx.lineTo(16, 8)
                ctx.stroke()
            } else if (root.icon === "volume") {
                ctx.beginPath()
                ctx.moveTo(2, 7)
                ctx.lineTo(6, 7)
                ctx.lineTo(11, 3)
                ctx.lineTo(11, 15)
                ctx.lineTo(6, 11)
                ctx.lineTo(2, 11)
                ctx.closePath()
                ctx.fill()
                ctx.beginPath()
                ctx.arc(11, 9, 3.2, -0.7, 0.7)
                ctx.stroke()
                ctx.beginPath()
                ctx.arc(11, 9, 5.4, -0.7, 0.7)
                ctx.stroke()
            }
        }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }

    ToolTip.visible: mouse.containsMouse && root.tooltipText.length > 0
    ToolTip.delay: 500
    ToolTip.text: root.tooltipText

    Connections {
        target: root
        function onIconChanged() { canvas.requestPaint() }
        function onActiveChanged() { canvas.requestPaint() }
    }
}
