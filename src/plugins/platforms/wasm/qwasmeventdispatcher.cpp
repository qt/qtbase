// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmeventdispatcher.h"
#include "qwasmintegration.h"

#include <QtGui/qpa/qwindowsysteminterface.h>

QT_BEGIN_NAMESPACE

// Note: All event dispatcher functionality is implemented in QEventDispatcherWasm
// in QtCore, except for processPostedEvents() below which uses API from QtGui.
bool QWasmEventDispatcher::processPostedEvents()
{
    QEventDispatcherWasm::processPostedEvents();
    return QWindowSystemInterface::sendWindowSystemEvents(QEventLoop::AllEvents);
}

void QWasmEventDispatcher::onLoaded()
{
    // This function is called when the application is ready to paint
    // the first frame. Send the qtlaoder onLoaded event first (via
    // the base class implementation), and then enable/call requestUpdate
    // to deliver a frame.
    QEventDispatcherWasm::onLoaded();

    // Make sure all screens have a defined size; and pick
    // up size changes due to onLoaded event handling.
    QWasmIntegration *wasmIntegration = QWasmIntegration::get();
    wasmIntegration->resizeAllScreens();

    wasmIntegration->releaseRequesetUpdateHold();
}

QT_END_NAMESPACE
