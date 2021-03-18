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

#include <QtCore/qvariant.h>
#include <QtCore/qdebug.h>
#include <QtCore/qtextstream.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qfile.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qstandardpaths.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qhash.h>

#ifndef QT_NO_OPENGL
#include <private/qopengl_p.h>
#endif

#include <QtCore/qt_windows.h>
#include <private/qsystemlibrary_p.h>
#include <d3d9.h>

QT_BEGIN_NAMESPACE

static const DWORD VENDOR_ID_AMD = 0x1002;

static GpuDescription adapterIdentifierToGpuDescription(const D3DADAPTER_IDENTIFIER9 &adapterIdentifier)
{
    GpuDescription result;
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
    return result;
}

class QDirect3D9Handle
{
public:
    Q_DISABLE_COPY_MOVE(QDirect3D9Handle)

    QDirect3D9Handle();
    ~QDirect3D9Handle();

    bool isValid() const { return m_direct3D9 != nullptr; }

    UINT adapterCount() const { return m_direct3D9 ? m_direct3D9->GetAdapterCount() : 0u; }
    bool retrieveAdapterIdentifier(UINT n, D3DADAPTER_IDENTIFIER9 *adapterIdentifier) const;

private:
    QSystemLibrary m_d3d9lib;
    IDirect3D9 *m_direct3D9 = nullptr;
};

QDirect3D9Handle::QDirect3D9Handle() :
    m_d3d9lib(QStringLiteral("d3d9"))
{
    using PtrDirect3DCreate9 = IDirect3D9 *(WINAPI *)(UINT);

    if (m_d3d9lib.load()) {
        if (auto direct3DCreate9 = (PtrDirect3DCreate9)m_d3d9lib.resolve("Direct3DCreate9"))
            m_direct3D9 = direct3DCreate9(D3D_SDK_VERSION);
    }
}

QDirect3D9Handle::~QDirect3D9Handle()
{
    if (m_direct3D9)
       m_direct3D9->Release();
}

bool QDirect3D9Handle::retrieveAdapterIdentifier(UINT n, D3DADAPTER_IDENTIFIER9 *adapterIdentifier) const
{
    return m_direct3D9
        && SUCCEEDED(m_direct3D9->GetAdapterIdentifier(n, 0, adapterIdentifier));
}

GpuDescription GpuDescription::detect()
{
    GpuDescription result;
    QDirect3D9Handle direct3D9;
    if (!direct3D9.isValid())
        return result;

    D3DADAPTER_IDENTIFIER9 adapterIdentifier;
    bool isAMD = false;
    // Adapter "0" is D3DADAPTER_DEFAULT which returns the default adapter. In
    // multi-GPU, multi-screen setups this is the GPU that is associated with
    // the "main display" in the Display Settings, and this is the GPU OpenGL
    // and D3D uses by default. Therefore querying any additional adapters is
    // futile and not useful for our purposes in general, except for
    // identifying a few special cases later on.
    if (direct3D9.retrieveAdapterIdentifier(0, &adapterIdentifier)) {
        result = adapterIdentifierToGpuDescription(adapterIdentifier);
        isAMD = result.vendorId == VENDOR_ID_AMD;
    }

    // Detect QTBUG-50371 (having AMD as the default adapter results in a crash
    // when starting apps on a screen connected to the Intel card) by looking
    // for a default AMD adapter and an additional non-AMD one.
    if (isAMD) {
        const UINT adapterCount = direct3D9.adapterCount();
        for (UINT adp = 1; adp < adapterCount; ++adp) {
            if (direct3D9.retrieveAdapterIdentifier(adp, &adapterIdentifier)
                && adapterIdentifier.VendorId != VENDOR_ID_AMD) {
                // Bingo. Now figure out the display for the AMD card.
                DISPLAY_DEVICE dd;
                memset(&dd, 0, sizeof(dd));
                dd.cb = sizeof(dd);
                for (int dev = 0; EnumDisplayDevices(nullptr, dev, &dd, 0); ++dev) {
                    if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) {
                        // DeviceName is something like \\.\DISPLAY1 which can be used to
                        // match with the MONITORINFOEX::szDevice queried by QWindowsScreen.
                        result.gpuSuitableScreen = QString::fromWCharArray(dd.DeviceName);
                        break;
                    }
                }
                break;
            }
        }
    }

    return result;
}

