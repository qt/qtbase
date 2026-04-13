// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwasmeventdispatcher.h"
#include "qwasmintegration.h"

#include <QtGui/qpa/qwindowsysteminterface.h>

QT_BEGIN_NAMESPACE

QWasmEventDispatcher::QWasmEventDispatcher()
    :QEventDispatcherWasm()
{

}

// Note: All event dispatcher functionality is implemented in QEventDispatcherWasm
// in QtCore, except for processPostedEvents() below which uses API from QtGui.
bool QWasmEventDispatcher::sendPostedEvents()
{
    QEventDispatcherWasm::sendPostedEvents();
    return QWindowSystemInterface::sendWindowSystemEvents(QEventLoop::AllEvents);
}

QT_END_NAMESPACE
