#include "qt/PlayerController.h"

#include "qt/QtAudioOutput.h"
#include "qt/QtVideoOutput.h"

#include <QFileInfo>
#include <QTimer>
#include <QUrl>
#include <algorithm>
#include <cmath>
#include <memory>

PlayerController::PlayerController(QObject* parent) : QObject(parent) {
    auto audio = std::make_unique<QtAudioOutput>();
    audioOutput_ = audio.get();
    audioOutput_->setVolume(volume_);
    player_.SetAudioOutput(std::move(audio));

    auto video = std::make_unique<QtVideoOutput>();
    videoOutput_ = video.get();
    connect(videoOutput_, &QtVideoOutput::frameArrived, this, &PlayerController::onFrameArrived);
    player_.SetVideoOutput(std::move(video));

    timer_ = new QTimer(this);
    timer_->setInterval(16);
    connect(timer_, &QTimer::timeout, this, &PlayerController::tick);
    timer_->start();

    refreshStatus();
}

PlayerController::~PlayerController() { player_.Close(); }

bool PlayerController::hasMedia() const { return player_.HasMedia(); }

bool PlayerController::hasVideo() const { return mediaInfo_.hasVideo; }

bool PlayerController::hasAudio() const { return mediaInfo_.hasAudio; }

bool PlayerController::paused() const { return player_.IsPaused(); }

double PlayerController::position() const { return player_.GetPositionSeconds(); }

double PlayerController::duration() const { return player_.GetDurationSeconds(); }

double PlayerController::speed() const { return player_.GetSpeed(); }

QVariantList PlayerController::supportedSpeeds() const {
    QVariantList values;
    for (const double speed : player::Player::GetSupportedSpeeds()) {
        values.push_back(speed);
    }
    return values;
}

QString PlayerController::mediaSummary() const { return mediaSummary_; }

QString PlayerController::statusText() const { return statusText_; }

QString PlayerController::mediaTitle() const { return mediaTitle_; }

QString PlayerController::mediaPath() const { return mediaPath_; }

float PlayerController::volume() const { return volume_; }

QImage PlayerController::currentFrame() const {
    if (videoOutput_ == nullptr) {
        return {};
    }
    return videoOutput_->currentFrame();
}

void PlayerController::openPath(const QString& pathOrUrl) {
    const QString path = localPathFromUserInput(pathOrUrl);
    if (path.isEmpty()) {
        statusText_ = QStringLiteral("未选择文件。");
        emit statusChanged();
        return;
    }

    std::string errorMessage;
    if (!player_.Open(path.toStdString(), errorMessage)) {
        mediaInfo_ = {};
        mediaTitle_.clear();
        mediaPath_.clear();
        mediaSummary_.clear();
        statusText_ = QString::fromStdString(errorMessage);
        emit mediaChanged();
        emit statusChanged();
        emit playbackStateChanged();
        emit positionChanged();
        emit frameReady();
        return;
    }

    if (audioOutput_ != nullptr) {
        audioOutput_->setVolume(volume_);
    }

    lastPaused_ = player_.IsPaused();
    refreshMediaInfo();
    refreshStatus();
    emit mediaChanged();
    emit playbackStateChanged();
    emit speedChanged();
    emit positionChanged();
}

void PlayerController::togglePause() {
    if (!player_.HasMedia()) {
        return;
    }
    player_.TogglePause();
    lastPaused_ = player_.IsPaused();
    refreshStatus();
    emit playbackStateChanged();
}

void PlayerController::seek(const double seconds) {
    if (!player_.HasMedia()) {
        return;
    }
    player_.RequestSeek(seconds);
    emit positionChanged();
}

void PlayerController::seekRelative(const double deltaSeconds) {
    if (!player_.HasMedia()) {
        return;
    }
    player_.StepRelativeSeek(deltaSeconds);
    emit positionChanged();
}

void PlayerController::setSpeed(const double speed) {
    player_.SetSpeed(speed);
    emit speedChanged();
}

void PlayerController::increaseSpeed() {
    player_.IncreaseSpeed();
    emit speedChanged();
}

void PlayerController::decreaseSpeed() {
    player_.DecreaseSpeed();
    emit speedChanged();
}

void PlayerController::setVolume(const float volume) {
    const float clamped = std::clamp(volume, 0.0f, 1.0f);
    if (std::abs(clamped - volume_) < 0.0005f) {
        return;
    }
    volume_ = clamped;
    if (audioOutput_ != nullptr) {
        audioOutput_->setVolume(volume_);
    }
    emit volumeChanged();
}

void PlayerController::close() {
    player_.Close();
    mediaInfo_ = {};
    mediaTitle_.clear();
    mediaPath_.clear();
    mediaSummary_.clear();
    lastPaused_ = false;
    refreshStatus();
    emit mediaChanged();
    emit playbackStateChanged();
    emit positionChanged();
    emit frameReady();
}

void PlayerController::tick() {
    if (!player_.HasMedia()) {
        return;
    }

    player_.Update();
    if (mediaInfo_.hasVideo) {
        player_.RenderVideo(player::Rect{0, 0, std::max(1, mediaInfo_.width), std::max(1, mediaInfo_.height)});
    }

    refreshStatus();
    emit positionChanged();

    const bool nowPaused = player_.IsPaused();
    if (nowPaused != lastPaused_) {
        lastPaused_ = nowPaused;
        emit playbackStateChanged();
    }
}

void PlayerController::onFrameArrived() { emit frameReady(); }

void PlayerController::refreshMediaInfo() {
    mediaInfo_ = player_.GetMediaInfo();
    mediaPath_ = QString::fromStdString(mediaInfo_.path);
    mediaTitle_ = QFileInfo(mediaPath_).fileName();
    if (mediaTitle_.isEmpty()) {
        mediaTitle_ = mediaPath_;
    }

    QStringList parts;
    if (!mediaInfo_.formatName.empty()) {
        parts << QString::fromStdString(mediaInfo_.formatName);
    }
    if (mediaInfo_.hasVideo) {
        QString video = QStringLiteral("视频 %1  %2×%3")
                            .arg(QString::fromStdString(mediaInfo_.videoCodec),
                                 QString::number(mediaInfo_.width), QString::number(mediaInfo_.height));
        if (mediaInfo_.frameRate > 0.0) {
            video += QStringLiteral("  %1 fps").arg(mediaInfo_.frameRate, 0, 'f', 2);
        }
        parts << video;
    }
    if (mediaInfo_.hasAudio) {
        parts << QStringLiteral("音频 %1  %2 Hz  %3 ch")
                     .arg(QString::fromStdString(mediaInfo_.audioCodec), QString::number(mediaInfo_.sampleRate),
                          QString::number(mediaInfo_.channels));
    }
    mediaSummary_ = parts.join(QStringLiteral("  ·  "));
}

void PlayerController::refreshStatus() {
    const QString next = QString::fromStdString(player_.GetStatusText());
    if (next != statusText_) {
        statusText_ = next;
        emit statusChanged();
    }
}

QString PlayerController::localPathFromUserInput(const QString& pathOrUrl) {
    const QUrl url(pathOrUrl);
    if (url.isLocalFile()) {
        return url.toLocalFile();
    }
    if (pathOrUrl.startsWith(QStringLiteral("file:"), Qt::CaseInsensitive)) {
        return QUrl(pathOrUrl).toLocalFile();
    }
    return pathOrUrl;
}
