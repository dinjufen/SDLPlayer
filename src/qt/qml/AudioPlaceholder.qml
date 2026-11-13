import QtQuick

Item {
    Column {
        anchors.centerIn: parent
        spacing: 12
        width: Math.min(parent.width - 80, 640)

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "仅音频"
            color: Theme.textTertiary
            font.family: Theme.uiFontFamily
            font.pixelSize: 13
            font.letterSpacing: 2
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width
            text: player.mediaTitle
            color: Theme.textPrimary
            font.family: Theme.uiFontFamily
            font.pixelSize: 26
            font.weight: Font.Medium
            elide: Text.ElideMiddle
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.NoWrap
        }
    }
}
