/****************************************************************************
**
** Copyright (C) 2014 Ivan Komissarov <ABBAPOH@gmail.com>
** Copyright (C) 2016 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qstorageinfo_p.h"

#include <QtCore/qdiriterator.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qtextstream.h>

#include <QtCore/private/qcore_unix_p.h>
#include <QtCore/private/qlocale_tools_p.h>

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

#if __has_include(<paths.h>)
#  include <paths.h>
#endif
#ifndef _PATH_MOUNTED
#  define _PATH_MOUNTED     "/etc/mnttab"
#endif

QT_BEGIN_NAMESPACE

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
#elif defined(Q_OS_ANDROID)
    QFile file;
    QByteArray m_rootPath;
    QByteArray m_fileSystemType;
    QByteArray m_device;
    QByteArray m_options;
#elif defined(Q_OS_LINUX) || defined(Q_OS_HURD)
    struct mountinfoent : public mntent {
        // Details from proc(5) section from /proc/<pid>/mountinfo:
        //(1)  mount ID: a unique ID for the mount (may be reused after umount(2)).
        int mount_id;
        //(2)  parent ID: the ID of the parent mount (or of self for the top of the mount tree).
//      int parent_id;
        //(3)  major:minor: the value of st_dev for files on this filesystem (see stat(2)).
        dev_t rdev;
        //(4)  root: the pathname of the directory in the filesystem which forms the root of this mount.
        char *subvolume;
        //(5)  mount point: the pathname of the mount point relative to the process's root directory.
//      char *mnt_dir;      // in mntent
        //(6)  mount options: per-mount options.
//      char *mnt_opts;     // in mntent
        //(7)  optional fields: zero or more fields of the form "tag[:value]"; see below.
//      int flags;
        //(8)  separator: the end of the optional fields is marked by a single hyphen.

        //(9)  filesystem type: the filesystem type in the form "type[.subtype]".
//      char *mnt_type;     // in mntent
        //(10) mount source: filesystem-specific information or "none".
//      char *mnt_fsname;   // in mntent
        //(11) super options: per-superblock options.
        char *superopts;
    };

    FILE *fp;
    QByteArray buffer;
    mountinfoent mnt;
    bool usingMountinfo;
#elif defined(Q_OS_HAIKU)
    BVolumeRoster m_volumeRoster;

    QByteArray m_rootPath;
    QByteArray m_fileSystemType;
    QByteArray m_device;
#endif
};

template <typename String>
static bool isParentOf(const String &parent, const QString &dirName)
{
    return dirName.startsWith(parent) &&
            (dirName.size() == parent.size() || dirName.at(parent.size()) == QLatin1Char('/') ||
             parent.size() == 1);
}

static bool shouldIncludeFs(const QStorageIterator &it)
{
    /*
     * This function implements a heuristic algorithm to determine whether a
     * given mount should be reported to the user. Our objective is to list
     * only entries that the end-user would find useful.
     *
     * We therefore ignore:
     *  - mounted in /dev, /proc, /sys: special mounts
     *    (this will catch /sys/fs/cgroup, /proc/sys/fs/binfmt_misc, /dev/pts,
     *    some of which are tmpfs on Linux)
     *  - mounted in /var/run or /var/lock: most likely pseudofs
     *    (on earlier systemd versions, /var/run was a bind-mount of /run, so
     *    everything would be unnecessarily duplicated)
     *  - filesystem type is "rootfs": artifact of the root-pivot on some Linux
     *    initrd
     *  - if the filesystem total size is zero, it's a pseudo-fs (not checked here).
     */

    QString mountDir = it.rootPath();
    if (isParentOf(QLatin1String("/dev"), mountDir)
        || isParentOf(QLatin1String("/proc"), mountDir)
        || isParentOf(QLatin1String("/sys"), mountDir)
        || isParentOf(QLatin1String("/var/run"), mountDir)
        || isParentOf(QLatin1String("/var/lock"), mountDir)) {
        return false;
    }

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    if (it.fileSystemType() == "rootfs")
        return false;
#endif

    // size checking in mountedVolumes()
    return true;
}

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
#elif defined(Q_OS_ANDROID)

inline QStorageIterator::QStorageIterator()
{
    file.setFileName(_PATH_MOUNTED);
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
    // If file is virtual, file.readLine() may succeed even when file.atEnd().
    do {
        const QByteArray line = file.readLine();
        if (line.isEmpty() && file.atEnd())
            return false;
        data = line.split(' ');
    } while (data.count() < 4);

    m_device = data.at(0);
    m_rootPath = data.at(1);
    m_fileSystemType = data.at(2);
    m_options = data.at(3);

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
    return m_options;
}

inline QByteArray QStorageIterator::subvolume() const
{
    return QByteArray();
}
#elif defined(Q_OS_LINUX) || defined(Q_OS_HURD)

