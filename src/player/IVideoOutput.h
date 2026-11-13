#pragma once

#include <cstdint>

namespace player {

struct VideoFrameView {
    const std::uint8_t* pixels = nullptr;
    int width = 0;
    int height = 0;
    int pitch = 0;
};

struct Rect {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
};

class IVideoOutput {
public:
    virtual ~IVideoOutput() = default;

    virtual bool Present(const VideoFrameView& frame, const Rect& destination) = 0;
    virtual void Reset() = 0;
};

}  // namespace player
