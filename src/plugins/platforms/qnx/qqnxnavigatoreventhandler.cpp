/***************************************************************************
**
** Copyright (C) 2012 Research In Motion
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqnxnavigatoreventhandler.h"

#include <QDebug>
#include <QGuiApplication>
#include <QWindowSystemInterface>

QT_BEGIN_NAMESPACE

QQnxNavigatorEventHandler::QQnxNavigatorEventHandler(QObject *parent)
    : QObject(parent)
{
}

bool QQnxNavigatorEventHandler::handleOrientationCheck(int angle)
{
    // reply to navigator that (any) orientation is acceptable
#if defined(QQNXNAVIGATOREVENTHANDLER_DEBUG)
    qDebug() << Q_FUNC_INFO << "angle=" << angle;
#endif

    // TODO: check if top window flags prohibit orientation change
    return true;
}

void QQnxNavigatorEventHandler::handleOrientationChange(int angle)
{
    // update screen geometry and reply to navigator that we're ready
#if defined(QQNXNAVIGATOREVENTHANDLER_DEBUG)
    qDebug() << Q_FUNC_INFO << "angle=" << angle;
#endif

    emit rotationChanged(angle);
}

void QQnxNavigatorEventHandler::handleSwipeDown()
{
    // simulate menu key press
#if defined(QQNXNAVIGATOREVENTHANDLER_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    QWindow *w = QGuiApplication::focusWindow();
    QWindowSystemInterface::handleKeyEvent(w, QEvent::KeyPress, Qt::Key_Menu, Qt::NoModifier);
    QWindowSystemInterface::handleKeyEvent(w, QEvent::KeyRelease, Qt::Key_Menu, Qt::NoModifier);
}

void QQnxNavigatorEventHandler::handleExit()
{
    // shutdown everything
#if defined(QQNXNAVIGATOREVENTHANDLER_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    QCoreApplication::quit();
}

void QQnxNavigatorEventHandler::handleWindowGroupActivated(const QByteArray &id)
{
#if defined(QQNXNAVIGATOREVENTHANDLER_DEBUG)
    qDebug() << Q_FUNC_INFO << id;
#endif

    Q_EMIT windowGroupActivated(id);
}

void QQnxNavigatorEventHandler::handleWindowGroupDeactivated(const QByteArray &id)
{
#if defined(QQNXNAVIGATOREVENTHANDLER_DEBUG)
    qDebug() << Q_FUNC_INFO << id;
#endif

    Q_EMIT windowGroupDeactivated(id);
}

QT_END_NAMESPACE
