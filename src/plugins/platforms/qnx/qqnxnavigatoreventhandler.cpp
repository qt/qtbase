// Copyright (C) 2012 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqnxnavigatoreventhandler.h"

#include "qqnxintegration.h"
#include "qqnxscreen.h"

#include <QDebug>
#include <QGuiApplication>
#include <qpa/qwindowsysteminterface.h>

Q_LOGGING_CATEGORY(lcQpaQnxNavigatorEvents, "qt.qpa.qnx.navigator.events");

QT_BEGIN_NAMESPACE

QQnxNavigatorEventHandler::QQnxNavigatorEventHandler(QObject *parent)
    : QObject(parent)
{
}

bool QQnxNavigatorEventHandler::handleOrientationCheck(int angle)
{
    // reply to navigator that (any) orientation is acceptable
    // TODO: check if top window flags prohibit orientation change
    qCDebug(lcQpaQnxNavigatorEvents, "angle=%d", angle);
    return true;
}

void QQnxNavigatorEventHandler::handleOrientationChange(int angle)
{
    // update screen geometry and reply to navigator that we're ready
    qCDebug(lcQpaQnxNavigatorEvents, "angle=%d", angle);
    emit rotationChanged(angle);
}

void QQnxNavigatorEventHandler::handleSwipeDown()
{
    qCDebug(lcQpaQnxNavigatorEvents) << Q_FUNC_INFO;

    Q_EMIT swipeDown();
}

void QQnxNavigatorEventHandler::handleExit()
{
    // shutdown everything
    qCDebug(lcQpaQnxNavigatorEvents) << Q_FUNC_INFO;
    QCoreApplication::quit();
}

void QQnxNavigatorEventHandler::handleWindowGroupActivated(const QByteArray &id)
{
    qCDebug(lcQpaQnxNavigatorEvents) << Q_FUNC_INFO << id;
    Q_EMIT windowGroupActivated(id);
}

void QQnxNavigatorEventHandler::handleWindowGroupDeactivated(const QByteArray &id)
{
    qCDebug(lcQpaQnxNavigatorEvents) << Q_FUNC_INFO << id;
    Q_EMIT windowGroupDeactivated(id);
}

void QQnxNavigatorEventHandler::handleWindowGroupStateChanged(const QByteArray &id, Qt::WindowState state)
{
    qCDebug(lcQpaQnxNavigatorEvents) << Q_FUNC_INFO << id;
    Q_EMIT windowGroupStateChanged(id, state);
}

QT_END_NAMESPACE
