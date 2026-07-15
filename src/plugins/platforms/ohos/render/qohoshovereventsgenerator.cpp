// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/private/qohoscommon_p.h>
#include <optional>
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

    std::optional<QPointF> m_optCurrentLocalPosition;
    std::optional<QPointF> m_optCurrentGlobalPosition;
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
    const auto localPosition = m_optCurrentLocalPosition.value_or(fallbackLocalPositionWhenUnknown);
    const auto globalPosition = m_optCurrentGlobalPosition.value_or(fallbackGlobalPositionWhenUnknown);

    m_imEventHandlerRef.visitInQtThreadIfAlive(
        [isHover, localPosition, globalPosition, qWindowRef = m_qWindowRef](QOhosInputMethodEventHandler &imEventHandler) {
            QWindow *qWindow = qWindowRef.data();
            if (qWindow != nullptr) {
                imEventHandler.onHoverEvent(
                    QOhosHoverEvent {
                        .targetWindow = qWindow,
                        .localPosition = localPosition,
                        .globalPosition = globalPosition,
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
