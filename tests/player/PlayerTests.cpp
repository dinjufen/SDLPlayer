#include "player/Player.h"

#include <cmath>
#include <exception>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

void Require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void RequireNear(double actual, double expected, const std::string& message) {
    if (std::abs(actual - expected) > 0.0001) {
        std::ostringstream details;
        details << message << " expected " << expected << " but got " << actual;
        throw std::runtime_error(details.str());
    }
}

void InitialStateIsIdle() {
    player::Player player;

    Require(!player.HasMedia(), "new player should not have media");
    Require(!player.IsPaused(), "new player should not be paused");
    RequireNear(player.GetSpeed(), 1.0, "new player speed");
    RequireNear(player.GetPositionSeconds(), 0.0, "new player position");
    RequireNear(player.GetDurationSeconds(), 0.0, "new player duration");
    Require(player.GetStatusText() == "Open a media file with O.", "new player status text");

    const player::MediaInfo info = player.GetMediaInfo();
    Require(!info.hasAudio, "new player media info should not report audio");
    Require(!info.hasVideo, "new player media info should not report video");
    Require(info.path.empty(), "new player media info path should be empty");
}

void NoMediaControlsDoNotCreatePlaybackState() {
    player::Player player;

    player.SetPaused(true);
    player.TogglePause();
    player.RequestSeek(15.0);
    player.StepRelativeSeek(5.0);
    player.Update();

    Require(!player.HasMedia(), "controls without media should not create media state");
    Require(!player.IsPaused(), "pause controls without media should be ignored");
    RequireNear(player.GetPositionSeconds(), 0.0, "seek without media should be ignored");
}

void SpeedSelectionUsesSupportedValues() {
    player::Player player;

    player.SetSpeed(1.13);
    RequireNear(player.GetSpeed(), 1.25, "speed should snap to nearest supported value");

    player.IncreaseSpeed();
    RequireNear(player.GetSpeed(), 1.5, "increase speed should move to the next supported value");

    player.DecreaseSpeed();
    RequireNear(player.GetSpeed(), 1.25, "decrease speed should move to the previous supported value");

    player.SetSpeed(10.0);
    RequireNear(player.GetSpeed(), 2.0, "speed above range should snap to maximum supported value");

    player.IncreaseSpeed();
    RequireNear(player.GetSpeed(), 2.0, "increase speed at maximum should stay at maximum");

    player.CycleSpeed();
    RequireNear(player.GetSpeed(), 0.5, "cycle speed at maximum should wrap to minimum");

    player.DecreaseSpeed();
    RequireNear(player.GetSpeed(), 0.5, "decrease speed at minimum should stay at minimum");
}

void InvalidOpenReportsFailureAndReturnsToIdle() {
    player::Player player;
    std::string errorMessage;

    const bool opened = player.Open("__sdlplayer_missing_media_file__.mp4", errorMessage);

    Require(!opened, "opening a missing file should fail");
    Require(!errorMessage.empty(), "failed open should report an error message");
    Require(errorMessage.find("Failed to open media") != std::string::npos,
            "failed open should include the media open failure context");
    Require(!player.HasMedia(), "failed open should leave player without media");
    RequireNear(player.GetPositionSeconds(), 0.0, "failed open should leave position at zero");
    Require(player.GetStatusText() == errorMessage, "failed open should expose the error as status text");
}

void RunTest(const char* name, void (*test)()) {
    std::cerr << "[run] " << name << '\n' << std::flush;
    test();
    std::cout << "[pass] " << name << '\n' << std::flush;
}

}  // namespace

int main() {
    try {
        RunTest("InitialStateIsIdle", InitialStateIsIdle);
        RunTest("NoMediaControlsDoNotCreatePlaybackState", NoMediaControlsDoNotCreatePlaybackState);
        RunTest("SpeedSelectionUsesSupportedValues", SpeedSelectionUsesSupportedValues);
        RunTest("InvalidOpenReportsFailureAndReturnsToIdle", InvalidOpenReportsFailureAndReturnsToIdle);
    } catch (const std::exception& error) {
        std::cerr << "[fail] " << error.what() << '\n';
        return 1;
    }

    return 0;
}
