// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSNATIVEMOUSEEVENTSHANDLER_H
#define QOHOSNATIVEMOUSEEVENTSHANDLER_H

#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qglobal.h>
#include <QtGui/qwindow.h>
#include <qarkui/input.h>
#include <qohosinputmethodeventhandler.h>
#include <qohosplugincore.h>
#include <render/qohoshovereventsgenerator.h>

QT_BEGIN_NAMESPACE

QOhosConsumer<QArkUi::NativeNodeMouseEvent> makeQOhosNativeMouseEventsHandler(
    QtOhos::QThreadSafeRef<QWindow> qWindowRef,
    QtOhos::QThreadSafeRef<QOhosInputMethodEventHandler> imEventHandlerRef,
    std::shared_ptr<QOhosHoverEventsGenerator> hoverEventsGenerator);

QT_END_NAMESPACE

#endif
