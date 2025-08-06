// Copyright (C) 2018 Intel Corporation.
// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2013 Samuel Gaist <samuel.gaist@edeltech.ch>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qplatformdefs.h"
#include "qfilesystemengine_p.h"
#include "qfile.h"
#include "qstorageinfo.h"
#include "qurl.h"

#include <QtCore/qoperatingsystemversion.h>
#include <QtCore/private/qcore_unix_p.h>
#include <QtCore/private/qfiledevice_p.h>
#include <QtCore/private/qfunctions_p.h>
#include <QtCore/qvarlengtharray.h>
#ifndef QT_BOOTSTRAPPED
# include <QtCore/qstandardpaths.h>
# include <QtCore/private/qtemporaryfile_p.h>
#endif // QT_BOOTSTRAPPED

#include <grp.h>
#include <pwd.h>
#include <stdlib.h> // for realpath()
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include <chrono>
#include <memory> // for std::unique_ptr

#if __has_include(<paths.h>)
# include <paths.h>
#endif
#ifndef _PATH_TMP           // from <paths.h>
# define _PATH_TMP          "/tmp"
#endif

#if __has_include(<sys/disk.h>)
// BSDs (including Apple Darwin)
# include <sys/disk.h>
#endif

#if defined(Q_OS_DARWIN)
# include <QtCore/private/qcore_mac_p.h>
# include <CoreFoundation/CFBundle.h>
# include <UniformTypeIdentifiers/UTType.h>
# include <UniformTypeIdentifiers/UTCoreTypes.h>
# include <Foundation/Foundation.h>
# include <copyfile.h>
#endif

#ifdef Q_OS_MACOS
#include <CoreServices/CoreServices.h>
#endif

#if defined(QT_PLATFORM_UIKIT)
#include <MobileCoreServices/MobileCoreServices.h>
#endif

#if defined(Q_OS_LINUX)
#  include <sys/ioctl.h>
#  include <sys/sendfile.h>
#  include <linux/fs.h>

// in case linux/fs.h is too old and doesn't define it:
#ifndef FICLONE
#  define FICLONE       _IOW(0x94, 9, int)
#endif
#endif

#if defined(Q_OS_VXWORKS)
# include <sys/statfs.h>
# if __has_include(<dosFsLib.h>)
#  include <dosFsLib.h>
# endif
#endif

#if defined(Q_OS_ANDROID)
// statx() is disabled on Android because quite a few systems
// come with sandboxes that kill applications that make system calls outside a
// whitelist and several Android vendors can't be bothered to update the list.
#  undef STATX_BASIC_STATS
#endif

#ifndef STATX_ALL
struct statx { mode_t stx_mode; };      // dummy
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static QByteArray &removeTrailingSlashes(QByteArray &path)
{
    // Darwin doesn't support trailing /'s, so remove for everyone
    while (path.size() > 1 && path.endsWith('/'))
        path.chop(1);

    return path;
}

enum {
#ifdef Q_OS_ANDROID
    // On Android, the link(2) system call has been observed to always fail
    // with EACCES, regardless of whether there are permission problems or not.
    SupportsHardlinking = false
#else
    SupportsHardlinking = true
#endif
};

#if defined(Q_OS_DARWIN)
static inline bool hasResourcePropertyFlag(const QFileSystemMetaData &data,
                                           const QFileSystemEntry &entry,
                                           CFStringRef key, QCFType<CFURLRef> &url)
{
    if (!url) {
        QCFString path = CFStringCreateWithFileSystemRepresentation(0,
            entry.nativeFilePath().constData());
        if (!path)
           return false;

        url = CFURLCreateWithFileSystemPath(0, path, kCFURLPOSIXPathStyle,
            data.hasFlags(QFileSystemMetaData::DirectoryType));
    }
    if (!url)
        return false;

    CFBooleanRef value;
    if (CFURLCopyResourcePropertyForKey(url, key, &value, NULL)) {
        if (value == kCFBooleanTrue)
            return true;
    }

    return false;
}

static bool isPackage(const QFileSystemMetaData &data, const QFileSystemEntry &entry,
                      QCFType<CFURLRef> &cachedUrl)
{
    if (!data.isDirectory())
        return false;

    QFileInfo info(entry.filePath());
    QString suffix = info.suffix();

    if (suffix.length() > 0) {
        // First step: is it a bundle?
        const auto *utType = [UTType typeWithFilenameExtension:suffix.toNSString()];
        if ([utType conformsToType:UTTypeBundle])
            return true;

        // Second step: check if an application knows the package type
        QCFType<CFStringRef> path = entry.filePath().toCFString();
        QCFType<CFURLRef> url = CFURLCreateWithFileSystemPath(0, path, kCFURLPOSIXPathStyle, true);

        UInt32 type, creator;
        // Well created packages have the PkgInfo file
        if (CFBundleGetPackageInfoInDirectory(url, &type, &creator))
            return true;

#ifdef Q_OS_MACOS
        // Find if an application other than Finder claims to know how to handle the package
        QCFType<CFURLRef> application = LSCopyDefaultApplicationURLForURL(url,
            kLSRolesEditor | kLSRolesViewer, nullptr);

        if (application) {
            QCFType<CFBundleRef> bundle = CFBundleCreate(kCFAllocatorDefault, application);
            CFStringRef identifier = CFBundleGetIdentifier(bundle);
            QString applicationId = QString::fromCFString(identifier);
            if (applicationId != "com.apple.finder"_L1)
                return true;
        }
#endif
    }

    // Third step: check if the directory has the package bit set
    return hasResourcePropertyFlag(data, entry, kCFURLIsPackageKey, cachedUrl);
}
#endif

#ifdef Q_OS_VXWORKS
static inline void forceRequestedPermissionsOnVxWorks(QByteArray dirName, mode_t mode)
{
    if (mode == 0) {
        chmod(dirName, 0);
    }
}
#endif

namespace {
namespace GetFileTimes {
qint64 time_t_toMsecs(time_t t)
{
    using namespace std::chrono;
    return milliseconds{seconds{t}}.count();
}

// fallback set
[[maybe_unused]] qint64 atime(const QT_STATBUF &statBuffer, ulong)
{
    return time_t_toMsecs(statBuffer.st_atime);
}
[[maybe_unused]] qint64 birthtime(const QT_STATBUF &, ulong)
{
    return Q_INT64_C(0);
}
[[maybe_unused]] qint64 ctime(const QT_STATBUF &statBuffer, ulong)
{
    return time_t_toMsecs(statBuffer.st_ctime);
}
[[maybe_unused]] qint64 mtime(const QT_STATBUF &statBuffer, ulong)
{
    return time_t_toMsecs(statBuffer.st_mtime);
}

// T is either a stat.timespec or statx.statx_timestamp,
// both have tv_sec and tv_nsec members
template<typename T>
qint64 timespecToMSecs(const T &spec)
{
    using namespace std::chrono;
    const nanoseconds nsecs = seconds{spec.tv_sec} + nanoseconds{spec.tv_nsec};
    return duration_cast<milliseconds>(nsecs).count();
}

// Xtim, POSIX.1-2008
template <typename T>
[[maybe_unused]] static typename std::enable_if<(&T::st_atim, true), qint64>::type
atime(const T &statBuffer, int)
{ return timespecToMSecs(statBuffer.st_atim); }

template <typename T>
[[maybe_unused]] static typename std::enable_if<(&T::st_birthtim, true), qint64>::type
birthtime(const T &statBuffer, int)
{ return timespecToMSecs(statBuffer.st_birthtim); }

template <typename T>
[[maybe_unused]] static typename std::enable_if<(&T::st_ctim, true), qint64>::type
ctime(const T &statBuffer, int)
{ return timespecToMSecs(statBuffer.st_ctim); }

template <typename T>
[[maybe_unused]] static typename std::enable_if<(&T::st_mtim, true), qint64>::type
mtime(const T &statBuffer, int)
{ return timespecToMSecs(statBuffer.st_mtim); }

#ifndef st_mtimespec
// Xtimespec
template <typename T>
[[maybe_unused]] static typename std::enable_if<(&T::st_atimespec, true), qint64>::type
atime(const T &statBuffer, int)
{ return timespecToMSecs(statBuffer.st_atimespec); }

template <typename T>
[[maybe_unused]] static typename std::enable_if<(&T::st_birthtimespec, true), qint64>::type
birthtime(const T &statBuffer, int)
{ return timespecToMSecs(statBuffer.st_birthtimespec); }

template <typename T>
[[maybe_unused]] static typename std::enable_if<(&T::st_ctimespec, true), qint64>::type
ctime(const T &statBuffer, int)
{ return timespecToMSecs(statBuffer.st_ctimespec); }

template <typename T>
[[maybe_unused]] static typename std::enable_if<(&T::st_mtimespec, true), qint64>::type
mtime(const T &statBuffer, int)
{ return timespecToMSecs(statBuffer.st_mtimespec); }
#endif

#if !defined(st_mtimensec) && !defined(__alpha__)
// Xtimensec
template <typename T>
[[maybe_unused]] static typename std::enable_if<(&T::st_atimensec, true), qint64>::type
atime(const T &statBuffer, int)
{ return statBuffer.st_atime * Q_INT64_C(1000) + statBuffer.st_atimensec / 1000000; }

template <typename T>
[[maybe_unused]] static typename std::enable_if<(&T::st_birthtimensec, true), qint64>::type
birthtime(const T &statBuffer, int)
{ return statBuffer.st_birthtime * Q_INT64_C(1000) + statBuffer.st_birthtimensec / 1000000; }

template <typename T>
[[maybe_unused]] static typename std::enable_if<(&T::st_ctimensec, true), qint64>::type
ctime(const T &statBuffer, int)
{ return statBuffer.st_ctime * Q_INT64_C(1000) + statBuffer.st_ctimensec / 1000000; }

template <typename T>
[[maybe_unused]] static typename std::enable_if<(&T::st_mtimensec, true), qint64>::type
mtime(const T &statBuffer, int)
{ return statBuffer.st_mtime * Q_INT64_C(1000) + statBuffer.st_mtimensec / 1000000; }
#endif
} // namespace GetFileTimes
} // unnamed namespace

