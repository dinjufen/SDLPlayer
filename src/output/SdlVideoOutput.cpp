#include "output/SdlVideoOutput.h"

#include <SDL.h>

namespace player {

SdlVideoOutput::SdlVideoOutput(SDL_Renderer* renderer) : renderer_(renderer) {}

SdlVideoOutput::~SdlVideoOutput() { Reset(); }

bool SdlVideoOutput::Present(const VideoFrameView& frame, const Rect& destination) {
    if (renderer_ == nullptr || frame.pixels == nullptr || frame.width <= 0 || frame.height <= 0) {
        return false;
    }

    if (texture_ == nullptr || textureWidth_ != frame.width || textureHeight_ != frame.height) {
        if (texture_ != nullptr) {
            SDL_DestroyTexture(texture_);
            texture_ = nullptr;
        }

        texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_BGRA32, SDL_TEXTUREACCESS_STREAMING, frame.width,
                                     frame.height);
        if (texture_ == nullptr) {
            textureWidth_ = 0;
            textureHeight_ = 0;
            return false;
        }

        textureWidth_ = frame.width;
        textureHeight_ = frame.height;
    }

    if (SDL_UpdateTexture(texture_, nullptr, frame.pixels, frame.pitch) != 0) {
        return false;
    }

    const SDL_Rect dest{destination.x, destination.y, destination.w, destination.h};
    return SDL_RenderCopy(renderer_, texture_, nullptr, &dest) == 0;
}

void SdlVideoOutput::Reset() {
    if (texture_ != nullptr) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }
    textureWidth_ = 0;
    textureHeight_ = 0;
}

}  // namespace player
