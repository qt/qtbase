// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2014 Ivan Komissarov <ABBAPOH@gmail.com>
// Copyright (C) 2016 Intel Corporation.
// Copyright (C) 2023 Ahmad Samir <a.samirh78@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qstorageinfo_linux_p.h"

#include <private/qcore_unix_p.h>
#include <private/qlocale_tools_p.h>
#include <private/qtools_p.h>

#include <QtCore/qdirlisting.h>
#include <QtCore/qsystemdetection.h>

#include <q20memory.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/syscall.h>

#include <linux/types.h>
#include <linux/mount.h>

// so we don't have to #include <linux/fs.h>, which is known to cause conflicts
#ifndef FSLABEL_MAX
#  define FSLABEL_MAX           256
#endif
#ifndef FS_IOC_GETFSLABEL
#  define FS_IOC_GETFSLABEL     _IOR(0x94, 49, char[FSLABEL_MAX])
#endif

#ifndef STATX_MNT_ID_UNIQUE
#  define STATX_MNT_ID_UNIQUE   0x4000U
#endif

// or <linux/statfs.h>
#ifndef ST_RDONLY
#  define ST_RDONLY             0x0001  /* mount read-only */
#endif

#if defined(Q_OS_ANDROID)
// statx() is disabled on Android because quite a few systems
// come with sandboxes that kill applications that make system calls outside a
// whitelist and several Android vendors can't be bothered to update the list.
#  undef STATX_BASIC_STATS
// Likewise for listmount() and statmount() (disabling statx() and listmount()
// disables statmount()).
#  undef LSMT_ROOT

#include <private/qjnihelpers_p.h>
#endif

QT_BEGIN_NAMESPACE

// So we don't depend on linux/mount.h defining the version we need (we need at
// least 6.13, with sb_source).
namespace QtPrivate {
struct statmount {
        __u32 size;             /* Total size, including strings */
        __u32 mnt_opts;         /* [str] Options (comma separated, escaped) */
        __u64 mask;             /* What results were written */
        __u32 sb_dev_major;     /* Device ID */
        __u32 sb_dev_minor;
        __u64 sb_magic;         /* ..._SUPER_MAGIC */
        __u32 sb_flags;         /* SB_{RDONLY,SYNCHRONOUS,DIRSYNC,LAZYTIME} */
        __u32 fs_type;          /* [str] Filesystem type */
        __u64 mnt_id;           /* Unique ID of mount */
        __u64 mnt_parent_id;    /* Unique ID of parent (for root == mnt_id) */
        __u32 mnt_id_old;       /* Reused IDs used in proc/.../mountinfo */
        __u32 mnt_parent_id_old;
        __u64 mnt_attr;         /* MOUNT_ATTR_... */
        __u64 mnt_propagation;  /* MS_{SHARED,SLAVE,PRIVATE,UNBINDABLE} */
        __u64 mnt_peer_group;   /* ID of shared peer group */
        __u64 mnt_master;       /* Mount receives propagation from this ID */
        __u64 propagate_from;   /* Propagation from in current namespace */
        __u32 mnt_root;         /* [str] Root of mount relative to root of fs */
        __u32 mnt_point;        /* [str] Mountpoint relative to current root */
        __u64 mnt_ns_id;        /* ID of the mount namespace */
        __u32 fs_subtype;       /* [str] Subtype of fs_type (if any) */
        __u32 sb_source;        /* [str] Source string of the mount */
        __u32 opt_num;          /* Number of fs options */
        __u32 opt_array;        /* [str] Array of nul terminated fs options */
        __u32 opt_sec_num;      /* Number of security options */
        __u32 opt_sec_array;    /* [str] Array of nul terminated security options */
        __u64 __spare2[46];
        char str[];             /* Variable size part containing strings */
};
} // namespace QtPrivate

using namespace Qt::StringLiterals;

static const char MountInfoPath[] = "/proc/self/mountinfo";

