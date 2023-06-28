// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmeventdispatcher.h"

#include <QtGui/qpa/qwindowsysteminterface.h>

QT_BEGIN_NAMESPACE

// Note: All event dispatcher functionality is implemented in QEventDispatcherWasm
// in QtCore, except for processPostedEvents() below which uses API from QtGui.
bool QWasmEventDispatcher::processPostedEvents()
{
    QEventDispatcherWasm::processPostedEvents();
    return QWindowSystemInterface::sendWindowSystemEvents(QEventLoop::AllEvents);
}

QT_END_NAMESPACE
