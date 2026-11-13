#include "qt/QtAudioOutput.h"

#include <QAudioDevice>
#include <QAudioSink>
#include <QIODevice>
#include <QMediaDevices>
#include <algorithm>
#include <cstring>
#include <cmath>

class QtAudioOutput::PullDevice final : public QIODevice {
public:
    explicit PullDevice(QtAudioOutput* owner) : owner_(owner) {}

    bool isSequential() const override { return true; }

    qint64 bytesAvailable() const override { return 16384 + QIODevice::bytesAvailable(); }

protected:
    qint64 readData(char* data, qint64 maxlen) override {
        if (owner_ == nullptr || owner_->fill_ == nullptr || data == nullptr || maxlen <= 0) {
            if (data != nullptr && maxlen > 0) {
                std::memset(data, 0, static_cast<std::size_t>(maxlen));
            }
            return std::max<qint64>(0, maxlen);
        }

        owner_->fill_(reinterpret_cast<std::uint8_t*>(data), static_cast<int>(maxlen));
        return maxlen;
    }

    qint64 writeData(const char*, qint64) override { return -1; }

private:
    QtAudioOutput* owner_ = nullptr;
};

QtAudioOutput::QtAudioOutput(QObject* parent) : QObject(parent) {}

QtAudioOutput::~QtAudioOutput() { Close(); }

bool QtAudioOutput::Open(const player::AudioOutputFormat& format, FillCallback fill, std::string& errorMessage) {
    Close();

    if (format.bytesPerSample != 2) {
        errorMessage = "Qt audio output currently supports 16-bit samples only.";
        return false;
    }

    fill_ = std::move(fill);

    audioFormat_ = {};
    audioFormat_.setSampleRate(format.sampleRate);
    audioFormat_.setChannelCount(format.channels);
    audioFormat_.setSampleFormat(QAudioFormat::Int16);

    const QAudioDevice outputDevice = QMediaDevices::defaultAudioOutput();
    if (outputDevice.isNull()) {
        fill_ = {};
        errorMessage = "No default audio output device is available.";
        return false;
    }

    if (!outputDevice.isFormatSupported(audioFormat_)) {
        fill_ = {};
        errorMessage = "The default audio device does not support 48 kHz stereo S16.";
        return false;
    }

    sink_ = std::make_unique<QAudioSink>(outputDevice, audioFormat_);
    device_ = std::make_unique<PullDevice>(this);
    if (!device_->open(QIODevice::ReadOnly)) {
        Close();
        errorMessage = "Failed to open the Qt audio pull device.";
        return false;
    }

    applyVolume();
    // Stay suspended until Start(), matching the SDL backend's paused-on-open behavior.
    sink_->start(device_.get());
    if (sink_->error() != QAudio::NoError) {
        errorMessage = "Failed to start QAudioSink.";
        Close();
        return false;
    }
    sink_->suspend();
    return true;
}

void QtAudioOutput::Close() {
    if (sink_ != nullptr) {
        sink_->stop();
        sink_.reset();
    }
    if (device_ != nullptr) {
        device_->close();
        device_.reset();
    }
    fill_ = {};
}

void QtAudioOutput::Start() {
    if (sink_ != nullptr) {
        sink_->resume();
    }
}

void QtAudioOutput::Stop() {
    if (sink_ != nullptr) {
        sink_->suspend();
    }
}

void QtAudioOutput::setVolume(const float volume) {
    volume_ = std::clamp(volume, 0.0f, 1.0f);
    applyVolume();
}

void QtAudioOutput::applyVolume() {
    if (sink_ != nullptr) {
        sink_->setVolume(static_cast<qreal>(volume_));
    }
}