QVector<GpuDescription> GpuDescription::detectAll()
{
    QVector<GpuDescription> result;
    QDirect3D9Handle direct3D9;
    if (const UINT adapterCount = direct3D9.adapterCount()) {
        for (UINT adp = 0; adp < adapterCount; ++adp) {
            D3DADAPTER_IDENTIFIER9 adapterIdentifier;
            if (direct3D9.retrieveAdapterIdentifier(adp, &adapterIdentifier))
                result.append(adapterIdentifierToGpuDescription(adapterIdentifier));
        }
    }
    return result;
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug d, const GpuDescription &gd)
{
    QDebugStateSaver s(d);
    d.nospace();
    d << Qt::hex << Qt::showbase << "GpuDescription(vendorId=" << gd.vendorId
      << ", deviceId=" << gd.deviceId << ", subSysId=" << gd.subSysId
      << Qt::dec << Qt::noshowbase << ", revision=" << gd.revision
      << ", driver: " << gd.driverName
      << ", version=" << gd.driverVersion << ", " << gd.description
      << gd.gpuSuitableScreen << ')';
    return d;
}
#endif // !QT_NO_DEBUG_STREAM

// Return printable string formatted like the output of the dxdiag tool.
QString GpuDescription::toString() const
{
    QString result;
    QTextStream str(&result);
    str <<   "         Card name         : " << description
        << "\n       Driver Name         : " << driverName
        << "\n    Driver Version         : " << driverVersion.toString()
        << "\n         Vendor ID         : 0x" << qSetPadChar(u'0')
        << Qt::uppercasedigits << Qt::hex << qSetFieldWidth(4) << vendorId
        << "\n         Device ID         : 0x" << qSetFieldWidth(4) << deviceId
        << "\n         SubSys ID         : 0x" << qSetFieldWidth(8) << subSysId
        << "\n       Revision ID         : 0x" << qSetFieldWidth(4) << revision
        << Qt::dec;
    if (!gpuSuitableScreen.isEmpty())
        str << "\nGL windows forced to screen: " << gpuSuitableScreen;
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
    return QWindowsOpenGLTester::InvalidRenderer;
}

QWindowsOpenGLTester::Renderer QWindowsOpenGLTester::requestedRenderer()
{
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
    return QWindowsOpenGLTester::InvalidRenderer;
}

static inline QString resolveBugListFile(const QString &fileName)
{
    if (QFileInfo(fileName).isAbsolute())
        return fileName;
    // Try QLibraryInfo::SettingsPath which is typically empty unless specified in qt.conf,
    // then resolve via QStandardPaths::ConfigLocation.
    const QString settingsPath = QLibraryInfo::location(QLibraryInfo::SettingsPath);
    if (!settingsPath.isEmpty()) { // SettingsPath is empty unless specified in qt.conf.
        const QFileInfo fi(settingsPath + u'/' + fileName);
        if (fi.isFile())
            return fi.absoluteFilePath();
    }
    return QStandardPaths::locate(QStandardPaths::ConfigLocation, fileName);
}

#ifndef QT_NO_OPENGL
typedef QHash<QOpenGLConfig::Gpu, QWindowsOpenGLTester::Renderers> SupportedRenderersCache;
Q_GLOBAL_STATIC(SupportedRenderersCache, supportedRenderersCache)
#endif

