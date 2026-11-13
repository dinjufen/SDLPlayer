#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "platform/Platform.h"

#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>

#include <string>
#include <vector>

namespace {

std::string WideToUtf8(const std::wstring& value) {
    if (value.empty()) {
        return {};
    }

    const int size = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string utf8(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, utf8.data(), size, nullptr, nullptr);
    utf8.pop_back();
    return utf8;
}

}  // namespace

namespace platform {

std::optional<std::string> ShowOpenMediaFileDialog() {
    std::vector<wchar_t> pathBuffer(4096, L'\0');
    const wchar_t filter[] =
        L"Media Files\0*.mp4;*.m4v;*.mkv;*.mov;*.avi;*.flv;*.mp3;*.aac;*.wav;*.m3u8\0All Files\0*.*\0\0";

    OPENFILENAMEW dialog = {};
    dialog.lStructSize = sizeof(dialog);
    dialog.lpstrFilter = filter;
    dialog.lpstrFile = pathBuffer.data();
    dialog.nMaxFile = static_cast<DWORD>(pathBuffer.size());
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    dialog.lpstrTitle = L"Open media";

    if (!GetOpenFileNameW(&dialog)) {
        return std::nullopt;
    }

    return WideToUtf8(pathBuffer.data());
}

TTF_Font* OpenUiFont(const int pointSize) {
    const std::vector<std::string> candidates = {
        "C:\\Windows\\Fonts\\msyh.ttc",
        "C:\\Windows\\Fonts\\segoeui.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
    };

    for (const std::string& candidate : candidates) {
        if (TTF_Font* font = TTF_OpenFont(candidate.c_str(), pointSize); font != nullptr) {
            return font;
        }
    }

    return nullptr;
}

std::optional<std::string> CommandLineMediaPath(int, char*[]) {
    int wideArgCount = 0;
    LPWSTR* wideArguments = CommandLineToArgvW(GetCommandLineW(), &wideArgCount);
    if (wideArguments == nullptr) {
        return std::nullopt;
    }

    std::optional<std::string> result;
    if (wideArgCount > 1) {
        result = WideToUtf8(wideArguments[1]);
    }

    LocalFree(wideArguments);
    return result;
}

}  // namespace platform
