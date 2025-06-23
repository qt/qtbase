// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2014 Ivan Komissarov <ABBAPOH@gmail.com>
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qstorageinfo_p.h"

#include <QtCore/qfileinfo.h>
#include <QtCore/qtextstream.h>

#include <QtCore/private/qcore_unix_p.h>
#include <QtCore/private/qlocale_tools_p.h>

#include <errno.h>
#include <sys/stat.h>

#if defined(Q_OS_BSD4)
#  include <sys/mount.h>
#  include <sys/statvfs.h>
#elif defined(Q_OS_HURD)
#  include <mntent.h>
#  include <sys/statvfs.h>
#  include <sys/sysmacros.h>
#elif defined(Q_OS_SOLARIS)
#  include <sys/mnttab.h>
#  include <sys/statvfs.h>
#elif defined(Q_OS_HAIKU)
#  include <Directory.h>
#  include <Path.h>
#  include <Volume.h>
#  include <VolumeRoster.h>
#  include <fs_info.h>
#  include <sys/statvfs.h>
#else
#  include <sys/statvfs.h>
#endif

#if defined(Q_OS_BSD4)
#  if defined(Q_OS_NETBSD)
#    define QT_STATFSBUF struct statvfs
#    define QT_STATFS    ::statvfs
#  else
#    define QT_STATFSBUF struct statfs
#    define QT_STATFS    ::statfs
#  endif

#  if !defined(ST_RDONLY)
#    define ST_RDONLY MNT_RDONLY
#  endif
#  if !defined(_STATFS_F_FLAGS) && !defined(Q_OS_NETBSD)
#    define _STATFS_F_FLAGS 1
#  endif
#elif defined(Q_OS_HAIKU) || defined(Q_OS_CYGWIN)
#  define QT_STATFSBUF struct statvfs
#  define QT_STATFS    ::statvfs
#else
#  if defined(QT_LARGEFILE_SUPPORT)
#    define QT_STATFSBUF struct statvfs64
#    define QT_STATFS    ::statvfs64
#  else
#    define QT_STATFSBUF struct statvfs
#    define QT_STATFS    ::statvfs
#  endif // QT_LARGEFILE_SUPPORT
#endif // Q_OS_BSD4

#if __has_include(<paths.h>)
#  include <paths.h>
#endif
#ifndef _PATH_MOUNTED
#  define _PATH_MOUNTED     "/etc/mnttab"
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class QStorageIterator
{
public:
    QStorageIterator();
    ~QStorageIterator();

    inline bool isValid() const;
    inline bool next();
    inline QString rootPath() const;
    inline QByteArray fileSystemType() const;
    inline QByteArray device() const;
    inline QByteArray options() const;
    inline QByteArray subvolume() const;
private:
#if defined(Q_OS_BSD4)
    QT_STATFSBUF *stat_buf;
    int entryCount;
    int currentIndex;
#elif defined(Q_OS_SOLARIS)
    FILE *fp;
    mnttab mnt;
#elif defined(Q_OS_HURD)
    FILE *fp;
    QByteArray buffer;
    struct mntent mnt;
#elif defined(Q_OS_HAIKU)
    BVolumeRoster m_volumeRoster;

    QByteArray m_rootPath;
    QByteArray m_fileSystemType;
    QByteArray m_device;
#endif
};

#if defined(Q_OS_BSD4)

#ifndef MNT_NOWAIT
#  define MNT_NOWAIT 0
#endif

inline QStorageIterator::QStorageIterator()
    : entryCount(::getmntinfo(&stat_buf, MNT_NOWAIT)),
      currentIndex(-1)
{
}

inline QStorageIterator::~QStorageIterator()
{
}

inline bool QStorageIterator::isValid() const
{
    return entryCount != -1;
}

inline bool QStorageIterator::next()
{
    return ++currentIndex < entryCount;
}

inline QString QStorageIterator::rootPath() const
{
    return QFile::decodeName(stat_buf[currentIndex].f_mntonname);
}

inline QByteArray QStorageIterator::fileSystemType() const
{
    return QByteArray(stat_buf[currentIndex].f_fstypename);
}

