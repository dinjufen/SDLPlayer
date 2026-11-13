#include "output/SdlAudioOutput.h"

#include <cstring>

namespace player {

SdlAudioOutput::~SdlAudioOutput() { Close(); }

bool SdlAudioOutput::Open(const AudioOutputFormat& format, FillCallback fill, std::string& errorMessage) {
    Close();

    if (format.bytesPerSample != 2) {
        errorMessage = "SDL audio output currently supports 16-bit samples only.";
        return false;
    }

    fill_ = std::move(fill);

    SDL_AudioSpec desired = {};
    desired.freq = format.sampleRate;
    desired.channels = static_cast<Uint8>(format.channels);
    desired.format = AUDIO_S16SYS;
    desired.samples = 2048;
    desired.callback = &SdlAudioOutput::Callback;
    desired.userdata = this;

    SDL_AudioSpec obtained = {};
    device_ = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
    if (device_ == 0) {
        fill_ = {};
        errorMessage = "Failed to open SDL audio device: " + std::string(SDL_GetError());
        return false;
    }

    return true;
}

void SdlAudioOutput::Close() {
    if (device_ != 0) {
        SDL_PauseAudioDevice(device_, 1);
        SDL_CloseAudioDevice(device_);
        device_ = 0;
    }
    fill_ = {};
}

void SdlAudioOutput::Start() {
    if (device_ != 0) {
        SDL_PauseAudioDevice(device_, 0);
    }
}

void SdlAudioOutput::Stop() {
    if (device_ != 0) {
        SDL_PauseAudioDevice(device_, 1);
    }
}

void SdlAudioOutput::Callback(void* userdata, Uint8* stream, int length) {
    auto* self = static_cast<SdlAudioOutput*>(userdata);
    if (self->fill_ != nullptr) {
        self->fill_(stream, length);
        return;
    }

    std::memset(stream, 0, static_cast<std::size_t>(length));
}

}  // namespace player
