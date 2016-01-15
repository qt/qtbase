/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qplatformnativeinterface.h"

QT_BEGIN_NAMESPACE

/*!
    \class QPlatformNativeInterface
    \since 5.0
    \internal
    \preliminary
    \ingroup qpa

    \brief The QPlatformNativeInterface class provides an abstraction for retrieving native
    resource handles.
 */

void *QPlatformNativeInterface::nativeResourceForIntegration(const QByteArray &resource)
{
    Q_UNUSED(resource);
    return 0;
}

void *QPlatformNativeInterface::nativeResourceForScreen(const QByteArray &resource, QScreen *screen)
{
    Q_UNUSED(resource);
    Q_UNUSED(screen);
    return 0;
}

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

#ifndef QT_NO_CURSOR
void *QPlatformNativeInterface::nativeResourceForCursor(const QByteArray &resource, const QCursor &cursor)
{
    Q_UNUSED(resource);
    Q_UNUSED(cursor);
    return Q_NULLPTR;
}
#endif // !QT_NO_CURSOR

QPlatformNativeInterface::NativeResourceForIntegrationFunction QPlatformNativeInterface::nativeResourceFunctionForIntegration(const QByteArray &resource)
{
    Q_UNUSED(resource);
    return 0;
}

QPlatformNativeInterface::NativeResourceForContextFunction QPlatformNativeInterface::nativeResourceFunctionForContext(const QByteArray &resource)
{
    Q_UNUSED(resource);
    return 0;
}

QPlatformNativeInterface::NativeResourceForScreenFunction QPlatformNativeInterface::nativeResourceFunctionForScreen(const QByteArray &resource)
{
    Q_UNUSED(resource);
    return 0;
}

QPlatformNativeInterface::NativeResourceForWindowFunction QPlatformNativeInterface::nativeResourceFunctionForWindow(const QByteArray &resource)
{
    Q_UNUSED(resource);
    return 0;
}

QPlatformNativeInterface::NativeResourceForBackingStoreFunction QPlatformNativeInterface::nativeResourceFunctionForBackingStore(const QByteArray &resource)
{
    Q_UNUSED(resource);
    return 0;
}

QFunctionPointer QPlatformNativeInterface::platformFunction(const QByteArray &function) const
{
    Q_UNUSED(function);
    return Q_NULLPTR;
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

QT_END_NAMESPACE