inline QByteArray QStorageIterator::device() const
{
    return QByteArray(stat_buf[currentIndex].f_mntfromname);
}

inline QByteArray QStorageIterator::options() const
{
    return QByteArray();
}

inline QByteArray QStorageIterator::subvolume() const
{
    return QByteArray();
}
#elif defined(Q_OS_SOLARIS)

inline QStorageIterator::QStorageIterator()
{
    const int fd = qt_safe_open(_PATH_MOUNTED, O_RDONLY);
    fp = ::fdopen(fd, "r");
}

inline QStorageIterator::~QStorageIterator()
{
    if (fp)
        ::fclose(fp);
}

inline bool QStorageIterator::isValid() const
{
    return fp != nullptr;
}

inline bool QStorageIterator::next()
{
    return ::getmntent(fp, &mnt) == 0;
}

inline QString QStorageIterator::rootPath() const
{
    return QFile::decodeName(mnt.mnt_mountp);
}

inline QByteArray QStorageIterator::fileSystemType() const
{
    return QByteArray(mnt.mnt_fstype);
}

inline QByteArray QStorageIterator::device() const
{
    return QByteArray(mnt.mnt_mntopts);
}

inline QByteArray QStorageIterator::subvolume() const
{
    return QByteArray();
}

#elif defined(Q_OS_HURD)

static const int bufferSize = 1024; // 2 paths (mount point+device) and metainfo;
                                    // should be enough

inline QStorageIterator::QStorageIterator() :
    buffer(QByteArray(bufferSize, 0))
{
    fp = ::setmntent(_PATH_MOUNTED, "r");
}

inline QStorageIterator::~QStorageIterator()
{
    if (fp)
        ::endmntent(fp);
}

inline bool QStorageIterator::isValid() const
{
    return fp != nullptr;
}

inline bool QStorageIterator::next()
{
    return ::getmntent_r(fp, &mnt, buffer.data(), buffer.size()) != nullptr;
}

inline QString QStorageIterator::rootPath() const
{
    return QFile::decodeName(mnt.mnt_dir);
}

inline QByteArray QStorageIterator::fileSystemType() const
{
    return QByteArray(mnt.mnt_type);
}

inline QByteArray QStorageIterator::device() const
{
    return QByteArray(mnt.mnt_fsname);
}

inline QByteArray QStorageIterator::options() const
{
    return QByteArray(mnt.mnt_opts);
}

inline QByteArray QStorageIterator::subvolume() const
{
    return QByteArray();
}
#elif defined(Q_OS_HAIKU)
inline QStorageIterator::QStorageIterator()
{
}

inline QStorageIterator::~QStorageIterator()
{
}

inline bool QStorageIterator::isValid() const
{
    return true;
}

inline bool QStorageIterator::next()
{
    BVolume volume;

    if (m_volumeRoster.GetNextVolume(&volume) != B_OK)
        return false;

    BDirectory directory;
    if (volume.GetRootDirectory(&directory) != B_OK)
        return false;

    const BPath path(&directory);

    fs_info fsInfo;
    memset(&fsInfo, 0, sizeof(fsInfo));

    if (fs_stat_dev(volume.Device(), &fsInfo) != 0)
        return false;

    m_rootPath = path.Path();
    m_fileSystemType = QByteArray(fsInfo.fsh_name);

    const QByteArray deviceName(fsInfo.device_name);
    m_device = (deviceName.isEmpty() ? QByteArray::number(qint32(volume.Device())) : deviceName);

    return true;
}

inline QString QStorageIterator::rootPath() const
{
    return QFile::decodeName(m_rootPath);
}

inline QByteArray QStorageIterator::fileSystemType() const
{
    return m_fileSystemType;
}

inline QByteArray QStorageIterator::device() const
{
    return m_device;
}

inline QByteArray QStorageIterator::options() const
{
    return QByteArray();
}

inline QByteArray QStorageIterator::subvolume() const
{
    return QByteArray();
}

#else

inline QStorageIterator::QStorageIterator()
{
}

inline QStorageIterator::~QStorageIterator()
{
}

inline bool QStorageIterator::isValid() const
{
    return false;
}

