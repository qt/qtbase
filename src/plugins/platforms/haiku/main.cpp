// Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "main.h"
#include "qhaikuintegration.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QPlatformIntegration *QHaikuIntegrationPlugin::create(const QString& system, const QStringList& paramList)
{
    if (!system.compare("haiku"_L1, Qt::CaseInsensitive))
        return new QHaikuIntegration(paramList);

    return nullptr;
}

QT_END_NAMESPACE
