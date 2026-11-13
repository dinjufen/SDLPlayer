#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace player {

struct AudioOutputFormat {
    int sampleRate = 48000;
    int channels = 2;
    int bytesPerSample = 2;
};

class IAudioOutput {
public:
    using FillCallback = std::function<void(std::uint8_t* stream, int length)>;

    virtual ~IAudioOutput() = default;

    virtual bool Open(const AudioOutputFormat& format, FillCallback fill, std::string& errorMessage) = 0;
    virtual void Close() = 0;
    virtual void Start() = 0;
    virtual void Stop() = 0;
};

}  // namespace player
