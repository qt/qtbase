/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#include "qoperatingsystemversion_win_p.h"

#include "qoperatingsystemversion_p.h"

#include <qt_windows.h>
#include <qbytearray.h>

QT_BEGIN_NAMESPACE

static inline OSVERSIONINFOEX determineWinOsVersion()
{
    OSVERSIONINFOEX result = { sizeof(OSVERSIONINFOEX), 0, 0, 0, 0, {'\0'}, 0, 0, 0, 0, 0};

    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (Q_UNLIKELY(!ntdll))
        return result;

    typedef NTSTATUS (NTAPI *RtlGetVersionFunction)(LPOSVERSIONINFO);

    // RtlGetVersion is documented public API but we must load it dynamically
    // because linking to it at load time will not pass the Windows App Certification Kit
    // https://msdn.microsoft.com/en-us/library/windows/hardware/ff561910.aspx
    RtlGetVersionFunction pRtlGetVersion = reinterpret_cast<RtlGetVersionFunction>(
        reinterpret_cast<QFunctionPointer>(GetProcAddress(ntdll, "RtlGetVersion")));
    if (Q_UNLIKELY(!pRtlGetVersion))
        return result;

    // GetVersionEx() has been deprecated in Windows 8.1 and will return
    // only Windows 8 from that version on, so use the kernel API function.
    pRtlGetVersion(reinterpret_cast<LPOSVERSIONINFO>(&result)); // always returns STATUS_SUCCESS
    return result;
}

OSVERSIONINFOEX qWindowsVersionInfo()
{
    OSVERSIONINFOEX realResult = determineWinOsVersion();
#ifdef QT_DEBUG
    {
        if (Q_UNLIKELY(qEnvironmentVariableIsSet("QT_WINVER_OVERRIDE"))) {
            OSVERSIONINFOEX result = realResult;
            result.dwMajorVersion = 0;
            result.dwMinorVersion = 0;

            // Erase any build number and service pack information
            result.dwBuildNumber = 0;
            result.szCSDVersion[0] = L'\0';
            result.wServicePackMajor = 0;
            result.wServicePackMinor = 0;

            const QByteArray winVerOverride = qgetenv("QT_WINVER_OVERRIDE");
            if (winVerOverride == "WINDOWS10" || winVerOverride == "2016") {
                result.dwMajorVersion = 10;
            } else {
                return realResult;
            }

            if (winVerOverride == "2016") {
                // If the current host OS is a domain controller and the override OS
                // is also a server type OS, preserve that information
                if (result.wProductType == VER_NT_WORKSTATION)
                    result.wProductType = VER_NT_SERVER;
            } else {
                // Any other OS must be a workstation OS type
                result.wProductType = VER_NT_WORKSTATION;
            }
        }
    }
#endif
    return realResult;
}

QOperatingSystemVersion QOperatingSystemVersion::current()
{
    static QOperatingSystemVersion v = [](){
        QOperatingSystemVersion v;
        v.m_os = currentType();
        const OSVERSIONINFOEX osv = qWindowsVersionInfo();
        v.m_major = osv.dwMajorVersion;
        v.m_minor = osv.dwMinorVersion;
        v.m_micro = osv.dwBuildNumber;
        return v;
    }();
    return v;
}

QT_END_NAMESPACE
