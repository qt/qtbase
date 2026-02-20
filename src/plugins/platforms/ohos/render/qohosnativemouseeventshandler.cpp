// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/private/qohoscommon_p.h>
#include <QtGui/private/qhighdpiscaling_p.h>
#include <chrono>
#include <cstdint>
#include <functional>
#include <qarkui/input.h>
#include <render/qohosbatchingrequestshandler.h>
#include <render/qohosnativemouseeventshandler.h>
#include <vector>

namespace ch = std::chrono;

QT_BEGIN_NAMESPACE

namespace {

constexpr auto mouseMotionEventMinAgeForDrop = ch::milliseconds(20);

QOhosOptional<Qt::MouseButton> tryMapNativeNodeMouseButtonToQt(std::int32_t button)
{
    switch (button) {
    case ::UI_MOUSE_EVENT_BUTTON_NONE:
        return makeEmptyQOhosOptional();
    case ::UI_MOUSE_EVENT_BUTTON_LEFT:
        return makeQOhosOptional(Qt::LeftButton);
    case ::UI_MOUSE_EVENT_BUTTON_RIGHT:
        return makeQOhosOptional(Qt::RightButton);
    case ::UI_MOUSE_EVENT_BUTTON_MIDDLE:
        return makeQOhosOptional(Qt::MiddleButton);
    case ::UI_MOUSE_EVENT_BUTTON_BACK:
        return makeQOhosOptional(Qt::BackButton);
    case ::UI_MOUSE_EVENT_BUTTON_FORWARD:
        return makeQOhosOptional(Qt::ForwardButton);
    }

    return makeEmptyQOhosOptional();
};

QOhosOptional<QEvent::Type> tryMapNativeNodeMouseActionToQt(std::int32_t action)
{
    switch (action) {
    case ::UI_MOUSE_EVENT_ACTION_UNKNOWN:
        return makeEmptyQOhosOptional();
    case ::UI_MOUSE_EVENT_ACTION_PRESS:
        return makeQOhosOptional(QEvent::MouseButtonPress);
    case ::UI_MOUSE_EVENT_ACTION_RELEASE:
        return makeQOhosOptional(QEvent::MouseButtonRelease);
    case ::UI_MOUSE_EVENT_ACTION_MOVE:
        return makeQOhosOptional(QEvent::MouseMove);
    }

    return makeEmptyQOhosOptional();
}

class QOhosNativeNodeMouseInputHandler final : public std::enable_shared_from_this<QOhosNativeNodeMouseInputHandler>
{
public:
    QOhosNativeNodeMouseInputHandler(
        QtOhos::QThreadSafeRef<QWindow> qWindowRef,
        QtOhos::QThreadSafeRef<QOhosInputMethodEventHandler> imEventHandlerRef,
        std::shared_ptr<QOhosHoverEventsGenerator> hoverEventsGenerator);

    void handleMouseEvent(QArkUi::NativeNodeMouseEvent nativeNodeMouseEvent);

private:
    struct MouseEvent
    {
        ch::steady_clock::time_point timestamp;
        QOhosMouseEvent mouseEvent;
    };

    void processMouseEventsInQtThread(std::vector<MouseEvent> &&batch);

    static bool mayDropMouseEvent(
        ch::steady_clock::time_point now, const MouseEvent &event, const MouseEvent &nextEvent);

    QtOhos::QThreadSafeRef<QWindow> m_qWindowRef;
    QtOhos::QThreadSafeRef<QOhosInputMethodEventHandler> m_imEventHandlerRef;
    std::shared_ptr<QOhosHoverEventsGenerator> m_hoverEventsGenerator;

