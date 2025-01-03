// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmeventdispatcher.h"

#include <QtGui/qpa/qwindowsysteminterface.h>

QT_BEGIN_NAMESPACE

// Note: All event dispatcher functionality is implemented in QEventDispatcherWasm
// in QtCore, except for processWindowSystemEvents() below which uses API from QtGui.
void QWasmEventDispatcher::processWindowSystemEvents(QEventLoop::ProcessEventsFlags flags)
{
    QWindowSystemInterface::sendWindowSystemEvents(flags);
}

QT_END_NAMESPACE
