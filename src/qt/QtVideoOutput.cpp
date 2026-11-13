#include "qt/QtVideoOutput.h"

#include <QMutexLocker>
#include <cstring>

QtVideoOutput::QtVideoOutput(QObject* parent) : QObject(parent) {}

QtVideoOutput::~QtVideoOutput() { Reset(); }

bool QtVideoOutput::Present(const player::VideoFrameView& frame, const player::Rect&) {
    if (frame.pixels == nullptr || frame.width <= 0 || frame.height <= 0 || frame.pitch < frame.width * 4) {
        return false;
    }

    // On little-endian, QImage::Format_ARGB32 stores bytes as B,G,R,A — the same as AV_PIX_FMT_BGRA.
    QImage image(frame.width, frame.height, QImage::Format_ARGB32);
    if (image.isNull()) {
        return false;
    }

    for (int y = 0; y < frame.height; ++y) {
        std::memcpy(image.scanLine(y), frame.pixels + static_cast<std::size_t>(y) * static_cast<std::size_t>(frame.pitch),
                    static_cast<std::size_t>(frame.width) * 4u);
    }

    {
        const QMutexLocker locker(&mutex_);
        frame_ = std::move(image);
    }

    emit frameArrived();
    return true;
}

void QtVideoOutput::Reset() {
    const QMutexLocker locker(&mutex_);
    frame_ = {};
}

QImage QtVideoOutput::currentFrame() const {
    const QMutexLocker locker(&mutex_);
    return frame_;
}
