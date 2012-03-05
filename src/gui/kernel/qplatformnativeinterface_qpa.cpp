/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qplatformnativeinterface_qpa.h"

QT_BEGIN_NAMESPACE

void *QPlatformNativeInterface::nativeResourceForWindow(const QByteArray &resource, QWindow *window)
{
    Q_UNUSED(resource);
    Q_UNUSED(window);
    return 0;
}

void *QPlatformNativeInterface::nativeResourceForContext(const QByteArray &resource, QOpenGLContext *context)
{
    Q_UNUSED(resource);
    Q_UNUSED(context);
    return 0;
}

void * QPlatformNativeInterface::nativeResourceForBackingStore(const QByteArray &resource, QBackingStore *backingStore)
{
    Q_UNUSED(resource);
    Q_UNUSED(backingStore);
    return 0;
}

/*!
    Contains generic window properties that the platform may utilize.
*/
QVariantMap QPlatformNativeInterface::windowProperties(QPlatformWindow *window) const
{
    Q_UNUSED(window)
    return QVariantMap();
}

/*!
    Returns a window property with \a name.

    If the property does not exist, returns a default-constructed value.
*/
QVariant QPlatformNativeInterface::windowProperty(QPlatformWindow *window, const QString &name) const
{
    Q_UNUSED(window);
    Q_UNUSED(name);
    return QVariant();
}

/*!
    Returns a window property with \a name. If the value does not exist, defaultValue is returned.
*/
QVariant QPlatformNativeInterface::windowProperty(QPlatformWindow *window, const QString &name, const QVariant &defaultValue) const
{
    Q_UNUSED(window);
    Q_UNUSED(name);
    Q_UNUSED(defaultValue);
    return QVariant();
}

/*!
    Sets a window property with \a name to \a value.
*/
void QPlatformNativeInterface::setWindowProperty(QPlatformWindow *window, const QString &name, const QVariant &value)
{
    Q_UNUSED(window);
    Q_UNUSED(name);
    Q_UNUSED(value);
}

/*!
    \typedef QPlatformNativeInterface::EventFilter
    \since 5.0

    A function with the following signature that can be used as an
    event filter:

    \code
    bool myEventFilter(void *message, long *result);
    \endcode

    \sa setEventFilter()
*/

/*!
    \fn EventFilter QPlatformNativeInterface::setEventFilter(const QByteArray &eventType, EventFilter filter)
    \since 5.0

    Replaces the event filter function for the native interface with
    \a filter and returns the pointer to the replaced event filter
    function. Only the current event filter function is called. If you
    want to use both filter functions, save the replaced EventFilter
    in a place where you can call it from.

    The event filter function set here is called for all messages
    received from the platform if they are given type \eventType.
    It is \e not called for messages that are not meant for Qt objects.

    The type of event is specific to the platform plugin chosen at run-time.

    The event filter function should return \c true if the message should
    be filtered, (i.e. stopped). It should return \c false to allow
    processing the message to continue.

    By default, no event filter function is set. For example, this function
    returns a null EventFilter the first time it is called.

    \note The filter function here receives native messages,
    for example, MSG or XEvent structs. It is called by the platform plugin.
*/
QPlatformNativeInterface::EventFilter QPlatformNativeInterface::setEventFilter(const QByteArray &eventType, QPlatformNativeInterface::EventFilter filter)
{
    Q_UNUSED(eventType);
    Q_UNUSED(filter);
    return 0;
}

QT_END_NAMESPACE