static std::optional<dev_t> deviceNumber(QByteArrayView devno)
{
    // major:minor
    auto it = devno.cbegin();
    auto r = qstrntoll(it, devno.size(), 10);
    if (!r.ok())
        return std::nullopt;
    int rdevmajor = int(r.result);
    it += r.used;

    if (*it != ':')
        return std::nullopt;

    r = qstrntoll(++it, devno.size() - r.used + 1, 10);
    if (!r.ok())
        return std::nullopt;

    return makedev(rdevmajor, r.result);
}

// Helper function to parse paths that the kernel inserts escape sequences
// for.
static QByteArray parseMangledPath(QByteArrayView path)
{
    // The kernel escapes with octal the following characters:
    //  space ' ', tab '\t', backslash '\\', and newline '\n'
    // See:
    // https://codebrowser.dev/linux/linux/fs/proc_namespace.c.html#show_mountinfo
    // https://codebrowser.dev/linux/linux/fs/seq_file.c.html#mangle_path

    QByteArray ret(path.size(), '\0');
    char *dst = ret.data();
    const char *src = path.data();
    const char *srcEnd = path.data() + path.size();
    while (src != srcEnd) {
        switch (*src) {
        case ' ': // Shouldn't happen
            return {};

        case '\\': {
            // It always uses exactly three octal characters.
            ++src;
            char c = (*src++ - '0') << 6;
            c |= (*src++ - '0') << 3;
            c |= (*src++ - '0');
            *dst++ = c;
            break;
        }

        default:
            *dst++ = *src++;
            break;
        }
    }
    // If "path" contains any of the characters this method is demangling,
    // "ret" would be oversized with extra '\0' characters at the end.
    ret.resize(dst - ret.data());
    return ret;
}

// Indexes into the "fields" std::array in parseMountInfo()
static constexpr short MountId = 0;
// static constexpr short ParentId = 1;
static constexpr short DevNo = 2;
static constexpr short FsRoot = 3;
static constexpr short MountPoint = 4;
static constexpr short MountOptions = 5;
// static constexpr short OptionalFields = 6;
// static constexpr short Separator = 7;
static constexpr short FsType = 8;
static constexpr short MountSource = 9;
static constexpr short SuperOptions = 10;
static constexpr short FieldCount = 11;

// Splits a line from /proc/self/mountinfo into fields; fields are separated
// by a single space.
static void tokenizeLine(std::array<QByteArrayView, FieldCount> &fields, QByteArrayView line)
{
    size_t fieldIndex = 0;
    qsizetype from = 0;
    const char *begin = line.data();
    const qsizetype len = line.size();
    qsizetype spaceIndex = -1;
    while ((spaceIndex = line.indexOf(' ', from)) != -1 && fieldIndex < FieldCount) {
        fields[fieldIndex] = QByteArrayView{begin + from, begin + spaceIndex};
        from = spaceIndex;

        // Skip "OptionalFields" and Separator fields
        if (fieldIndex == MountOptions) {
            static constexpr char separatorField[] = " - ";
            const qsizetype sepIndex = line.indexOf(separatorField, from);
            if (sepIndex == -1) {
                qCWarning(lcStorageInfo,
                          "Malformed line (missing '-' separator field) while parsing '%s':\n%s",
                          MountInfoPath, line.constData());
                fields.fill({});
                return;
            }

            from = sepIndex + strlen(separatorField);
            // Continue parsing at FsType field
            fieldIndex = FsType;
            continue;
        }

        if (from + 1 < len)
            ++from; // Skip the space at spaceIndex

        ++fieldIndex;
    }

    // Currently we don't use the last field, so just check the index
    if (fieldIndex != SuperOptions) {
        qCInfo(lcStorageInfo,
               "Expected %d fields while parsing line from '%s', but found %zu instead:\n%.*s",
               FieldCount, MountInfoPath, fieldIndex, int(line.size()), line.data());
        fields.fill({});
    }
}

