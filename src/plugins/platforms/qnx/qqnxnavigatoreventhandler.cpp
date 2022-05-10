// Copyright (C) 2012 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
