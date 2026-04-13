#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <SDL.h>
#include <SDL_ttf.h>
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>

#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "player/Player.h"

namespace {

struct UiButton {
    SDL_Rect rect{};
    std::string label;
};

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

TTF_Font* OpenUiFont() {
    const std::vector<std::string> candidates = {
        "C:\\Windows\\Fonts\\msyh.ttc",
        "C:\\Windows\\Fonts\\segoeui.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
    };

    for (const std::string& candidate : candidates) {
        if (TTF_Font* font = TTF_OpenFont(candidate.c_str(), 18); font != nullptr) {
            return font;
        }
    }

    return nullptr;
}

std::string FormatTime(const double seconds) {
    const int total = std::max(0, static_cast<int>(seconds));
    const int hours = total / 3600;
    const int minutes = (total % 3600) / 60;
    const int secs = total % 60;

    std::ostringstream builder;
    if (hours > 0) {
        builder << hours << ':' << std::setw(2) << std::setfill('0') << minutes << ':' << std::setw(2) << secs;
    } else {
        builder << minutes << ':' << std::setw(2) << std::setfill('0') << secs;
    }

    return builder.str();
}

bool PointInRect(const int x, const int y, const SDL_Rect& rect) {
    return x >= rect.x && x < rect.x + rect.w && y >= rect.y && y < rect.y + rect.h;
}

double ClampRatio(const double value) { return std::clamp(value, 0.0, 1.0); }

void FillRect(SDL_Renderer* renderer, const SDL_Rect& rect, const SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);
}

void DrawRect(SDL_Renderer* renderer, const SDL_Rect& rect, const SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawRect(renderer, &rect);
}

void DrawText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, const int x, const int y,
              const SDL_Color color) {
    if (font == nullptr || text.empty()) {
        return;
    }

    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (surface == nullptr) {
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture == nullptr) {
        SDL_FreeSurface(surface);
        return;
    }

    SDL_Rect destination = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, nullptr, &destination);

    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

void DrawButton(SDL_Renderer* renderer, TTF_Font* font, const UiButton& button, const SDL_Color background,
                const SDL_Color textColor) {
    FillRect(renderer, button.rect, background);
    DrawRect(renderer, button.rect, SDL_Color{70, 82, 94, 255});

    if (font == nullptr) {
        return;
    }

    int textWidth = 0;
    int textHeight = 0;
    TTF_SizeUTF8(font, button.label.c_str(), &textWidth, &textHeight);
    const int textX = button.rect.x + (button.rect.w - textWidth) / 2;
    const int textY = button.rect.y + (button.rect.h - textHeight) / 2;
    DrawText(renderer, font, button.label, textX, textY, textColor);
}

