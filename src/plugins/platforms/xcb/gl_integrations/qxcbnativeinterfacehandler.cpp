// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qxcbnativeinterfacehandler.h"

#include "qxcbnativeinterface.h"

QT_BEGIN_NAMESPACE

QXcbNativeInterfaceHandler::QXcbNativeInterfaceHandler(QXcbNativeInterface *nativeInterface)
    : m_native_interface(nativeInterface)
{
    m_native_interface->addHandler(this);
}
QXcbNativeInterfaceHandler::~QXcbNativeInterfaceHandler()
{
    m_native_interface->removeHandler(this);
}

QPlatformNativeInterface::NativeResourceForIntegrationFunction QXcbNativeInterfaceHandler::nativeResourceFunctionForIntegration(const QByteArray &resource) const
{
    Q_UNUSED(resource);
    return nullptr;
}

QPlatformNativeInterface::NativeResourceForContextFunction QXcbNativeInterfaceHandler::nativeResourceFunctionForContext(const QByteArray &resource) const
{
    Q_UNUSED(resource);
    return nullptr;
}

QPlatformNativeInterface::NativeResourceForScreenFunction QXcbNativeInterfaceHandler::nativeResourceFunctionForScreen(const QByteArray &resource) const
{
    Q_UNUSED(resource);
    return nullptr;
}

QPlatformNativeInterface::NativeResourceForWindowFunction QXcbNativeInterfaceHandler::nativeResourceFunctionForWindow(const QByteArray &resource) const
{
    Q_UNUSED(resource);
    return nullptr;
}

QPlatformNativeInterface::NativeResourceForBackingStoreFunction QXcbNativeInterfaceHandler::nativeResourceFunctionForBackingStore(const QByteArray &resource) const
{
    Q_UNUSED(resource);
    return nullptr;
}

QFunctionPointer QXcbNativeInterfaceHandler::platformFunction(const QByteArray &function) const
{
    Q_UNUSED(function);
    return nullptr;
}

QT_END_NAMESPACE
