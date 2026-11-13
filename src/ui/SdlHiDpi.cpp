#include "ui/SdlHiDpi.h"

#include <SDL.h>

namespace ui {

void ApplyWindowLogicalScale(SDL_Window* window, SDL_Renderer* renderer) {
    if (window == nullptr || renderer == nullptr) {
        return;
    }

    int windowWidth = 0;
    int windowHeight = 0;
    int outputWidth = 0;
    int outputHeight = 0;
    SDL_GetWindowSize(window, &windowWidth, &windowHeight);
    SDL_GetRendererOutputSize(renderer, &outputWidth, &outputHeight);

    const float scaleX =
        windowWidth > 0 ? static_cast<float>(outputWidth) / static_cast<float>(windowWidth) : 1.0f;
    const float scaleY =
        windowHeight > 0 ? static_cast<float>(outputHeight) / static_cast<float>(windowHeight) : 1.0f;
    SDL_RenderSetScale(renderer, scaleX, scaleY);
}

}  // namespace ui
