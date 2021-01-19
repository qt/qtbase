/****************************************************************************
**
** Copyright (C) 2014 Ivan Komissarov <ABBAPOH@gmail.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

QStorageInfo QStorageInfoPrivate::root()
{
    Q_UNIMPLEMENTED();
    return QStorageInfo();
}

QT_END_NAMESPACE