std::vector<MountInfo> doParseMountInfo(const QByteArray &mountinfo, FilterMountInfo filter)
{
    // https://www.kernel.org/doc/Documentation/filesystems/proc.txt:
    // 36 35 98:0 /mnt1 /mnt2 rw,noatime master:1 - ext3 /dev/root rw,errors=continue
    // (1)(2)(3)   (4)   (5)      (6)      (7)   (8) (9)   (10)         (11)

    auto it = mountinfo.cbegin();
    const auto end = mountinfo.cend();
    auto nextLine = [&it, &end]() -> QByteArrayView {
        auto nIt = std::find(it, end, '\n');
        if (nIt != end) {
            QByteArrayView ba(it, nIt);
            it = ++nIt; // Advance
            return ba;
        }
        return {};
    };

    std::vector<MountInfo> infos;
    std::array<QByteArrayView, FieldCount> fields;
    QByteArrayView line;

    auto checkField = [&line](QByteArrayView field) {
        if (field.isEmpty()) {
            qDebug("Failed to parse line from %s:\n%.*s", MountInfoPath, int(line.size()),
                   line.data());
            return false;
        }
        return true;
    };

    // mountinfo has a stable format, no empty lines
    while (!(line = nextLine()).isEmpty()) {
        fields.fill({});
        tokenizeLine(fields, line);

        MountInfo info;
        if (auto r = qstrntoll(fields[MountId].data(), fields[MountId].size(), 10); r.ok()) {
            info.mntid = r.result;
        } else {
            checkField({});
            continue;
        }

        QByteArray mountP = parseMangledPath(fields[MountPoint]);
        if (!checkField(mountP))
            continue;
        info.mountPoint = QFile::decodeName(mountP);

        if (!checkField(fields[FsType]))
            continue;
        info.fsType = fields[FsType].toByteArray();

        if (filter == FilterMountInfo::Filtered
                && !QStorageInfoPrivate::shouldIncludeFs(info.mountPoint, info.fsType))
            continue;

        std::optional<dev_t> devno = deviceNumber(fields[DevNo]);
        if (!devno) {
            checkField({});
            continue;
        }
        info.stDev = *devno;

        QByteArrayView fsRootView = fields[FsRoot];
        if (!checkField(fsRootView))
            continue;

        // If the filesystem root is "/" -- it's not a *sub*-volume/bind-mount,
        // in that case we leave info.fsRoot empty
        if (fsRootView != "/") {
            info.fsRoot = parseMangledPath(fsRootView);
            if (!checkField(info.fsRoot))
                continue;
        }

        info.device = parseMangledPath(fields[MountSource]);
        if (!checkField(info.device))
            continue;

        infos.push_back(std::move(info));
    }
    return infos;
}

namespace {
struct AutoFileDescriptor
{
    int fd = -1;
    explicit AutoFileDescriptor() = default;
    AutoFileDescriptor(const QString &path, int mode = QT_OPEN_RDONLY)
        : fd(qt_safe_open(QFile::encodeName(path), mode))
    {}
    ~AutoFileDescriptor() { if (fd >= 0) qt_safe_close(fd); }
    operator int() const noexcept { return fd; }
};

struct QStorageLabelRetriever
{
    struct Entry {
        QString label;
        quint64 deviceId;
    };
    QList<Entry> labelMap;
    bool populated = false;
    QString operator()(const QStorageInfoPrivate &d, int fd, quint64 devid);

private:
    static QList<Entry> allLabels();
};
}
Q_DECLARE_TYPEINFO(QStorageLabelRetriever::Entry, Q_RELOCATABLE_TYPE);

