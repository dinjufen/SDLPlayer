#include "qt/VideoItem.h"

#include "qt/PlayerController.h"

#include <QImage>
#include <QQuickWindow>
#include <QSGImageNode>
#include <QSGTexture>

VideoItem::VideoItem(QQuickItem* parent) : QQuickItem(parent) {
    setFlag(ItemHasContents, true);
    setAntialiasing(true);
}

void VideoItem::setController(PlayerController* controller) {
    if (controller_ == controller) {
        return;
    }

    if (controller_ != nullptr) {
        disconnect(controller_, &PlayerController::frameReady, this, &QQuickItem::update);
        disconnect(controller_, &PlayerController::mediaChanged, this, &QQuickItem::update);
    }

    controller_ = controller;

    if (controller_ != nullptr) {
        connect(controller_, &PlayerController::frameReady, this, &QQuickItem::update);
        connect(controller_, &PlayerController::mediaChanged, this, &QQuickItem::update);
    }

    emit controllerChanged();
    update();
}

QSGNode* VideoItem::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) {
    const QImage image = controller_ != nullptr ? controller_->currentFrame() : QImage();
    if (image.isNull() || image.width() <= 0 || image.height() <= 0 || width() <= 0 || height() <= 0) {
        delete oldNode;
        return nullptr;
    }

    auto* node = static_cast<QSGImageNode*>(oldNode);
    if (node == nullptr) {
        if (window() == nullptr) {
            return nullptr;
        }
        node = window()->createImageNode();
        node->setOwnsTexture(true);
        node->setFiltering(QSGTexture::Linear);
        node->setMipmapFiltering(QSGTexture::None);
        node->setAnisotropyLevel(QSGTexture::AnisotropyNone);
    }

    QSGTexture* texture = window()->createTextureFromImage(image, QQuickWindow::TextureHasAlphaChannel);
    if (texture == nullptr) {
        delete node;
        return nullptr;
    }
    texture->setFiltering(QSGTexture::Linear);
    node->setTexture(texture);

    const qreal viewWidth = width();
    const qreal viewHeight = height();
    const qreal sourceAspect = static_cast<qreal>(image.width()) / static_cast<qreal>(image.height());
    const qreal viewAspect = viewWidth / viewHeight;

    QRectF destination(0, 0, viewWidth, viewHeight);
    if (sourceAspect > viewAspect) {
        const qreal letterboxedHeight = viewWidth / sourceAspect;
        destination = QRectF(0, (viewHeight - letterboxedHeight) * 0.5, viewWidth, letterboxedHeight);
    } else {
        const qreal letterboxedWidth = viewHeight * sourceAspect;
        destination = QRectF((viewWidth - letterboxedWidth) * 0.5, 0, letterboxedWidth, viewHeight);
    }

    node->setRect(destination);
    node->setSourceRect(QRectF(0, 0, image.width(), image.height()));
    return node;
}
