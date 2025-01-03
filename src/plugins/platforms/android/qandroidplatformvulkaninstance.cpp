// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidplatformvulkaninstance.h"

QT_BEGIN_NAMESPACE

QAndroidPlatformVulkanInstance::QAndroidPlatformVulkanInstance(QVulkanInstance *instance)
    : m_instance(instance)
{
    m_lib.setFileName(QStringLiteral("vulkan"));

    if (!m_lib.load()) {
        qWarning("Failed to load %s", qPrintable(m_lib.fileName()));
        return;
    }

    init(&m_lib);
}

void QAndroidPlatformVulkanInstance::createOrAdoptInstance()
{
    initInstance(m_instance, QByteArrayList() << QByteArrayLiteral("VK_KHR_android_surface"));
}

QAndroidPlatformVulkanInstance::~QAndroidPlatformVulkanInstance()
{
}

QT_END_NAMESPACE
