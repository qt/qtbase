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

#include "qwindowsopengltester.h"
#include "qwindowscontext.h"

#include <QtCore/QVariantMap>
#include <QtCore/QDebug>
#include <QtCore/QTextStream>
#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QStandardPaths>
#include <QtCore/QLibraryInfo>
#include <QtCore/QHash>

#ifndef QT_NO_OPENGL
#include <private/qopengl_p.h>
#endif

#ifndef Q_OS_WINCE
#  include <QtCore/qt_windows.h>
#  include <private/qsystemlibrary_p.h>
#  include <d3d9.h>
#endif

QT_BEGIN_NAMESPACE

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
        result.vendorId = adapterIdentifier.VendorId;
        result.deviceId = adapterIdentifier.DeviceId;
        result.revision = adapterIdentifier.Revision;
        result.subSysId = adapterIdentifier.SubSysId;
        QVector<int> version(4, 0);
        version[0] = HIWORD(adapterIdentifier.DriverVersion.HighPart); // Product
        version[1] = LOWORD(adapterIdentifier.DriverVersion.HighPart); // Version
        version[2] = HIWORD(adapterIdentifier.DriverVersion.LowPart); // Sub version
        version[3] = LOWORD(adapterIdentifier.DriverVersion.LowPart); // build
        result.driverVersion = QVersionNumber(version);
        result.driverName = adapterIdentifier.Driver;
        result.description = adapterIdentifier.Description;
    }
    return result;
#else // !Q_OS_WINCE
    GpuDescription result;
    result.vendorId = result.deviceId = result.revision =1;
    result.driverVersion = QVersionNumber(1, 1, 1);
    result.driverName = result.description = QByteArrayLiteral("Generic");
    return result;
#endif
}

#ifndef QT_NO_DEBUG_STREAM
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
#endif // !QT_NO_DEBUG_STREAM

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
    result.insert(QStringLiteral("driverProduct"), QVariant(driverVersion.segmentAt(0)));
    result.insert(QStringLiteral("driverVersion"), QVariant(driverVersion.segmentAt(1)));
    result.insert(QStringLiteral("driverSubVersion"), QVariant(driverVersion.segmentAt(2)));
    result.insert(QStringLiteral("driverBuild"), QVariant(driverVersion.segmentAt(3)));
    result.insert(QStringLiteral("driverVersionString"), driverVersion.toString());
    result.insert(QStringLiteral("description"), QVariant(QLatin1String(description)));
    result.insert(QStringLiteral("printable"), QVariant(toString()));
    return result;
}

QWindowsOpenGLTester::Renderer QWindowsOpenGLTester::requestedGlesRenderer()
{
#ifndef Q_OS_WINCE
    const char platformVar[] = "QT_ANGLE_PLATFORM";
    if (qEnvironmentVariableIsSet(platformVar)) {
        const QByteArray anglePlatform = qgetenv(platformVar);
        if (anglePlatform == "d3d11")
            return QWindowsOpenGLTester::AngleRendererD3d11;
        if (anglePlatform == "d3d9")
            return QWindowsOpenGLTester::AngleRendererD3d9;
        if (anglePlatform == "warp")
            return QWindowsOpenGLTester::AngleRendererD3d11Warp;
        qCWarning(lcQpaGl) << "Invalid value set for " << platformVar << ": " << anglePlatform;
    }
#endif // !Q_OS_WINCE
    return QWindowsOpenGLTester::InvalidRenderer;
}

