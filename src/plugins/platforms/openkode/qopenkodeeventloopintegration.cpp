/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qopenkodeeventloopintegration.h"

#include <QDebug>

#include <KD/kd.h>
#include <KD/ATX_keyboard.h>

QT_BEGIN_NAMESPACE

static const int QT_EVENT_WAKEUP_EVENTLOOP = KD_EVENT_USER + 1;

void kdprocessevent( const KDEvent *event)
{
    switch (event->type) {
    case KD_EVENT_INPUT:
        qDebug() << "KD_EVENT_INPUT";
        break;
    case KD_EVENT_INPUT_POINTER:
        qDebug() << "KD_EVENT_INPUT_POINTER";
        break;
    case KD_EVENT_WINDOW_CLOSE:
        qDebug() << "KD_EVENT_WINDOW_CLOSE";
        break;
    case KD_EVENT_WINDOWPROPERTY_CHANGE:
        qDebug() << "KD_EVENT_WINDOWPROPERTY_CHANGE";
        qDebug() << event->data.windowproperty.pname;
        break;
    case KD_EVENT_WINDOW_FOCUS:
        qDebug() << "KD_EVENT_WINDOW_FOCUS";
        break;
    case KD_EVENT_WINDOW_REDRAW:
        qDebug() << "KD_EVENT_WINDOW_REDRAW";
        break;
    case KD_EVENT_USER:
        qDebug() << "KD_EVENT_USER";
        break;
    case KD_EVENT_INPUT_KEY_ATX:
        qDebug() << "KD_EVENT_INPUT_KEY_ATX";
        break;
    case QT_EVENT_WAKEUP_EVENTLOOP:
        QPlatformEventLoopIntegration::processEvents();
        break;
    default:
        break;
    }

    kdDefaultEvent(event);

}

QOpenKODEEventLoopIntegration::QOpenKODEEventLoopIntegration()
    : m_quit(false)
{
    m_kdThread = kdThreadSelf();
    kdInstallCallback(&kdprocessevent,QT_EVENT_WAKEUP_EVENTLOOP,this);
}

void QOpenKODEEventLoopIntegration::startEventLoop()
{

    while(!m_quit) {
        qint64 msec = nextTimerEvent();
        const KDEvent *event = kdWaitEvent(msec);
        if (event) {
            kdDefaultEvent(event);
            while ((event = kdWaitEvent(0)) != 0) {
                kdDefaultEvent(event);
            }
        }
        QPlatformEventLoopIntegration::processEvents();
    }
}

void QOpenKODEEventLoopIntegration::quitEventLoop()
{
    m_quit = true;
}

void QOpenKODEEventLoopIntegration::qtNeedsToProcessEvents()
{
    KDEvent *event = kdCreateEvent();
    event->type = QT_EVENT_WAKEUP_EVENTLOOP;
    event->userptr = this;
    kdPostThreadEvent(event,m_kdThread);
}

QT_END_NAMESPACE
