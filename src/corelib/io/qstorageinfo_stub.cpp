// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include "qstorageinfo_p.h"

QT_BEGIN_NAMESPACE

void QStorageInfoPrivate::initRootPath()
{
    Q_UNIMPLEMENTED();
    rootPath = QString();
}

void QStorageInfoPrivate::doStat()
{
    Q_UNIMPLEMENTED();
}

QList<QStorageInfo> QStorageInfoPrivate::mountedVolumes()
{
    Q_UNIMPLEMENTED();
    return QList<QStorageInfo>();
}

QT_END_NAMESPACE
