#include "platform/Platform.h"

#include <array>
#include <cstdio>
#include <string>
#include <vector>

namespace {

std::optional<std::string> ReadPipeLine(FILE* pipe) {
    std::string result;
    std::array<char, 1024> buffer{};
    while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        result += buffer.data();
    }

    while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
        result.pop_back();
    }

    if (result.empty()) {
        return std::nullopt;
    }

    return result;
}

}  // namespace

namespace platform {

std::optional<std::string> ShowOpenMediaFileDialog() {
    // Linux is not a product target. zenity is a best-effort development fallback.
    FILE* pipe = popen(
        "command -v zenity >/dev/null 2>&1 && zenity --file-selection "
        "--title='Open media' "
        "--file-filter='Media files | *.mp4 *.m4v *.mkv *.mov *.avi *.flv *.mp3 *.aac *.wav *.m3u8' "
        "--file-filter='All files | *'",
        "r");
    if (pipe == nullptr) {
        return std::nullopt;
    }

    const std::optional<std::string> result = ReadPipeLine(pipe);
    if (pclose(pipe) != 0) {
        return std::nullopt;
    }

    return result;
}

TTF_Font* OpenUiFont(const int pointSize) {
    const std::vector<std::string> candidates = {
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/opentype/noto/NotoSansCJKsc-Regular.otf",
        "/usr/share/fonts/truetype/wqy/wqy-microhei.ttc",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
    };

    for (const std::string& candidate : candidates) {
        if (TTF_Font* font = TTF_OpenFont(candidate.c_str(), pointSize); font != nullptr) {
            return font;
        }
    }

    return nullptr;
}

std::optional<std::string> CommandLineMediaPath(int argc, char* argv[]) {
    if (argc > 1 && argv != nullptr && argv[1] != nullptr && argv[1][0] != '\0') {
        return std::string(argv[1]);
    }

    return std::nullopt;
}

}  // namespace platform
