// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSNATIVENODEKEYEVENT_H
#define QOHOSNATIVENODEKEYEVENT_H

#include <QtCore/qflags.h>
#include <QtCore/qglobal.h>
#include <arkui/native_key_event.h>
#include <qohoskeyevent.h>
#include <qohoskeyeventconverthelpers.h>

QT_BEGIN_NAMESPACE

std::shared_ptr<QOhosKeyEvent> makeQOhosNativeNodeKeyEvent(
    ::ArkUI_KeyEventType keyEventType, ::ArkUI_KeyCode keyCode, QFlags<OhosKeyboardModifier> keysFlags);

std::shared_ptr<QOhosKeyEvent> makeEmptyQOhosNativeNodeKeyEvent();

QT_END_NAMESPACE

#endif // QOHOSNATIVENODEKEYEVENT_H
