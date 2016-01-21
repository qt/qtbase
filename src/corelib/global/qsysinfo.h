/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <QtCore/qglobal.h>

#ifndef QSYSINFO_H
#define QSYSINFO_H

QT_BEGIN_NAMESPACE

/*
   System information
*/

class QString;
class Q_CORE_EXPORT QSysInfo {
public:
    enum Sizes {
        WordSize = (sizeof(void *)<<3)
    };

#if defined(QT_BUILD_QMAKE)
    enum Endian {
        BigEndian,
        LittleEndian
    };
    /* needed to bootstrap qmake */
    static const int ByteOrder;
#elif defined(Q_BYTE_ORDER)
    enum Endian {
        BigEndian,
        LittleEndian

#  ifdef Q_QDOC
        , ByteOrder = <platform-dependent>
#  elif Q_BYTE_ORDER == Q_BIG_ENDIAN
        , ByteOrder = BigEndian
#  elif Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        , ByteOrder = LittleEndian
#  else
#    error "Undefined byte order"
#  endif
    };
#endif
    enum WinVersion {
        WV_None     = 0x0000,

        WV_32s      = 0x0001,
        WV_95       = 0x0002,
        WV_98       = 0x0003,
        WV_Me       = 0x0004,
        WV_DOS_based= 0x000f,

        /* codenames */
        WV_NT       = 0x0010,
        WV_2000     = 0x0020,
        WV_XP       = 0x0030,
        WV_2003     = 0x0040,
        WV_VISTA    = 0x0080,
        WV_WINDOWS7 = 0x0090,
        WV_WINDOWS8 = 0x00a0,
        WV_WINDOWS8_1 = 0x00b0,
        WV_WINDOWS10 = 0x00c0,
        WV_NT_based = 0x00f0,

        /* version numbers */
        WV_4_0      = WV_NT,
        WV_5_0      = WV_2000,
        WV_5_1      = WV_XP,
        WV_5_2      = WV_2003,
        WV_6_0      = WV_VISTA,
        WV_6_1      = WV_WINDOWS7,
        WV_6_2      = WV_WINDOWS8,
        WV_6_3      = WV_WINDOWS8_1,
        WV_10_0     = WV_WINDOWS10,

        WV_CE       = 0x0100,
        WV_CENET    = 0x0200,
        WV_CE_5     = 0x0300,
        WV_CE_6     = 0x0400,
        WV_CE_based = 0x0f00
    };
#if defined(Q_OS_WIN) || defined(Q_OS_CYGWIN)
    static const WinVersion WindowsVersion;
    static WinVersion windowsVersion();
#else
    static const WinVersion WindowsVersion = WV_None;
    static WinVersion windowsVersion() { return WV_None; }
#endif

#define Q_MV_OSX(major, minor) (major == 10 ? minor + 2 : (major == 9 ? 1 : 0))
#define Q_MV_IOS(major, minor) (QSysInfo::MV_IOS | major << 4 | minor)
    enum MacVersion {
        MV_None    = 0xffff,
        MV_Unknown = 0x0000,

        /* version */
        MV_9 = Q_MV_OSX(9, 0),
        MV_10_0 = Q_MV_OSX(10, 0),
        MV_10_1 = Q_MV_OSX(10, 1),
        MV_10_2 = Q_MV_OSX(10, 2),
        MV_10_3 = Q_MV_OSX(10, 3),
        MV_10_4 = Q_MV_OSX(10, 4),
        MV_10_5 = Q_MV_OSX(10, 5),
        MV_10_6 = Q_MV_OSX(10, 6),
        MV_10_7 = Q_MV_OSX(10, 7),
        MV_10_8 = Q_MV_OSX(10, 8),
        MV_10_9 = Q_MV_OSX(10, 9),
        MV_10_10 = Q_MV_OSX(10, 10),
        MV_10_11 = Q_MV_OSX(10, 11),

        /* codenames */
        MV_CHEETAH = MV_10_0,
        MV_PUMA = MV_10_1,
        MV_JAGUAR = MV_10_2,
        MV_PANTHER = MV_10_3,
        MV_TIGER = MV_10_4,
        MV_LEOPARD = MV_10_5,
        MV_SNOWLEOPARD = MV_10_6,
        MV_LION = MV_10_7,
        MV_MOUNTAINLION = MV_10_8,
        MV_MAVERICKS = MV_10_9,
        MV_YOSEMITE = MV_10_10,
        MV_ELCAPITAN = MV_10_11,

        /* iOS */
        MV_IOS     = 1 << 8,
        MV_IOS_4_3 = Q_MV_IOS(4, 3),
        MV_IOS_5_0 = Q_MV_IOS(5, 0),
        MV_IOS_5_1 = Q_MV_IOS(5, 1),
        MV_IOS_6_0 = Q_MV_IOS(6, 0),
        MV_IOS_6_1 = Q_MV_IOS(6, 1),
        MV_IOS_7_0 = Q_MV_IOS(7, 0),
        MV_IOS_7_1 = Q_MV_IOS(7, 1),
        MV_IOS_8_0 = Q_MV_IOS(8, 0),
        MV_IOS_8_1 = Q_MV_IOS(8, 1),
        MV_IOS_8_2 = Q_MV_IOS(8, 2),
        MV_IOS_8_3 = Q_MV_IOS(8, 3),
        MV_IOS_8_4 = Q_MV_IOS(8, 4),
        MV_IOS_9_0 = Q_MV_IOS(9, 0)
    };
#if defined(Q_OS_MAC)
    static const MacVersion MacintoshVersion;
    static MacVersion macVersion();
#else
    static const MacVersion MacintoshVersion = MV_None;
    static MacVersion macVersion() { return MV_None; }
#endif

    static QString buildCpuArchitecture();
    static QString currentCpuArchitecture();
    static QString buildAbi();

    static QString kernelType();
    static QString kernelVersion();
    static QString productType();
    static QString productVersion();
    static QString prettyProductName();

    static QString machineHostName();
};

QT_END_NAMESPACE
#endif // QSYSINFO_H
