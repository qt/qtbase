/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QPLATFORMDEFS_WIN_H
#define QPLATFORMDEFS_WIN_H

// Since we need to include winsock2.h, we need to define WIN32_LEAN_AND_MEAN
// somewhere above so windows.h won't include winsock.h.
#include <winsock2.h>
#include <mswsock.h>
#undef interface
#include <wincrypt.h>
#include <winioctl.h>

QT_BEGIN_NAMESPACE

#ifndef NS_NLA

#define NS_NLA 15

#ifndef NLA_NAMESPACE_GUID
enum NLA_BLOB_DATA_TYPE {
    NLA_RAW_DATA = 0,
    NLA_INTERFACE = 1,
    NLA_802_1X_LOCATION = 2,
    NLA_CONNECTIVITY = 3,
    NLA_ICS = 4
};

enum NLA_CONNECTIVITY_TYPE {
    NLA_NETWORK_AD_HOC = 0,
    NLA_NETWORK_MANAGED = 1,
    NLA_NETWORK_UNMANAGED = 2,
    NLA_NETWORK_UNKNOWN = 3
};

enum NLA_INTERNET {
    NLA_INTERNET_UNKNOWN = 0,
    NLA_INTERNET_NO = 1,
    NLA_INTERNET_YES = 2
};

struct NLA_BLOB {
    struct {
        NLA_BLOB_DATA_TYPE type;
        DWORD dwSize;
        DWORD nextOffset;
    } header;

    union {
        // NLA_RAW_DATA
        CHAR rawData[1];

        // NLA_INTERFACE
        struct {
            DWORD dwType;
            DWORD dwSpeed;
            CHAR adapterName[1];
        } interfaceData;

        // NLA_802_1X_LOCATION
        struct {
            CHAR information[1];
        } locationData;

        // NLA_CONNECTIVITY
        struct {
            NLA_CONNECTIVITY_TYPE type;
            NLA_INTERNET internet;
        } connectivity;

        // NLA_ICS
        struct {
            struct {
                DWORD speed;
                DWORD type;
                DWORD state;
                WCHAR machineName[256];
                WCHAR sharedAdapterName[256];
            } remote;
        } ICS;
    } data;
};
#endif // NLA_NAMESPACE_GUID

#endif

enum NDIS_MEDIUM {
    NdisMedium802_3 = 0,
};

enum NDIS_PHYSICAL_MEDIUM {
    NdisPhysicalMediumWirelessLan = 1,
    NdisPhysicalMediumBluetooth = 10,
    NdisPhysicalMediumWiMax = 12,
};

#define OID_GEN_MEDIA_SUPPORTED 0x00010103
#define OID_GEN_PHYSICAL_MEDIUM 0x00010202

#define IOCTL_NDIS_QUERY_GLOBAL_STATS \
    CTL_CODE(FILE_DEVICE_PHYSICAL_NETCARD, 0, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)

QT_END_NAMESPACE

#endif // QPLATFORMDEFS_WIN_H