// converts QT_STATBUF::st_mode to QFSMD
// the \a attributes parameter is OS-specific
static QFileSystemMetaData::MetaDataFlags
flagsFromStMode(mode_t mode, [[maybe_unused]] quint64 attributes)
{
    // inode exists
    QFileSystemMetaData::MetaDataFlags entryFlags = QFileSystemMetaData::ExistsAttribute;

    if (mode & S_IRUSR)
        entryFlags |= QFileSystemMetaData::OwnerReadPermission;
    if (mode & S_IWUSR)
        entryFlags |= QFileSystemMetaData::OwnerWritePermission;
    if (mode & S_IXUSR)
        entryFlags |= QFileSystemMetaData::OwnerExecutePermission;

    if (mode & S_IRGRP)
        entryFlags |= QFileSystemMetaData::GroupReadPermission;
    if (mode & S_IWGRP)
        entryFlags |= QFileSystemMetaData::GroupWritePermission;
    if (mode & S_IXGRP)
        entryFlags |= QFileSystemMetaData::GroupExecutePermission;

    if (mode & S_IROTH)
        entryFlags |= QFileSystemMetaData::OtherReadPermission;
    if (mode & S_IWOTH)
        entryFlags |= QFileSystemMetaData::OtherWritePermission;
    if (mode & S_IXOTH)
        entryFlags |= QFileSystemMetaData::OtherExecutePermission;

    // Type
    Q_ASSERT(!S_ISLNK(mode));   // can only happen with lstat()
    if ((mode & S_IFMT) == S_IFREG)
        entryFlags |= QFileSystemMetaData::FileType;
    else if ((mode & S_IFMT) == S_IFDIR)
        entryFlags |= QFileSystemMetaData::DirectoryType;
    else if ((mode & S_IFMT) != S_IFBLK)    // char devices, sockets, FIFOs
        entryFlags |= QFileSystemMetaData::SequentialType;

    // OS-specific flags
    // Potential flags for the future:
    // UF_APPEND and STATX_ATTR_APPEND
    // UF_COMPRESSED and STATX_ATTR_COMPRESSED
    // UF_IMMUTABLE and STATX_ATTR_IMMUTABLE
    // UF_NODUMP and STATX_ATTR_NODUMP

#if defined(Q_OS_VXWORKS) && __has_include(<dosFsLib.h>)
    if (attributes & DOS_ATTR_RDONLY) {
        // on a DOS FS, stat() always returns 0777 bits set in st_mode
        // when DOS FS is read only the write permissions are removed
        entryFlags &= ~QFileSystemMetaData::OwnerWritePermission;
        entryFlags &= ~QFileSystemMetaData::GroupWritePermission;
        entryFlags &= ~QFileSystemMetaData::OtherWritePermission;
    }
#endif
    return entryFlags;
}

#ifdef STATX_BASIC_STATS
static int qt_real_statx(int fd, const char *pathname, int flags, struct statx *statxBuffer)
{
    unsigned mask = STATX_BASIC_STATS | STATX_BTIME;
    int ret = statx(fd, pathname, flags | AT_NO_AUTOMOUNT, mask, statxBuffer);
    return ret == -1 ? -errno : 0;
}

static int qt_statx(const char *pathname, struct statx *statxBuffer)
{
    return qt_real_statx(AT_FDCWD, pathname, 0, statxBuffer);
}

static int qt_lstatx(const char *pathname, struct statx *statxBuffer)
{
    return qt_real_statx(AT_FDCWD, pathname, AT_SYMLINK_NOFOLLOW, statxBuffer);
}

static int qt_fstatx(int fd, struct statx *statxBuffer)
{
    return qt_real_statx(fd, "", AT_EMPTY_PATH, statxBuffer);
}

inline void QFileSystemMetaData::fillFromStatxBuf(const struct statx &statxBuffer)
{
    // Permissions
    MetaDataFlags flags = flagsFromStMode(statxBuffer.stx_mode, statxBuffer.stx_attributes);
    entryFlags |= flags;
    knownFlagsMask |= flags | PosixStatFlags;

    // Attributes
    if (statxBuffer.stx_nlink == 0)
        entryFlags |= QFileSystemMetaData::WasDeletedAttribute;
    size_ = qint64(statxBuffer.stx_size);

    // Times
    using namespace GetFileTimes;
    accessTime_ = timespecToMSecs(statxBuffer.stx_atime);
    metadataChangeTime_ = timespecToMSecs(statxBuffer.stx_ctime);
    modificationTime_ = timespecToMSecs(statxBuffer.stx_mtime);
    const bool birthMask = statxBuffer.stx_mask & STATX_BTIME;
    birthTime_ = birthMask ? timespecToMSecs(statxBuffer.stx_btime) : 0;

    userId_ = statxBuffer.stx_uid;
    groupId_ = statxBuffer.stx_gid;
}
#else
static int qt_statx(const char *, struct statx *)
{ return -ENOSYS; }

static int qt_lstatx(const char *, struct statx *)
{ return -ENOSYS; }

static int qt_fstatx(int, struct statx *)
{ return -ENOSYS; }

inline void QFileSystemMetaData::fillFromStatxBuf(const struct statx &)
{ }
#endif

//static
bool QFileSystemEngine::fillMetaData(int fd, QFileSystemMetaData &data)
{
    auto getSizeForBlockDev = [&](mode_t st_mode) {
#ifdef BLKGETSIZE64
        // Linux
        if (quint64 sz; (st_mode & S_IFMT) == S_IFBLK && ioctl(fd, BLKGETSIZE64, &sz) == 0)
            data.size_ = sz;        // returns byte count
#elif defined(BLKGETSIZE)
        // older Linux
        if (ulong sz; (st_mode & S_IFMT) == S_IFBLK && ioctl(fd, BLKGETSIZE, &sz) == 0)
            data.size_ = sz * 512;  // returns 512-byte sector count
#elif defined(DKIOCGETBLOCKCOUNT)
        // Apple Darwin
        qint32 blksz;
        if (quint64 count; (st_mode & S_IFMT) == S_IFBLK
                && ioctl(fd, DKIOCGETBLOCKCOUNT, &count) == 0
                && ioctl(fd, DKIOCGETBLOCKSIZE, &blksz) == 0)
            data.size_ = count * blksz;
#elif defined(DIOCGMEDIASIZE)
        // FreeBSD
        // see Linux-compat implementation in
        // http://fxr.watson.org/fxr/source/compat/linux/linux_ioctl.c?v=FREEBSD-13-STABLE#L282
        // S_IFCHR is correct: FreeBSD doesn't have block devices any more
        if (QT_OFF_T sz; (st_mode & S_IFMT) == S_IFCHR && ioctl(fd, DIOCGMEDIASIZE, &sz) == 0)
            data.size_ = sz;        // returns byte count
#else
        Q_UNUSED(st_mode);
#endif
    };
    data.entryFlags &= ~QFileSystemMetaData::PosixStatFlags;
    data.knownFlagsMask |= QFileSystemMetaData::PosixStatFlags;

    struct statx statxBuffer;

    int ret = qt_fstatx(fd, &statxBuffer);
    if (ret != -ENOSYS) {
        if (ret == 0) {
            data.fillFromStatxBuf(statxBuffer);
            getSizeForBlockDev(statxBuffer.stx_mode);
            return true;
        }
        return false;
    }

    QT_STATBUF statBuffer;

    if (QT_FSTAT(fd, &statBuffer) == 0) {
        data.fillFromStatBuf(statBuffer);
        getSizeForBlockDev(statBuffer.st_mode);
        return true;
    }

    return false;
}

#if defined(_DEXTRA_FIRST)
static void fillStat64fromStat32(struct stat64 *statBuf64, const struct stat &statBuf32)
{
    statBuf64->st_mode = statBuf32.st_mode;
    statBuf64->st_size = statBuf32.st_size;
#if _POSIX_VERSION >= 200809L
    statBuf64->st_ctim = statBuf32.st_ctim;
    statBuf64->st_mtim = statBuf32.st_mtim;
    statBuf64->st_atim = statBuf32.st_atim;
#else
    statBuf64->st_ctime = statBuf32.st_ctime;
    statBuf64->st_mtime = statBuf32.st_mtime;
    statBuf64->st_atime = statBuf32.st_atime;
#endif
    statBuf64->st_uid = statBuf32.st_uid;
    statBuf64->st_gid = statBuf32.st_gid;
}
#endif

