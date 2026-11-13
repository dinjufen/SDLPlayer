#pragma once

struct SDL_Renderer;
struct SDL_Window;

namespace ui {

// Layout and mouse events stay in window coordinates. This scales the renderer
// so those coordinates map onto the HiDPI/Retina drawable size.
void ApplyWindowLogicalScale(SDL_Window* window, SDL_Renderer* renderer);

}  // namespace ui