static const int bufferSize = 1024; // 2 paths (mount point+device) and metainfo;
                                    // should be enough

inline QStorageIterator::QStorageIterator() :
    buffer(QByteArray(bufferSize, 0))
{
    fp = nullptr;

#ifdef Q_OS_LINUX
    // first, try to open /proc/self/mountinfo, which has more details
    fp = ::fopen("/proc/self/mountinfo", "re");
#endif
    if (fp) {
        usingMountinfo = true;
    } else {
        usingMountinfo = false;
        fp = ::setmntent(_PATH_MOUNTED, "r");
    }
}

inline QStorageIterator::~QStorageIterator()
{
    if (fp) {
        if (usingMountinfo)
            ::fclose(fp);
        else
            ::endmntent(fp);
    }
}

inline bool QStorageIterator::isValid() const
{
    return fp != nullptr;
}

inline bool QStorageIterator::next()
{
    mnt.subvolume = nullptr;
    mnt.superopts = nullptr;
    if (!usingMountinfo)
        return ::getmntent_r(fp, &mnt, buffer.data(), buffer.size()) != nullptr;

    // Helper function to parse paths that the kernel inserts escape sequences
    // for. The unescaped string is left at \a src and is properly
    // NUL-terminated. Returns a pointer to the delimiter that terminated the
    // path, or nullptr if it failed.
    auto parseMangledPath = [](char *src) {
        // The kernel escapes with octal the following characters:
        //  space ' ', tab '\t', backslask '\\', and newline '\n'
        char *dst = src;
        while (*src) {
            switch (*src) {
            case ' ':
                // Unescaped space: end of the field.
                *dst = '\0';
                return src;

            default:
                *dst++ = *src++;
                break;

            case '\\':
                // It always uses exactly three octal characters.
                ++src;
                char c = (*src++ - '0') << 6;
                c |= (*src++ - '0') << 3;
                c |= (*src++ - '0');
                *dst++ = c;
                break;
            }
        }

        // Found a NUL before the end of the field.
        src = nullptr;
        return src;
    };

    char *ptr = buffer.data();
    if (fgets(ptr, buffer.size(), fp) == nullptr)
        return false;

    size_t len = strlen(buffer.data());
    if (len == 0)
        return false;
    while (Q_UNLIKELY(ptr[len - 1] != '\n' && !feof(fp))) {
        // buffer wasn't large enough. Enlarge and try again.
        // (we're readidng from the kernel, so OOM is unlikely)
        buffer.resize((buffer.size() + 4096) & ~4095);
        ptr = buffer.data();
        if (fgets(ptr + len, buffer.size() - len, fp) == nullptr)
            return false;

        len += strlen(ptr + len);
        Q_ASSERT(len < size_t(buffer.size()));
    }
    ptr[len - 1] = '\0';

    // parse the line
    bool ok;
    mnt.mnt_freq = 0;
    mnt.mnt_passno = 0;

    mnt.mount_id = qstrtoll(ptr, const_cast<const char **>(&ptr), 10, &ok);
    if (!ptr || !ok)
        return false;

    int parent_id = qstrtoll(ptr, const_cast<const char **>(&ptr), 10, &ok);
    Q_UNUSED(parent_id);
    if (!ptr || !ok)
        return false;

    int rdevmajor = qstrtoll(ptr, const_cast<const char **>(&ptr), 10, &ok);
    if (!ptr || !ok)
        return false;
    if (*ptr != ':')
        return false;
    int rdevminor = qstrtoll(ptr + 1, const_cast<const char **>(&ptr), 10, &ok);
    if (!ptr || !ok)
        return false;
    mnt.rdev = makedev(rdevmajor, rdevminor);

    if (*ptr != ' ')
        return false;

    mnt.subvolume = ++ptr;
    ptr = parseMangledPath(ptr);
    if (!ptr)
        return false;

    // unset a subvolume of "/" -- it's not a *sub* volume
    if (mnt.subvolume + 1 == ptr)
        *mnt.subvolume = '\0';

    mnt.mnt_dir = ++ptr;
    ptr = parseMangledPath(ptr);
    if (!ptr)
        return false;

    mnt.mnt_opts = ++ptr;
    ptr = strchr(ptr, ' ');
    if (!ptr)
        return false;

    // we don't parse the flags, so just find the separator
    if (char *const dashed = strstr(ptr, " - ")) {
        *ptr = '\0';
        ptr = dashed + strlen(" - ") - 1;
    } else {
        return false;
    }

    mnt.mnt_type = ++ptr;
    ptr = strchr(ptr, ' ');
    if (!ptr)
        return false;
    *ptr = '\0';

    mnt.mnt_fsname = ++ptr;
    ptr = parseMangledPath(ptr);
    if (!ptr)
        return false;

    mnt.superopts = ++ptr;
    ptr += strcspn(ptr, " \n");
    *ptr = '\0';

    return true;
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
    // check that the device exists
    if (mnt.mnt_fsname[0] == '/' && access(mnt.mnt_fsname, F_OK) != 0) {
        // It doesn't, so let's try to resolve the dev_t from /dev/block.
        // Note how strlen("4294967295") == digits10 + 1, so we need to add 1
        // for each number, plus the ':'.
        char buf[sizeof("/dev/block/") + 2 * std::numeric_limits<unsigned>::digits10 + 3];
        QByteArray dev(PATH_MAX, Qt::Uninitialized);
        char *devdata = dev.data();

        snprintf(buf, sizeof(buf), "/dev/block/%u:%u", major(mnt.rdev), minor(mnt.rdev));
        if (realpath(buf, devdata)) {
            dev.truncate(strlen(devdata));
            return dev;
        }
    }
    return QByteArray(mnt.mnt_fsname);
}

