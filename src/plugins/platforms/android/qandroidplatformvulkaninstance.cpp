/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of plugins of the Qt Toolkit.
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
