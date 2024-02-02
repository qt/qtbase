// Copyright (C) 2016 Intel Corporation
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QStorageInfo>

void printVolumes(const QList<QStorageInfo> &volumes, int (*printer)(const char *, ...))
{
    // Sample output:
    //  Filesystem (Type)            Size  Available BSize  Label            Mounted on
    //  /dev/sda2 (ext4)    RO     388480     171218  1024                   /boot
    //  /dev/mapper/system-root (btrfs) RW
    //                          214958080   39088272  4096                   /
    //  /dev/disk1s2 (hfs)  RW  488050672  419909696  4096  Macintosh HD2    /Volumes/Macintosh HD2

    int fsColumnWidth = 25;
    int labelColumnWidth = 20;
    for (const QStorageInfo &info : volumes) {
        int len = 3 + info.device().size() + info.fileSystemType().size();
        fsColumnWidth = qMax(fsColumnWidth, len);
        if (QString subvol = info.subvolume(); !subvol.isEmpty())
            labelColumnWidth = qMax(labelColumnWidth, int(subvol.size() + strlen("subvol=")));
        else
            labelColumnWidth = qMax(labelColumnWidth, int(info.name().size()));
    }

    printer("%*s          Size  Available BSize  %*s Mounted on\n",
            -fsColumnWidth, "Filesystem (Type)",
            -labelColumnWidth, "Label");
    for (const QStorageInfo &info : volumes) {
        QByteArray fsAndType = info.device();
        if (info.fileSystemType() != fsAndType)
            fsAndType += " (" + info.fileSystemType() + ')';

        printer("%*s R%c ", -fsColumnWidth, fsAndType.constData(), info.isReadOnly() ? 'O' : 'W');
        printer("%10llu %10llu %5u  ", info.bytesTotal() / 1024, info.bytesFree() / 1024, info.blockSize());
        if (!info.subvolume().isEmpty())
            printer("subvol=%*s ", -labelColumnWidth + int(strlen("subvol=")), qPrintable(info.subvolume()));
        else
            printer("%*s ", -labelColumnWidth, qPrintable(info.name()));
        printer("%s\n", qPrintable(info.rootPath()));
    }
}