inline QByteArray QStorageIterator::options() const
{
    // Merge the two options, starting with the superblock options and letting
    // the per-mount options override.
    const char *superopts = mnt.superopts;

    // Both mnt_opts and superopts start with "ro" or "rw", so we can skip the
    // superblock's field (see show_mountinfo() in fs/proc_namespace.c).
    if (superopts && superopts[0] == 'r') {
        if (superopts[2] == '\0')       // no other superopts besides "ro" / "rw"?
            superopts = nullptr;
        else if (superopts[2] == ',')
            superopts += 3;
    }

    if (superopts)
        return QByteArray(superopts) + ',' + mnt.mnt_opts;
    return QByteArray(mnt.mnt_opts);
}

inline QByteArray QStorageIterator::subvolume() const
{
    return QByteArray(mnt.subvolume);
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
        if (isParentOf(mountDir, oldRootPath) && maxLength < mountDir.length()) {
            maxLength = mountDir.length();
            rootPath = mountDir;
            device = it.device();
            fileSystemType = fsName;
            subvolume = it.subvolume();
        }
    }
}

#ifdef Q_OS_LINUX
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
            if (str.at(i) == QLatin1Char('\\') &&
                str.at(i+1) == QLatin1Char('x')) {
                bool bOk;
                const int code = str.midRef(i+2, 2).toInt(&bOk, 16);
                // only decode characters between 0x20 and 0x7f but not
                // the backslash to prevent collisions
                if (bOk && code >= 0x20 && code < 0x80 && code != '\\') {
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
#endif

static inline QString retrieveLabel(const QByteArray &device)
{
#ifdef Q_OS_LINUX
    static const char pathDiskByLabel[] = "/dev/disk/by-label";

    QFileInfo devinfo(QFile::decodeName(device));
    QString devicePath = devinfo.canonicalFilePath();

    QDirIterator it(QLatin1String(pathDiskByLabel), QDir::NoDotAndDotDot);
    while (it.hasNext()) {
        it.next();
        QFileInfo fileInfo(it.fileInfo());
        if (fileInfo.isSymLink() && fileInfo.symLinkTarget() == devicePath)
            return decodeFsEncString(fileInfo.fileName());
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

#if defined(Q_OS_INTEGRITY) || (defined(Q_OS_BSD4) && !defined(Q_OS_NETBSD)) || defined(Q_OS_RTEMS)
        bytesTotal = statfs_buf.f_blocks * statfs_buf.f_bsize;
        bytesFree = statfs_buf.f_bfree * statfs_buf.f_bsize;
        bytesAvailable = statfs_buf.f_bavail * statfs_buf.f_bsize;
#else
        bytesTotal = statfs_buf.f_blocks * statfs_buf.f_frsize;
        bytesFree = statfs_buf.f_bfree * statfs_buf.f_frsize;
        bytesAvailable = statfs_buf.f_bavail * statfs_buf.f_frsize;
#endif
        blockSize = statfs_buf.f_bsize;
#if defined(Q_OS_ANDROID) || defined(Q_OS_BSD4) || defined(Q_OS_INTEGRITY) || defined(Q_OS_RTEMS)
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
        if (!shouldIncludeFs(it))
            continue;

        const QString mountDir = it.rootPath();
        QStorageInfo info(mountDir);
        info.d->device = it.device();
        info.d->fileSystemType = it.fileSystemType();
        info.d->subvolume = it.subvolume();
        if (info.bytesTotal() == 0 && info != root())
            continue;
        volumes.append(info);
    }

    return volumes;
}

QStorageInfo QStorageInfoPrivate::root()
{
    return QStorageInfo(QStringLiteral("/"));
}

QT_END_NAMESPACE
