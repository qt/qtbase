// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSNATIVEXCOMPONENTINPUTHANDLER_H
#define QOHOSNATIVEXCOMPONENTINPUTHANDLER_H

#include <QtCore/qpointer.h>
#include <QtCore/qglobal.h>
#include <QtGui/qwindow.h>
#include <QtGui/qinputdevice.h>
#include <ace/xcomponent/native_interface_xcomponent.h>
#include <arkui/ui_input_event.h>
#include <qohosinputmethodeventhandler.h>
#include <qohosplugincore.h>
#include <render/qohosnativegestureshandler.h>
#include <render/qxcomponent.h>

QT_BEGIN_NAMESPACE

class QOhosNativeXComponentInputHandler final
    : public QEnableSharedFromThis<QOhosNativeXComponentInputHandler>
{
public:
    explicit QOhosNativeXComponentInputHandler(
        QXComponentRender xcomponent,
        QtOhos::QThreadSafeRef<QWindow> qWindowRef,
        QtOhos::QThreadSafeRef<QOhosInputMethodEventHandler> imEventHandlerRef);

    void handleTouchEvent(void *nativeWindow);
    void handleMouseEvent(void *nativeWindow);
    void handleHoverEvent(void *nativeWindow, bool isHover);
    void handleKeyEvent();

private:
    struct MouseEvent
    {
        std::chrono::steady_clock::time_point timestamp;
        ::OH_NativeXComponent_MouseEvent event;
    };

    struct TouchEvent
    {
        std::chrono::steady_clock::time_point timestamp;
        std::chrono::nanoseconds touchTimeStamp;
        std::vector<QOhosTouchEventTouchPointData> touchPoints;
        QInputDevice::DeviceType deviceType;
        QFlags<OhosKeyboardModifier> modifiers;
    };

    static bool mayDropMouseEvent(
        std::chrono::steady_clock::time_point now,
        const MouseEvent &event, const MouseEvent &nextEvent);

    void processMouseEventsInQtThread(std::vector<MouseEvent> &&batch);
    void processTouchEventsInQtThread(std::vector<TouchEvent> &&batch);
    void invokeMethodLaterIfSelfExists(
        QtOhos::QObjectThreadSafeRef ctxRef,
        std::function<void(QOhosNativeXComponentInputHandler &component)>);

    QXComponentRender m_xComponent;
    QtOhos::QThreadSafeRef<QWindow> m_qWindowRef;
    QtOhos::QThreadSafeRef<QOhosInputMethodEventHandler> m_imEventHandlerRef;

    std::function<void(std::function<void(std::vector<MouseEvent> &)>)> m_optMouseEventsHandler;
    QOhosConsumer<TouchEvent> m_optTouchEventsHandler;

    std::shared_ptr<QOhosKeyEvent> m_lastKeyEvent;
};

QT_END_NAMESPACE

#endif