QWindowsOpenGLTester::Renderers QWindowsOpenGLTester::detectSupportedRenderers(const GpuDescription &gpu,
                                                                               Renderer requested)
{
#if defined(QT_NO_OPENGL)
    Q_UNUSED(gpu)
    Q_UNUSED(requested)
    return 0;
#else
    QOpenGLConfig::Gpu qgpu = QOpenGLConfig::Gpu::fromDevice(gpu.vendorId, gpu.deviceId, gpu.driverVersion, gpu.description);
    SupportedRenderersCache *srCache = supportedRenderersCache();
    SupportedRenderersCache::const_iterator it = srCache->constFind(qgpu);
    if (it != srCache->cend())
        return *it;

    QWindowsOpenGLTester::Renderers result(QWindowsOpenGLTester::AngleRendererD3d11
        | QWindowsOpenGLTester::AngleRendererD3d9
        | QWindowsOpenGLTester::AngleRendererD3d11Warp
        | QWindowsOpenGLTester::SoftwareRasterizer);

    // Don't test for GL if explicitly requested or GLES only is requested
    if (requested == DesktopGl
        || ((requested & GlesMask) == 0 && testDesktopGL())) {
            result |= QWindowsOpenGLTester::DesktopGl;
    }

    QSet<QString> features; // empty by default -> nothing gets disabled
    if (!qEnvironmentVariableIsSet("QT_NO_OPENGL_BUGLIST")) {
        const char bugListFileVar[] = "QT_OPENGL_BUGLIST";
        QString buglistFileName = QStringLiteral(":/qt-project.org/windows/openglblacklists/default.json");
        if (qEnvironmentVariableIsSet(bugListFileVar)) {
            const QString fileName = resolveBugListFile(QFile::decodeName(qgetenv(bugListFileVar)));
            if (!fileName.isEmpty())
                buglistFileName = fileName;
        }
        features = QOpenGLConfig::gpuFeatures(qgpu, buglistFileName);
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
    if (features.contains(QStringLiteral("disable_program_cache"))) {
        qCDebug(lcQpaGl) << "Disabling program cache: " << gpu;
        result |= DisableProgramCacheFlag;
    }
    srCache->insert(qgpu, result);
    return result;
#endif // !QT_NO_OPENGL
}

QWindowsOpenGLTester::Renderers QWindowsOpenGLTester::supportedRenderers(Renderer requested)
{
    const GpuDescription gpu = GpuDescription::detect();
    const QWindowsOpenGLTester::Renderers result = detectSupportedRenderers(gpu, requested);
    qCDebug(lcQpaGl) << __FUNCTION__ << gpu << requested << "renderer: " << result;
    return result;
}

bool QWindowsOpenGLTester::testDesktopGL()
{
#if !defined(QT_NO_OPENGL)
    typedef HGLRC (WINAPI *CreateContextType)(HDC);
    typedef BOOL (WINAPI *DeleteContextType)(HGLRC);
    typedef BOOL (WINAPI *MakeCurrentType)(HDC, HGLRC);
    typedef PROC (WINAPI *WglGetProcAddressType)(LPCSTR);

    HMODULE lib = nullptr;
    HWND wnd = nullptr;
    HDC dc = nullptr;
    HGLRC context = nullptr;
    LPCTSTR className = L"qtopengltest";

    CreateContextType CreateContext = nullptr;
    DeleteContextType DeleteContext = nullptr;
    MakeCurrentType MakeCurrent = nullptr;
    WglGetProcAddressType WGL_GetProcAddress = nullptr;

    bool result = false;

    // Test #1: Load opengl32.dll and try to resolve an OpenGL 2 function.
    // This will typically fail on systems that do not have a real OpenGL driver.
    lib = LoadLibraryA("opengl32.dll");
    if (lib) {
        CreateContext = reinterpret_cast<CreateContextType>(
            reinterpret_cast<QFunctionPointer>(::GetProcAddress(lib, "wglCreateContext")));
        if (!CreateContext)
            goto cleanup;
        DeleteContext = reinterpret_cast<DeleteContextType>(
            reinterpret_cast<QFunctionPointer>(::GetProcAddress(lib, "wglDeleteContext")));
        if (!DeleteContext)
            goto cleanup;
        MakeCurrent = reinterpret_cast<MakeCurrentType>(
            reinterpret_cast<QFunctionPointer>(::GetProcAddress(lib, "wglMakeCurrent")));
        if (!MakeCurrent)
            goto cleanup;
        WGL_GetProcAddress = reinterpret_cast<WglGetProcAddressType>(
            reinterpret_cast<QFunctionPointer>(::GetProcAddress(lib, "wglGetProcAddress")));
        if (!WGL_GetProcAddress)
            goto cleanup;

        WNDCLASS wclass;
        wclass.cbClsExtra = 0;
        wclass.cbWndExtra = 0;
        wclass.hInstance = static_cast<HINSTANCE>(GetModuleHandle(nullptr));
        wclass.hIcon = nullptr;
        wclass.hCursor = nullptr;
        wclass.hbrBackground = HBRUSH(COLOR_BACKGROUND);
        wclass.lpszMenuName = nullptr;
        wclass.lpfnWndProc = DefWindowProc;
        wclass.lpszClassName = className;
        wclass.style = CS_OWNDC;
        if (!RegisterClass(&wclass))
            goto cleanup;
        wnd = CreateWindow(className, L"qtopenglproxytest", WS_OVERLAPPED,
                           0, 0, 640, 480, nullptr, nullptr, wclass.hInstance, nullptr);
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
        auto GetString = reinterpret_cast<GetString_t>(
            reinterpret_cast<QFunctionPointer>(::GetProcAddress(lib, "glGetString")));
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
        MakeCurrent(nullptr, nullptr);
    if (context)
        DeleteContext(context);
    if (dc && wnd)
        ReleaseDC(wnd, dc);
    if (wnd) {
        DestroyWindow(wnd);
        UnregisterClass(className, GetModuleHandle(nullptr));
    }
    // No FreeLibrary. Some implementations, Mesa in particular, deadlock when trying to unload.

    return result;
#else
    return false;
#endif // !QT_NO_OPENGL
}

QT_END_NAMESPACE
