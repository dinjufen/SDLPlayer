#pragma once

#include "player/Player.h"

#include <QImage>
#include <QObject>
#include <QString>
#include <QVariantList>

class QTimer;
class QtAudioOutput;
class QtVideoOutput;

class PlayerController final : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool hasMedia READ hasMedia NOTIFY mediaChanged)
    Q_PROPERTY(bool hasVideo READ hasVideo NOTIFY mediaChanged)
    Q_PROPERTY(bool hasAudio READ hasAudio NOTIFY mediaChanged)
    Q_PROPERTY(bool paused READ paused NOTIFY playbackStateChanged)
    Q_PROPERTY(double position READ position NOTIFY positionChanged)
    Q_PROPERTY(double duration READ duration NOTIFY mediaChanged)
    Q_PROPERTY(double speed READ speed WRITE setSpeed NOTIFY speedChanged)
    Q_PROPERTY(QVariantList supportedSpeeds READ supportedSpeeds CONSTANT)
    Q_PROPERTY(QString mediaSummary READ mediaSummary NOTIFY mediaChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)
    Q_PROPERTY(QString mediaTitle READ mediaTitle NOTIFY mediaChanged)
    Q_PROPERTY(QString mediaPath READ mediaPath NOTIFY mediaChanged)
    Q_PROPERTY(float volume READ volume WRITE setVolume NOTIFY volumeChanged)

public:
    explicit PlayerController(QObject* parent = nullptr);
    ~PlayerController() override;

    bool hasMedia() const;
    bool hasVideo() const;
    bool hasAudio() const;
    bool paused() const;
    double position() const;
    double duration() const;
    double speed() const;
    QVariantList supportedSpeeds() const;
    QString mediaSummary() const;
    QString statusText() const;
    QString mediaTitle() const;
    QString mediaPath() const;
    float volume() const;

    QImage currentFrame() const;

public slots:
    void openPath(const QString& pathOrUrl);
    void togglePause();
    void seek(double seconds);
    void seekRelative(double deltaSeconds);
    void setSpeed(double speed);
    void increaseSpeed();
    void decreaseSpeed();
    void setVolume(float volume);
    void close();

signals:
    void mediaChanged();
    void playbackStateChanged();
    void positionChanged();
    void speedChanged();
    void statusChanged();
    void volumeChanged();
    void frameReady();

private slots:
    void tick();
    void onFrameArrived();

private:
    void refreshMediaInfo();
    void refreshStatus();
    static QString localPathFromUserInput(const QString& pathOrUrl);

    player::Player player_;
    QtAudioOutput* audioOutput_ = nullptr;
    QtVideoOutput* videoOutput_ = nullptr;
    QTimer* timer_ = nullptr;
    player::MediaInfo mediaInfo_;
    QString statusText_;
    QString mediaTitle_;
    QString mediaPath_;
    QString mediaSummary_;
    float volume_ = 1.0f;
    bool lastPaused_ = false;
};