void QFileSystemMetaData::fillFromStatBuf(const QT_STATBUF &statBuffer)
{
    quint64 attributes = 0;
#if defined(UF_SETTABLE)        // BSDs (incl. Darwin)
    attributes = statBuffer.st_flags;
#elif defined(Q_OS_VXWORKS) && __has_include(<dosFsLib.h>)
    attributes = statBuffer.st_attrib;
#endif
    // Permissions
    MetaDataFlags flags = flagsFromStMode(statBuffer.st_mode, attributes);
    entryFlags |= flags;
    knownFlagsMask |= flags | PosixStatFlags;

    // Attributes
    if (statBuffer.st_nlink == 0)
        entryFlags |= QFileSystemMetaData::WasDeletedAttribute;
    size_ = statBuffer.st_size;

    // Times
    accessTime_ = GetFileTimes::atime(statBuffer, 0);
    birthTime_ = GetFileTimes::birthtime(statBuffer, 0);
    metadataChangeTime_ = GetFileTimes::ctime(statBuffer, 0);
    modificationTime_ = GetFileTimes::mtime(statBuffer, 0);

    userId_ = statBuffer.st_uid;
    groupId_ = statBuffer.st_gid;
}

void QFileSystemMetaData::fillFromDirEnt(const QT_DIRENT &entry)
{
#if defined(_DEXTRA_FIRST)
    knownFlagsMask = {};
    entryFlags = {};
    for (dirent_extra *extra = _DEXTRA_FIRST(&entry); _DEXTRA_VALID(extra, &entry);
         extra = _DEXTRA_NEXT(extra)) {
        if (extra->d_type == _DTYPE_STAT || extra->d_type == _DTYPE_LSTAT) {

            const struct dirent_extra_stat * const extra_stat =
                    reinterpret_cast<struct dirent_extra_stat *>(extra);

            // Remember whether this was a link or not, this saves an lstat() call later.
            if (extra->d_type == _DTYPE_LSTAT) {
                knownFlagsMask |= QFileSystemMetaData::LinkType;
                if (S_ISLNK(extra_stat->d_stat.st_mode))
                    entryFlags |= QFileSystemMetaData::LinkType;
            }

            // For symlinks, the extra type _DTYPE_LSTAT doesn't work for filling out the meta data,
            // as we need the stat() information there, not the lstat() information.
            // In this case, don't use the extra information.
            // Unfortunately, readdir() never seems to return extra info of type _DTYPE_STAT, so for
            // symlinks, we always incur the cost of an extra stat() call later.
            if (S_ISLNK(extra_stat->d_stat.st_mode) && extra->d_type == _DTYPE_LSTAT)
                continue;

#if defined(QT_USE_XOPEN_LFS_EXTENSIONS) && defined(QT_LARGEFILE_SUPPORT)
            // Even with large file support, d_stat is always of type struct stat, not struct stat64,
            // so it needs to be converted
            struct stat64 statBuf;
            fillStat64fromStat32(&statBuf, extra_stat->d_stat);
            fillFromStatBuf(statBuf);
#else
            fillFromStatBuf(extra_stat->d_stat);
#endif
            knownFlagsMask |= QFileSystemMetaData::PosixStatFlags;
            if (!S_ISLNK(extra_stat->d_stat.st_mode)) {
                knownFlagsMask |= QFileSystemMetaData::ExistsAttribute;
                entryFlags |= QFileSystemMetaData::ExistsAttribute;
            }
        }
    }
#elif defined(_DIRENT_HAVE_D_TYPE) || defined(Q_OS_BSD4)
    // BSD4 includes OS X and iOS

    // ### This will clear all entry flags and knownFlagsMask
    switch (entry.d_type)
    {
    case DT_DIR:
        knownFlagsMask = QFileSystemMetaData::LinkType
            | QFileSystemMetaData::FileType
            | QFileSystemMetaData::DirectoryType
            | QFileSystemMetaData::SequentialType
            | QFileSystemMetaData::ExistsAttribute;

        entryFlags = QFileSystemMetaData::DirectoryType
            | QFileSystemMetaData::ExistsAttribute;

        break;

    case DT_BLK:
        knownFlagsMask = QFileSystemMetaData::LinkType
            | QFileSystemMetaData::FileType
            | QFileSystemMetaData::DirectoryType
            | QFileSystemMetaData::BundleType
            | QFileSystemMetaData::AliasType
            | QFileSystemMetaData::SequentialType
            | QFileSystemMetaData::ExistsAttribute;

        entryFlags = QFileSystemMetaData::ExistsAttribute;

        break;

    case DT_CHR:
    case DT_FIFO:
    case DT_SOCK:
        // ### System attribute
        knownFlagsMask = QFileSystemMetaData::LinkType
            | QFileSystemMetaData::FileType
            | QFileSystemMetaData::DirectoryType
            | QFileSystemMetaData::BundleType
            | QFileSystemMetaData::AliasType
            | QFileSystemMetaData::SequentialType
            | QFileSystemMetaData::ExistsAttribute;

        entryFlags = QFileSystemMetaData::SequentialType
            | QFileSystemMetaData::ExistsAttribute;

        break;

    case DT_LNK:
        knownFlagsMask = QFileSystemMetaData::LinkType;
        entryFlags = QFileSystemMetaData::LinkType;
        break;

    case DT_REG:
        knownFlagsMask = QFileSystemMetaData::LinkType
            | QFileSystemMetaData::FileType
            | QFileSystemMetaData::DirectoryType
            | QFileSystemMetaData::BundleType
            | QFileSystemMetaData::SequentialType
            | QFileSystemMetaData::ExistsAttribute;

        entryFlags = QFileSystemMetaData::FileType
            | QFileSystemMetaData::ExistsAttribute;

        break;

    case DT_UNKNOWN:
    default:
        clear();
    }
#else
    Q_UNUSED(entry);
#endif
}

//static
QFileSystemEntry QFileSystemEngine::getLinkTarget(const QFileSystemEntry &link, QFileSystemMetaData &data)
{
    Q_CHECK_FILE_NAME(link, link);

    QByteArray s = qt_readlink(link.nativeFilePath().constData());
    if (s.size() > 0) {
        QString ret;
        if (!data.hasFlags(QFileSystemMetaData::DirectoryType))
            fillMetaData(link, data, QFileSystemMetaData::DirectoryType);
        if (data.isDirectory() && s[0] != '/') {
            QDir parent(link.filePath());
            parent.cdUp();
            ret = parent.path();
            if (!ret.isEmpty() && !ret.endsWith(u'/'))
                ret += u'/';
        }
        ret += QFile::decodeName(s);

        if (!ret.startsWith(u'/'))
            ret.prepend(absoluteName(link).path() + u'/');
        ret = QDir::cleanPath(ret);
        if (ret.size() > 1 && ret.endsWith(u'/'))
            ret.chop(1);
        return QFileSystemEntry(ret);
    }
#if defined(Q_OS_DARWIN)
    {
        QCFString path = CFStringCreateWithFileSystemRepresentation(0,
            QFile::encodeName(QDir::cleanPath(link.filePath())).data());
        if (!path)
            return QFileSystemEntry();

        QCFType<CFURLRef> url = CFURLCreateWithFileSystemPath(0, path, kCFURLPOSIXPathStyle,
            data.hasFlags(QFileSystemMetaData::DirectoryType));
        if (!url)
            return QFileSystemEntry();

        QCFType<CFDataRef> bookmarkData = CFURLCreateBookmarkDataFromFile(0, url, NULL);
        if (!bookmarkData)
            return QFileSystemEntry();

        QCFType<CFURLRef> resolvedUrl = CFURLCreateByResolvingBookmarkData(0,
            bookmarkData,
            (CFURLBookmarkResolutionOptions)(kCFBookmarkResolutionWithoutUIMask
                | kCFBookmarkResolutionWithoutMountingMask), NULL, NULL, NULL, NULL);
        if (!resolvedUrl)
            return QFileSystemEntry();

        QCFString cfstr(CFURLCopyFileSystemPath(resolvedUrl, kCFURLPOSIXPathStyle));
        if (!cfstr)
            return QFileSystemEntry();

        return QFileSystemEntry(QString::fromCFString(cfstr));
    }
#endif
    return QFileSystemEntry();
}

//static
QFileSystemEntry QFileSystemEngine::getRawLinkPath(const QFileSystemEntry &link,
                                                   QFileSystemMetaData &data)
{
    Q_UNUSED(data)
    const QByteArray path = qt_readlink(link.nativeFilePath().constData());
    const QString ret = QFile::decodeName(path);
    return QFileSystemEntry(ret);
}

//static
QFileSystemEntry QFileSystemEngine::canonicalName(const QFileSystemEntry &entry, QFileSystemMetaData &data)
{
    Q_CHECK_FILE_NAME(entry, entry);
    char *resolved_name = nullptr;

#ifdef PATH_MAX
    // use the stack to avoid the overhead of memory allocation
    char stack_result[PATH_MAX + 1];
#else
    // system with unlimited file paths -> must use heap
    std::nullptr_t stack_result = nullptr;
    auto freer = qScopeGuard([&] { free(resolved_name); });
#endif

# if defined(Q_OS_DARWIN) || defined(Q_OS_ANDROID)
    // On some Android and macOS versions, realpath() will return a path even if
    // it does not exist. To work around this, we check existence in advance.
    if (!data.hasFlags(QFileSystemMetaData::ExistsAttribute))
        fillMetaData(entry, data, QFileSystemMetaData::ExistsAttribute);

    if (!data.exists())
        errno = ENOENT;
    else
        resolved_name = realpath(entry.nativeFilePath().constData(), stack_result);
# else
    resolved_name = realpath(entry.nativeFilePath().constData(), stack_result);
# endif
    if (resolved_name) {
        data.knownFlagsMask |= QFileSystemMetaData::ExistsAttribute;
        data.entryFlags |= QFileSystemMetaData::ExistsAttribute;
        return QFileSystemEntry(resolved_name, QFileSystemEntry::FromNativePath{});
    } else if (errno == ENOENT || errno == ENOTDIR) { // file doesn't exist
        data.knownFlagsMask |= QFileSystemMetaData::ExistsAttribute;
        data.entryFlags &= ~(QFileSystemMetaData::ExistsAttribute);
        return QFileSystemEntry();
    }
    return entry;
}

