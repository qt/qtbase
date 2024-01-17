// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2014 Ivan Komissarov <ABBAPOH@gmail.com>
// Copyright (C) 2016 Intel Corporation.
// Copyright (C) 2023 Ahmad Samir <a.samirh78@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qstorageinfo_linux_p.h"

#include "qdiriterator.h"
#include <private/qcore_unix_p.h>
#include <private/qtools_p.h>

#include <q20memory.h>

#include <linux/mount.h>
#include <sys/ioctl.h>
#include <sys/statfs.h>

// so we don't have to #include <linux/fs.h>, which is known to cause conflicts
#ifndef FSLABEL_MAX
#  define FSLABEL_MAX           256
#endif
#ifndef FS_IOC_GETFSLABEL
#  define FS_IOC_GETFSLABEL     _IOR(0x94, 49, char[FSLABEL_MAX])
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

// udev encodes the labels with ID_LABEL_FS_ENC which is done with
// blkid_encode_string(). Within this function some 1-byte utf-8
// characters not considered safe (e.g. '\' or ' ') are encoded as hex
static QString decodeFsEncString(QString &&str)
{
    using namespace QtMiscUtils;
    qsizetype start = str.indexOf(u'\\');
    if (start < 0)
        return std::move(str);

    // decode in-place
    QString decoded = std::move(str);
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

static QDirIterator devicesByLabel()
{
    static const char pathDiskByLabel[] = "/dev/disk/by-label";
    static constexpr auto LabelFileFilter =
            QDir::AllEntries | QDir::System | QDir::Hidden | QDir::NoDotAndDotDot;

    return QDirIterator(QLatin1StringView(pathDiskByLabel), LabelFileFilter);
}

static inline auto retrieveLabels()
{
    struct Entry {
        QString label;
        quint64 deviceId;
    };
    QList<Entry> result;

    QDirIterator it = devicesByLabel();
    while (it.hasNext()) {
        QFileInfo fileInfo = it.nextFileInfo();
        quint64 deviceId = retrieveDeviceId(QFile::encodeName(fileInfo.filePath()));
        if (!deviceId)
            continue;
        result.emplaceBack(Entry{ decodeFsEncString(fileInfo.fileName()), deviceId });
    }
    return result;
}

static std::optional<QString> retrieveLabelViaIoctl(const QString &path)
{
    // FS_IOC_GETFSLABEL was introduced in v4.18; previously it was btrfs-specific.
    int fd = qt_safe_open(QFile::encodeName(path).constData(), QT_OPEN_RDONLY);
    if (fd < 0)
        return std::nullopt;

    // Note: it doesn't append the null terminator (despite what the man page
    // says) and the return code on success (0) does not indicate the length.
    char label[FSLABEL_MAX] = {};
    int r = ioctl(fd, FS_IOC_GETFSLABEL, &label);
    close(fd);
    if (r < 0)
        return std::nullopt;
    return QString::fromUtf8(label);
}

static inline QString retrieveLabel(const QStorageInfoPrivate &d, quint64 deviceId)
{
    if (auto label = retrieveLabelViaIoctl(d.rootPath))
        return *label;

    deviceId = retrieveDeviceId(d.device, deviceId);
    if (!deviceId)
        return QString();

    QDirIterator it = devicesByLabel();
    while (it.hasNext()) {
        QFileInfo fileInfo = it.nextFileInfo();
        QString name = fileInfo.fileName();
        if (retrieveDeviceId(QFile::encodeName(fileInfo.filePath())) == deviceId)
            return decodeFsEncString(std::move(name));
    }
    return QString();
}

void QStorageInfoPrivate::retrieveVolumeInfo()
{
    struct statfs64 statfs_buf;
    int result;
    QT_EINTR_LOOP(result, statfs64(QFile::encodeName(rootPath).constData(), &statfs_buf));
    if (result == 0) {
        valid = true;
        ready = true;

        bytesTotal = statfs_buf.f_blocks * statfs_buf.f_frsize;
        bytesFree = statfs_buf.f_bfree * statfs_buf.f_frsize;
        bytesAvailable = statfs_buf.f_bavail * statfs_buf.f_frsize;
        blockSize = int(statfs_buf.f_bsize);
        readOnly = (statfs_buf.f_flags & MS_RDONLY) != 0;
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

void QStorageInfoPrivate::doStat()
{
    retrieveVolumeInfo();
    if (!ready)
        return;

    rootPath = QFileInfo(rootPath).canonicalFilePath();
    if (rootPath.isEmpty())
        return;

    std::vector<MountInfo> infos = parseMountInfo();
    if (infos.empty()) {
        rootPath = u'/';
        return;
    }

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
    MountInfo *best = nullptr;
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
    if (best) {
        auto stDev = best->stDev;
        setFromMountInfo(std::move(*best));
        name = retrieveLabel(*this, stDev);
    }
}

QList<QStorageInfo> QStorageInfoPrivate::mountedVolumes()
{
    std::vector<MountInfo> infos = parseMountInfo(FilterMountInfo::Filtered);
    if (infos.empty())
        return QList{root()};

    std::optional<decltype(retrieveLabels())> labelMap;
    auto labelForDevice = [&labelMap](const QStorageInfoPrivate &d, quint64 devid) {
        if (d.fileSystemType == "tmpfs")
            return QString();

        if (auto label = retrieveLabelViaIoctl(d.rootPath))
            return *label;

        devid = retrieveDeviceId(d.device, devid);
        if (!devid)
            return QString();

        if (!labelMap)
            labelMap = retrieveLabels();
        for (auto &[deviceLabel, deviceId] : std::as_const(*labelMap)) {
            if (devid == deviceId)
                return deviceLabel;
        }
        return QString();
    };

    QList<QStorageInfo> volumes;
    for (MountInfo &info : infos) {
        QStorageInfoPrivate d(std::move(info));
        d.retrieveVolumeInfo();
        if (d.bytesTotal <= 0 && d.rootPath != u'/')
            continue;
        if (info.stDev != deviceIdForPath(d.rootPath))
            continue;       // probably something mounted over this mountpoint
        d.name = labelForDevice(d, info.stDev);
        volumes.emplace_back(QStorageInfo(*new QStorageInfoPrivate(std::move(d))));
    }
    return volumes;
}

QT_END_NAMESPACE
