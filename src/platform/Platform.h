#pragma once

#include <SDL_ttf.h>

#include <optional>
#include <string>

namespace platform {

std::optional<std::string> ShowOpenMediaFileDialog();
TTF_Font* OpenUiFont(int pointSize = 18);
std::optional<std::string> CommandLineMediaPath(int argc, char* argv[]);

}  // namespace platform
