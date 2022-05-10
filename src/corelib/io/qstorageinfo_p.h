// Copyright (C) 2014 Ivan Komissarov <ABBAPOH@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
#if defined(Q_OS_WIN)
    void retrieveVolumeInfo();
    void retrieveDiskFreeSpace();
    bool queryStorageProperty();
    void queryFileFsSectorSizeInformation();
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
