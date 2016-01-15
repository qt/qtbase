/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qplatformdefs.h"
#include "qcoreapplication.h"
#include "qunixeventdispatcher_qpa_p.h"
#include "private/qguiapplication_p.h"

#include <qpa/qwindowsysteminterface.h>
#include <QtCore/QElapsedTimer>
#include <QtCore/QAtomicInt>
#include <QtCore/QSemaphore>

#include <QtCore/QDebug>

#include <errno.h>

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

bool QUnixEventDispatcherQPA::hasPendingEvents()
{
    extern uint qGlobalPostedEventsCount(); // from qapplication.cpp
    return qGlobalPostedEventsCount() || QWindowSystemInterface::windowSystemEventsQueued();
}

void QUnixEventDispatcherQPA::flush()
{
    if(qApp)
        qApp->sendPostedEvents();
}

QT_END_NAMESPACE
