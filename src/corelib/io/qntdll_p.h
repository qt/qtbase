// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QNTDLL_P_H
#define QNTDLL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qglobal_p.h>

#include <winternl.h>

QT_BEGIN_NAMESPACE

// keep the following structure as is, taken from
// https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/ns-ntddk-_file_fs_sector_size_information
// Unfortunately we can't include the ntddk.h header, so we duplicate the code here.
typedef struct _FILE_FS_SECTOR_SIZE_INFORMATION {
    ULONG LogicalBytesPerSector;
    ULONG PhysicalBytesPerSectorForAtomicity;
    ULONG PhysicalBytesPerSectorForPerformance;
    ULONG FileSystemEffectivePhysicalBytesPerSectorForAtomicity;
    ULONG Flags;
    ULONG ByteOffsetForSectorAlignment;
    ULONG ByteOffsetForPartitionAlignment;
} FILE_FS_SECTOR_SIZE_INFORMATION, *PFILE_FS_SECTOR_SIZE_INFORMATION;

#if !defined(Q_CC_MINGW)
// keep the following enumeration as is, taken from
// https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/ne-wdm-_fsinfoclass
// Unfortunately we can't include the wdm.h header, so we duplicate the code here.
typedef enum _FSINFOCLASS {
    FileFsVolumeInformation,
    FileFsLabelInformation,
    FileFsSizeInformation,
    FileFsDeviceInformation,
    FileFsAttributeInformation,
    FileFsControlInformation,
    FileFsFullSizeInformation,
    FileFsObjectIdInformation,
    FileFsDriverPathInformation,
    FileFsVolumeFlagsInformation,
    FileFsSectorSizeInformation,
    FileFsDataCopyInformation,
    FileFsMetadataSizeInformation,
    FileFsFullSizeInformationEx,
    FileFsMaximumInformation
} FS_INFORMATION_CLASS, *PFS_INFORMATION_CLASS;
#endif

QT_END_NAMESPACE

#endif // QNTDLL_P_H
