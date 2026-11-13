#pragma once

#include "player/IVideoOutput.h"

#include <QImage>
#include <QMutex>
#include <QObject>

class QtVideoOutput final : public QObject, public player::IVideoOutput {
    Q_OBJECT

public:
    explicit QtVideoOutput(QObject* parent = nullptr);
    QtVideoOutput(const QtVideoOutput&) = delete;
    QtVideoOutput& operator=(const QtVideoOutput&) = delete;
    ~QtVideoOutput() override;

    bool Present(const player::VideoFrameView& frame, const player::Rect& destination) override;
    void Reset() override;

    QImage currentFrame() const;

signals:
    void frameArrived();

private:
    mutable QMutex mutex_;
    QImage frame_;
};
