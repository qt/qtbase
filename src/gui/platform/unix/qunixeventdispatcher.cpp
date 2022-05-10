// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformdefs.h"
#include "qcoreapplication.h"
#include "qunixeventdispatcher_qpa_p.h"
#include "private/qguiapplication_p.h"

#include <qpa/qwindowsysteminterface.h>

#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

QT_USE_NAMESPACE


QUnixEventDispatcherQPA::QUnixEventDispatcherQPA(QObject *parent)
    : QEventDispatcherUNIX(parent)
{ }

QUnixEventDispatcherQPA::~QUnixEventDispatcherQPA()
{ }

bool QUnixEventDispatcherQPA::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    const bool didSendEvents = QEventDispatcherUNIX::processEvents(flags);
    return QWindowSystemInterface::sendWindowSystemEvents(flags) || didSendEvents;
}

QT_END_NAMESPACE
