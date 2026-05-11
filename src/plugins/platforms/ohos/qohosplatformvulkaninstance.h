// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSPLATFORMVULKANINSTANCE_H
#define QOHOSPLATFORMVULKANINSTANCE_H

#include <QtGui/private/qbasicvulkanplatforminstance_p.h>
#include <QtCore/qlibrary.h>

QT_BEGIN_NAMESPACE

class QOhosPlatformVulkanInstance : public QBasicPlatformVulkanInstance
{
public:
    explicit QOhosPlatformVulkanInstance(QVulkanInstance *instance);

    void createOrAdoptInstance() override;

private:
    QVulkanInstance *m_instance;
    QLibrary m_lib;
};

QT_END_NAMESPACE

#endif // QOHOSPLATFORMVULKANINSTANCE_H
