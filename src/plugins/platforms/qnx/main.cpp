// Copyright (C) 2011 - 2014 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include "main.h"
#include "qqnxintegration.h"
#include "qqnxlgmon.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QPlatformIntegration *QQnxIntegrationPlugin::create(const QString& system, const QStringList& paramList)
{
    if (!system.compare("qnx"_L1, Qt::CaseInsensitive)) {
        qqnxLgmonInit();
        return new QQnxIntegration(paramList);
    }

    return 0;
}

QT_END_NAMESPACE
