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

#ifndef QSTORAGEINFO_P_H
#define QSTORAGEINFO_P_H

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
#include "qstorageinfo.h"

QT_BEGIN_NAMESPACE

class QStorageInfoPrivate : public QSharedData
{
public:
    inline QStorageInfoPrivate() : QSharedData(),
        bytesTotal(-1), bytesFree(-1), bytesAvailable(-1), blockSize(-1),
        readOnly(false), ready(false), valid(false)
    {}

    void initRootPath();
    void doStat();

    static QList<QStorageInfo> mountedVolumes();
    static QStorageInfo root();

protected:
#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
    void retrieveVolumeInfo();
    void retrieveDiskFreeSpace();
#elif defined(Q_OS_MAC)
    void retrievePosixInfo();
    void retrieveUrlProperties(bool initRootPath = false);
    void retrieveLabel();
#elif defined(Q_OS_UNIX)
    void retrieveVolumeInfo();
#endif

public:
    QString rootPath;
    QByteArray device;
    QByteArray subvolume;
    QByteArray fileSystemType;
    QString name;

    qint64 bytesTotal;
    qint64 bytesFree;
    qint64 bytesAvailable;
    int blockSize;

    bool readOnly;
    bool ready;
    bool valid;
};

QT_END_NAMESPACE

#endif // QSTORAGEINFO_P_H
