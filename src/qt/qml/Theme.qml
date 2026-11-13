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

    // Qt Quick 的 font 值类型只有 family，没有 families（那是 QFont 的 C++ API）。
    readonly property string uiFontFamily: firstAvailable(uiFontFamilies)
    readonly property string monoFontFamily: firstAvailable(monoFontFamilies)

    function firstAvailable(candidates) {
        const available = Qt.fontFamilies()
        for (let i = 0; i < candidates.length; ++i) {
            if (available.indexOf(candidates[i]) !== -1)
                return candidates[i]
        }
        return candidates.length > 0 ? candidates[candidates.length - 1] : ""
    }
}