std::string BuildMediaSummary(const player::MediaInfo& info) {
    if (!info.hasAudio && !info.hasVideo) {
        return "No media loaded.";
    }

    std::ostringstream builder;
    builder << info.formatName;

    if (info.hasVideo) {
        builder << " | video " << info.videoCodec << " | " << info.width << "x" << info.height;
        if (info.frameRate > 0.0) {
            builder << " | " << std::fixed << std::setprecision(2) << info.frameRate << " fps";
        }
    }

    if (info.hasAudio) {
        builder << " | audio " << info.audioCodec << " | " << info.sampleRate << " Hz"
                << " | " << info.channels << " ch";
    }

    return builder.str();
}

}  // namespace

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    if (TTF_Init() != 0) {
        SDL_Log("TTF_Init failed: %s", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("SDL FFmpeg Player", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 760,
                                          SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (window == nullptr) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer =
        SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
    if (renderer == nullptr) {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    TTF_Font* font = OpenUiFont();
    if (font == nullptr) {
        SDL_Log("Failed to load a system font: %s", TTF_GetError());
    }

    player::Player player;
    std::string bannerText = "Press O to open a media file.";

    int wideArgCount = 0;
    LPWSTR* wideArguments = CommandLineToArgvW(GetCommandLineW(), &wideArgCount);
    if (wideArguments != nullptr && wideArgCount > 1) {
        std::string errorMessage;
        if (player.Open(WideToUtf8(wideArguments[1]), errorMessage)) {
            bannerText = "Opened from command line.";
        } else {
            bannerText = errorMessage;
        }
    }

    bool running = true;
    bool draggingProgress = false;
    double draggingRatio = 0.0;
    bool showSpeedMenu = false;
    const std::vector<double> speedOptions = {0.5, 0.75, 1.0, 1.25, 1.5, 1.75, 2.0};

    auto openSelection = [&player, &bannerText](const std::string& path) {
        std::string errorMessage;
        if (player.Open(path, errorMessage)) {
            bannerText = "Opened: " + path;
        } else {
            bannerText = errorMessage;
        }
    };

    while (running) {
        int windowWidth = 0;
        int windowHeight = 0;
        SDL_GetWindowSize(window, &windowWidth, &windowHeight);

        const SDL_Rect contentRect = {24, 24, windowWidth - 48, windowHeight - 220};
        const SDL_Rect controlsRect = {24, windowHeight - 172, windowWidth - 48, 148};
        const SDL_Rect progressRect = {controlsRect.x + 24, controlsRect.y + 24, controlsRect.w - 48, 18};
        const UiButton openButton{{controlsRect.x + 24, controlsRect.y + 76, 140, 40}, "Open (O)"};
        const UiButton playButton{
            {controlsRect.x + 182, controlsRect.y + 76, 140, 40}, player.IsPaused() ? "Play" : "Pause"};

        std::ostringstream speedLabel;
        speedLabel << std::fixed << std::setprecision(2) << player.GetSpeed() << "x";
        const UiButton speedButton{{windowWidth - 164, windowHeight - 96, 140, 40}, speedLabel.str()};

        SDL_Event event;
        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_QUIT) {
                running = false;
                continue;
            }

            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        if (showSpeedMenu) {
                            showSpeedMenu = false;
                        } else {
                            running = false;
                        }
                        break;
                    case SDLK_o: {
                        if (const auto selection = ShowOpenMediaFileDialog(); selection.has_value()) {
                            openSelection(*selection);
                        }
                        break;
                    }
                    case SDLK_SPACE:
                        player.TogglePause();
                        break;
                    case SDLK_LEFT:
                        player.StepRelativeSeek(-5.0);
                        break;
                    case SDLK_RIGHT:
                        player.StepRelativeSeek(5.0);
                        break;
                    case SDLK_UP:
                        player.IncreaseSpeed();
                        break;
                    case SDLK_DOWN:
                        player.DecreaseSpeed();
                        break;
                    default:
                        break;
                }
            }
            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                const int mouseX = event.button.x;
                const int mouseY = event.button.y;

                if (PointInRect(mouseX, mouseY, openButton.rect)) {
                    if (const auto selection = ShowOpenMediaFileDialog(); selection.has_value()) {
                        openSelection(*selection);
                    }
                } else if (PointInRect(mouseX, mouseY, playButton.rect)) {
                    player.TogglePause();
                } else if (PointInRect(mouseX, mouseY, speedButton.rect)) {
                    showSpeedMenu = !showSpeedMenu;
                } else if (showSpeedMenu) {
                    // Check if clicking on speed menu items
                    bool foundSpeedSelection = false;
                    for (size_t i = 0; i < speedOptions.size(); ++i) {
                        const SDL_Rect speedItemRect = {speedButton.rect.x, speedButton.rect.y - static_cast<int>(speedOptions.size() - i) * 45, 140, 40};
                        if (PointInRect(mouseX, mouseY, speedItemRect)) {
                            player.SetSpeed(speedOptions[i]);
                            showSpeedMenu = false;
                            foundSpeedSelection = true;
                            break;
                        }
                    }
                    if (!foundSpeedSelection) {
                        showSpeedMenu = false;
                    }
                } else if (PointInRect(mouseX, mouseY, progressRect) && player.GetDurationSeconds() > 0.0) {
                    draggingProgress = true;
                    draggingRatio = ClampRatio(static_cast<double>(mouseX - progressRect.x) / progressRect.w);
                }
            }

            if (event.type == SDL_MOUSEMOTION && draggingProgress && player.GetDurationSeconds() > 0.0) {
                draggingRatio = ClampRatio(static_cast<double>(event.motion.x - progressRect.x) / progressRect.w);
            }

            if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT && draggingProgress) {
                draggingProgress = false;
                player.RequestSeek(draggingRatio * player.GetDurationSeconds());
            }
        }

        player.Update();

        const double durationSeconds = player.GetDurationSeconds();
        double positionSeconds = player.GetPositionSeconds();
        if (draggingProgress && durationSeconds > 0.0) {
            positionSeconds = draggingRatio * durationSeconds;
        }

        SDL_SetRenderDrawColor(renderer, 12, 16, 22, 255);
        SDL_RenderClear(renderer);

        FillRect(renderer, contentRect, SDL_Color{6, 10, 14, 255});
        if (!player.RenderVideo(renderer, contentRect)) {
            DrawText(renderer, font, player.HasMedia() ? "Audio-only media" : "No video frame", contentRect.x + 28,
                     contentRect.y + 28, SDL_Color{214, 220, 226, 255});
            DrawText(renderer, font, "Open a file to start learning the playback pipeline.", contentRect.x + 28,
                     contentRect.y + 60, SDL_Color{146, 156, 168, 255});
        }

        FillRect(renderer, controlsRect, SDL_Color{20, 25, 33, 245});
        DrawRect(renderer, controlsRect, SDL_Color{42, 50, 60, 255});

        FillRect(renderer, progressRect, SDL_Color{37, 44, 54, 255});
        DrawRect(renderer, progressRect, SDL_Color{63, 74, 86, 255});

        const double ratio = durationSeconds > 0.0 ? ClampRatio(positionSeconds / durationSeconds) : 0.0;
        SDL_Rect filledRect = progressRect;
        filledRect.w = std::max(0, static_cast<int>(ratio * progressRect.w));
        FillRect(renderer, filledRect, SDL_Color{74, 144, 226, 255});

        SDL_Rect handle = {progressRect.x + filledRect.w - 6, progressRect.y - 4, 12, progressRect.h + 8};
        FillRect(renderer, handle, SDL_Color{230, 236, 243, 255});

        DrawButton(renderer, font, openButton, SDL_Color{40, 47, 58, 255}, SDL_Color{240, 243, 247, 255});
        DrawButton(renderer, font, playButton, SDL_Color{52, 93, 176, 255}, SDL_Color{245, 248, 252, 255});
        DrawButton(renderer, font, speedButton, SDL_Color{40, 47, 58, 255}, SDL_Color{240, 243, 247, 255});

        // Draw speed menu if open
        if (showSpeedMenu) {
            for (size_t i = 0; i < speedOptions.size(); ++i) {
                const SDL_Rect speedItemRect = {speedButton.rect.x, speedButton.rect.y - static_cast<int>(speedOptions.size() - i) * 45, 140, 40};
                const double currentSpeed = player.GetSpeed();
                const double speed = speedOptions[i];
                const bool isCurrentSpeed = std::abs(currentSpeed - speed) < 0.01;
                
                const SDL_Color bgColor = isCurrentSpeed ? SDL_Color{52, 93, 176, 255} : SDL_Color{40, 47, 58, 255};
                const SDL_Color textColor = isCurrentSpeed ? SDL_Color{245, 248, 252, 255} : SDL_Color{240, 243, 247, 255};
                
                std::ostringstream speedOptionLabel;
                speedOptionLabel << std::fixed << std::setprecision(2) << speed << "x";
                
                FillRect(renderer, speedItemRect, bgColor);
                DrawRect(renderer, speedItemRect, SDL_Color{70, 82, 94, 255});
                
                int textWidth = 0;
                int textHeight = 0;
                TTF_SizeUTF8(font, speedOptionLabel.str().c_str(), &textWidth, &textHeight);
                const int textX = speedItemRect.x + (speedItemRect.w - textWidth) / 2;
                const int textY = speedItemRect.y + (speedItemRect.h - textHeight) / 2;
                DrawText(renderer, font, speedOptionLabel.str(), textX, textY, textColor);
            }
        }

        const std::string timeText = FormatTime(positionSeconds) + " / " + FormatTime(durationSeconds);
        DrawText(renderer, font, timeText, progressRect.x, progressRect.y + 28, SDL_Color{236, 240, 245, 255});
        DrawText(renderer, font, player.GetStatusText(), controlsRect.x + 470, controlsRect.y + 84,
                 SDL_Color{166, 176, 188, 255});

        const player::MediaInfo info = player.GetMediaInfo();
        const std::string fileName =
            info.path.empty() ? "No file loaded" : std::filesystem::path(info.path).filename().string();
        DrawText(renderer, font, fileName, contentRect.x + 18, contentRect.y + contentRect.h - 58,
                 SDL_Color{235, 239, 245, 255});
        DrawText(renderer, font, BuildMediaSummary(info), contentRect.x + 18, contentRect.y + contentRect.h - 30,
                 SDL_Color{150, 161, 174, 255});

        DrawText(renderer, font, bannerText, 28, 6, SDL_Color{140, 160, 185, 255});
        DrawText(renderer, font, "Hotkeys: O=open  Space=play/pause  Left/Right=seek  Up/Down=speed", 28,
                 windowHeight - 24, SDL_Color{120, 130, 143, 255});

        SDL_RenderPresent(renderer);
    }

    player.Close();

    if (wideArguments != nullptr) {
        LocalFree(wideArguments);
    }

    if (font != nullptr) {
        TTF_CloseFont(font);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
