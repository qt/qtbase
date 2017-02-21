/****************************************************************************
**
** Copyright (C) 2016 Intel Corporation
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
    foreach (const QStorageInfo &info, volumes) {
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
