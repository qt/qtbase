/****************************************************************************
**
** Copyright (C) 2014 Ivan Komissarov <ABBAPOH@gmail.com>
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qstorageinfo_p.h"

#include <QtCore/qfileinfo.h>
#include <QtCore/private/qcore_mac_p.h>

#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFURLEnumerator.h>

#include <sys/mount.h>

#define QT_STATFSBUF struct statfs
#define QT_STATFS    ::statfs

QT_BEGIN_NAMESPACE

void QStorageInfoPrivate::initRootPath()
{
    rootPath = QFileInfo(rootPath).canonicalFilePath();

    if (rootPath.isEmpty())
        return;

    retrieveUrlProperties(true);
}

void QStorageInfoPrivate::doStat()
{
    initRootPath();

    if (rootPath.isEmpty())
        return;

    retrieveLabel();
    retrievePosixInfo();
    retrieveUrlProperties();
}

void QStorageInfoPrivate::retrievePosixInfo()
{
    QT_STATFSBUF statfs_buf;
    int result = QT_STATFS(QFile::encodeName(rootPath).constData(), &statfs_buf);
    if (result == 0) {
        device = QByteArray(statfs_buf.f_mntfromname);
        readOnly = (statfs_buf.f_flags & MNT_RDONLY) != 0;
        fileSystemType = QByteArray(statfs_buf.f_fstypename);
        blockSize = statfs_buf.f_bsize;
    }
}

static inline qint64 CFDictionaryGetInt64(CFDictionaryRef dictionary, const void *key)
{
    CFNumberRef cfNumber = (CFNumberRef)CFDictionaryGetValue(dictionary, key);
    if (!cfNumber)
        return -1;
    qint64 result;
    bool ok = CFNumberGetValue(cfNumber, kCFNumberSInt64Type, &result);
    if (!ok)
        return -1;
    return result;
}

void QStorageInfoPrivate::retrieveUrlProperties(bool initRootPath)
{
    static const void *rootPathKeys[] = { kCFURLVolumeURLKey };
    static const void *propertyKeys[] = {
        // kCFURLVolumeNameKey, // 10.7
        // kCFURLVolumeLocalizedNameKey, // 10.7
        kCFURLVolumeTotalCapacityKey,
        kCFURLVolumeAvailableCapacityKey,
        // kCFURLVolumeIsReadOnlyKey // 10.7
    };
    size_t size = (initRootPath ? sizeof(rootPathKeys) : sizeof(propertyKeys)) / sizeof(void*);
    QCFType<CFArrayRef> keys = CFArrayCreate(kCFAllocatorDefault,
                                             initRootPath ? rootPathKeys : propertyKeys,
                                             size,
                                             Q_NULLPTR);

    if (!keys)
        return;

    const QCFString cfPath = rootPath;
    if (initRootPath)
        rootPath.clear();

    QCFType<CFURLRef> url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
                                                          cfPath,
                                                          kCFURLPOSIXPathStyle,
                                                          true);
    if (!url)
        return;

    CFErrorRef error;
    QCFType<CFDictionaryRef> map = CFURLCopyResourcePropertiesForKeys(url, keys, &error);

    if (!map)
        return;

    if (initRootPath) {
        const CFURLRef rootUrl = (CFURLRef)CFDictionaryGetValue(map, kCFURLVolumeURLKey);
        if (!rootUrl)
            return;

        rootPath = QCFString(CFURLCopyFileSystemPath(rootUrl, kCFURLPOSIXPathStyle));
        valid = true;
        ready = true;

        return;
    }

    bytesTotal = CFDictionaryGetInt64(map, kCFURLVolumeTotalCapacityKey);
    bytesAvailable = CFDictionaryGetInt64(map, kCFURLVolumeAvailableCapacityKey);
    bytesFree = bytesAvailable;
}

void QStorageInfoPrivate::retrieveLabel()
{
    QCFString path = CFStringCreateWithFileSystemRepresentation(0,
        QFile::encodeName(rootPath).constData());
    if (!path)
        return;

    QCFType<CFURLRef> url = CFURLCreateWithFileSystemPath(0, path, kCFURLPOSIXPathStyle, true);
    if (!url)
        return;

    QCFType<CFURLRef> volumeUrl;
    if (!CFURLCopyResourcePropertyForKey(url, kCFURLVolumeURLKey, &volumeUrl, NULL))
        return;

    QCFString volumeName;
    if (!CFURLCopyResourcePropertyForKey(url, kCFURLNameKey, &volumeName, NULL))
        return;

    name = volumeName;
}

QList<QStorageInfo> QStorageInfoPrivate::mountedVolumes()
{
    QList<QStorageInfo> volumes;

    QCFType<CFURLEnumeratorRef> enumerator;
    enumerator = CFURLEnumeratorCreateForMountedVolumes(Q_NULLPTR,
                                                        kCFURLEnumeratorSkipInvisibles,
                                                        Q_NULLPTR);

    CFURLEnumeratorResult result = kCFURLEnumeratorSuccess;
    do {
        CFURLRef url;
        CFErrorRef error;
        result = CFURLEnumeratorGetNextURL(enumerator, &url, &error);
        if (result == kCFURLEnumeratorSuccess) {
            const QCFString urlString = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
            volumes.append(QStorageInfo(urlString));
        }
    } while (result != kCFURLEnumeratorEnd);

    return volumes;
}

QStorageInfo QStorageInfoPrivate::root()
{
    return QStorageInfo(QStringLiteral("/"));
}

QT_END_NAMESPACE
