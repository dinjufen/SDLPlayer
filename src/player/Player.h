#pragma once

#include "player/IAudioOutput.h"
#include "player/IVideoOutput.h"

#include <memory>
#include <string>
#include <vector>

namespace player {

struct MediaInfo {
    bool hasAudio = false;
    bool hasVideo = false;
    std::string path;
    std::string formatName;
    std::string videoCodec;
    std::string audioCodec;
    int width = 0;
    int height = 0;
    double durationSeconds = 0.0;
    double frameRate = 0.0;
    int sampleRate = 0;
    int channels = 0;
};

class Player {
public:
    Player();
    ~Player();

    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;

    void SetAudioOutput(std::unique_ptr<IAudioOutput> output);
    void SetVideoOutput(std::unique_ptr<IVideoOutput> output);

    bool Open(const std::string& path, std::string& errorMessage);
    void Close();

    void Update();
    bool RenderVideo(const Rect& targetRect);

    void TogglePause();
    void SetPaused(bool paused);
    bool IsPaused() const;

    void RequestSeek(double seconds);
    void StepRelativeSeek(double deltaSeconds);

    void CycleSpeed();
    void IncreaseSpeed();
    void DecreaseSpeed();
    void SetSpeed(double speed);
    double GetSpeed() const;

    static std::vector<double> GetSupportedSpeeds();

    bool HasMedia() const;
    double GetPositionSeconds() const;
    double GetDurationSeconds() const;
    MediaInfo GetMediaInfo() const;
    std::string GetStatusText() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace player