// udev encodes the labels with ID_LABEL_FS_ENC which is done with
// blkid_encode_string(). Within this function some 1-byte utf-8
// characters not considered safe (e.g. '\' or ' ') are encoded as hex
static QString decodeFsEncString(QString decoded)
{
    using namespace QtMiscUtils;
    const qsizetype start = decoded.indexOf(u'\\');
    if (start < 0)
        return decoded;

    // decode in-place
    auto ptr = reinterpret_cast<char16_t *>(decoded.begin());
    qsizetype in = start;
    qsizetype out = start;
    qsizetype size = decoded.size();

    while (in < size) {
        Q_ASSERT(ptr[in] == u'\\');
        if (size - in >= 4 && ptr[in + 1] == u'x') {    // we need four characters: \xAB
            int c = fromHex(ptr[in + 2]) << 4;
            c |= fromHex(ptr[in + 3]);
            if (Q_UNLIKELY(c < 0))
                c = QChar::ReplacementCharacter;        // bad hex sequence
            ptr[out++] = c;
            in += 4;
        }

        for ( ; in < size; ++in) {
            char16_t c = ptr[in];
            if (c == u'\\')
                break;
            ptr[out++] = c;
        }
    }
    decoded.resize(out);
    return decoded;
}

static inline dev_t deviceIdForPath(const QString &device)
{
    QT_STATBUF st;
    if (QT_STAT(QFile::encodeName(device), &st) < 0)
        return 0;
    return st.st_dev;
}

static inline quint64 mountIdForPath(int fd, unsigned mountIdType = 0)
{
    if (fd < 0)
        return 0;
#if defined(STATX_BASIC_STATS) && defined(STATX_MNT_ID)
    // STATX_MNT_ID was added in kernel v5.8
    struct statx st;
    if (mountIdType == 0)
        mountIdType = STATX_MNT_ID;
    int r = statx(fd, "", AT_EMPTY_PATH | AT_NO_AUTOMOUNT, mountIdType, &st);
    if (r == 0 && (st.stx_mask & mountIdType))
        return st.stx_mnt_id;
#else
    Q_UNUSED(mountIdType);
#endif
    return 0;
}

static inline quint64 retrieveDeviceId(const QByteArray &device, quint64 deviceId = 0)
{
    // major = 0 implies an anonymous block device, so we need to stat() the
    // actual device to get its dev_t. This is required for btrfs (and possibly
    // others), which always uses them for all the subvolumes (including the
    // root):
    // https://codebrowser.dev/linux/linux/fs/btrfs/disk-io.c.html#btrfs_init_fs_root
    // https://codebrowser.dev/linux/linux/fs/super.c.html#get_anon_bdev
    // For everything else, we trust the parameter.
    if (major(deviceId) != 0)
        return deviceId;

    // don't even try to stat() a relative path or "/"
    if (device.size() < 2 || !device.startsWith('/'))
        return 0;

    QT_STATBUF st;
    if (QT_STAT(device, &st) < 0)
        return 0;
    if (!S_ISBLK(st.st_mode))
        return 0;
    return st.st_rdev;
}

static QDirListing devicesByLabel()
{
    static const char pathDiskByLabel[] = "/dev/disk/by-label";
    static constexpr auto LabelFileFilter = QDirListing::IteratorFlag::IncludeHidden;
    return QDirListing(QLatin1StringView(pathDiskByLabel), LabelFileFilter);
}

auto QStorageLabelRetriever::allLabels() -> QList<Entry>
{
    QList<Entry> result;

    for (const auto &dirEntry : devicesByLabel()) {
        quint64 deviceId = retrieveDeviceId(QFile::encodeName(dirEntry.filePath()));
        if (!deviceId)
            continue;
        result.emplaceBack(Entry{ decodeFsEncString(dirEntry.fileName()), deviceId });
    }
    return result;
}

