#pragma once

#include "player/IVideoOutput.h"

struct SDL_Renderer;
struct SDL_Texture;

namespace player {

class SdlVideoOutput final : public IVideoOutput {
public:
    explicit SdlVideoOutput(SDL_Renderer* renderer);
    SdlVideoOutput(const SdlVideoOutput&) = delete;
    SdlVideoOutput& operator=(const SdlVideoOutput&) = delete;
    ~SdlVideoOutput() override;

    bool Present(const VideoFrameView& frame, const Rect& destination) override;
    void Reset() override;

private:
    SDL_Renderer* renderer_ = nullptr;
    SDL_Texture* texture_ = nullptr;
    int textureWidth_ = 0;
    int textureHeight_ = 0;
};

}  // namespace player
