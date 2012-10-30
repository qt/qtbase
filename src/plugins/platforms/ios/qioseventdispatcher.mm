/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

/****************************************************************************
**
** Copyright (c) 2007-2008, Apple, Inc.
**
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**
**   * Redistributions of source code must retain the above copyright notice,
**     this list of conditions and the following disclaimer.
**
**   * Redistributions in binary form must reproduce the above copyright notice,
**     this list of conditions and the following disclaimer in the documentation
**     and/or other materials provided with the distribution.
**
**   * Neither the name of Apple, Inc. nor the names of its contributors
**     may be used to endorse or promote products derived from this software
**     without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
** CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
** EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
** PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
** PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
** LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
** NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
****************************************************************************/

#include "qioseventdispatcher.h"
#include <qdebug.h>

QT_BEGIN_NAMESPACE
QT_USE_NAMESPACE

QIOSEventDispatcher::QIOSEventDispatcher(QObject *parent)
{
    Q_UNUSED(parent);
    qDebug() << __FUNCTION__ << "eventdispatcher not implemented";
}

QIOSEventDispatcher::~QIOSEventDispatcher()
{}

bool QIOSEventDispatcher::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    Q_UNUSED(flags);
    return false;
}

bool QIOSEventDispatcher::hasPendingEvents()
{
    return false;
}

void QIOSEventDispatcher::registerSocketNotifier(QSocketNotifier *notifier)
{
    Q_UNUSED(notifier);
}

void QIOSEventDispatcher::unregisterSocketNotifier(QSocketNotifier *notifier)
{
    Q_UNUSED(notifier);
}

void QIOSEventDispatcher::registerTimer(int timerId, int interval, Qt::TimerType timerType, QObject *object)
{
    Q_UNUSED(timerId);
    Q_UNUSED(interval);
    Q_UNUSED(timerType);
    Q_UNUSED(object);
}

bool QIOSEventDispatcher::unregisterTimer(int timerId)
{
    Q_UNUSED(timerId);
    return false;
}

bool QIOSEventDispatcher::unregisterTimers(QObject *object)
{
    Q_UNUSED(object);
    return false;
}

QList<QAbstractEventDispatcher::TimerInfo> QIOSEventDispatcher::registeredTimers(QObject *object) const
{
    Q_UNUSED(object);
    return QList<TimerInfo>();
}

int QIOSEventDispatcher::remainingTime(int timerId)
{
    Q_UNUSED(timerId);
    return 0;
}

void QIOSEventDispatcher::wakeUp()
{}

void QIOSEventDispatcher::interrupt()
{}

void QIOSEventDispatcher::flush()
{}

QT_END_NAMESPACE

