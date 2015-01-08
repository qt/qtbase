/****************************************************************************
**
** Copyright (C) 2015 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
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
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwindowsopengltester.h"
#include "qwindowscontext.h"

#include <QtCore/QVariantMap>
#include <QtCore/QDebug>
#include <QtCore/QTextStream>

#ifndef Q_OS_WINCE
#  include <QtCore/qt_windows.h>
#  include <private/qsystemlibrary_p.h>
#  include <d3d9.h>
#endif

QT_BEGIN_NAMESPACE

QString GpuDriverVersion::toString() const
{
    return QString::number(product)
        + QLatin1Char('.') + QString::number(version)
        + QLatin1Char('.') + QString::number(subVersion)
        + QLatin1Char('.') + QString::number(build);
}

int GpuDriverVersion::compare(const GpuDriverVersion &rhs) const
{
    if (product < rhs.product)
        return -1;
    if (product > rhs.product)
        return 1;
    if (version < rhs.version)
        return -1;
    if (version > rhs.version)
        return 1;
    if (subVersion < rhs.subVersion)
        return -1;
    if (subVersion > rhs.subVersion)
        return 1;
    if (build < rhs.build)
        return -1;
    if (build > rhs.build)
        return 1;
    return 0;
}

GpuDescription GpuDescription::detect()
{
#ifndef Q_OS_WINCE
    typedef IDirect3D9 * (WINAPI *PtrDirect3DCreate9)(UINT);

    GpuDescription result;
    QSystemLibrary d3d9lib(QStringLiteral("d3d9"));
    if (!d3d9lib.load())
        return result;
    PtrDirect3DCreate9 direct3DCreate9 = (PtrDirect3DCreate9)d3d9lib.resolve("Direct3DCreate9");
    if (!direct3DCreate9)
        return result;
    IDirect3D9 *direct3D9 = direct3DCreate9(D3D_SDK_VERSION);
    if (!direct3D9)
        return result;
    D3DADAPTER_IDENTIFIER9 adapterIdentifier;
    const HRESULT hr = direct3D9->GetAdapterIdentifier(0, 0, &adapterIdentifier);
    direct3D9->Release();
    if (SUCCEEDED(hr)) {
        result.vendorId = int(adapterIdentifier.VendorId);
        result.deviceId = int(adapterIdentifier.DeviceId);
        result.revision = int(adapterIdentifier.Revision);
        result.subSysId = int(adapterIdentifier.SubSysId);
        result.driverVersion.product = HIWORD(adapterIdentifier.DriverVersion.HighPart);
        result.driverVersion.version = LOWORD(adapterIdentifier.DriverVersion.HighPart);
        result.driverVersion.subVersion = HIWORD(adapterIdentifier.DriverVersion.LowPart);
        result.driverVersion.build = LOWORD(adapterIdentifier.DriverVersion.LowPart);
        result.driverName = adapterIdentifier.Driver;
        result.description = adapterIdentifier.Description;
    }
    return result;
#else // !Q_OS_WINCE
    GpuDescription result;
    result.vendorId = result.deviceId = result.revision
        = result.driverVersion.product = result.driverVersion.version
        = result.driverVersion.build = 1;
    result.driverName = result.description = QByteArrayLiteral("Generic");
    return result;
#endif
}

QDebug operator<<(QDebug d, const GpuDriverVersion &v)
{
    QDebugStateSaver s(d);
    d.nospace();
    d << v.product << '.' << v.version << '.' << v.subVersion << '.' << v.build;
    return d;
}

QDebug operator<<(QDebug d, const GpuDescription &gd)
{
    QDebugStateSaver s(d);
    d.nospace();
    d << hex << showbase << "GpuDescription(vendorId=" << gd.vendorId
      << ", deviceId=" << gd.deviceId << ", subSysId=" << gd.subSysId
      << dec << noshowbase << ", revision=" << gd.revision
      << ", driver: " << gd.driverName
      << ", version=" << gd.driverVersion << ", " << gd.description << ')';
    return d;
}

// Return printable string formatted like the output of the dxdiag tool.
QString GpuDescription::toString() const
{
    QString result;
    QTextStream str(&result);
    str <<   "         Card name: " << description
        << "\n       Driver Name: " << driverName
        << "\n    Driver Version: " << driverVersion.toString()
        << "\n         Vendor ID: 0x" << qSetPadChar(QLatin1Char('0'))
        << uppercasedigits << hex << qSetFieldWidth(4) << vendorId
        << "\n         Device ID: 0x" << qSetFieldWidth(4) << deviceId
        << "\n         SubSys ID: 0x" << qSetFieldWidth(8) << subSysId
        << "\n       Revision ID: 0x" << qSetFieldWidth(4) << revision
        << dec;
    return result;
}

QVariant GpuDescription::toVariant() const
{
    QVariantMap result;
    result.insert(QStringLiteral("vendorId"), QVariant(vendorId));
    result.insert(QStringLiteral("deviceId"), QVariant(deviceId));
    result.insert(QStringLiteral("subSysId"),QVariant(subSysId));
    result.insert(QStringLiteral("revision"), QVariant(revision));
    result.insert(QStringLiteral("driver"), QVariant(QLatin1String(driverName)));
    result.insert(QStringLiteral("driverProduct"), QVariant(driverVersion.product));
    result.insert(QStringLiteral("driverVersion"), QVariant(driverVersion.version));
    result.insert(QStringLiteral("driverSubVersion"), QVariant(driverVersion.subVersion));
    result.insert(QStringLiteral("driverBuild"), QVariant(driverVersion.build));
    result.insert(QStringLiteral("driverVersionString"), driverVersion.toString());
    result.insert(QStringLiteral("description"), QVariant(QLatin1String(description)));
    result.insert(QStringLiteral("printable"), QVariant(toString()));
    return result;
}

bool QWindowsOpenGLTester::testDesktopGL()
{
#ifndef Q_OS_WINCE
    HMODULE lib = 0;
    HWND wnd = 0;
    HDC dc = 0;
    HGLRC context = 0;
    LPCTSTR className = L"qtopengltest";

    HGLRC (WINAPI * CreateContext)(HDC dc) = 0;
    BOOL (WINAPI * DeleteContext)(HGLRC context) = 0;
    BOOL (WINAPI * MakeCurrent)(HDC dc, HGLRC context) = 0;
    PROC (WINAPI * WGL_GetProcAddress)(LPCSTR name) = 0;

    bool result = false;

    // Test #1: Load opengl32.dll and try to resolve an OpenGL 2 function.
    // This will typically fail on systems that do not have a real OpenGL driver.
    lib = LoadLibraryA("opengl32.dll");
    if (lib) {
        CreateContext = reinterpret_cast<HGLRC (WINAPI *)(HDC)>(::GetProcAddress(lib, "wglCreateContext"));
        if (!CreateContext)
            goto cleanup;
        DeleteContext = reinterpret_cast<BOOL (WINAPI *)(HGLRC)>(::GetProcAddress(lib, "wglDeleteContext"));
        if (!DeleteContext)
            goto cleanup;
        MakeCurrent = reinterpret_cast<BOOL (WINAPI *)(HDC, HGLRC)>(::GetProcAddress(lib, "wglMakeCurrent"));
        if (!MakeCurrent)
            goto cleanup;
        WGL_GetProcAddress = reinterpret_cast<PROC (WINAPI *)(LPCSTR)>(::GetProcAddress(lib, "wglGetProcAddress"));
        if (!WGL_GetProcAddress)
            goto cleanup;

        WNDCLASS wclass;
        wclass.cbClsExtra = 0;
        wclass.cbWndExtra = 0;
        wclass.hInstance = (HINSTANCE) GetModuleHandle(0);
        wclass.hIcon = 0;
        wclass.hCursor = 0;
        wclass.hbrBackground = (HBRUSH) (COLOR_BACKGROUND);
        wclass.lpszMenuName = 0;
        wclass.lpfnWndProc = DefWindowProc;
        wclass.lpszClassName = className;
        wclass.style = CS_OWNDC;
        if (!RegisterClass(&wclass))
            goto cleanup;
        wnd = CreateWindow(className, L"qtopenglproxytest", WS_OVERLAPPED,
                           0, 0, 640, 480, 0, 0, wclass.hInstance, 0);
        if (!wnd)
            goto cleanup;
        dc = GetDC(wnd);
        if (!dc)
            goto cleanup;

        PIXELFORMATDESCRIPTOR pfd;
        memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
        pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_GENERIC_FORMAT;
        pfd.iPixelType = PFD_TYPE_RGBA;
        // Use the GDI functions. Under the hood this will call the wgl variants in opengl32.dll.
        int pixelFormat = ChoosePixelFormat(dc, &pfd);
        if (!pixelFormat)
            goto cleanup;
        if (!SetPixelFormat(dc, pixelFormat, &pfd))
            goto cleanup;
        context = CreateContext(dc);
        if (!context)
            goto cleanup;
        if (!MakeCurrent(dc, context))
            goto cleanup;

        // Now that there is finally a context current, try doing something useful.
        if (WGL_GetProcAddress("glCreateShader")) {
            result = true;
            qCDebug(lcQpaGl, "OpenGL 2.0 entry points available");
        } else {
            qCDebug(lcQpaGl, "OpenGL 2.0 entry points not found");
        }
    } else {
        qCDebug(lcQpaGl, "Failed to load opengl32.dll");
    }

cleanup:
    if (MakeCurrent)
        MakeCurrent(0, 0);
    if (context)
        DeleteContext(context);
    if (dc && wnd)
        ReleaseDC(wnd, dc);
    if (wnd) {
        DestroyWindow(wnd);
        UnregisterClass(className, GetModuleHandle(0));
    }
    // No FreeLibrary. Some implementations, Mesa in particular, deadlock when trying to unload.

    return result;
#else // !Q_OS_WINCE
    return false;
#endif
}

QT_END_NAMESPACE
