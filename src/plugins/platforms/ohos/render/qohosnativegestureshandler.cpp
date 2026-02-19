// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosnativegestureshandler.h"
#include <QtCore/private/qohoslogger_p.h>
#include <private/qhighdpiscaling_p.h>
#include <qohosinputmethodeventhandler.h>
#include <qohosjsmain.h>
#include <qohosplatformintegration.h>
#include <qohosutils.h>
#include <render/qohosbatchingrequestshandler.h>
#include <utility>
#include <vector>

QT_BEGIN_NAMESPACE

namespace {

constexpr auto gestureEventMinAgeForDrop = std::chrono::milliseconds(20);

class QOhosNativeGesturesHandler final : public QEnableSharedFromThis<QOhosNativeGesturesHandler>
{
public:
    QOhosNativeGesturesHandler(
        QtOhos::QThreadSafeRef<QWindow> qWindowRef,
        std::function<void(QOhosNativeGestureEvent &)> qtThreadEventTransformer);

    void processGestureEventInJsThread(const QOhosNativeGestureEvent &event);

private:
    void processGestureEventsInQtThread(std::vector<QOhosNativeGestureEvent> &&batch);

    QtOhos::QThreadSafeRef<QWindow> m_qWindowRef;
    std::function<void(QOhosNativeGestureEvent &)> m_qtThreadEventTransformer;
    QOhosConsumer<QOhosNativeGestureEvent> m_gestureEventsHandler;
};

QOhosNativeGesturesHandler::QOhosNativeGesturesHandler(
    QtOhos::QThreadSafeRef<QWindow> qWindowRef,
    std::function<void(QOhosNativeGestureEvent &)> qtThreadEventTransformer)
    : m_qWindowRef(qWindowRef)
    , m_qtThreadEventTransformer(std::move(qtThreadEventTransformer))
{
}

void QOhosNativeGesturesHandler::processGestureEventInJsThread(
    const QOhosNativeGestureEvent &event)
{
    if (!m_gestureEventsHandler) {
        auto weakSelf = sharedFromThis().toWeakRef();
        m_gestureEventsHandler = makeQtOhosSimpleBatchingQtRequestsHandler<QOhosNativeGestureEvent>(
            m_qWindowRef.toQObjectThreadSafeRef(),
            [weakSelf](std::vector<QOhosNativeGestureEvent> &&batch) {
                auto sharedSelf = weakSelf.toStrongRef();
                if (!sharedSelf.isNull())
                    sharedSelf->processGestureEventsInQtThread(std::move(batch));
            });
    }

    m_gestureEventsHandler(event);
}

void QOhosNativeGesturesHandler::processGestureEventsInQtThread(std::vector<QOhosNativeGestureEvent> &&batch)
{
    auto *inputMethodEventHandler =
        QOhosPlatformIntegration::instance()->inputMethodEventHandler();

    if (inputMethodEventHandler == nullptr) {
        qOhosPrintfError("Cannot dispatch gesture event: inputMethodEventHandler is nullptr.");
        return;
    }

    const auto now = std::chrono::steady_clock::now();

    const auto mayDropGestureEvent = [now](const auto &event, const auto &nextEvent) {
        return now - event.timestamp >= gestureEventMinAgeForDrop
            && event.gestureType == nextEvent.gestureType
            && event.deviceType == nextEvent.deviceType;
    };

    batch.erase(
        QtOhos::removeMatchingWithLookahead(
            batch.begin(), batch.end(), mayDropGestureEvent),
        batch.end());

    const auto *platformScreen = m_qWindowRef.data()->screen()->handle();

    for (auto &event : batch) {
        m_qtThreadEventTransformer(event);

        inputMethodEventHandler->onGestureEventFromNativeNode(
            QOhosGestureEvent {
                .targetWindow = m_qWindowRef.data(),
                .timestamp = event.gestureTimestamp,
                .value = event.value,
                .localPosition = event.localPosition,
                .screenPosition = QHighDpi::fromNativePixels(event.displayBasedPosition, platformScreen),
                .gestureType = event.gestureType,
                .deviceType = event.deviceType,
            });
    }
}

}

QOhosConsumer<const QOhosNativeGestureEvent &> makeQOhosNativeGesturesHandler(
    QtOhos::QThreadSafeRef<QWindow> qWindowRef,
    std::function<void(QOhosNativeGestureEvent &)> optQtThreadEventTransformer)
{
    auto gesturesHandler = QSharedPointer<QOhosNativeGesturesHandler>::create(
        qWindowRef,
        optQtThreadEventTransformer
            ? std::move(optQtThreadEventTransformer)
            : [](QOhosNativeGestureEvent &) {
            });
    return [gesturesHandler](const QOhosNativeGestureEvent &event) {
        gesturesHandler->processGestureEventInJsThread(event);
    };
}

QT_END_NAMESPACE
