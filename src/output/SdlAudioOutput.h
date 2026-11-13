#pragma once

#include "player/IAudioOutput.h"

#include <SDL.h>

namespace player {

class SdlAudioOutput final : public IAudioOutput {
public:
    SdlAudioOutput() = default;
    SdlAudioOutput(const SdlAudioOutput&) = delete;
    SdlAudioOutput& operator=(const SdlAudioOutput&) = delete;
    ~SdlAudioOutput() override;

    bool Open(const AudioOutputFormat& format, FillCallback fill, std::string& errorMessage) override;
    void Close() override;
    void Start() override;
    void Stop() override;

private:
    static void Callback(void* userdata, Uint8* stream, int length);

    SDL_AudioDeviceID device_ = 0;
    FillCallback fill_;
};

}  // namespace player
