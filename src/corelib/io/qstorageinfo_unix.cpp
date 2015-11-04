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

#include <QtCore/qdiriterator.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qtextstream.h>

#include <QtCore/private/qcore_unix_p.h>

#include <errno.h>
#include <sys/stat.h>

#if defined(Q_OS_BSD4)
#  include <sys/mount.h>
#  include <sys/statvfs.h>
#elif defined(Q_OS_ANDROID)
#  include <sys/mount.h>
#  include <sys/vfs.h>
#  include <mntent.h>
#elif defined(Q_OS_LINUX) || defined(Q_OS_HURD)
#  include <mntent.h>
#  include <sys/statvfs.h>
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
#  if !defined(_STATFS_F_FLAGS)
#    define _STATFS_F_FLAGS 1
#  endif
#elif defined(Q_OS_ANDROID)
#  define QT_STATFS    ::statfs
#  define QT_STATFSBUF struct statfs
#  if !defined(ST_RDONLY)
#    define ST_RDONLY 1 // hack for missing define on Android
#  endif
#elif defined(Q_OS_HAIKU)
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

QT_BEGIN_NAMESPACE

static bool isPseudoFs(const QString &mountDir, const QByteArray &type)
{
    if (mountDir.startsWith(QLatin1String("/dev"))
        || mountDir.startsWith(QLatin1String("/proc"))
        || mountDir.startsWith(QLatin1String("/sys"))
        || mountDir.startsWith(QLatin1String("/var/run"))
        || mountDir.startsWith(QLatin1String("/var/lock"))) {
        return true;
    }
    if (type == "tmpfs")
        return true;
#if defined(Q_OS_LINUX)
    if (type == "rootfs" || type == "rpc_pipefs")
        return true;
#endif

    return false;
}

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
private:
#if defined(Q_OS_BSD4)
    QT_STATFSBUF *stat_buf;
    int entryCount;
    int currentIndex;
#elif defined(Q_OS_SOLARIS)
    FILE *fp;
    mnttab mnt;
#elif defined(Q_OS_ANDROID)
    QFile file;
    QByteArray m_rootPath;
    QByteArray m_fileSystemType;
    QByteArray m_device;
#elif defined(Q_OS_LINUX) || defined(Q_OS_HURD)
    FILE *fp;
    mntent mnt;
    QByteArray buffer;
#elif defined(Q_OS_HAIKU)
    BVolumeRoster m_volumeRoster;

    QByteArray m_rootPath;
    QByteArray m_fileSystemType;
    QByteArray m_device;
#endif
};

#if defined(Q_OS_BSD4)

inline QStorageIterator::QStorageIterator()
    : entryCount(::getmntinfo(&stat_buf, 0)),
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

#elif defined(Q_OS_SOLARIS)

static const char pathMounted[] = "/etc/mnttab";

inline QStorageIterator::QStorageIterator()
{
    const int fd = qt_safe_open(pathMounted, O_RDONLY);
    fp = ::fdopen(fd, "r");
}

inline QStorageIterator::~QStorageIterator()
{
    if (fp)
        ::fclose(fp);
}

inline bool QStorageIterator::isValid() const
{
    return fp != Q_NULLPTR;
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

#elif defined(Q_OS_ANDROID)

static const QLatin1String pathMounted("/proc/mounts");

inline QStorageIterator::QStorageIterator()
{
    file.setFileName(pathMounted);
    file.open(QIODevice::ReadOnly | QIODevice::Text);
}

inline QStorageIterator::~QStorageIterator()
{
}

inline bool QStorageIterator::isValid() const
{
    return file.isOpen();
}

inline bool QStorageIterator::next()
{
    QList<QByteArray> data;
    do {
        const QByteArray line = file.readLine();
        data = line.split(' ');
    } while (data.count() < 3 && !file.atEnd());

    if (file.atEnd())
        return false;
    m_device = data.at(0);
    m_rootPath = data.at(1);
    m_fileSystemType = data.at(2);

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

#elif defined(Q_OS_LINUX) || defined(Q_OS_HURD)

static const char pathMounted[] = "/etc/mtab";
static const int bufferSize = 1024; // 2 paths (mount point+device) and metainfo;
                                    // should be enough

inline QStorageIterator::QStorageIterator() :
    buffer(QByteArray(bufferSize, 0))
{
    fp = ::setmntent(pathMounted, "r");
}

inline QStorageIterator::~QStorageIterator()
{
    if (fp)
        ::endmntent(fp);
}

inline bool QStorageIterator::isValid() const
{
    return fp != Q_NULLPTR;
}

inline bool QStorageIterator::next()
{
    return ::getmntent_r(fp, &mnt, buffer.data(), buffer.size()) != Q_NULLPTR;
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

#endif

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
        if (isPseudoFs(mountDir, fsName))
            continue;
        // we try to find most suitable entry
        if (oldRootPath.startsWith(mountDir) && maxLength < mountDir.length()) {
            maxLength = mountDir.length();
            rootPath = mountDir;
            device = it.device();
            fileSystemType = fsName;
        }
    }
}

static inline QString retrieveLabel(const QByteArray &device)
{
#ifdef Q_OS_LINUX
    static const char pathDiskByLabel[] = "/dev/disk/by-label";

    QDirIterator it(QLatin1String(pathDiskByLabel), QDir::NoDotAndDotDot);
    while (it.hasNext()) {
        it.next();
        QFileInfo fileInfo(it.fileInfo());
        if (fileInfo.isSymLink() && fileInfo.symLinkTarget().toLocal8Bit() == device)
            return fileInfo.fileName();
    }
#elif defined Q_OS_HAIKU
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

#if defined(Q_OS_BSD4) && !defined(Q_OS_NETBSD)
        bytesTotal = statfs_buf.f_blocks * statfs_buf.f_bsize;
        bytesFree = statfs_buf.f_bfree * statfs_buf.f_bsize;
        bytesAvailable = statfs_buf.f_bavail * statfs_buf.f_bsize;
#else
        bytesTotal = statfs_buf.f_blocks * statfs_buf.f_frsize;
        bytesFree = statfs_buf.f_bfree * statfs_buf.f_frsize;
        bytesAvailable = statfs_buf.f_bavail * statfs_buf.f_frsize;
#endif
        blockSize = statfs_buf.f_bsize;
#if defined(Q_OS_ANDROID) || defined (Q_OS_BSD4)
#if defined(_STATFS_F_FLAGS)
        readOnly = (statfs_buf.f_flags & ST_RDONLY) != 0;
#endif
#else
        readOnly = (statfs_buf.f_flag & ST_RDONLY) != 0;
#endif
    }
}

QList<QStorageInfo> QStorageInfoPrivate::mountedVolumes()
{
    QStorageIterator it;
    if (!it.isValid())
        return QList<QStorageInfo>() << root();

    QList<QStorageInfo> volumes;

    while (it.next()) {
        const QString mountDir = it.rootPath();
        const QByteArray fsName = it.fileSystemType();
        if (isPseudoFs(mountDir, fsName))
            continue;

        volumes.append(QStorageInfo(mountDir));
    }

    return volumes;
}

QStorageInfo QStorageInfoPrivate::root()
{
    return QStorageInfo(QStringLiteral("/"));
}

QT_END_NAMESPACE