//static
QFileSystemEntry QFileSystemEngine::absoluteName(const QFileSystemEntry &entry)
{
    Q_CHECK_FILE_NAME(entry, entry);

    if (entry.isAbsolute() && entry.isClean())
        return entry;

    QByteArray orig = entry.nativeFilePath();
    QByteArray result;
    if (orig.isEmpty() || !orig.startsWith('/')) {
        QFileSystemEntry cur(currentPath());
        result = cur.nativeFilePath();
    }
    if (!orig.isEmpty() && !(orig.size() == 1 && orig[0] == '.')) {
        if (!result.isEmpty() && !result.endsWith('/'))
            result.append('/');
        result.append(orig);
    }

    if (result.size() == 1 && result[0] == '/')
        return QFileSystemEntry(result, QFileSystemEntry::FromNativePath());
    const bool isDir = result.endsWith('/');

    /* as long as QDir::cleanPath() operates on a QString we have to convert to a string here.
     * ideally we never convert to a string since that loses information. Please fix after
     * we get a QByteArray version of QDir::cleanPath()
     */
    QFileSystemEntry resultingEntry(result, QFileSystemEntry::FromNativePath());
    QString stringVersion = QDir::cleanPath(resultingEntry.filePath());
    if (isDir)
        stringVersion.append(u'/');
    return QFileSystemEntry(stringVersion);
}

//static
QByteArray QFileSystemEngine::id(const QFileSystemEntry &entry)
{
    Q_CHECK_FILE_NAME(entry, QByteArray());

    QT_STATBUF statResult;
    if (QT_STAT(entry.nativeFilePath().constData(), &statResult)) {
        if (errno != ENOENT)
            qErrnoWarning("stat() failed for '%s'", entry.nativeFilePath().constData());
        return QByteArray();
    }
    QByteArray result = QByteArray::number(quint64(statResult.st_dev), 16);
    result += ':';
    result += QByteArray::number(quint64(statResult.st_ino));
    return result;
}

//static
QByteArray QFileSystemEngine::id(int fd)
{
    QT_STATBUF statResult;
    if (QT_FSTAT(fd, &statResult)) {
        qErrnoWarning("fstat() failed for fd %d", fd);
        return QByteArray();
    }
    QByteArray result = QByteArray::number(quint64(statResult.st_dev), 16);
    result += ':';
    result += QByteArray::number(quint64(statResult.st_ino));
    return result;
}

//static
QString QFileSystemEngine::resolveUserName(uint userId)
{
#if QT_CONFIG(thread) && defined(_POSIX_THREAD_SAFE_FUNCTIONS) && !defined(Q_OS_OPENBSD)
    long size_max = sysconf(_SC_GETPW_R_SIZE_MAX);
    if (size_max == -1)
        size_max = 1024;
    QVarLengthArray<char, 1024> buf(size_max);
#endif

#if !defined(Q_OS_INTEGRITY) && !defined(Q_OS_WASM)
    struct passwd *pw = nullptr;
#if QT_CONFIG(thread) && defined(_POSIX_THREAD_SAFE_FUNCTIONS) && !defined(Q_OS_OPENBSD) && !defined(Q_OS_VXWORKS)
    struct passwd entry;
    getpwuid_r(userId, &entry, buf.data(), buf.size(), &pw);
#else
    pw = getpwuid(userId);
#endif
    if (pw)
        return QFile::decodeName(QByteArray(pw->pw_name));
#else // Integrity || WASM
    Q_UNUSED(userId);
#endif
    return QString();
}

//static
QString QFileSystemEngine::resolveGroupName(uint groupId)
{
#if QT_CONFIG(thread) && defined(_POSIX_THREAD_SAFE_FUNCTIONS) && !defined(Q_OS_OPENBSD)
    long size_max = sysconf(_SC_GETPW_R_SIZE_MAX);
    if (size_max == -1)
        size_max = 1024;
    QVarLengthArray<char, 1024> buf(size_max);
#endif

#if !defined(Q_OS_INTEGRITY) && !defined(Q_OS_WASM)
    struct group *gr = nullptr;
#if QT_CONFIG(thread) && defined(_POSIX_THREAD_SAFE_FUNCTIONS) && !defined(Q_OS_OPENBSD) && !defined(Q_OS_VXWORKS) && (!defined(Q_OS_ANDROID) || defined(Q_OS_ANDROID) && (__ANDROID_API__ >= 24))
    size_max = sysconf(_SC_GETGR_R_SIZE_MAX);
    if (size_max == -1)
        size_max = 1024;
    buf.resize(size_max);
    struct group entry;
    // Some large systems have more members than the POSIX max size
    // Loop over by doubling the buffer size (upper limit 250k)
    for (long size = size_max; size < 256000; size += size)
    {
        buf.resize(size);
        // ERANGE indicates that the buffer was too small
        if (!getgrgid_r(groupId, &entry, buf.data(), buf.size(), &gr)
            || errno != ERANGE)
            break;
    }
#else
    gr = getgrgid(groupId);
#endif
    if (gr)
        return QFile::decodeName(QByteArray(gr->gr_name));
#else // Integrity || WASM || VxWorks
    Q_UNUSED(groupId);
#endif
    return QString();
}

#if defined(Q_OS_DARWIN)
//static
QString QFileSystemEngine::bundleName(const QFileSystemEntry &entry)
{
    QCFType<CFURLRef> url = CFURLCreateWithFileSystemPath(0, QCFString(entry.filePath()),
            kCFURLPOSIXPathStyle, true);
    if (QCFType<CFDictionaryRef> dict = CFBundleCopyInfoDictionaryForURL(url)) {
        if (CFTypeRef name = (CFTypeRef)CFDictionaryGetValue(dict, kCFBundleNameKey)) {
            if (CFGetTypeID(name) == CFStringGetTypeID())
                return QString::fromCFString((CFStringRef)name);
        }
    }
    return QString();
}
#endif

