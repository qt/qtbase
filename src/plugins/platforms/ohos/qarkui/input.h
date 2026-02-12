// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QARKUI__INPUT_H
#define QARKUI__INPUT_H

#include <QtCore/qglobal.h>
#include <QtGui/qinputdevice.h>
#include <arkui/ui_input_event.h>
#include <multimodalinput/oh_input_manager.h>
#include <qarkui/window.h>
#include <qohosdisplayinfo.h>
#include <qohosplugincore.h>

QT_BEGIN_NAMESPACE

namespace QArkUi {

struct MouseEvent
{
    JsWindowId jsWindowId;
    QOhosDisplayInfo::JsDisplayId jsDisplayId;
    ::Input_MouseEventButton button;
    ::Input_MouseEventAction action;
    QPoint displayPosition;
    std::chrono::microseconds actionTime;

    static QOhosOptional<MouseEvent> createFromNativeEvent(const ::Input_MouseEvent *event);
};

struct KeyEvent
{
    JsWindowId jsWindowId;
    QOhosDisplayInfo::JsDisplayId jsDisplayId;
    std::chrono::microseconds actionTime;
    ::Input_KeyEventAction action;
    std::int32_t keyCode;

    static QOhosOptional<KeyEvent> createFromNativeEvent(const ::Input_KeyEvent *event);
};

struct TouchEvent
{
    JsWindowId jsWindowId;
    QOhosDisplayInfo::JsDisplayId jsDisplayId;
    QPoint displayPosition;
    ::Input_TouchEventAction action;
    std::int32_t fingerId;
    std::chrono::microseconds actionTime;

    static QOhosOptional<TouchEvent> createFromNativeEvent(const ::Input_TouchEvent *event);
};

QInputDevice::DeviceType getTouchDeviceType(const ::ArkUI_UIInputEvent *inputEvent);

}

QT_END_NAMESPACE

#endif