QWindowsOpenGLTester::Renderer QWindowsOpenGLTester::requestedRenderer()
{
#ifndef Q_OS_WINCE
    const char openGlVar[] = "QT_OPENGL";
    if (QCoreApplication::testAttribute(Qt::AA_UseOpenGLES)) {
        const Renderer glesRenderer = QWindowsOpenGLTester::requestedGlesRenderer();
        return glesRenderer != InvalidRenderer ? glesRenderer : Gles;
    }
    if (QCoreApplication::testAttribute(Qt::AA_UseDesktopOpenGL))
        return QWindowsOpenGLTester::DesktopGl;
    if (QCoreApplication::testAttribute(Qt::AA_UseSoftwareOpenGL))
        return QWindowsOpenGLTester::SoftwareRasterizer;
    if (qEnvironmentVariableIsSet(openGlVar)) {
        const QByteArray requested = qgetenv(openGlVar);
        if (requested == "angle") {
            const Renderer glesRenderer = QWindowsOpenGLTester::requestedGlesRenderer();
            return glesRenderer != InvalidRenderer ? glesRenderer : Gles;
        }
        if (requested == "desktop")
            return QWindowsOpenGLTester::DesktopGl;
        if (requested == "software")
            return QWindowsOpenGLTester::SoftwareRasterizer;
        qCWarning(lcQpaGl) << "Invalid value set for " << openGlVar << ": " << requested;
    }
#endif // !Q_OS_WINCE
    return QWindowsOpenGLTester::InvalidRenderer;
}

#ifndef Q_OS_WINCE

static inline QString resolveBugListFile(const QString &fileName)
{
    if (QFileInfo(fileName).isAbsolute())
        return fileName;
    // Try QLibraryInfo::SettingsPath which is typically empty unless specified in qt.conf,
    // then resolve via QStandardPaths::ConfigLocation.
    const QString settingsPath = QLibraryInfo::location(QLibraryInfo::SettingsPath);
    if (!settingsPath.isEmpty()) { // SettingsPath is empty unless specified in qt.conf.
        const QFileInfo fi(settingsPath + QLatin1Char('/') + fileName);
        if (fi.isFile())
            return fi.absoluteFilePath();
    }
    return QStandardPaths::locate(QStandardPaths::ConfigLocation, fileName);
}

#  ifndef QT_NO_OPENGL
typedef QHash<QOpenGLConfig::Gpu, QWindowsOpenGLTester::Renderers> SupportedRenderersCache;
Q_GLOBAL_STATIC(SupportedRenderersCache, supportedRenderersCache)
#  endif

#endif // !Q_OS_WINCE

QWindowsOpenGLTester::Renderers QWindowsOpenGLTester::detectSupportedRenderers(const GpuDescription &gpu, bool glesOnly)
{
    Q_UNUSED(gpu)
    Q_UNUSED(glesOnly)
#if defined(QT_NO_OPENGL)
    return 0;
#elif defined(Q_OS_WINCE)
    return QWindowsOpenGLTester::Gles;
#else
    QOpenGLConfig::Gpu qgpu = QOpenGLConfig::Gpu::fromDevice(gpu.vendorId, gpu.deviceId, gpu.driverVersion, gpu.description);
    SupportedRenderersCache *srCache = supportedRenderersCache();
    SupportedRenderersCache::const_iterator it = srCache->find(qgpu);
    if (it != srCache->cend())
        return *it;

    QWindowsOpenGLTester::Renderers result(QWindowsOpenGLTester::AngleRendererD3d11
        | QWindowsOpenGLTester::AngleRendererD3d9
        | QWindowsOpenGLTester::AngleRendererD3d11Warp
        | QWindowsOpenGLTester::SoftwareRasterizer);

    if (!glesOnly && testDesktopGL())
        result |= QWindowsOpenGLTester::DesktopGl;

    QSet<QString> features;
    const char bugListFileVar[] = "QT_OPENGL_BUGLIST";
    if (qEnvironmentVariableIsSet(bugListFileVar)) {
        const QString fileName = resolveBugListFile(QFile::decodeName(qgetenv(bugListFileVar)));
        if (!fileName.isEmpty())
            features = QOpenGLConfig::gpuFeatures(qgpu, fileName);
    } else {
        features = QOpenGLConfig::gpuFeatures(qgpu, QStringLiteral(":/qt-project.org/windows/openglblacklists/default.json"));
    }
    qCDebug(lcQpaGl) << "GPU features:" << features;

    if (features.contains(QStringLiteral("disable_desktopgl"))) { // Qt-specific
        qCDebug(lcQpaGl) << "Disabling Desktop GL: " << gpu;
        result &= ~QWindowsOpenGLTester::DesktopGl;
    }
    if (features.contains(QStringLiteral("disable_angle"))) { // Qt-specific keyword
        qCDebug(lcQpaGl) << "Disabling ANGLE: " << gpu;
        result &= ~QWindowsOpenGLTester::GlesMask;
    } else {
        if (features.contains(QStringLiteral("disable_d3d11"))) { // standard keyword
            qCDebug(lcQpaGl) << "Disabling D3D11: " << gpu;
            result &= ~QWindowsOpenGLTester::AngleRendererD3d11;
        }
        if (features.contains(QStringLiteral("disable_d3d9"))) { // Qt-specific
            qCDebug(lcQpaGl) << "Disabling D3D9: " << gpu;
            result &= ~QWindowsOpenGLTester::AngleRendererD3d9;
        }
    }
    if (features.contains(QStringLiteral("disable_rotation"))) {
        qCDebug(lcQpaGl) << "Disabling rotation: " << gpu;
        result |= DisableRotationFlag;
    }
    srCache->insert(qgpu, result);
    return result;
#endif // !Q_OS_WINCE && !QT_NO_OPENGL
}