    std::function<void(std::function<void(std::vector<MouseEvent> &)>)> m_optMouseEventsHandler;
};

QOhosNativeNodeMouseInputHandler::QOhosNativeNodeMouseInputHandler(
    QtOhos::QThreadSafeRef<QWindow> qWindowRef,
    QtOhos::QThreadSafeRef<QOhosInputMethodEventHandler> imEventHandlerRef,
    std::shared_ptr<QOhosHoverEventsGenerator> hoverEventsGenerator)
    : m_qWindowRef(qWindowRef)
    , m_imEventHandlerRef(imEventHandlerRef)
    , m_hoverEventsGenerator(hoverEventsGenerator)
{
}

void QOhosNativeNodeMouseInputHandler::handleMouseEvent(QArkUi::NativeNodeMouseEvent nativeNodeMouseEvent)
{
    auto eventType = tryMapNativeNodeMouseActionToQt(nativeNodeMouseEvent.action);
    if (!eventType.hasValue()) {
        qOhosPrintfDebug(
            "%s: got unsupported action in mouse event (%d), ignoring",
            Q_FUNC_INFO, nativeNodeMouseEvent.action);
        return;
    }

    QOhosMouseEvent mouseEvent = {
        .timestampMs = nativeNodeMouseEvent.timestampMs,
        .localPosition = nativeNodeMouseEvent.localPosition,
        .globalPosition = nativeNodeMouseEvent.displayPosition,
        .button = tryMapNativeNodeMouseButtonToQt(nativeNodeMouseEvent.button).valueOr(Qt::NoButton),
        .eventType = eventType.value(),
        .modifiers = nativeNodeMouseEvent.modifiers,
    };

    m_hoverEventsGenerator->handleQOhosMouseEvent(mouseEvent);

    if (!m_optMouseEventsHandler) {
        auto weakSelf = QtOhos::makeWeakPtr(shared_from_this());
        m_optMouseEventsHandler = makeQtOhosBatchingQtRequestsHandler<std::vector<MouseEvent>>(
            m_imEventHandlerRef.toQObjectThreadSafeRef(),
            [weakSelf](std::vector<MouseEvent> &&batch) {
                auto sharedSelf = weakSelf.lock();
                if (sharedSelf)
                    sharedSelf->processMouseEventsInQtThread(std::move(batch));
            });
    }

    m_optMouseEventsHandler(
        [&](std::vector<MouseEvent> &batch) {
            auto now = std::chrono::steady_clock::now();
            MouseEvent newEvent{now, mouseEvent};
            if (!batch.empty() && mayDropMouseEvent(now, batch.back(), newEvent))
                batch.pop_back();
            batch.push_back(newEvent);
        });
}

void QOhosNativeNodeMouseInputHandler::processMouseEventsInQtThread(std::vector<MouseEvent> &&batch)
{
    auto now = ch::steady_clock::now();

    QWindow *targetWindow = m_qWindowRef.data();
    if (targetWindow == nullptr) {
        qOhosPrintfWarning(
            "%s: QWindow reference '%s' for node not valid. Rejecting mouse events batch.",
            Q_FUNC_INFO, m_qWindowRef.refName().c_str());
        return;
    }

    QOhosInputMethodEventHandler *eventHandler = m_imEventHandlerRef.data();
    if (eventHandler == nullptr) {
        qOhosPrintfWarning(
            "%s: Input method event handler referece '%s' for node not valid. Rejecting mouse events batch.",
            Q_FUNC_INFO, m_qWindowRef.refName().c_str());
        return;
    }

    for (std::size_t i = 0; i < batch.size(); ++i) {
        if (i + 1 < batch.size() && mayDropMouseEvent(now, batch[i], batch[i + 1]))
            continue;

        auto mouseEvent = batch[i].mouseEvent;
        mouseEvent.targetWindow = targetWindow;
        auto *platformScreen = static_cast<QOhosPlatformScreen *>(mouseEvent.targetWindow->screen()->handle());
        mouseEvent.localPosition = QHighDpi::toNative(mouseEvent.localPosition, platformScreen->pixelScalingCoefficient());
        mouseEvent.globalPosition = QHighDpi::toNative(mouseEvent.globalPosition, platformScreen->pixelScalingCoefficient());
        eventHandler->onMouseEvent(mouseEvent);
    }
}

bool QOhosNativeNodeMouseInputHandler::mayDropMouseEvent(
    ch::steady_clock::time_point now, const MouseEvent &event, const MouseEvent &nextEvent)
{
    return
        now - event.timestamp >= mouseMotionEventMinAgeForDrop
        && event.mouseEvent.eventType == QEvent::MouseMove
        && nextEvent.mouseEvent.eventType == QEvent::MouseMove;
}

}

QOhosConsumer<QArkUi::NativeNodeMouseEvent> makeQOhosNativeMouseEventsHandler(
    QtOhos::QThreadSafeRef<QWindow> qWindowRef,
    QtOhos::QThreadSafeRef<QOhosInputMethodEventHandler> imEventHandlerRef,
    std::shared_ptr<QOhosHoverEventsGenerator> hoverEventsGenerator)
{
    auto mouseInputHandler = std::make_shared<QOhosNativeNodeMouseInputHandler>(qWindowRef, imEventHandlerRef, hoverEventsGenerator);
    return [mouseInputHandler](QArkUi::NativeNodeMouseEvent nativeNodeMouseEvent) {
        mouseInputHandler->handleMouseEvent(nativeNodeMouseEvent);
    };
}

QT_END_NAMESPACE
