/***************************************************************************
**
** Copyright (C) 2012 Research In Motion
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

#include "qqnxnavigatoreventhandler.h"

#include "qqnxintegration.h"
#include "qqnxscreen.h"

#include <QDebug>
#include <QGuiApplication>
#include <qpa/qwindowsysteminterface.h>

#if defined(QQNXNAVIGATOREVENTHANDLER_DEBUG)
#define qNavigatorEventHandlerDebug qDebug
#else
#define qNavigatorEventHandlerDebug QT_NO_QDEBUG_MACRO
#endif

QT_BEGIN_NAMESPACE

QQnxNavigatorEventHandler::QQnxNavigatorEventHandler(QObject *parent)
    : QObject(parent)
{
}

bool QQnxNavigatorEventHandler::handleOrientationCheck(int angle)
{
    // reply to navigator that (any) orientation is acceptable
    // TODO: check if top window flags prohibit orientation change
    qNavigatorEventHandlerDebug("angle=%d", angle);
    return true;
}

void QQnxNavigatorEventHandler::handleOrientationChange(int angle)
{
    // update screen geometry and reply to navigator that we're ready
    qNavigatorEventHandlerDebug("angle=%d", angle);
    emit rotationChanged(angle);
}

void QQnxNavigatorEventHandler::handleSwipeDown()
{
    qNavigatorEventHandlerDebug();

    Q_EMIT swipeDown();
}

void QQnxNavigatorEventHandler::handleExit()
{
    // shutdown everything
    qNavigatorEventHandlerDebug();
    QCoreApplication::quit();
}

void QQnxNavigatorEventHandler::handleWindowGroupActivated(const QByteArray &id)
{
    qNavigatorEventHandlerDebug() << id;
    Q_EMIT windowGroupActivated(id);
}

void QQnxNavigatorEventHandler::handleWindowGroupDeactivated(const QByteArray &id)
{
    qNavigatorEventHandlerDebug() << id;
    Q_EMIT windowGroupDeactivated(id);
}

void QQnxNavigatorEventHandler::handleWindowGroupStateChanged(const QByteArray &id, Qt::WindowState state)
{
    qNavigatorEventHandlerDebug() << id;
    Q_EMIT windowGroupStateChanged(id, state);
}

QT_END_NAMESPACE
