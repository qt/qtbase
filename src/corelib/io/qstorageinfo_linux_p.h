// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2014 Ivan Komissarov <ABBAPOH@gmail.com>
// Copyright (C) 2016 Intel Corporation.
// Copyright (C) 2023 Ahmad Samir <a.samirh78@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSTORAGEINFO_LINUX_P_H
#define QSTORAGEINFO_LINUX_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.
// This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qstorageinfo_p.h"

#include <QtCore/qsystemdetection.h>
#include <QtCore/private/qlocale_tools_p.h>

#include <sys/sysmacros.h> // makedev()

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
using MountInfo = QStorageInfoPrivate::MountInfo;

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

// parseMountInfo() is called from:
// - QStorageInfoPrivate::initRootPath(), where a list of all mounted volumes is needed
// - QStorageInfoPrivate::mountedVolumes(), where some filesystem types are ignored
//   (see shouldIncludefs())
enum class FilterMountInfo {
    All,
    Filtered,
};

[[maybe_unused]] static std::vector<MountInfo>
doParseMountInfo(const QByteArray &mountinfo, FilterMountInfo filter = FilterMountInfo::All)
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

        if (filter == FilterMountInfo::Filtered && !shouldIncludeFs(info.mountPoint, info.fsType))
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

QT_END_NAMESPACE

#endif // QSTORAGEINFO_LINUX_P_H
