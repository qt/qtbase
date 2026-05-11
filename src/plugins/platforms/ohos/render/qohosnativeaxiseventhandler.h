// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSNATIVEAXISEVENTHANDLER_H
#define QOHOSNATIVEAXISEVENTHANDLER_H

#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qglobal.h>
#include <QtGui/qwindow.h>
#include <arkui/ui_input_event.h>
#include <qohosinputmethodeventhandler.h>
#include <qohosplugincore.h>

QT_BEGIN_NAMESPACE

QOhosConsumer<::ArkUI_UIInputEvent *> makeQOhosNativeAxisEventHandler(
    QtOhos::QThreadSafeRef<QWindow> qWindowRef,
    QtOhos::QThreadSafeRef<QOhosInputMethodEventHandler> imEventHandlerRef);

QT_END_NAMESPACE

#endif
