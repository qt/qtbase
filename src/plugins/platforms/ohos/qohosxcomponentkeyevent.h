// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSXCOMPONENTKEYEVENT_H
#define QOHOSXCOMPONENTKEYEVENT_H

#include <QtCore/qflags.h>
#include <QtCore/qglobal.h>
#include <ace/xcomponent/native_interface_xcomponent.h>
#include <qohoskeyevent.h>
#include <qohoskeyeventconverthelpers.h>

QT_BEGIN_NAMESPACE

std::shared_ptr<QOhosKeyEvent> makeQOhosXComponentKeyEvent(
    ::OH_NativeXComponent_KeyAction keyAction, ::OH_NativeXComponent_KeyCode keyCode,
    QFlags<OhosKeyboardModifier> keysFlags);

QT_END_NAMESPACE

#endif // QOHOSXCOMPONENTKEYEVENT_H
