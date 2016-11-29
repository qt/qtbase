/****************************************************************************
**
** Copyright (C) 2016 Intel Corporation
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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
        printer("%-16s %s\n", qPrintable(info.name()), qPrintable(info.rootPath()));
    }
}