//static
bool QFileSystemEngine::fillMetaData(const QFileSystemEntry &entry, QFileSystemMetaData &data,
        QFileSystemMetaData::MetaDataFlags what)
{
    Q_CHECK_FILE_NAME(entry, false);

    // Detection of WasDeletedAttribute is imperfect: in general, if we can
    // successfully stat() or access() a file, it hasn't been deleted (though
    // there are exceptions, like /proc/XXX/fd/ entries on Linux). So we have
    // to restore this flag in case we fail to stat() anything.
    auto hadBeenDeleted = data.entryFlags & QFileSystemMetaData::WasDeletedAttribute;

#if defined(Q_OS_DARWIN)
    if (what & (QFileSystemMetaData::BundleType | QFileSystemMetaData::CaseSensitive)) {
        if (!data.hasFlags(QFileSystemMetaData::DirectoryType))
            what |= QFileSystemMetaData::DirectoryType;
    }
    if (what & QFileSystemMetaData::AliasType)
        what |= QFileSystemMetaData::LinkType;
#endif // defined(Q_OS_DARWIN)

    bool needLstat = what.testAnyFlag(QFileSystemMetaData::LinkType);

#ifdef UF_HIDDEN
    if (what & QFileSystemMetaData::HiddenAttribute) {
        // Some OSes (BSDs) have the ability to mark directory entries as
        // hidden besides the usual Unix way of naming them with a leading dot.
        // For those OSes, we must lstat() the entry itself so we can find
        // out if a symlink is hidden or not.
        needLstat = true;
    }
#endif // defined(Q_OS_DARWIN)

    // if we're asking for any of the stat(2) flags, then we're getting them all
    if (what & QFileSystemMetaData::PosixStatFlags)
        what |= QFileSystemMetaData::PosixStatFlags;

    data.entryFlags &= ~what;

    const QByteArray nativeFilePath = entry.nativeFilePath();
    int entryErrno = 0; // innocent until proven otherwise

    // first, we may try lstat(2). Possible outcomes:
    //  - success and is a symlink: filesystem entry exists, but we need stat(2)
    //    -> statResult = -1;
    //  - success and is not a symlink: filesystem entry exists and we're done
    //    -> statResult = 0
    //  - failure: really non-existent filesystem entry
    //    -> entryExists = false; statResult = 0;
    //    both stat(2) and lstat(2) may generate a number of different errno
    //    conditions, but of those, the only ones that could happen and the
    //    entry still exist are EACCES, EFAULT, ENOMEM and EOVERFLOW. If we get
    //    EACCES or ENOMEM, then we have no choice on how to proceed, so we may
    //    as well conclude it doesn't exist; EFAULT can't happen and EOVERFLOW
    //    shouldn't happen because we build in _LARGEFIE64.
    union {
        QT_STATBUF statBuffer;
        struct statx statxBuffer;
    };
    int statResult = -1;
    if (needLstat) {
        mode_t mode = 0;
        statResult = qt_lstatx(nativeFilePath, &statxBuffer);
        if (statResult == -ENOSYS) {
            // use lstst(2)
            statResult = QT_LSTAT(nativeFilePath, &statBuffer);
            if (statResult == 0)
                mode = statBuffer.st_mode;
        } else if (statResult == 0) {
            statResult = 1; // record it was statx(2) that succeeded
            mode = statxBuffer.stx_mode;
        }

        if (statResult >= 0) {
#ifdef UF_HIDDEN
            // currently only supported on systems with no statx() call
            Q_ASSERT(statResult == 0);
            if (statBuffer.st_flags & UF_HIDDEN)
                data.entryFlags |= QFileSystemMetaData::HiddenAttribute;
            data.knownFlagsMask |= QFileSystemMetaData::HiddenAttribute;
#endif

            if (S_ISLNK(mode)) {
               // it's a symlink, we don't know if the file "exists"
                data.entryFlags |= QFileSystemMetaData::LinkType;
                statResult = -1;    // force stat(2) below
            } else {
                // it's a reagular file and it exists
                if (statResult)
                    data.fillFromStatxBuf(statxBuffer);
                else
                    data.fillFromStatBuf(statBuffer);
            }
        } else {
            // it doesn't exist
            entryErrno = errno;
            statResult = -1;
            data.knownFlagsMask |= QFileSystemMetaData::ExistsAttribute;
        }

        data.knownFlagsMask |= QFileSystemMetaData::LinkType;
    }

    // second, we try a regular stat(2)
    if (statResult == -1 && (what & QFileSystemMetaData::PosixStatFlags)) {
        if (entryErrno == 0) {
            data.entryFlags &= ~QFileSystemMetaData::PosixStatFlags;
            statResult = qt_statx(nativeFilePath, &statxBuffer);
            if (statResult == -ENOSYS) {
                // use stat(2)
                statResult = QT_STAT(nativeFilePath, &statBuffer);
                if (statResult == 0)
                    data.fillFromStatBuf(statBuffer);
            } else if (statResult == 0) {
                data.fillFromStatxBuf(statxBuffer);
            }
        }

        if (statResult != 0) {
            entryErrno = errno;
            data.birthTime_ = 0;
            data.metadataChangeTime_ = 0;
            data.modificationTime_ = 0;
            data.accessTime_ = 0;
            data.size_ = 0;
            data.userId_ = (uint) -2;
            data.groupId_ = (uint) -2;

            // reset the mask
            data.knownFlagsMask |= QFileSystemMetaData::PosixStatFlags
                | QFileSystemMetaData::ExistsAttribute;
        }
    }

    // third, we try access(2)
    if (what & (QFileSystemMetaData::UserPermissions | QFileSystemMetaData::ExistsAttribute)) {
#if defined(Q_OS_VXWORKS)
        // on VxWorks if the filesystem is not POSIX, access() always returns false, despite the
        // file is readable
        struct statfs statBuf;
        if (statfs(nativeFilePath, &statBuf) != 0) {
            what &= ~QFileSystemMetaData::LinkType; // don't clear link: could be broken symlink
            data.clearFlags(what);
            return false;
        }
        if (statBuf.f_type != NFSV2_MAGIC && statBuf.f_type != NFSV3_MAGIC &&
            statBuf.f_type != HRFS_MAGIC) {
#if __has_include(<dosFsLib.h>)
            if (data.entryFlags & QFileSystemMetaData::OwnerWritePermission) {
                data.entryFlags |= QFileSystemMetaData::UserWritePermission;
            }
            if (data.entryFlags & QFileSystemMetaData::OwnerExecutePermission) {
                data.entryFlags |= QFileSystemMetaData::UserExecutePermission;
            }
#endif
            data.entryFlags |= QFileSystemMetaData::UserReadPermission |
                    QFileSystemMetaData::ExistsAttribute;
            return true;
        }
#endif
        // calculate user permissions
        auto checkAccess = [&](QFileSystemMetaData::MetaDataFlag flag, int mode) {
            if (entryErrno != 0 || (what & flag) == 0)
                return;
            if (QT_ACCESS(nativeFilePath, mode) == 0) {
                // access ok (and file exists)
                data.entryFlags |= flag | QFileSystemMetaData::ExistsAttribute;
            } else if (errno != EACCES && errno != EROFS) {
                entryErrno = errno;
            }
        };
        checkAccess(QFileSystemMetaData::UserReadPermission, R_OK);
        checkAccess(QFileSystemMetaData::UserWritePermission, W_OK);
        checkAccess(QFileSystemMetaData::UserExecutePermission, X_OK);

        // if we still haven't found out if the file exists, try F_OK
        if (entryErrno == 0 && (data.entryFlags & QFileSystemMetaData::ExistsAttribute) == 0) {
            if (QT_ACCESS(nativeFilePath, F_OK) == -1)
                entryErrno = errno;
            else
                data.entryFlags |= QFileSystemMetaData::ExistsAttribute;
        }

        data.knownFlagsMask |= (what & QFileSystemMetaData::UserPermissions) |
                QFileSystemMetaData::ExistsAttribute;
    }

#if defined(Q_OS_DARWIN)
    QCFType<CFURLRef> cachedUrl;
    if (what & QFileSystemMetaData::AliasType) {
        if (entryErrno == 0 && hasResourcePropertyFlag(data, entry, kCFURLIsAliasFileKey, cachedUrl)) {
            // kCFURLIsAliasFileKey includes symbolic links, so filter those out
            if (!(data.entryFlags & QFileSystemMetaData::LinkType))
                data.entryFlags |= QFileSystemMetaData::AliasType;
        }
        data.knownFlagsMask |= QFileSystemMetaData::AliasType;
    }

    if (what & QFileSystemMetaData::BundleType) {
        if (entryErrno == 0 && isPackage(data, entry, cachedUrl))
            data.entryFlags |= QFileSystemMetaData::BundleType;

        data.knownFlagsMask |= QFileSystemMetaData::BundleType;
    }

    if (what & QFileSystemMetaData::CaseSensitive) {
        if (entryErrno == 0 && hasResourcePropertyFlag(data, entry,
            kCFURLVolumeSupportsCaseSensitiveNamesKey, cachedUrl))
            data.entryFlags |= QFileSystemMetaData::CaseSensitive;
        data.knownFlagsMask |= QFileSystemMetaData::CaseSensitive;
    }
#endif

    if (what & QFileSystemMetaData::HiddenAttribute
            && !data.isHidden()) {        
        // reusing nativeFilePath from above instead of entry.fileName(), to
        // avoid memory allocation for the QString result.
        qsizetype lastSlash = nativeFilePath.size();

        while (lastSlash && nativeFilePath.at(lastSlash - 1) == '/')
            --lastSlash;        // skip ending slashes
        while (lastSlash && nativeFilePath.at(lastSlash - 1) != '/')
            --lastSlash;        // skip non-slashes
        --lastSlash;            // point to the slash or -1 if no slash

        if (nativeFilePath.at(lastSlash + 1) == '.')
            data.entryFlags |= QFileSystemMetaData::HiddenAttribute;
        data.knownFlagsMask |= QFileSystemMetaData::HiddenAttribute;
    }

    if (entryErrno != 0) {
        what &= ~QFileSystemMetaData::LinkType; // don't clear link: could be broken symlink
        data.clearFlags(what);

        // see comment at the top
        data.entryFlags |= hadBeenDeleted;
        data.knownFlagsMask |= hadBeenDeleted;

        return false;
    }
    return true;
}

// static
auto QFileSystemEngine::cloneFile(int srcfd, int dstfd, const QFileSystemMetaData &knownData) -> TriStateResult
{
    QT_STATBUF statBuffer;
    if (knownData.hasFlags(QFileSystemMetaData::PosixStatFlags) &&
            knownData.isFile()) {
        statBuffer.st_mode = S_IFREG;
    } else if (knownData.hasFlags(QFileSystemMetaData::PosixStatFlags) &&
               knownData.isDirectory()) {
        errno = EISDIR;
        return TriStateResult::Failed;   // fcopyfile(3) returns success on directories
    } else if (QT_FSTAT(srcfd, &statBuffer) == -1) {
        // errno was set
        return TriStateResult::Failed;
    } else if (!S_ISREG((statBuffer.st_mode))) {
        // not a regular file, let QFile do the copy
        return TriStateResult::NotSupported;
    }

    [[maybe_unused]] auto destinationIsEmpty = [dstfd]() {
        QT_STATBUF statBuffer;
        return QT_FSTAT(dstfd, &statBuffer) == 0 && statBuffer.st_size == 0;
    };
    Q_ASSERT(destinationIsEmpty());

#if defined(Q_OS_LINUX)
    // first, try FICLONE (only works on regular files and only on certain fs)
    if (::ioctl(dstfd, FICLONE, srcfd) == 0)
        return TriStateResult::Success;
#elif defined(Q_OS_DARWIN)
    // try fcopyfile
    if (fcopyfile(srcfd, dstfd, nullptr, COPYFILE_DATA | COPYFILE_STAT) == 0)
        return TriStateResult::Success;
    switch (errno) {
    case ENOTSUP:
    case ENOMEM:
        return TriStateResult::NotSupported;    // let QFile try
    }
    return TriStateResult::Failed;
#endif

#if QT_CONFIG(copy_file_range)
    // Second, try copy_file_range. Tested on Linux & FreeBSD: FreeBSD can copy
    // across mountpoints, Linux currently (6.12) can only if the source and
    // destination mountpoints are the same filesystem type.
    QT_OFF_T srcoffset = 0;
    ssize_t copied;
    do {
        copied = ::copy_file_range(srcfd, &srcoffset, dstfd, nullptr, SSIZE_MAX, 0);
    } while (copied > 0 || (copied < 0 && errno == EINTR));
    if (copied == 0)
        return TriStateResult::Success;         // EOF -> success
    if (srcoffset) {
        // some bytes were copied, so this is a real error (like ENOSPC).
        copied = ftruncate(dstfd, 0);
        return TriStateResult::Failed;
    }
    if (errno != EXDEV)
        return TriStateResult::Failed;
#endif

#if defined(Q_OS_LINUX)
    // For Linux, try sendfile (it can send to some special types too).
    // sendfile(2) is limited in the kernel to 2G - 4k
    const size_t SendfileSize = 0x7ffff000;

    ssize_t n = ::sendfile(dstfd, srcfd, nullptr, SendfileSize);
    if (n == -1) {
        switch (errno) {
        case ENOSPC:
        case EIO:
            return TriStateResult::Failed;
        }

        // if we got an error here, give up and try at an upper layer
        return TriStateResult::NotSupported;
    }

    while (n) {
        n = ::sendfile(dstfd, srcfd, nullptr, SendfileSize);
        if (n == -1) {
            // uh oh, this is probably a real error (like ENOSPC)
            n = ftruncate(dstfd, 0);
            n = lseek(srcfd, 0, SEEK_SET);
            n = lseek(dstfd, 0, SEEK_SET);
            return TriStateResult::Failed;
        }
    }

    return TriStateResult::Success;
#else
    Q_UNUSED(dstfd);
    return TriStateResult::NotSupported;
#endif
}

