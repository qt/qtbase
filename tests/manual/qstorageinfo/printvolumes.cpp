// Copyright (C) 2016 Intel Corporation
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/QStorageInfo>

void printVolumes(const QList<QStorageInfo> &volumes, int (*printer)(const char *, ...))
{
    // Sample output:
    //  Filesystem (Type)            Size  Available BSize  Label            Mounted on
    //  /dev/sda2 (ext4)    RO     388480     171218  1024                   /boot
    //  /dev/mapper/system-root (btrfs) RW
    //                          214958080   39088272  4096                   /
    //  /dev/disk1s2 (hfs)  RW  488050672  419909696  4096  Macintosh HD2    /Volumes/Macintosh HD2

    printer("Filesystem (Type)            Size  Available BSize  Label            Mounted on\n");
    for (const QStorageInfo &info : volumes) {
        QByteArray fsAndType = info.device();
        if (info.fileSystemType() != fsAndType)
            fsAndType += " (" + info.fileSystemType() + ')';

        printer("%-19s R%c ", fsAndType.constData(), info.isReadOnly() ? 'O' : 'W');
        if (fsAndType.size() > 19)
            printer("\n%23s", "");

        printer("%10llu %10llu %5u  ", info.bytesTotal() / 1024, info.bytesFree() / 1024, info.blockSize());
        if (!info.subvolume().isEmpty())
            printer("subvol=%-18s ", qPrintable(info.subvolume()));
        else
            printer("%-25s ", qPrintable(info.name()));
        printer("%s\n", qPrintable(info.rootPath()));
    }
}
