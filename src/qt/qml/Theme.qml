pragma Singleton
import QtQuick

QtObject {
    id: theme

    readonly property color background: "#0B0D10"
    readonly property color canvas: "#07080A"
    readonly property color accent: "#7AA2FF"
    readonly property color accentMuted: "#4E6FB8"
    readonly property color textPrimary: "#F2F4F7"
    readonly property color textSecondary: "#9AA3B2"
    readonly property color textTertiary: "#6B7380"
    readonly property color glass: "#CC12151A"
    readonly property color glassBorder: "#1AFFFFFF"
    readonly property color track: "#33FFFFFF"
    readonly property color error: "#E88989"
    readonly property int controlBarHeight: 64
    readonly property int radius: 18

    readonly property var uiFontFamilies: [
        "PingFang SC", "PingFang TC", ".AppleSystemUIFont", "SF Pro Text", "Helvetica Neue",
        "Segoe UI", "Microsoft YaHei UI", "Microsoft YaHei",
        "Noto Sans CJK SC", "Noto Sans", "sans-serif"
    ]
    readonly property var monoFontFamilies: [
        "SF Mono", "Menlo", "Cascadia Mono", "Consolas", "Sarasa Mono SC", "monospace"
    ]
}