static QSystemError createDirectoryWithParents(const QByteArray &path, mode_t mode)
{
#ifdef Q_OS_WASM
    if (path == "/")
        return {};
#endif

    auto tryMkDir = [&path, mode]() -> QSystemError {
        if (QT_MKDIR(path, mode) == 0) {
#ifdef Q_OS_VXWORKS
            forceRequestedPermissionsOnVxWorks(path, mode);
#endif
            return {};
        }
        // On macOS with APFS mkdir sets errno to EISDIR, QTBUG-97110
        if (errno == EISDIR)
            return {};
        if (errno == EEXIST || errno == EROFS) {
            // ::mkdir() can fail if the dir already exists (it may have been
            // created by another thread or another process)
            QT_STATBUF st;
            if (QT_STAT(path.constData(), &st) != 0)
                return QSystemError::stdError(errno);
            const bool isDir = (st.st_mode & S_IFMT) == S_IFDIR;
            return isDir ? QSystemError{} : QSystemError::stdError(EEXIST);
        }
        return QSystemError::stdError(errno);
    };

    QSystemError result = tryMkDir();
    if (result.ok())
        return result;

    // Only handle non-existing dir components in the path
    if (result.errorCode != ENOENT)
        return result;

    qsizetype slash = path.lastIndexOf('/');
    while (slash > 0 && path[slash - 1] == '/')
        --slash;

    if (slash < 1)
        return result;

    // mkdir failed because the parent dir doesn't exist, so try to create it
    QByteArray parentPath = path.first(slash);
    if (result = createDirectoryWithParents(parentPath, mode); !result.ok())
        return result;

    // try again
    return tryMkDir();
}

bool QFileSystemEngine::mkpath(const QFileSystemEntry &entry,
                              std::optional<QFile::Permissions> permissions)
{
    QByteArray path = entry.nativeFilePath();
    Q_CHECK_FILE_NAME(path, false);

    mode_t mode = permissions ? QtPrivate::toMode_t(*permissions) : 0777;
    return createDirectoryWithParents(removeTrailingSlashes(path), mode).ok();
}

bool QFileSystemEngine::mkdir(const QFileSystemEntry &entry,
                              std::optional<QFile::Permissions> permissions)
{
    QByteArray path = entry.nativeFilePath();
    Q_CHECK_FILE_NAME(path, false);

    mode_t mode = permissions ? QtPrivate::toMode_t(*permissions) : 0777;
    auto result = QT_MKDIR(removeTrailingSlashes(path), mode) == 0;
#if defined(Q_OS_VXWORKS)
    if (result)
        forceRequestedPermissionsOnVxWorks(path, mode);
#endif
    return result;
}

bool QFileSystemEngine::rmdir(const QFileSystemEntry &entry)
{
    const QByteArray path = entry.nativeFilePath();
    Q_CHECK_FILE_NAME(path, false);
    return ::rmdir(path.constData()) == 0;
}

bool QFileSystemEngine::rmpath(const QFileSystemEntry &entry)
{
    QByteArray path = QFile::encodeName(QDir::cleanPath(entry.filePath()));
    Q_CHECK_FILE_NAME(path, false);

    if (::rmdir(path.constData()) != 0)
        return false; // Only return false if `entry` couldn't be deleted

    const char sep = QDir::separator().toLatin1();
    qsizetype slash = path.lastIndexOf(sep);
    // `slash > 0` because truncate(0) would make `path` empty
    for (; slash > 0; slash = path.lastIndexOf(sep)) {
        path.truncate(slash);
        if (::rmdir(path.constData()) != 0)
            break;
    }

    return true;
}

//static
bool QFileSystemEngine::createLink(const QFileSystemEntry &source, const QFileSystemEntry &target, QSystemError &error)
{
    Q_CHECK_FILE_NAME(source, false);
    Q_CHECK_FILE_NAME(target, false);

    if (::symlink(source.nativeFilePath().constData(), target.nativeFilePath().constData()) == 0)
        return true;
    error = QSystemError(errno, QSystemError::StandardLibraryError);
    return false;
}

#if defined(QT_BOOTSTRAPPED) || !defined(AT_FDCWD) || defined(Q_OS_ANDROID) || !QT_CONFIG(datestring) || defined(Q_OS_VXWORKS)
// bootstrapped tools don't need this, and we don't want QStorageInfo
//static
bool QFileSystemEngine::supportsMoveFileToTrash()
{
    return false;
}

//static
bool QFileSystemEngine::moveFileToTrash(const QFileSystemEntry &, QFileSystemEntry &,
                                        QSystemError &error)
{
    error = QSystemError(ENOSYS, QSystemError::StandardLibraryError);
    return false;
}
#elif defined(Q_OS_DARWIN)
// see qfilesystemengine_mac.mm
#else
/*
    Implementing as per https://specifications.freedesktop.org/trash-spec/1.0/
*/
//static
bool QFileSystemEngine::supportsMoveFileToTrash()
{
    return true;
}

namespace {
struct FreeDesktopTrashOperation
{
    /*
        "A trash directory contains two subdirectories, named info and files."
    */
    QString trashPath;
    int filesDirFd = -1;
    int infoDirFd = -1;
    qsizetype volumePrefixLength = 0;

    // relative file paths to the filesDirFd and infoDirFd from above
    QByteArray tempTrashFileName;
    QByteArray infoFilePath;

    int infoFileFd = -1;        // if we've already opened it
    ~FreeDesktopTrashOperation()
    {
        close();
    }

    constexpr bool isTrashDirOpen() const { return filesDirFd != -1 && infoDirFd != -1; }

    void close()
    {
        int savedErrno = errno;
        if (infoFileFd != -1) {
            Q_ASSERT(infoDirFd != -1);
            Q_ASSERT(!infoFilePath.isEmpty());
            Q_ASSERT(!trashPath.isEmpty());

            QT_CLOSE(infoFileFd);
            unlinkat(infoDirFd, infoFilePath, 0);
            infoFileFd = -1;
        }
        if (!tempTrashFileName.isEmpty()) {
            Q_ASSERT(filesDirFd != -1);
            unlinkat(filesDirFd, tempTrashFileName, 0);
        }
        if (filesDirFd >= 0)
            QT_CLOSE(filesDirFd);
        if (infoDirFd >= 0)
            QT_CLOSE(infoDirFd);
        filesDirFd = infoDirFd = -1;
        errno = savedErrno;
    }

    bool tryCreateInfoFile(const QString &filePath, QSystemError &error)
    {
        QByteArray p = QFile::encodeName(filePath) + ".trashinfo";
        infoFileFd = qt_safe_openat(infoDirFd, p, QT_OPEN_RDWR | QT_OPEN_CREAT | QT_OPEN_EXCL, 0666);
        if (infoFileFd < 0) {
            error = QSystemError(errno, QSystemError::StandardLibraryError);
            return false;
        }
        infoFilePath = std::move(p);
        return true;
    }

    void commit()
    {
        QT_CLOSE(infoFileFd);
        infoFileFd = -1;
        tempTrashFileName = {};
    }

    // opens a directory and returns the file descriptor
    static int openDirFd(int dfd, const char *path, int mode)
    {
        mode |= QT_OPEN_RDONLY | O_DIRECTORY;
        return qt_safe_openat(dfd, path, mode);
    }

    // opens an XDG Trash directory that is a subdirectory of dfd, creating if necessary
    static int openOrCreateDir(int dfd, const char *path, int openmode = 0)
    {
        // try to open it as a dir, first
        int fd = openDirFd(dfd, path, openmode);
        if (fd >= 0 || errno != ENOENT)
            return fd;

        // try to mkdirat
        if (mkdirat(dfd, path, 0700) < 0)
            return -1;

        // try to open it again
        return openDirFd(dfd, path, openmode);
    }

