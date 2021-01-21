/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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