static std::optional<QString> retrieveLabelViaIoctl(int fd)
{
    // FS_IOC_GETFSLABEL was introduced in v4.18; previously it was btrfs-specific.
    if (fd < 0)
        return std::nullopt;

    // Note: it doesn't append the null terminator (despite what the man page
    // says) and the return code on success (0) does not indicate the length.
    char label[FSLABEL_MAX] = {};
    int r = ioctl(fd, FS_IOC_GETFSLABEL, &label);
    if (r < 0)
        return std::nullopt;
    return QString::fromUtf8(label);
}

static inline QString retrieveLabel(const QStorageInfoPrivate &d, int fd, quint64 deviceId)
{
    if (auto label = retrieveLabelViaIoctl(fd))
        return *label;

    deviceId = retrieveDeviceId(d.device, deviceId);
    if (!deviceId)
        return QString();

    for (const auto &dirEntry : devicesByLabel()) {
        if (retrieveDeviceId(QFile::encodeName(dirEntry.filePath())) == deviceId)
            return decodeFsEncString(dirEntry.fileName());
    }
    return QString();
}

QString QStorageLabelRetriever::operator()(const QStorageInfoPrivate &d, int fd, quint64 devid)
{
    if (d.fileSystemType == "tmpfs")
        return QString();

    if (auto label = retrieveLabelViaIoctl(fd))
        return *label;

    devid = retrieveDeviceId(d.device, devid);
    if (!devid)
        return QString();

    if (!populated) {
        labelMap = allLabels();
        populated = true;
    }
    for (auto &[deviceLabel, deviceId] : std::as_const(labelMap)) {
        if (devid == deviceId)
            return deviceLabel;
    }
    return QString();
}

void QStorageInfoPrivate::retrieveVolumeInfo()
{
    struct statfs64 statfs_buf;
    int result;
    QT_EINTR_LOOP(result, statfs64(QFile::encodeName(rootPath).constData(), &statfs_buf));
    valid = ready = (result == 0);
    if (valid) {
        bytesTotal = statfs_buf.f_blocks * statfs_buf.f_frsize;
        bytesFree = statfs_buf.f_bfree * statfs_buf.f_frsize;
        bytesAvailable = statfs_buf.f_bavail * statfs_buf.f_frsize;
        blockSize = int(statfs_buf.f_bsize);
        readOnly = (statfs_buf.f_flags & ST_RDONLY) != 0;
    }
}

static auto
doStatMount(QStorageInfoPrivate *d, const AutoFileDescriptor &fd, quint64 mountid,
            QStorageLabelRetriever &labelRetriever)
{
    enum TriStateResult : qint8 {
        NotSupported = -1,
        Skip = 0,
        Add = 1,
    };

#ifdef __NR_statmount
    using QtPrivate::statmount;
    constexpr int NumStringsInRequest = 4; // mnt_point, sb_type, sb_source, mnt_root
    // the parameters we'll request
    static constexpr quint64 StatMountRequestParams = 0x23b;
    // the minimum set of parameters we need
    static constexpr quint64 StatMountMinParams = StatMountRequestParams;
#  ifdef STATMOUNT_SB_SOURCE        // added in 6.13
    // confirm our numeric value above is correct
    constexpr quint64 SymbolicStatMountParams = STATMOUNT_SB_BASIC | STATMOUNT_MNT_BASIC
            | STATMOUNT_MNT_POINT | STATMOUNT_MNT_ROOT | STATMOUNT_FS_TYPE
            | STATMOUNT_SB_SOURCE;
    static_assert(SymbolicStatMountParams == StatMountRequestParams);
#  endif

    QVarLengthArray<char, 1024> smbuf(1024);
    auto sm = new (smbuf.data()) struct statmount;
    struct mnt_id_req req = {};
    req.size = sizeof(req);
    req.mnt_id = mountid;
    req.param = StatMountRequestParams;

    int r = syscall(__NR_statmount, &req, sm, smbuf.size(), 0);
    if (r < 0 && errno == EOVERFLOW) {
        // try with heap
        smbuf.resize(sizeof(struct statmount) + NumStringsInRequest * PATH_MAX);
        sm = new (smbuf.data()) struct statmount;
        r = syscall(__NR_statmount, &req, sm, smbuf.size(), 0);
    }
    if (r == 0 && (sm->mask & StatMountMinParams) == StatMountMinParams) {
        d->rootPath = QFile::decodeName(sm->str + sm->mnt_point);
        d->fileSystemType.assign(sm->str + sm->fs_type);
        if (!QStorageInfoPrivate::shouldIncludeFs(d->rootPath, d->fileSystemType))
            return Skip;

        d->device = QByteArray(sm->str + sm->sb_source);
        if (QByteArrayView subv(sm->str + sm->mnt_root); subv != "/")
            d->subvolume.assign(subv);
        else
            d->subvolume.clear();

        quint64 devid = makedev(sm->sb_dev_major, sm->sb_dev_minor);
        if (fd.fd >= 0)
            d->name = labelRetriever(*d, fd, devid);    // already open
        else
            d->name = labelRetriever(*d, AutoFileDescriptor(d->rootPath), devid);
        return Add;
    }
#else
    Q_UNUSED(d);
    Q_UNUSED(fd);
    Q_UNUSED(mountid);
    Q_UNUSED(labelRetriever);
#endif // !__NR_statmount
    return NotSupported;
}

