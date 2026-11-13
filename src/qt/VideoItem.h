#pragma once

#include <QQuickItem>
#include <QtQml/qqmlregistration.h>

class PlayerController;

class VideoItem final : public QQuickItem {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(PlayerController* controller READ controller WRITE setController NOTIFY controllerChanged)

public:
    explicit VideoItem(QQuickItem* parent = nullptr);

    PlayerController* controller() const { return controller_; }
    void setController(PlayerController* controller);

signals:
    void controllerChanged();

protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* data) override;

private:
    PlayerController* controller_ = nullptr;
};
