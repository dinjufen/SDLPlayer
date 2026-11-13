import QtQuick

Item {
    id: root
    signal openRequested()

    Column {
        anchors.centerIn: parent
        spacing: 14

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "打开一个媒体文件开始播放"
            color: Theme.textPrimary
            font.families: Theme.uiFontFamilies
            font.pixelSize: 22
            font.weight: Font.Medium
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "将文件拖放到窗口，或按 O 打开"
            color: Theme.textTertiary
            font.families: Theme.uiFontFamilies
            font.pixelSize: 13
        }

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 108
            height: 36
            radius: 18
            color: Qt.rgba(122 / 255, 162 / 255, 255 / 255, 0.16)
            border.color: Qt.rgba(122 / 255, 162 / 255, 255 / 255, 0.35)
            border.width: 1

            Text {
                anchors.centerIn: parent
                text: "打开文件"
                color: Theme.accent
                font.families: Theme.uiFontFamilies
                font.pixelSize: 13
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.openRequested()
            }
        }
    }
}