inline bool QStorageIterator::next()
{
    return false;
}

inline QString QStorageIterator::rootPath() const
{
    return QString();
}

inline QByteArray QStorageIterator::fileSystemType() const
{
    return QByteArray();
}

inline QByteArray QStorageIterator::device() const
{
    return QByteArray();
}

inline QByteArray QStorageIterator::options() const
{
    return QByteArray();
}

inline QByteArray QStorageIterator::subvolume() const
{
    return QByteArray();
}
#endif

static inline QString retrieveLabel(const QByteArray &device)
{
#if defined Q_OS_HAIKU
    fs_info fsInfo;
    memset(&fsInfo, 0, sizeof(fsInfo));

    int32 pos = 0;
    dev_t dev;
    while ((dev = next_dev(&pos)) >= 0) {
        if (fs_stat_dev(dev, &fsInfo) != 0)
            continue;

        if (qstrcmp(fsInfo.device_name, device.constData()) == 0)
            return QString::fromLocal8Bit(fsInfo.volume_name);
    }
#else
    Q_UNUSED(device);
#endif

    return QString();
}

void QStorageInfoPrivate::doStat()
{
    valid = ready = false;
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
    QT_EINTR_LOOP(result, QT_STATFS(QFile::encodeName(rootPath).constData(), &statfs_buf));
    valid = ready = (result == 0);
    if (valid) {
#if defined(Q_OS_INTEGRITY) || (defined(Q_OS_BSD4) && !defined(Q_OS_NETBSD)) || defined(Q_OS_RTEMS)
        bytesTotal = statfs_buf.f_blocks * statfs_buf.f_bsize;
        bytesFree = statfs_buf.f_bfree * statfs_buf.f_bsize;
        bytesAvailable = statfs_buf.f_bavail * statfs_buf.f_bsize;
#else
        bytesTotal = statfs_buf.f_blocks * statfs_buf.f_frsize;
        bytesFree = statfs_buf.f_bfree * statfs_buf.f_frsize;
        bytesAvailable = statfs_buf.f_bavail * statfs_buf.f_frsize;
#endif
        blockSize = int(statfs_buf.f_bsize);
#if defined(Q_OS_ANDROID) || defined(Q_OS_BSD4) || defined(Q_OS_INTEGRITY) || defined(Q_OS_RTEMS)
#if defined(_STATFS_F_FLAGS)
        readOnly = (statfs_buf.f_flags & ST_RDONLY) != 0;
#endif
#else
        readOnly = (statfs_buf.f_flag & ST_RDONLY) != 0;
#endif
    }
}

void QStorageInfoPrivate::initRootPath()
{
    rootPath = QFileInfo(rootPath).canonicalFilePath();

    if (rootPath.isEmpty())
        return;

    QStorageIterator it;
    if (!it.isValid()) {
        rootPath = QStringLiteral("/");
        return;
    }

    int maxLength = 0;
    const QString oldRootPath = rootPath;
    rootPath.clear();

    while (it.next()) {
        const QString mountDir = it.rootPath();
        const QByteArray fsName = it.fileSystemType();
        // we try to find most suitable entry
        if (maxLength < mountDir.size() && isParentOf(mountDir, oldRootPath)) {
            maxLength = mountDir.size();
            rootPath = mountDir;
            device = it.device();
            fileSystemType = fsName;
            subvolume = it.subvolume();
        }
    }
}

QList<QStorageInfo> QStorageInfoPrivate::mountedVolumes()
{
    QStorageIterator it;
    if (!it.isValid())
        return QList<QStorageInfo>() << root();

    QList<QStorageInfo> volumes;

    while (it.next()) {
        if (!shouldIncludeFs(it.rootPath(), it.fileSystemType()))
            continue;

        const QString mountDir = it.rootPath();
        QStorageInfo info(mountDir);
        info.d->device = it.device();
        info.d->fileSystemType = it.fileSystemType();
        info.d->subvolume = it.subvolume();
        if (info.bytesTotal() <= 0 && info != root())
            continue;
        volumes.append(info);
    }

    return volumes;
}

QT_END_NAMESPACE
