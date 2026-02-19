// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/private/qohoscommon_p.h>
#include <QtGui/private/qhighdpiscaling_p.h>
#include <render/qohoshovereventsgenerator.h>

QT_BEGIN_NAMESPACE

namespace {

const QPointF fallbackLocalPositionWhenUnknown{0, 0};
const QPointF fallbackGlobalPositionWhenUnknown{0, 0};

class QOhosHoverEventsGeneratorImpl : public QOhosHoverEventsGenerator
{
public:
    QOhosHoverEventsGeneratorImpl(
        QtOhos::QThreadSafeRef<QWindow> qWindowRef,
        QtOhos::QThreadSafeRef<QOhosInputMethodEventHandler> imEventHandlerRef);

    void handleQOhosMouseEvent(const QOhosMouseEvent &mouseEvent) override;
    void handleQOhosHoverEvent(bool hovered) override;

private:
    void sendQtHoverEvent(bool isHover);

    QtOhos::QThreadSafeRef<QWindow> m_qWindowRef;
    QtOhos::QThreadSafeRef<QOhosInputMethodEventHandler> m_imEventHandlerRef;

    QOhosOptional<QPointF> m_optCurrentLocalPosition;
    QOhosOptional<QPointF> m_optCurrentGlobalPosition;
    bool m_hovered;
};

QOhosHoverEventsGeneratorImpl::QOhosHoverEventsGeneratorImpl(
    QtOhos::QThreadSafeRef<QWindow> qWindowRef,
    QtOhos::QThreadSafeRef<QOhosInputMethodEventHandler> imEventHandlerRef)
    : m_qWindowRef(qWindowRef)
    , m_imEventHandlerRef(imEventHandlerRef)
    , m_hovered(false)
{
}

void QOhosHoverEventsGeneratorImpl::handleQOhosMouseEvent(const QOhosMouseEvent &mouseEvent)
{
    m_optCurrentLocalPosition = mouseEvent.localPosition;
    m_optCurrentGlobalPosition = mouseEvent.globalPosition;

    if (!m_hovered) {
        sendQtHoverEvent(true);
        m_hovered = true;
    }
}

void QOhosHoverEventsGeneratorImpl::sendQtHoverEvent(bool isHover)
{
    const auto localPosition = m_optCurrentLocalPosition.valueOr(fallbackLocalPositionWhenUnknown);
    const auto globalPosition = m_optCurrentGlobalPosition.valueOr(fallbackGlobalPositionWhenUnknown);

    m_imEventHandlerRef.visitInQtThreadIfAlive(
        [isHover, localPosition, globalPosition, qWindowRef = m_qWindowRef](QOhosInputMethodEventHandler &imEventHandler) {
            QWindow *qWindow = qWindowRef.data();
            if (qWindow != nullptr) {
                auto *platformScreen = static_cast<QOhosPlatformScreen *>(qWindow->screen()->handle());
                const auto scaledLocalPosition = QHighDpi::toNative(localPosition, platformScreen->pixelScalingCoefficient());
                const auto scaledGlobalPosition = QHighDpi::toNative(globalPosition, platformScreen->pixelScalingCoefficient());
                imEventHandler.onHoverEvent(
                    QOhosHoverEvent {
                        .targetWindow = qWindow,
                        .localPosition = scaledLocalPosition,
                        .globalPosition = scaledGlobalPosition,
                        .isHover = isHover,
                    });
            }
        });
}

void QOhosHoverEventsGeneratorImpl::handleQOhosHoverEvent(bool hovered)
{
    if (!hovered && m_hovered) {
        sendQtHoverEvent(false);
        m_hovered = false;
    }
}

}

QOhosHoverEventsGenerator::QOhosHoverEventsGenerator() = default;

QOhosHoverEventsGenerator::~QOhosHoverEventsGenerator() = default;

std::shared_ptr<QOhosHoverEventsGenerator> makeQOhosHoverEventsGenerator(
    QtOhos::QThreadSafeRef<QWindow> qWindowRef,
    QtOhos::QThreadSafeRef<QOhosInputMethodEventHandler> imEventHandlerRef)
{
    return std::make_shared<QOhosHoverEventsGeneratorImpl>(qWindowRef, imEventHandlerRef);
}

QT_END_NAMESPACE