static bool doStatMount(QStorageInfoPrivate *d, const AutoFileDescriptor &fd)
{
    quint64 mountId = mountIdForPath(fd, STATX_MNT_ID_UNIQUE);
    if (!mountId)
        return false;

    QStorageLabelRetriever retriever;
    return doStatMount(d, fd, mountId, retriever) > 0;
}

QList<QStorageInfo> QStorageInfoPrivate::listMounts()
{
    QList<QStorageInfo> volumes;

#if defined(__NR_listmount) && defined(LSMT_ROOT)
    QStorageLabelRetriever labelRetriever;
    Q_DECL_UNINITIALIZED
    quint64 mounts[128];

    struct mnt_id_req req = {};
    req.size = sizeof(req);
    req.mnt_id = LSMT_ROOT;
    int r = syscall(__NR_listmount, &req, mounts, std::size(mounts), 0);
    if (r < 0)
        return volumes;

    std::unique_ptr<QStorageInfoPrivate> d;
    while (r > 0) {
        for (quint64 mntid : QSpan(mounts, r)) {
            if (!d)
                d = std::make_unique<QStorageInfoPrivate>();
            auto shouldAdd = doStatMount(d.get(), AutoFileDescriptor(), mntid, labelRetriever);
            if (shouldAdd < 0) {
                r = -1;
                break;          // API failure
            }
            if (!shouldAdd)
                continue;
            d->retrieveVolumeInfo();
            if (!d->ready)
                continue;       // mount point inaccessible
            volumes.emplace_back(QStorageInfo(*d.release()));
        }
        if (r < q20::ssize(mounts))
            break;              // done or failure

        // ask for more
        req.param = mounts[r - 1];
        r = syscall(__NR_listmount, &req, mounts, std::size(mounts), 0);
    }
    if (r < 0)
        volumes.clear();        // we've failed
#endif // __NR_listmount

    return volumes;
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

void QStorageInfoPrivate::doStat()
{
#ifdef Q_OS_ANDROID
    if (QtAndroidPrivate::isUncompressedNativeLibs()) {
        // We need to pass the actual file path on the file system to statfs64
        QString possibleApk = QtAndroidPrivate::resolveApkPath(rootPath);
        if (!possibleApk.isEmpty())
            rootPath = possibleApk;
    }
#endif

    retrieveVolumeInfo();
    if (!ready)
        return;

    AutoFileDescriptor fd(rootPath);
    if (doStatMount(this, fd))
        return;

    QFileInfo fileInfo(rootPath);
    if (QString canonicalPath = fileInfo.canonicalFilePath(); !canonicalPath.isEmpty()) {
        rootPath = std::move(canonicalPath);
    } else {
        // The volume is ready but canonization process failed (for example, a
        // mount point we're allowed to statfs() but not open(), as can happen
        // under SELinux on Android). The absolute path will have to do.
        rootPath = fileInfo.absoluteFilePath();
    }

    std::vector<MountInfo> infos = parseMountInfo();
    if (infos.empty()) {
        rootPath = u'/';
        return;
    }

    MountInfo *best = nullptr;
    if (quint64 mntid = mountIdForPath(fd)) {
        // We have the mount ID for this path, so find the matching line.
        auto it = std::find_if(infos.begin(), infos.end(),
                               [mntid](const MountInfo &info) { return info.mntid == mntid; });
        if (it != infos.end())
            best = q20::to_address(it);
    } else {
        // We have failed to get the mount ID for this path, usually because
        // the path cannot be open()ed by this user (e.g., /root), so we fall
        // back to a string search.
        // We iterate over the /proc/self/mountinfo list backwards because then any
        // matching isParentOf must be the actual mount point because it's the most
        // recent mount on that path. Linux does allow mounting over non-empty
        // directories, such as in:
        //   # mount | tail -2
        //   tmpfs on /tmp/foo/bar type tmpfs (rw,relatime,inode64)
        //   tmpfs on /tmp/foo type tmpfs (rw,relatime,inode64)
        //
        // We try to match the device ID in case there's a mount --move.
        // We can't *rely* on it because some filesystems like btrfs will assign
        // device IDs to subvolumes that aren't listed in /proc/self/mountinfo.

        const QString oldRootPath = std::exchange(rootPath, QString());
        const dev_t rootPathDevId = deviceIdForPath(oldRootPath);
        for (auto it = infos.rbegin(); it != infos.rend(); ++it) {
            if (!isParentOf(it->mountPoint, oldRootPath))
                continue;
            if (rootPathDevId == it->stDev) {
                // device ID matches; this is definitely the best option
                best = q20::to_address(it);
                break;
            }
            if (!best) {
                // if we can't find a device ID match, this parent path is probably
                // the correct one
                best = q20::to_address(it);
            }
        }
    }
    if (best) {
        auto stDev = best->stDev;
        setFromMountInfo(std::move(*best));
        name = retrieveLabel(*this, fd, stDev);
    }
}

QList<QStorageInfo> QStorageInfoPrivate::mountedVolumes()
{
    QList<QStorageInfo> volumes = listMounts();
    if (!volumes.isEmpty())
        return volumes;

    std::vector<MountInfo> infos = parseMountInfo(FilterMountInfo::Filtered);
    if (infos.empty()) {
        volumes.append(root());
        return volumes;
    }

    QStorageLabelRetriever labelForDevice;
    volumes.reserve(infos.size());
    for (auto it = infos.begin(); it != infos.end(); ++it) {
        MountInfo &info = *it;
        AutoFileDescriptor fd(info.mountPoint);

        // find out if the path as we see it matches this line from mountinfo
        quint64 mntid = mountIdForPath(fd);
        if (mntid == 0) {
            // statx failed, so scan the later lines to see if any is a parent
            // to this
            auto isParent = [&info](const MountInfo &maybeParent) {
                return isParentOf(maybeParent.mountPoint, info.mountPoint);
            };
            if (std::find_if(it + 1, infos.end(), isParent) != infos.end())
                continue;
        } else if (mntid != info.mntid) {
            continue;
        }

        const auto infoStDev = info.stDev;
        QStorageInfoPrivate d(std::move(info));
        d.retrieveVolumeInfo();
        if (d.bytesTotal <= 0 && d.rootPath != u'/')
            continue;
        d.name = labelForDevice(d, fd, infoStDev);
        volumes.emplace_back(QStorageInfo(*new QStorageInfoPrivate(std::move(d))));
    }
    return volumes;
}

QT_END_NAMESPACE
