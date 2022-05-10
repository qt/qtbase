// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOREGLOBALDATA_P_H
#define QCOREGLOBALDATA_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qglobal_p.h>
#include "QtCore/qstringlist.h"
#include "QtCore/qreadwritelock.h"
#include "QtCore/qhash.h"
#include "QtCore/qbytearray.h"
#include "QtCore/qmutex.h"

QT_BEGIN_NAMESPACE

struct QCoreGlobalData
{
    QCoreGlobalData();
    ~QCoreGlobalData();

    QHash<QString, QStringList> dirSearchPaths;
    QReadWriteLock dirSearchPathsLock;

    static QCoreGlobalData *instance();
};

QT_END_NAMESPACE
#endif // QCOREGLOBALDATA_P_H
