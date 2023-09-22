// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2014 Ivan Komissarov <ABBAPOH@gmail.com>
// Copyright (C) 2016 Intel Corporation.
// Copyright (C) 2023 Ahmad Samir <a.samirh78@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qstorageinfo_linux_p.h"

#include "qdiriterator.h"
#include <private/qcore_unix_p.h>

#if defined(Q_OS_ANDROID)
#  include <sys/mount.h>
#  include <sys/vfs.h>
#  define QT_STATFS    ::statfs
#  define QT_STATFSBUF struct statfs
#  if !defined(ST_RDONLY)
#    define ST_RDONLY 1 // hack for missing define on Android
#  endif
#else
#  include <sys/statvfs.h>
#  if defined(QT_LARGEFILE_SUPPORT)
#    define QT_STATFSBUF struct statvfs64
#    define QT_STATFS    ::statvfs64
#  else
#    define QT_STATFSBUF struct statvfs
#    define QT_STATFS    ::statvfs
#  endif // QT_LARGEFILE_SUPPORT
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

// udev encodes the labels with ID_LABEL_FS_ENC which is done with
// blkid_encode_string(). Within this function some 1-byte utf-8
// characters not considered safe (e.g. '\' or ' ') are encoded as hex
static QString decodeFsEncString(const QString &str)
{
    QString decoded;
    decoded.reserve(str.size());

    int i = 0;
    while (i < str.size()) {
        if (i <= str.size() - 4) {    // we need at least four characters \xAB
            if (QStringView{str}.sliced(i).startsWith("\\x"_L1)) {
                bool bOk;
                const int code = QStringView{str}.mid(i+2, 2).toInt(&bOk, 16);
                if (bOk && code >= 0x20 && code < 0x80) {
                    decoded += QChar(code);
                    i += 4;
                    continue;
                }
            }
        }
        decoded += str.at(i);
        ++i;
    }
    return decoded;
}

static inline QString retrieveLabel(const QByteArray &device)
{
    static const char pathDiskByLabel[] = "/dev/disk/by-label";

    QFileInfo devinfo(QFile::decodeName(device));
    QString devicePath = devinfo.canonicalFilePath();
    if (devicePath.isEmpty())
        return QString();

    QDirIterator it(QLatin1StringView(pathDiskByLabel), QDir::NoDotAndDotDot);
    while (it.hasNext()) {
        QFileInfo fileInfo = it.nextFileInfo();
        if (fileInfo.symLinkTarget() == devicePath)
            return decodeFsEncString(fileInfo.fileName());
    }
    return QString();
}

void QStorageInfoPrivate::doStat()
{
    initRootPath();
    if (rootPath.isEmpty())
        return;

    retrieveVolumeInfo();
    name = retrieveLabel(device);
}

void QStorageInfoPrivate::retrieveVolumeInfo()
{
    QT_STATFSBUF statfs_buf;
    int result;
    EINTR_LOOP(result, QT_STATFS(QFile::encodeName(rootPath).constData(), &statfs_buf));
    if (result == 0) {
        valid = true;
        ready = true;

        bytesTotal = statfs_buf.f_blocks * statfs_buf.f_frsize;
        bytesFree = statfs_buf.f_bfree * statfs_buf.f_frsize;
        bytesAvailable = statfs_buf.f_bavail * statfs_buf.f_frsize;
        blockSize = statfs_buf.f_bsize;

#if defined(Q_OS_ANDROID)
#if defined(_STATFS_F_FLAGS)
        readOnly = (statfs_buf.f_flags & ST_RDONLY) != 0;
#endif
#else
        readOnly = (statfs_buf.f_flag & ST_RDONLY) != 0;
#endif
    }
}

static std::vector<MountInfo> parseMountInfo(FilterMountInfo filter = FilterMountInfo::All)
{
    QFile file(u"/proc/self/mountinfo"_s);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};

    QByteArray mountinfo = file.readAll();
    file.close();

    return doParseMountInfo(mountinfo, filter);
}

void QStorageInfoPrivate::initRootPath()
{
    rootPath = QFileInfo(rootPath).canonicalFilePath();
    if (rootPath.isEmpty())
        return;

    std::vector<MountInfo> infos = parseMountInfo();
    if (infos.empty()) {
        rootPath = u'/';
        return;
    }

    qsizetype maxLength = 0;
    const QString oldRootPath = rootPath;
    rootPath.clear();

    const MountInfo *bestInfo = nullptr;
    for (const MountInfo &info : infos) {
        // we try to find most suitable entry
        qsizetype mpSize = info.mountPoint.size();
        if (isParentOf(info.mountPoint, oldRootPath) && maxLength < mpSize) {
            bestInfo = &info;
            maxLength = mpSize;
        }
    }
    if (bestInfo) {
        rootPath = bestInfo->mountPoint;
        device = bestInfo->device;
        fileSystemType = bestInfo->fsType;
        subvolume = bestInfo->fsRoot;
    }
}

QList<QStorageInfo> QStorageInfoPrivate::mountedVolumes()
{
    std::vector<MountInfo> infos = parseMountInfo(FilterMountInfo::Filtered);
    if (infos.empty())
        return QList{root()};

    QList<QStorageInfo> volumes;
    for (MountInfo &info : infos) {
        QStorageInfo storage(info.mountPoint);
        storage.d->device = info.device;
        storage.d->fileSystemType = info.fsType;
        storage.d->subvolume = info.fsRoot;
        if (storage.bytesTotal() == 0 && storage != root())
            continue;
        volumes.push_back(storage);
    }
    return volumes;
}

QT_END_NAMESPACE
