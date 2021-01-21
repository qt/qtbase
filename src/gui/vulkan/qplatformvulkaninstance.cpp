/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qplatformvulkaninstance.h"

QT_BEGIN_NAMESPACE

/*!
    \class QPlatformVulkanInstance
    \since 5.10
    \internal
    \preliminary
    \ingroup qpa

    \brief The QPlatformVulkanInstance class provides an abstraction for Vulkan instances.

    The platform Vulkan instance is responsible for loading a Vulkan library,
    resolving the basic entry points for creating instances, providing support
    for creating new or adopting existing VkInstances, and abstracting some
    WSI-specifics like checking if a given queue family can be used to present
    using a given window.

    \note platform plugins will typically subclass not this class, but rather
    QBasicVulkanPlatformInstance.

    \note Vulkan instance creation is split into two phases: a new
    QPlatformVulkanInstance is expected to load the Vulkan library and do basic
    initialization, after which the supported layers and extensions can be
    queried. Everything else is deferred into createOrAdoptInstance().
*/

class QPlatformVulkanInstancePrivate
{
public:
    QPlatformVulkanInstancePrivate() { }
};

QPlatformVulkanInstance::QPlatformVulkanInstance()
    : d_ptr(new QPlatformVulkanInstancePrivate)
{
}

QPlatformVulkanInstance::~QPlatformVulkanInstance()
{
}

void QPlatformVulkanInstance::presentAboutToBeQueued(QWindow *window)
{
    Q_UNUSED(window);
}

void QPlatformVulkanInstance::presentQueued(QWindow *window)
{
    Q_UNUSED(window);
}

void QPlatformVulkanInstance::setDebugFilters(const QVector<QVulkanInstance::DebugFilter> &filters)
{
    Q_UNUSED(filters);
}

QT_END_NAMESPACE
