#pragma once

#include "player/IAudioOutput.h"

#include <QAudioFormat>
#include <QObject>
#include <memory>

class QAudioSink;
class QIODevice;

class QtAudioOutput final : public QObject, public player::IAudioOutput {
    Q_OBJECT

public:
    explicit QtAudioOutput(QObject* parent = nullptr);
    QtAudioOutput(const QtAudioOutput&) = delete;
    QtAudioOutput& operator=(const QtAudioOutput&) = delete;
    ~QtAudioOutput() override;

    bool Open(const player::AudioOutputFormat& format, FillCallback fill, std::string& errorMessage) override;
    void Close() override;
    void Start() override;
    void Stop() override;

    float volume() const { return volume_; }
    void setVolume(float volume);

private:
    class PullDevice;

    void applyVolume();

    FillCallback fill_;
    QAudioFormat audioFormat_;
    std::unique_ptr<QAudioSink> sink_;
    std::unique_ptr<PullDevice> device_;
    float volume_ = 1.0f;
};