    // opens or makes the XDG Trash hierarchy on parentfd (may be -1) called targetDir
    QSystemError getTrashDir(int parentfd, QString targetDir, const QFileSystemEntry &source,
                             int openmode)
    {
        if (parentfd == AT_FDCWD)
            trashPath = targetDir;
        QByteArray nativePath = QFile::encodeName(targetDir);

        // open the directory
        int trashfd = openOrCreateDir(parentfd, nativePath, openmode);
        if (trashfd < 0 && errno != ENOENT)
            return QSystemError::stdError(errno);

        // check if it is ours (even if we've just mkdirat'ed it)
        if (QT_STATBUF st; QT_FSTAT(trashfd, &st) < 0)
            return QSystemError::stdError(errno);
        else if (st.st_uid != getuid())
            return QSystemError::stdError(errno);

        filesDirFd = openOrCreateDir(trashfd, "files");
        if (filesDirFd >= 0) {
            // try to link our file-to-be-trashed here
            QTemporaryFileName tfn("XXXXXX"_L1);
            for (int i = 0; i < 16; ++i) {
                QByteArray attempt = tfn.generateNext();
                if (linkat(AT_FDCWD, source.nativeFilePath(), filesDirFd, attempt, 0) == 0) {
                    tempTrashFileName = std::move(attempt);
                    break;
                }
                if (errno != EEXIST)
                    break;
            }

            // man 2 link on Linux has:
            // EPERM  The filesystem containing oldpath and newpath does not
            //        support the creation of hard links.
            // EPERM  oldpath is a directory.
            // EPERM  oldpath is marked immutable or append‐only.
            // EMLINK The file referred to by oldpath already has the maximum
            //        number of links to it.
            if (!tempTrashFileName.isEmpty() || errno == EPERM || errno == EMLINK)
                infoDirFd = openOrCreateDir(trashfd, "info");
        }

        if (infoDirFd < 0) {
            int saved_errno = errno;
            close();
            QT_CLOSE(trashfd);
            return QSystemError::stdError(saved_errno);
        }

        QT_CLOSE(trashfd);
        return {};
    }

    QSystemError openMountPointTrashLocation(const QFileSystemEntry &source,
                                             const QStorageInfo &sourceStorage)
    {
        /*
            Method 1:
            "An administrator can create an $topdir/.Trash directory. The permissions on this
            directories should permit all users who can trash files at all to write in it;
            and the “sticky bit” in the permissions must be set, if the file system supports
            it.
            When trashing a file from a non-home partition/device, an implementation
            (if it supports trashing in top directories) MUST check for the presence
            of $topdir/.Trash."
        */

        QSystemError error;
        const auto dotTrash = "/.Trash"_L1;
        const QString userID = QString::number(::getuid());
        QFileSystemEntry dotTrashDir(sourceStorage.rootPath() + dotTrash);

        // we MUST check that the sticky bit is set, and that it is not a symlink
        int genericTrashFd = openDirFd(AT_FDCWD, dotTrashDir.nativeFilePath(), O_NOFOLLOW);
        QT_STATBUF st = {};
        if (genericTrashFd < 0 && errno != ENOENT && errno != EACCES) {
            // O_DIRECTORY + O_NOFOLLOW produces ENOTDIR on Linux
            if (QT_LSTAT(dotTrashDir.nativeFilePath(), &st) == 0 && S_ISLNK(st.st_mode)) {
                // we SHOULD report the failed check to the administrator
                qCritical("Warning: '%s' is a symlink to '%s'",
                          dotTrashDir.nativeFilePath().constData(),
                          qt_readlink(dotTrashDir.nativeFilePath()).constData());
                error = QSystemError(ELOOP, QSystemError::StandardLibraryError);
            }
        } else if (genericTrashFd >= 0) {
            QT_FSTAT(genericTrashFd, &st);
            if ((st.st_mode & S_ISVTX) == 0) {
                // we SHOULD report the failed check to the administrator
                qCritical("Warning: '%s' doesn't have sticky bit set!",
                          dotTrashDir.nativeFilePath().constData());
                error = QSystemError(EPERM, QSystemError::StandardLibraryError);
            } else {
                /*
                    "If the directory exists and passes the checks, a subdirectory of the
                     $topdir/.Trash directory is to be used as the user's trash directory
                     for this partition/device. The name of this subdirectory is the numeric
                     identifier of the current user ($topdir/.Trash/$uid).
                     When trashing a file, if this directory does not exist for the current user,
                     the implementation MUST immediately create it, without any warnings or
                     delays for the user."
                */
                if (error = getTrashDir(genericTrashFd, userID, source, O_NOFOLLOW); error.ok()) {
                    // recreate the resulting path
                    trashPath = dotTrashDir.filePath() + u'/' + userID;
                }
            }
            QT_CLOSE(genericTrashFd);
        }

        /*
            Method 2:
            "If an $topdir/.Trash directory is absent, an $topdir/.Trash-$uid directory is to be
             used as the user's trash directory for this device/partition. [...] When trashing a
             file, if an $topdir/.Trash-$uid directory does not exist, the implementation MUST
             immediately create it, without any warnings or delays for the user."
        */
        if (!isTrashDirOpen())
            error = getTrashDir(AT_FDCWD, sourceStorage.rootPath() + dotTrash + u'-' + userID, source,
                                O_NOFOLLOW);

        if (isTrashDirOpen()) {
            volumePrefixLength = sourceStorage.rootPath().size();
            if (volumePrefixLength == 1)
                volumePrefixLength = 0;         // isRoot
            else
                ++volumePrefixLength;           // to include the slash
        }
        return isTrashDirOpen() ? QSystemError() : error;
    }

    QSystemError openHomeTrashLocation(const QFileSystemEntry &source)
    {
        QString topDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
        int openmode = 0;   // do allow following symlinks
        return getTrashDir(AT_FDCWD, topDir + "/Trash"_L1, source, openmode);
    }

    QSystemError findTrashFor(const QFileSystemEntry &source)
    {
        /*
           First, try the standard Trash in $XDG_DATA_DIRS:
           "Its name and location are $XDG_DATA_HOME/Trash"; $XDG_DATA_HOME is what
           QStandardPaths returns for GenericDataLocation. If that doesn't exist, then
           we are not running on a freedesktop.org-compliant environment, and give up.
         */
        if (QSystemError error = openHomeTrashLocation(source); error.ok())
            return QSystemError();
        else if (error.errorCode != EXDEV)
            return error;

        // didn't work, try to find the trash outside the home filesystem
        const QStorageInfo sourceStorage(source.filePath());
        if (!sourceStorage.isValid())
            return QSystemError::stdError(ENODEV);
        return openMountPointTrashLocation(source, sourceStorage);
    }
};
} // unnamed namespace

//static
bool QFileSystemEngine::moveFileToTrash(const QFileSystemEntry &source,
                                        QFileSystemEntry &newLocation, QSystemError &error)
{
    const QFileSystemEntry sourcePath = [&] {
        if (QString path = source.filePath(); path.size() > 1 && path.endsWith(u'/')) {
            path.chop(1);
            return absoluteName(QFileSystemEntry(path));
        }
        return absoluteName(source);
    }();
    FreeDesktopTrashOperation op;
    if (error = op.findTrashFor(sourcePath); !error.ok())
        return false;

    /*
        "The $trash/files directory contains the files and directories that were trashed.
         The names of files in this directory are to be determined by the implementation;
         the only limitation is that they must be unique within the directory. Even if a
         file with the same name and location gets trashed many times, each subsequent
         trashing must not overwrite a previous copy."

         We first try the unchanged base name, then try something different if it collides.

        "The $trash/info directory contains an "information file" for every file and directory
         in $trash/files. This file MUST have exactly the same name as the file or directory in
         $trash/files, plus the extension ".trashinfo"
         [...]
         When trashing a file or directory, the implementation MUST create the corresponding
         file in $trash/info first. Moreover, it MUST try to do this in an atomic fashion,
         so that if two processes try to trash files with the same filename this will result
         in two different trash files. On Unix-like systems this is done by generating a
         filename, and then opening with O_EXCL. If that succeeds the creation was atomic
         (at least on the same machine), if it fails you need to pick another filename."
    */
    QString uniqueTrashedName = sourcePath.fileName();
    if (!op.tryCreateInfoFile(uniqueTrashedName, error) && error.errorCode == EEXIST) {
        // we'll use a counter, starting with the file's inode number to avoid
        // collisions
        qulonglong counter;
        if (QT_STATBUF st; Q_LIKELY(QT_STAT(source.nativeFilePath(), &st) == 0)) {
            counter = st.st_ino;
        } else {
            error = QSystemError(errno, QSystemError::StandardLibraryError);
            return false;
        }

        QString uniqueTrashBase = std::move(uniqueTrashedName);
        for (;;) {
            uniqueTrashedName = QString::asprintf("%ls-%llu", qUtf16Printable(uniqueTrashBase),
                                                  counter++);
            if (op.tryCreateInfoFile(uniqueTrashedName, error))
                break;
            if (error.errorCode != EEXIST)
                return false;
        };
    }

    QByteArray info =
            "[Trash Info]\n"
            "Path=" + QUrl::toPercentEncoding(source.filePath().mid(op.volumePrefixLength), "/") + "\n"
            "DeletionDate=" + QDateTime::currentDateTime().toString(Qt::ISODate).toUtf8()
            + "\n";
    if (QT_WRITE(op.infoFileFd, info.data(), info.size()) < 0) {
        error = QSystemError(errno, QSystemError::StandardLibraryError);
        return false;
    }

    /*
       If we've already linked the file-to-be-trashed into the trash
       directory, we know it's in the same mountpoint and we won't get ENOSPC
       renaming the temporary file to the target name either.
    */
    bool renamed;
    if (op.tempTrashFileName.isEmpty()) {
        /*
           We did not get a link (we're trying to trash a directory or on a
           filesystem that doesn't support hardlinking), so rename straight
           from the original name. We might fail to rename if source and target
           are on different file systems.
         */
        renamed = renameat(AT_FDCWD, source.nativeFilePath(), op.filesDirFd,
                           QFile::encodeName(uniqueTrashedName)) == 0;
    } else {
        renamed = renameat(op.filesDirFd, op.tempTrashFileName, op.filesDirFd,
                           QFile::encodeName(uniqueTrashedName)) == 0;
        if (renamed)
            removeFile(sourcePath, error);  // success, delete the original file
    }
    if (!renamed) {
        error = QSystemError(errno, QSystemError::StandardLibraryError);
        return false;
    }

    op.commit();
    newLocation = QFileSystemEntry(op.trashPath + "/files/"_L1 + uniqueTrashedName);
    return true;
}
#endif // !Q_OS_DARWIN && (!QT_BOOTSTRAPPED && AT_FDCWD && !Q_OS_ANDROID && QT_CONFIG(datestring))

