// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qcoreglobaldata_p.h"

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC(QCoreGlobalData, globalInstance)

QCoreGlobalData::QCoreGlobalData()
{
}

QCoreGlobalData::~QCoreGlobalData()
{
}

QCoreGlobalData *QCoreGlobalData::instance()
{
    return globalInstance();
}

QT_END_NAMESPACE
