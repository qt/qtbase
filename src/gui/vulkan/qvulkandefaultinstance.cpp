/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "qvulkandefaultinstance_p.h"
#include <private/qrhivulkan_p.h>
#include <QLoggingCategory>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcGuiVk, "qt.vulkan")

static QVulkanInstance *s_vulkanInstance;
static QVulkanDefaultInstance::Flags s_vulkanInstanceFlags;

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
    // vkEnumerateInstanceVersion() says and request 1.2 or 1.1 based on the
    // result. To prevent future surprises, be conservative and ignore any > 1.2
    // versions for now. For 1.0 implementations nothing will be requested, the
    // default 0 in VkApplicationInfo means 1.0.
    //
    // Vulkan 1.0 is actually sufficient for 99% of Qt Quick (3D)'s
    // functionality. In addition, Vulkan implementations tend to enable 1.1 and 1.2
    // functionality regardless of the VkInstance API request. However, the
    // validation layer seems to take this fairly seriously, so we should be
    // prepared for using 1.1 and 1.2 features in a fully correct manner. This also
    // helps custom Vulkan code in applications, which is not under out control; it
    // is ideal if Vulkan 1.1 and 1.2 are usable without requiring such applications
    // to create their own QVulkanInstance just to be able to make an appropriate
    // setApiVersion() call on it.

    const QVersionNumber supportedVersion = s_vulkanInstance->supportedApiVersion();
    if (supportedVersion >= QVersionNumber(1, 2))
        s_vulkanInstance->setApiVersion(QVersionNumber(1, 2));
    else if (supportedVersion >= QVersionNumber(1, 1))
        s_vulkanInstance->setApiVersion(QVersionNumber(1, 2));
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
