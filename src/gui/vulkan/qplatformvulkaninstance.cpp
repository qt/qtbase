// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

void QPlatformVulkanInstance::setDebugFilters(const QList<QVulkanInstance::DebugFilter> &filters)
{
    Q_UNUSED(filters);
}

void QPlatformVulkanInstance::setDebugUtilsFilters(const QList<QVulkanInstance::DebugUtilsFilter> &filters)
{
    Q_UNUSED(filters);
}

void QPlatformVulkanInstance::beginFrame(QWindow *window)
{
    Q_UNUSED(window);
}

void QPlatformVulkanInstance::endFrame(QWindow *window)
{
    Q_UNUSED(window);
}


QT_END_NAMESPACE
