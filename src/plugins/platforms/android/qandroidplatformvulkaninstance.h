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

#ifndef QANDROIDPLATFORMVULKANINSTANCE_H
#define QANDROIDPLATFORMVULKANINSTANCE_H

#include <QtVulkanSupport/private/qbasicvulkanplatforminstance_p.h>
#include <QLibrary>

QT_BEGIN_NAMESPACE

class QAndroidPlatformVulkanInstance : public QBasicPlatformVulkanInstance
{
public:
    QAndroidPlatformVulkanInstance(QVulkanInstance *instance);
    ~QAndroidPlatformVulkanInstance();

    void createOrAdoptInstance() override;

private:
    QVulkanInstance *m_instance;
    QLibrary m_lib;
};

QT_END_NAMESPACE

#endif // QANDROIDPLATFORMVULKANINSTANCE_H