//static
bool QFileSystemEngine::copyFile(const QFileSystemEntry &source, const QFileSystemEntry &target, QSystemError &error)
{
    Q_UNUSED(source);
    Q_UNUSED(target);
    error = QSystemError(ENOSYS, QSystemError::StandardLibraryError); //Function not implemented
    return false;
}

//static
bool QFileSystemEngine::renameFile(const QFileSystemEntry &source, const QFileSystemEntry &target, QSystemError &error)
{
    QFileSystemEntry::NativePath srcPath = source.nativeFilePath();
    QFileSystemEntry::NativePath tgtPath = target.nativeFilePath();

    Q_CHECK_FILE_NAME(srcPath, false);
    Q_CHECK_FILE_NAME(tgtPath, false);

#if defined(RENAME_NOREPLACE) && QT_CONFIG(renameat2)
    if (renameat2(AT_FDCWD, srcPath, AT_FDCWD, tgtPath, RENAME_NOREPLACE) == 0)
        return true;

    // We can also get EINVAL for some non-local filesystems.
    if (errno != EINVAL) {
        error = QSystemError(errno, QSystemError::StandardLibraryError);
        return false;
    }
#endif
#if defined(Q_OS_DARWIN) && defined(RENAME_EXCL)
    if (renameatx_np(AT_FDCWD, srcPath, AT_FDCWD, tgtPath, RENAME_EXCL) == 0)
        return true;
    if (errno != ENOTSUP) {
        error = QSystemError(errno, QSystemError::StandardLibraryError);
        return false;
    }
#endif

    if (SupportsHardlinking && ::link(srcPath, tgtPath) == 0) {
        if (::unlink(srcPath) == 0)
            return true;

        // if we managed to link but can't unlink the source, it's likely
        // it's in a directory we don't have write access to; fail the
        // renaming instead
        int savedErrno = errno;

        // this could fail too, but there's nothing we can do about it now
        ::unlink(tgtPath);

        error = QSystemError(savedErrno, QSystemError::StandardLibraryError);
        return false;
    } else if (!SupportsHardlinking) {
        // man 2 link on Linux has:
        // EPERM  The filesystem containing oldpath and newpath does not
        //        support the creation of hard links.
        errno = EPERM;
    }

    switch (errno) {
    case EACCES:
    case EEXIST:
    case ENAMETOOLONG:
    case ENOENT:
    case ENOTDIR:
    case EROFS:
    case EXDEV:
        // accept the error from link(2) (especially EEXIST) and don't retry
        break;

    default:
        // fall back to rename()
        // ### Race condition. If a file is moved in after this, it /will/ be
        // overwritten.
        if (::rename(srcPath, tgtPath) == 0)
            return true;
    }

    error = QSystemError(errno, QSystemError::StandardLibraryError);
    return false;
}

//static
bool QFileSystemEngine::renameOverwriteFile(const QFileSystemEntry &source, const QFileSystemEntry &target, QSystemError &error)
{
    Q_CHECK_FILE_NAME(source, false);
    Q_CHECK_FILE_NAME(target, false);

    if (::rename(source.nativeFilePath().constData(), target.nativeFilePath().constData()) == 0)
        return true;
    error = QSystemError(errno, QSystemError::StandardLibraryError);
    return false;
}

//static
bool QFileSystemEngine::removeFile(const QFileSystemEntry &entry, QSystemError &error)
{
    Q_CHECK_FILE_NAME(entry, false);
    if (unlink(entry.nativeFilePath().constData()) == 0)
        return true;
    error = QSystemError(errno, QSystemError::StandardLibraryError);
    return false;

}

//static
bool QFileSystemEngine::setPermissions(const QFileSystemEntry &entry,
                                       QFile::Permissions permissions, QSystemError &error)
{
    Q_CHECK_FILE_NAME(entry, false);

    mode_t mode = QtPrivate::toMode_t(permissions);
    bool success = ::chmod(entry.nativeFilePath().constData(), mode) == 0;
    if (!success)
        error = QSystemError(errno, QSystemError::StandardLibraryError);
    return success;
}

//static
bool QFileSystemEngine::setPermissions(int fd, QFile::Permissions permissions, QSystemError &error)
{
    mode_t mode = QtPrivate::toMode_t(permissions);
    bool success = ::fchmod(fd, mode) == 0;
    if (!success)
        error = QSystemError(errno, QSystemError::StandardLibraryError);
    return success;
}

//static
bool QFileSystemEngine::setFileTime(int fd, const QDateTime &newDate, QFile::FileTime time, QSystemError &error)
{
    if (!newDate.isValid()
        || time == QFile::FileBirthTime || time == QFile::FileMetadataChangeTime) {
        error = QSystemError(EINVAL, QSystemError::StandardLibraryError);
        return false;
    }

#if QT_CONFIG(futimens)
    // UTIME_OMIT: leave file timestamp unchanged
    struct timespec ts[2] = {{0, UTIME_OMIT}, {0, UTIME_OMIT}};

    if (time == QFile::FileAccessTime || time == QFile::FileModificationTime) {
        const int idx = time == QFile::FileAccessTime ? 0 : 1;
        const std::chrono::milliseconds msecs{newDate.toMSecsSinceEpoch()};
        ts[idx] = durationToTimespec(msecs);
    }

    if (futimens(fd, ts) == -1) {
        error = QSystemError(errno, QSystemError::StandardLibraryError);
        return false;
    }

    return true;
#else
    Q_UNUSED(fd);
    error = QSystemError(ENOSYS, QSystemError::StandardLibraryError);
    return false;
#endif
}

QString QFileSystemEngine::homePath()
{
    QString home = qEnvironmentVariable("HOME");
    if (home.isEmpty())
        home = rootPath();
    return QDir::cleanPath(home);
}

QString QFileSystemEngine::rootPath()
{
    return u"/"_s;
}

static constexpr QLatin1StringView nativeTempPath() noexcept
{
    // _PATH_TMP usually ends in '/' and we don't want that
    QLatin1StringView temp = _PATH_TMP ""_L1;
    static_assert(_PATH_TMP[0] == '/', "_PATH_TMP needs to be absolute");
    static_assert(_PATH_TMP[1] != '\0', "Are you really sure _PATH_TMP should be the root dir??");
    if (temp.endsWith(u'/'))
        temp.chop(1);
    return temp;
}

QString QFileSystemEngine::tempPath()
{
#ifdef QT_UNIX_TEMP_PATH_OVERRIDE
    return QT_UNIX_TEMP_PATH_OVERRIDE ""_L1;
#else
    QString temp = qEnvironmentVariable("TMPDIR");
#  if defined(Q_OS_DARWIN) && !defined(QT_BOOTSTRAPPED)
    if (NSString *nsPath; temp.isEmpty() && (nsPath = NSTemporaryDirectory()))
        temp = QString::fromCFString((CFStringRef)nsPath);
#  endif
    if (temp.isEmpty())
        return nativeTempPath();

    // the environment variable may also end in '/'
    if (temp.size() > 1 && temp.endsWith(u'/'))
        temp.chop(1);

    QFileSystemEntry e(temp, QFileSystemEntry::FromInternalPath{});
    return QFileSystemEngine::absoluteName(e).filePath();
#endif
}

bool QFileSystemEngine::setCurrentPath(const QFileSystemEntry &path)
{
    int r;
    r = QT_CHDIR(path.nativeFilePath().constData());
    return r >= 0;
}

QFileSystemEntry QFileSystemEngine::currentPath()
{
    QFileSystemEntry result;
#if defined(__GLIBC__) && !defined(PATH_MAX)
    char *currentName = ::get_current_dir_name();
    if (currentName) {
        result = QFileSystemEntry(QByteArray(currentName), QFileSystemEntry::FromNativePath());
        ::free(currentName);
    }
#else
    char currentName[PATH_MAX+1];
    if (::getcwd(currentName, PATH_MAX)) {
#if defined(Q_OS_VXWORKS) && defined(VXWORKS_VXSIM)
        QByteArray dir(currentName);
        if (dir.indexOf(':') < dir.indexOf('/'))
            dir.remove(0, dir.indexOf(':')+1);

        qstrncpy(currentName, dir.constData(), PATH_MAX);
#endif
        result = QFileSystemEntry(QByteArray(currentName), QFileSystemEntry::FromNativePath());
    }
# if defined(QT_DEBUG)
    if (result.isEmpty())
        qWarning("QFileSystemEngine::currentPath: getcwd() failed");
# endif
#endif
    return result;
}

bool QFileSystemEngine::isCaseSensitive(const QFileSystemEntry &entry, QFileSystemMetaData &metaData)
{
#if defined(Q_OS_DARWIN)
    if (!metaData.hasFlags(QFileSystemMetaData::CaseSensitive))
        fillMetaData(entry, metaData, QFileSystemMetaData::CaseSensitive);
    return metaData.entryFlags.testFlag(QFileSystemMetaData::CaseSensitive);
#else
    Q_UNUSED(entry);
    Q_UNUSED(metaData);
    // FIXME: This may not be accurate for all file systems (QTBUG-28246)
    return true;
#endif
}

QT_END_NAMESPACE
