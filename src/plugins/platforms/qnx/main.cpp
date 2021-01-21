/***************************************************************************
**
** Copyright (C) 2011 - 2014 BlackBerry Limited. All rights reserved.
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

#include "main.h"
#include "qqnxintegration.h"
#include "qqnxlgmon.h"

QT_BEGIN_NAMESPACE

QPlatformIntegration *QQnxIntegrationPlugin::create(const QString& system, const QStringList& paramList)
{
    if (!system.compare(QLatin1String("qnx"), Qt::CaseInsensitive)) {
        qqnxLgmonInit();
        return new QQnxIntegration(paramList);
    }

    return 0;
}

QT_END_NAMESPACE
