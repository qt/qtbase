// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformnativeinterface.h"
#include <QtCore/qvariant.h>
#include <QtCore/qmap.h>
#include <QtGui/qcursor.h>

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
    return nullptr;
}

void *QPlatformNativeInterface::nativeResourceForScreen(const QByteArray &resource, QScreen *screen)
{
    Q_UNUSED(resource);
    Q_UNUSED(screen);
    return nullptr;
}

void *QPlatformNativeInterface::nativeResourceForWindow(const QByteArray &resource, QWindow *window)
{
    Q_UNUSED(resource);
    Q_UNUSED(window);
    return nullptr;
}

void *QPlatformNativeInterface::nativeResourceForContext(const QByteArray &resource, QOpenGLContext *context)
{
    Q_UNUSED(resource);
    Q_UNUSED(context);
    return nullptr;
}

void * QPlatformNativeInterface::nativeResourceForBackingStore(const QByteArray &resource, QBackingStore *backingStore)
{
    Q_UNUSED(resource);
    Q_UNUSED(backingStore);
    return nullptr;
}

#ifndef QT_NO_CURSOR
void *QPlatformNativeInterface::nativeResourceForCursor(const QByteArray &resource, const QCursor &cursor)
{
    Q_UNUSED(resource);
    Q_UNUSED(cursor);
    return nullptr;
}
#endif // !QT_NO_CURSOR

QPlatformNativeInterface::NativeResourceForIntegrationFunction QPlatformNativeInterface::nativeResourceFunctionForIntegration(const QByteArray &resource)
{
    Q_UNUSED(resource);
    return nullptr;
}

QPlatformNativeInterface::NativeResourceForContextFunction QPlatformNativeInterface::nativeResourceFunctionForContext(const QByteArray &resource)
{
    Q_UNUSED(resource);
    return nullptr;
}

QPlatformNativeInterface::NativeResourceForScreenFunction QPlatformNativeInterface::nativeResourceFunctionForScreen(const QByteArray &resource)
{
    Q_UNUSED(resource);
    return nullptr;
}

QPlatformNativeInterface::NativeResourceForWindowFunction QPlatformNativeInterface::nativeResourceFunctionForWindow(const QByteArray &resource)
{
    Q_UNUSED(resource);
    return nullptr;
}

QPlatformNativeInterface::NativeResourceForBackingStoreFunction QPlatformNativeInterface::nativeResourceFunctionForBackingStore(const QByteArray &resource)
{
    Q_UNUSED(resource);
    return nullptr;
}

QFunctionPointer QPlatformNativeInterface::platformFunction(const QByteArray &function) const
{
    Q_UNUSED(function);
    return nullptr;
}

/*!
    Contains generic window properties that the platform may utilize.
*/
QVariantMap QPlatformNativeInterface::windowProperties(QPlatformWindow *window) const
{
    Q_UNUSED(window);
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

#include "moc_qplatformnativeinterface.cpp"
