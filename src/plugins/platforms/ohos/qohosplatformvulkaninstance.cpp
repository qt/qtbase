// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosplatformvulkaninstance.h"
#include <QtCore/qdebug.h>

using namespace Qt::Literals::StringLiterals;

QT_BEGIN_NAMESPACE

QOhosPlatformVulkanInstance::QOhosPlatformVulkanInstance(QVulkanInstance *instance)
    : m_instance(instance)
{
    m_lib.setFileName(QStringLiteral("vulkan"));
    if (!m_lib.load()) {
        qWarning("Failed to load %s", qPrintable(m_lib.fileName()));
        return;
    }

    init(&m_lib);
}

void QOhosPlatformVulkanInstance::createOrAdoptInstance()
{
    initInstance(m_instance, {"VK_OHOS_surface"_ba});
}

QT_END_NAMESPACE
