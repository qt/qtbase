// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qvulkandefaultinstance_p.h"
#include <rhi/qrhi.h>
#include <QLoggingCategory>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcGuiVk, "qt.vulkan")

static QVulkanInstance *s_vulkanInstance;
Q_CONSTINIT static QVulkanDefaultInstance::Flags s_vulkanInstanceFlags;

QVulkanDefaultInstance::Flags QVulkanDefaultInstance::flags()
{
    return s_vulkanInstanceFlags;
}

// As always, calling this when hasInstance() is already true has no effect. (unless cleanup() is called)
void QVulkanDefaultInstance::setFlag(Flag flag, bool on)
{
    s_vulkanInstanceFlags.setFlag(flag, on);
}

bool QVulkanDefaultInstance::hasInstance()
{
    return s_vulkanInstance != nullptr;
}

QVulkanInstance *QVulkanDefaultInstance::instance()
{
    if (s_vulkanInstance)
        return s_vulkanInstance;

    s_vulkanInstance = new QVulkanInstance;

    // With a Vulkan implementation >= 1.1 we can check what
    // vkEnumerateInstanceVersion() says and request 1.3/1.2/1.1 based on the
    // result. To prevent future surprises, be conservative and ignore any > 1.3
    // versions for now. For 1.0 implementations nothing will be requested, the
    // default 0 in VkApplicationInfo means 1.0.
    //
    // Vulkan 1.0 is actually sufficient for 99% of Qt Quick (3D)'s
    // functionality. In addition, Vulkan implementations tend to enable 1.1+
    // functionality regardless of the VkInstance API request. However, the
    // validation layer seems to take this fairly seriously, so we should be
    // prepared for using 1.1+ features in a fully correct manner. This also
    // helps custom Vulkan code in applications, which is not under out
    // control; it is ideal if Vulkan 1.1+ versions are usable without
    // requiring such applications to create their own QVulkanInstance just to
    // be able to make an appropriate setApiVersion() call on it.

    const QVersionNumber supportedVersion = s_vulkanInstance->supportedApiVersion();
    if (supportedVersion >= QVersionNumber(1, 3))
        s_vulkanInstance->setApiVersion(QVersionNumber(1, 3));
    else if (supportedVersion >= QVersionNumber(1, 2))
        s_vulkanInstance->setApiVersion(QVersionNumber(1, 2));
    else if (supportedVersion >= QVersionNumber(1, 1))
        s_vulkanInstance->setApiVersion(QVersionNumber(1, 1));
    qCDebug(lcGuiVk) << "QVulkanDefaultInstance: Creating Vulkan instance"
                     << "Requesting Vulkan API" << s_vulkanInstance->apiVersion()
                     << "Instance-level version was reported as" << supportedVersion;

    if (s_vulkanInstanceFlags.testFlag(EnableValidation))
        s_vulkanInstance->setLayers({ "VK_LAYER_KHRONOS_validation" });

    s_vulkanInstance->setExtensions(QRhiVulkanInitParams::preferredInstanceExtensions());

    if (!s_vulkanInstance->create()) {
        qWarning("QVulkanDefaultInstance: Failed to create Vulkan instance");
        delete s_vulkanInstance;
        s_vulkanInstance = nullptr;
    }

    return s_vulkanInstance;
}

void QVulkanDefaultInstance::cleanup()
{
    delete s_vulkanInstance;
    s_vulkanInstance = nullptr;
}

QT_END_NAMESPACE