QWindowsOpenGLTester::Renderers QWindowsOpenGLTester::supportedGlesRenderers()
{
    const GpuDescription gpu = GpuDescription::detect();
    const QWindowsOpenGLTester::Renderers result = detectSupportedRenderers(gpu, true);
    qCDebug(lcQpaGl) << __FUNCTION__ << gpu << "renderer: " << result;
    return result;
}

QWindowsOpenGLTester::Renderers QWindowsOpenGLTester::supportedRenderers()
{
    const GpuDescription gpu = GpuDescription::detect();
    const QWindowsOpenGLTester::Renderers result = detectSupportedRenderers(gpu, false);
    qCDebug(lcQpaGl) << __FUNCTION__ << gpu << "renderer: " << result;
    return result;
}

bool QWindowsOpenGLTester::testDesktopGL()
{
#if !defined(QT_NO_OPENGL) && !defined(Q_OS_WINCE)
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
        wclass.hInstance = static_cast<HINSTANCE>(GetModuleHandle(0));
        wclass.hIcon = 0;
        wclass.hCursor = 0;
        wclass.hbrBackground = HBRUSH(COLOR_BACKGROUND);
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

        // Check the version. If we got 1.x then it's all hopeless and we can stop right here.
        typedef const GLubyte * (APIENTRY * GetString_t)(GLenum name);
        GetString_t GetString = reinterpret_cast<GetString_t>(::GetProcAddress(lib, "glGetString"));
        if (GetString) {
            if (const char *versionStr = reinterpret_cast<const char *>(GetString(GL_VERSION))) {
                const QByteArray version(versionStr);
                const int majorDot = version.indexOf('.');
                if (majorDot != -1) {
                    int minorDot = version.indexOf('.', majorDot + 1);
                    if (minorDot == -1)
                        minorDot = version.size();
                    const int major = version.mid(0, majorDot).toInt();
                    const int minor = version.mid(majorDot + 1, minorDot - majorDot - 1).toInt();
                    qCDebug(lcQpaGl, "Basic wglCreateContext gives version %d.%d", major, minor);
                    // Try to be as lenient as possible. Missing version, bogus values and
                    // such are all accepted. The driver may still be functional. Only
                    // check for known-bad cases, like versions "1.4.0 ...".
                    if (major == 1) {
                        result = false;
                        qCDebug(lcQpaGl, "OpenGL version too low");
                    }
                }
            }
        } else {
            result = false;
            qCDebug(lcQpaGl, "OpenGL 1.x entry points not found");
        }

        // Check for a shader-specific function.
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
#else
    return false;
#endif // !QT_NO_OPENGL && !Q_OS_WINCE
}

QT_END_NAMESPACE
