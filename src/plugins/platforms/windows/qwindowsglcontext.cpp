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

#include "qwindowsglcontext.h"
#include "qwindowscontext.h"
#include "qwindowswindow.h"
#include "qwindowsintegration.h"

#include <QtCore/qdebug.h>
#include <QtCore/qsysinfo.h>
#include <QtGui/qguiapplication.h>
#include <qpa/qplatformnativeinterface.h>
#include <QtPlatformHeaders/qwglnativecontext.h>

#include <algorithm>

#include <wingdi.h>
#include <GL/gl.h>

// #define DEBUG_GL

// ARB extension API
#ifndef WGL_ARB_multisample
#define WGL_SAMPLE_BUFFERS_ARB               0x2041
#define WGL_SAMPLES_ARB                      0x2042
#endif

#ifndef WGL_ARB_pixel_format
#define WGL_NUMBER_PIXEL_FORMATS_ARB   0x2000
#define WGL_DRAW_TO_WINDOW_ARB         0x2001
#define WGL_DRAW_TO_BITMAP_ARB         0x2002
#define WGL_ACCELERATION_ARB           0x2003
#define WGL_NEED_PALETTE_ARB           0x2004
#define WGL_NEED_SYSTEM_PALETTE_ARB    0x2005
#define WGL_SWAP_LAYER_BUFFERS_ARB     0x2006
#define WGL_SWAP_METHOD_ARB            0x2007
#define WGL_NUMBER_OVERLAYS_ARB        0x2008
#define WGL_NUMBER_UNDERLAYS_ARB       0x2009
#define WGL_TRANSPARENT_ARB            0x200A
#define WGL_TRANSPARENT_RED_VALUE_ARB  0x2037
#define WGL_TRANSPARENT_GREEN_VALUE_ARB 0x2038
#define WGL_TRANSPARENT_BLUE_VALUE_ARB 0x2039
#define WGL_TRANSPARENT_ALPHA_VALUE_ARB 0x203A
#define WGL_TRANSPARENT_INDEX_VALUE_ARB 0x203B
#define WGL_SHARE_DEPTH_ARB            0x200C
#define WGL_SHARE_STENCIL_ARB          0x200D
#define WGL_SHARE_ACCUM_ARB            0x200E
#define WGL_SUPPORT_GDI_ARB            0x200F
#define WGL_SUPPORT_OPENGL_ARB         0x2010
#define WGL_DOUBLE_BUFFER_ARB          0x2011
#define WGL_STEREO_ARB                 0x2012
#define WGL_PIXEL_TYPE_ARB             0x2013
#define WGL_COLOR_BITS_ARB             0x2014
#define WGL_RED_BITS_ARB               0x2015
#define WGL_RED_SHIFT_ARB              0x2016
#define WGL_GREEN_BITS_ARB             0x2017
#define WGL_GREEN_SHIFT_ARB            0x2018
#define WGL_BLUE_BITS_ARB              0x2019
#define WGL_BLUE_SHIFT_ARB             0x201A
#define WGL_ALPHA_BITS_ARB             0x201B
#define WGL_ALPHA_SHIFT_ARB            0x201C
#define WGL_ACCUM_BITS_ARB             0x201D
#define WGL_ACCUM_RED_BITS_ARB         0x201E
#define WGL_ACCUM_GREEN_BITS_ARB       0x201F
#define WGL_ACCUM_BLUE_BITS_ARB        0x2020
#define WGL_ACCUM_ALPHA_BITS_ARB       0x2021
#define WGL_DEPTH_BITS_ARB             0x2022
#define WGL_STENCIL_BITS_ARB           0x2023
#define WGL_AUX_BUFFERS_ARB            0x2024
#define WGL_NO_ACCELERATION_ARB        0x2025
#define WGL_GENERIC_ACCELERATION_ARB   0x2026
#define WGL_FULL_ACCELERATION_ARB      0x2027
#define WGL_SWAP_EXCHANGE_ARB          0x2028
#define WGL_SWAP_COPY_ARB              0x2029
#define WGL_SWAP_UNDEFINED_ARB         0x202A
#define WGL_TYPE_RGBA_ARB              0x202B
#define WGL_TYPE_COLORINDEX_ARB        0x202C
#endif

#ifndef WGL_ARB_create_context
#define WGL_CONTEXT_MAJOR_VERSION_ARB               0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB               0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB                 0x2093
#define WGL_CONTEXT_FLAGS_ARB                       0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB                0x9126
#define WGL_CONTEXT_DEBUG_BIT_ARB                   0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB      0x0002
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB            0x0001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB   0x0002
// Error codes returned by GetLastError().
#define ERROR_INVALID_VERSION_ARB                   0x2095
#define ERROR_INVALID_PROFILE_ARB                   0x2096
#endif

#ifndef GL_VERSION_3_2
#define GL_CONTEXT_PROFILE_MASK                     0x9126
#define GL_MAJOR_VERSION                            0x821B
#define GL_MINOR_VERSION                            0x821C
#define GL_NUM_EXTENSIONS                           0x821D
#define GL_CONTEXT_FLAGS                            0x821E
#define GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT      0x0001
#endif

#ifndef GL_CONTEXT_FLAG_DEBUG_BIT
#define GL_CONTEXT_FLAG_DEBUG_BIT 0x00000002
#endif

// Common GL and WGL constants
#define RESET_NOTIFICATION_STRATEGY_ARB 0x8256
#define LOSE_CONTEXT_ON_RESET_ARB 0x8252

#ifndef WGL_FRAMEBUFFER_SRGB_CAPABLE_EXT
#define WGL_FRAMEBUFFER_SRGB_CAPABLE_EXT 0x20A9
#endif

QT_BEGIN_NAMESPACE

QWindowsOpengl32DLL QOpenGLStaticContext::opengl32;

QFunctionPointer QWindowsOpengl32DLL::resolve(const char *name)
{
    return m_lib
        ? reinterpret_cast<QFunctionPointer>(::GetProcAddress(m_lib, name))
        : nullptr;
}

bool QWindowsOpengl32DLL::init(bool softwareRendering)
{
    const QByteArray opengl32 = QByteArrayLiteral("opengl32.dll");
    const QByteArray swopengl = QByteArrayLiteral("opengl32sw.dll");

    QByteArray openglDll = qgetenv("QT_OPENGL_DLL");
    if (openglDll.isEmpty())
        openglDll = softwareRendering ? swopengl : opengl32;

    openglDll = openglDll.toLower();
    m_nonOpengl32 = openglDll != opengl32;

    qCDebug(lcQpaGl) << "Qt: Using WGL and OpenGL from" << openglDll;

    m_lib = ::LoadLibraryA(openglDll.constData());
    if (!m_lib) {
        qErrnoWarning(::GetLastError(), "Failed to load %s", openglDll.constData());
        return false;
    }

    if (moduleIsNotOpengl32()) {
        // Load opengl32.dll always. GDI functions like ChoosePixelFormat do
        // GetModuleHandle for opengl32.dll and behave differently (and call back into
        // opengl32) when the module is present. This is fine for dummy contexts and windows.
        ::LoadLibraryA("opengl32.dll");
    }

    wglCreateContext = reinterpret_cast<HGLRC (WINAPI *)(HDC)>(resolve("wglCreateContext"));
    wglDeleteContext = reinterpret_cast<BOOL (WINAPI *)(HGLRC)>(resolve("wglDeleteContext"));
    wglGetCurrentContext = reinterpret_cast<HGLRC (WINAPI *)()>(resolve("wglGetCurrentContext"));
    wglGetCurrentDC = reinterpret_cast<HDC (WINAPI *)()>(resolve("wglGetCurrentDC"));
    wglGetProcAddress = reinterpret_cast<PROC (WINAPI *)(LPCSTR)>(resolve("wglGetProcAddress"));
    wglMakeCurrent = reinterpret_cast<BOOL (WINAPI *)(HDC, HGLRC)>(resolve("wglMakeCurrent"));
    wglShareLists = reinterpret_cast<BOOL (WINAPI *)(HGLRC, HGLRC)>(resolve("wglShareLists"));
    wglSwapBuffers = reinterpret_cast<BOOL (WINAPI *)(HDC)>(resolve("wglSwapBuffers"));
    wglSetPixelFormat = reinterpret_cast<BOOL (WINAPI *)(HDC, int, const PIXELFORMATDESCRIPTOR *)>(resolve("wglSetPixelFormat"));
    wglDescribePixelFormat = reinterpret_cast<int (WINAPI *)(HDC, int, UINT, PIXELFORMATDESCRIPTOR *)>(resolve("wglDescribePixelFormat"));

    glGetError = reinterpret_cast<GLenum (APIENTRY *)()>(resolve("glGetError"));
    glGetIntegerv = reinterpret_cast<void (APIENTRY *)(GLenum , GLint *)>(resolve("glGetIntegerv"));
    glGetString = reinterpret_cast<const GLubyte * (APIENTRY *)(GLenum )>(resolve("glGetString"));

    return wglCreateContext && glGetError && glGetString;
}

BOOL QWindowsOpengl32DLL::swapBuffers(HDC dc)
{
    return moduleIsNotOpengl32() ? wglSwapBuffers(dc) : SwapBuffers(dc);
}

BOOL QWindowsOpengl32DLL::setPixelFormat(HDC dc, int pf, const PIXELFORMATDESCRIPTOR *pfd)
{
    return moduleIsNotOpengl32() ? wglSetPixelFormat(dc, pf, pfd) : SetPixelFormat(dc, pf, pfd);
}

int QWindowsOpengl32DLL::describePixelFormat(HDC dc, int pf, UINT size, PIXELFORMATDESCRIPTOR *pfd)
{
    return moduleIsNotOpengl32() ? wglDescribePixelFormat(dc, pf, size, pfd) : DescribePixelFormat(dc, pf, size, pfd);
}

QWindowsOpenGLContext *QOpenGLStaticContext::createContext(QOpenGLContext *context)
{
    return new QWindowsGLContext(this, context);
}

template <class MaskType, class FlagType> inline bool testFlag(MaskType mask, FlagType flag)
{
    return (mask & MaskType(flag)) != 0;
}

static inline bool hasGLOverlay(const PIXELFORMATDESCRIPTOR &pd)
{ return (pd.bReserved & 0x0f) != 0; }

static inline bool isDirectRendering(const PIXELFORMATDESCRIPTOR &pfd)
{ return (pfd.dwFlags & PFD_GENERIC_ACCELERATED) || !(pfd.dwFlags & PFD_GENERIC_FORMAT); }

static inline void initPixelFormatDescriptor(PIXELFORMATDESCRIPTOR *d)
{
    memset(d, 0, sizeof(PIXELFORMATDESCRIPTOR));
    d->nSize = sizeof(PIXELFORMATDESCRIPTOR);
    d->nVersion = 1;
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug d, const PIXELFORMATDESCRIPTOR &pd)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d << "PIXELFORMATDESCRIPTOR "
        << "dwFlags=" << Qt::hex << Qt::showbase << pd.dwFlags << Qt::dec << Qt::noshowbase;
    if (pd.dwFlags & PFD_DRAW_TO_WINDOW) d << " PFD_DRAW_TO_WINDOW";
    if (pd.dwFlags & PFD_DRAW_TO_BITMAP) d << " PFD_DRAW_TO_BITMAP";
    if (pd.dwFlags & PFD_SUPPORT_GDI) d << " PFD_SUPPORT_GDI";
    if (pd.dwFlags & PFD_SUPPORT_OPENGL) d << " PFD_SUPPORT_OPENGL";
    if (pd.dwFlags & PFD_GENERIC_ACCELERATED) d << " PFD_GENERIC_ACCELERATED";
    if (pd.dwFlags & PFD_SUPPORT_DIRECTDRAW) d << " PFD_SUPPORT_DIRECTDRAW";
    if (pd.dwFlags & PFD_DIRECT3D_ACCELERATED) d << " PFD_DIRECT3D_ACCELERATED";
    if (pd.dwFlags & PFD_SUPPORT_COMPOSITION) d << " PFD_SUPPORT_COMPOSITION";
    if (pd.dwFlags & PFD_GENERIC_FORMAT) d << " PFD_GENERIC_FORMAT";
    if (pd.dwFlags & PFD_NEED_PALETTE) d << " PFD_NEED_PALETTE";
    if (pd.dwFlags & PFD_NEED_SYSTEM_PALETTE) d << " PFD_NEED_SYSTEM_PALETTE";
    if (pd.dwFlags & PFD_DOUBLEBUFFER) d << " PFD_DOUBLEBUFFER";
    if (pd.dwFlags & PFD_STEREO) d << " PFD_STEREO";
    if (pd.dwFlags & PFD_SWAP_LAYER_BUFFERS) d << " PFD_SWAP_LAYER_BUFFERS";
    if (hasGLOverlay(pd)) d << " overlay";
    d << " iPixelType=" << pd.iPixelType << " cColorBits=" << pd.cColorBits
        << " cRedBits=" << pd.cRedBits << " cRedShift=" << pd.cRedShift
        << " cGreenBits=" << pd.cGreenBits << " cGreenShift=" << pd.cGreenShift
        << " cBlueBits=" << pd.cBlueBits << " cBlueShift=" << pd.cBlueShift;
    d  << " cDepthBits=" << pd.cDepthBits;
    if (pd.cStencilBits)
        d << " cStencilBits=" << pd.cStencilBits;
    if (pd.cAuxBuffers)
        d << " cAuxBuffers=" << pd.cAuxBuffers;
    d << " iLayerType=" << pd.iLayerType;
    if (pd.dwVisibleMask)
        d << " dwVisibleMask=" << pd.dwVisibleMask;
    if (pd.cAlphaBits)
        d << " cAlphaBits=" << pd.cAlphaBits << " cAlphaShift=" << pd.cAlphaShift;
    if (pd.cAccumBits)
        d << " cAccumBits=" << pd.cAccumBits << " cAccumRedBits=" << pd.cAccumRedBits
        << " cAccumGreenBits=" << pd.cAccumGreenBits << " cAccumBlueBits=" << pd.cAccumBlueBits
        << " cAccumAlphaBits=" << pd.cAccumAlphaBits;
    return d;
}

QDebug operator<<(QDebug d, const QOpenGLStaticContext &s)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d << "OpenGL: " << s.vendor << ',' << s.renderer << " default "
        <<  s.defaultFormat;
    if (s.extensions &  QOpenGLStaticContext::SampleBuffers)
        d << ",SampleBuffers";
    if (s.hasExtensions())
        d << ", Extension-API present";
    d << "\nExtensions: " << (s.extensionNames.count(' ') + 1);
    if (QWindowsContext::verbose > 1)
        d << s.extensionNames;
    return d;
}

QDebug operator<<(QDebug d, const QWindowsOpenGLContextFormat &f)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d << "ContextFormat: v" << (f.version >> 8) << '.' << (f.version & 0xFF)
        << " profile: " << f.profile << " options: " << f.options;
    return d;
}
#endif // !QT_NO_DEBUG_STREAM

// Check whether an obtained PIXELFORMATDESCRIPTOR matches the request.
static inline bool
    isAcceptableFormat(const QWindowsOpenGLAdditionalFormat &additional,
                       const PIXELFORMATDESCRIPTOR &pfd,
                       bool ignoreGLSupport = false) // ARB format may not contain it.
{
    const bool pixmapRequested = testFlag(additional.formatFlags, QWindowsGLRenderToPixmap);
    const bool pixmapOk = !pixmapRequested || testFlag(pfd.dwFlags, PFD_DRAW_TO_BITMAP);
    const bool colorOk =  !pixmapRequested || pfd.cColorBits == additional.pixmapDepth;
    const bool glOk = ignoreGLSupport || testFlag(pfd.dwFlags, PFD_SUPPORT_OPENGL);
    const bool overlayOk = hasGLOverlay(pfd) == testFlag(additional.formatFlags, QWindowsGLOverlay);
    return pixmapOk && glOk && overlayOk && colorOk;
}

static void describeFormats(HDC hdc)
{
    const int pfiMax = QOpenGLStaticContext::opengl32.describePixelFormat(hdc, 0, 0, nullptr);
    for (int i = 0; i < pfiMax; i++) {
        PIXELFORMATDESCRIPTOR pfd;
        initPixelFormatDescriptor(&pfd);
        QOpenGLStaticContext::opengl32.describePixelFormat(hdc, i, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
        qCDebug(lcQpaGl) << '#' << i << '/' << pfiMax << ':' << pfd;
    }
}

// Classic GDI API
namespace GDI {
static QSurfaceFormat
    qSurfaceFormatFromPixelFormat(const PIXELFORMATDESCRIPTOR &pfd,
                                         QWindowsOpenGLAdditionalFormat *additionalIn = nullptr)
{
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    if (pfd.dwFlags & PFD_DOUBLEBUFFER)
        format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    format.setDepthBufferSize(pfd.cDepthBits);

    if (pfd.iPixelType == PFD_TYPE_RGBA)
        format.setAlphaBufferSize(pfd.cAlphaBits);
    format.setRedBufferSize(pfd.cRedBits);
    format.setGreenBufferSize(pfd.cGreenBits);
    format.setBlueBufferSize(pfd.cBlueBits);
    format.setStencilBufferSize(pfd.cStencilBits);
    format.setStereo(pfd.dwFlags & PFD_STEREO);
    if (additionalIn) {
        QWindowsOpenGLAdditionalFormat additional;
        if (isDirectRendering(pfd))
            additional.formatFlags |= QWindowsGLDirectRendering;
        if (hasGLOverlay(pfd))
            additional.formatFlags |= QWindowsGLOverlay;
        if (pfd.cAccumRedBits)
            additional.formatFlags |= QWindowsGLAccumBuffer;
        if (testFlag(pfd.dwFlags, PFD_DRAW_TO_BITMAP)) {
            additional.formatFlags |= QWindowsGLRenderToPixmap;
            additional.pixmapDepth = pfd.cColorBits;
        }
        *additionalIn = additional;
    }
    return format;
}

static PIXELFORMATDESCRIPTOR
    qPixelFormatFromSurfaceFormat(const QSurfaceFormat &format,
                                  const QWindowsOpenGLAdditionalFormat &additional)
{
    PIXELFORMATDESCRIPTOR pfd;
    initPixelFormatDescriptor(&pfd);
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.iLayerType  = PFD_MAIN_PLANE;
    pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_SUPPORT_COMPOSITION;
    const bool isPixmap = (additional.formatFlags & QWindowsGLRenderToPixmap) != 0;
    pfd.dwFlags |= isPixmap ? PFD_DRAW_TO_BITMAP : PFD_DRAW_TO_WINDOW;
    if (!(additional.formatFlags & QWindowsGLDirectRendering))
        pfd.dwFlags |= PFD_GENERIC_FORMAT;

    if (format.stereo())
        pfd.dwFlags |= PFD_STEREO;
    if (format.swapBehavior() != QSurfaceFormat::SingleBuffer && !isPixmap)
        pfd.dwFlags |= PFD_DOUBLEBUFFER;
    pfd.cDepthBits =
        format.depthBufferSize() >= 0 ? format.depthBufferSize() : 32;
    const int redBufferSize = format.redBufferSize();
    if (redBufferSize != -1)
        pfd.cRedBits = BYTE(redBufferSize);
    const int greenBufferSize = format.greenBufferSize();
    if (greenBufferSize != -1)
        pfd.cGreenBits = BYTE(greenBufferSize);
    const int blueBufferSize = format.blueBufferSize();
    if (blueBufferSize != -1)
        pfd.cBlueBits = BYTE(blueBufferSize);
    pfd.cAlphaBits = format.alphaBufferSize() > 0 ? format.alphaBufferSize() : 8;
    pfd.cStencilBits = format.stencilBufferSize() > 0 ? format.stencilBufferSize() : 8;
    if (additional.formatFlags & QWindowsGLAccumBuffer)
        pfd.cAccumRedBits = pfd.cAccumGreenBits = pfd.cAccumBlueBits = pfd.cAccumAlphaBits = 16;
    return pfd;
}

// Choose a suitable pixelformat using GDI WinAPI in case ARB
// functions cannot be found. First tries to find a suitable
// format using GDI function ChoosePixelFormat(). Since that
// does not handle overlay and direct-rendering requests, manually loop
// over the available formats to find the best one.
// Note: As of Windows 7, it seems direct-rendering is handled, so,
// the code might be obsolete?
//
// NB! When using an implementation with a name different than opengl32.dll
// this code path should not be used since it will result in a mess due to GDI
// relying on and possibly calling back into functions in opengl32.dll (and not
// the one we are using). This is not a problem usually since for Mesa, which
// we are most likely to ship with a name other than opengl32.dll, the ARB code
// path should work. Hence the early bail out below.
//
static int choosePixelFormat(HDC hdc, const QSurfaceFormat &format,
                            const QWindowsOpenGLAdditionalFormat &additional,
                            PIXELFORMATDESCRIPTOR *obtainedPfd)
{
    if (QOpenGLStaticContext::opengl32.moduleIsNotOpengl32()) {
        qWarning("Attempted to use GDI functions with a non-opengl32.dll library");
        return 0;
    }

    // 1) Try ChoosePixelFormat().
    PIXELFORMATDESCRIPTOR requestedPfd = qPixelFormatFromSurfaceFormat(format, QWindowsGLDirectRendering);
    initPixelFormatDescriptor(obtainedPfd);
    int pixelFormat = ChoosePixelFormat(hdc, &requestedPfd);
    if (pixelFormat >= 0) {
        DescribePixelFormat(hdc, pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), obtainedPfd);
        if (isAcceptableFormat(additional, *obtainedPfd))
            return pixelFormat;
    }
    // 2) No matching format found, manual search loop.
    const int pfiMax = DescribePixelFormat(hdc, 0, 0, nullptr);
    int bestScore = -1;
    int bestPfi = -1;
    const bool stereoRequested = format.stereo();
    const bool accumBufferRequested = testFlag(additional.formatFlags, QWindowsGLAccumBuffer);
    const bool doubleBufferRequested = format.swapBehavior() == QSurfaceFormat::DoubleBuffer;
    const bool directRenderingRequested = testFlag(additional.formatFlags, QWindowsGLDirectRendering);
    for (int pfi = 1; pfi <= pfiMax; pfi++) {
        PIXELFORMATDESCRIPTOR checkPfd;
        initPixelFormatDescriptor(&checkPfd);
        DescribePixelFormat(hdc, pfi, sizeof(PIXELFORMATDESCRIPTOR), &checkPfd);
        if (isAcceptableFormat(additional, checkPfd)) {
            int score = checkPfd.cColorBits + checkPfd.cAlphaBits + checkPfd.cStencilBits;
            if (accumBufferRequested)
                score += checkPfd.cAccumBits;
            if (doubleBufferRequested == testFlag(checkPfd.dwFlags, PFD_DOUBLEBUFFER))
                score += 1000;
            if (stereoRequested == testFlag(checkPfd.dwFlags, PFD_STEREO))
                score += 2000;
            if (directRenderingRequested == isDirectRendering(checkPfd))
                score += 4000;
            if (checkPfd.iPixelType == PFD_TYPE_RGBA)
                score += 8000;
            if (score > bestScore) {
                bestScore = score;
                bestPfi = pfi;
                *obtainedPfd = checkPfd;
            }
            qCDebug(lcQpaGl) << __FUNCTION__ << "    checking  " << pfi << '/' << pfiMax
                << " score=" << score << " (best " << bestPfi << '/' << bestScore << ") " << checkPfd;
        }
    } // for
    if (bestPfi > 0)
        pixelFormat = bestPfi;
    return pixelFormat;
}

static inline HGLRC createContext(HDC hdc, HGLRC shared)
{
    HGLRC result = QOpenGLStaticContext::opengl32.wglCreateContext(hdc);
    if (!result) {
        qErrnoWarning("%s: wglCreateContext failed.", __FUNCTION__);
        return nullptr;
    }
    if (shared && !QOpenGLStaticContext::opengl32.wglShareLists(shared, result))
        qErrnoWarning("%s: wglShareLists() failed.", __FUNCTION__);
    return result;
}
} // namespace GDI

// ARB OpenGL extension API
namespace ARB {
// Choose a suitable pixelformat using ARB extension functions.
static int choosePixelFormat(HDC hdc,
                             const QOpenGLStaticContext &staticContext,
                             const QSurfaceFormat &format,
                             const QWindowsOpenGLAdditionalFormat &additional,
                             PIXELFORMATDESCRIPTOR *obtainedPfd)
{
    enum { attribSize = 42 };
    if ((additional.formatFlags & QWindowsGLRenderToPixmap) || !staticContext.hasExtensions())
        return 0;

    int iAttributes[attribSize];
    std::fill(iAttributes, iAttributes + attribSize, int(0));
    int i = 0;
    iAttributes[i++] = WGL_ACCELERATION_ARB;
    iAttributes[i++] = testFlag(additional.formatFlags, QWindowsGLDirectRendering) ?
                       WGL_FULL_ACCELERATION_ARB : WGL_NO_ACCELERATION_ARB;
    iAttributes[i++] = WGL_SUPPORT_OPENGL_ARB;
    iAttributes[i++] = TRUE;
    iAttributes[i++] = WGL_DRAW_TO_WINDOW_ARB;
    iAttributes[i++] = TRUE;
    iAttributes[i++] = WGL_COLOR_BITS_ARB;

    iAttributes[i++] = (format.redBufferSize() > 0)
                       && (format.greenBufferSize() > 0)
                       && (format.blueBufferSize() > 0) ?
                       format.redBufferSize() + format.greenBufferSize() + format.blueBufferSize() :
                       24;
    switch (format.swapBehavior()) {
    case QSurfaceFormat::SingleBuffer:
        iAttributes[i++] = WGL_DOUBLE_BUFFER_ARB;
        iAttributes[i++] = FALSE;
        break;
    case QSurfaceFormat::DefaultSwapBehavior:
    case QSurfaceFormat::DoubleBuffer:
    case QSurfaceFormat::TripleBuffer:
        iAttributes[i++] = WGL_DOUBLE_BUFFER_ARB;
        iAttributes[i++] = TRUE;
        break;
    }
    if (format.stereo()) {
        iAttributes[i++] = WGL_STEREO_ARB;
        iAttributes[i++] = TRUE;
    }
    if (format.depthBufferSize() >= 0) {
        iAttributes[i++] = WGL_DEPTH_BITS_ARB;
        iAttributes[i++] = format.depthBufferSize();
    }
    iAttributes[i++] = WGL_PIXEL_TYPE_ARB;
    iAttributes[i++] = WGL_TYPE_RGBA_ARB;
    if (format.redBufferSize() >= 0) {
        iAttributes[i++] = WGL_RED_BITS_ARB;
        iAttributes[i++] = format.redBufferSize();
    }
    if (format.greenBufferSize() >= 0) {
        iAttributes[i++] = WGL_GREEN_BITS_ARB;
        iAttributes[i++] = format.greenBufferSize();
    }
    if (format.blueBufferSize() >= 0) {
        iAttributes[i++] = WGL_BLUE_BITS_ARB;
        iAttributes[i++] = format.blueBufferSize();
    }
    iAttributes[i++] = WGL_ALPHA_BITS_ARB;
    iAttributes[i++] = format.alphaBufferSize() >= 0 ? format.alphaBufferSize() : 8;
    if (additional.formatFlags & QWindowsGLAccumBuffer) {
        iAttributes[i++] = WGL_ACCUM_BITS_ARB;
        iAttributes[i++] = 16;
    }
    iAttributes[i++] = WGL_STENCIL_BITS_ARB;
    iAttributes[i++] = 8;
    if (additional.formatFlags & QWindowsGLOverlay) {
        iAttributes[i++] = WGL_NUMBER_OVERLAYS_ARB;
        iAttributes[i++] = 1;
    }
    const int samples = format.samples();
    const bool sampleBuffersRequested = samples > 1
            && testFlag(staticContext.extensions, QOpenGLStaticContext::SampleBuffers);
    int samplesValuePosition = 0;
    if (sampleBuffersRequested) {
        iAttributes[i++] = WGL_SAMPLE_BUFFERS_ARB;
        iAttributes[i++] = TRUE;
        iAttributes[i++] = WGL_SAMPLES_ARB;
        samplesValuePosition = i;
        iAttributes[i++] = format.samples();
    } else {
        iAttributes[i++] = WGL_SAMPLE_BUFFERS_ARB;
        iAttributes[i++] = FALSE;
    }
    // must be the last
    bool srgbRequested = format.colorSpace() == QSurfaceFormat::sRGBColorSpace;
    int srgbValuePosition = 0;
    if (srgbRequested) {
        srgbValuePosition = i;
        iAttributes[i++] = WGL_FRAMEBUFFER_SRGB_CAPABLE_EXT;
        iAttributes[i++] = TRUE;
    }
    // If sample or sRGB request cannot be satisfied, reduce request.
    int pixelFormat = 0;
    uint numFormats = 0;
    while (true) {
        const bool valid =
            staticContext.wglChoosePixelFormatARB(hdc, iAttributes, nullptr, 1,
                                               &pixelFormat, &numFormats)
                && numFormats >= 1;
        if (valid || (!sampleBuffersRequested && !srgbRequested))
            break;
        if (srgbRequested) {
            iAttributes[srgbValuePosition] = 0;
            srgbRequested = false;
        } else if (sampleBuffersRequested) {
            if (iAttributes[samplesValuePosition] > 1) {
                iAttributes[samplesValuePosition] /= 2;
            } else if (iAttributes[samplesValuePosition] == 1) {
                // Fallback in case it is unable to initialize with any
                // samples to avoid falling back to the GDI path
                // NB: The sample attributes needs to be at the end for this
                // to work correctly
                iAttributes[samplesValuePosition - 1] = FALSE;
                iAttributes[samplesValuePosition] = 0;
                iAttributes[samplesValuePosition + 1] = 0;
            } else {
                break;
            }
        }
    }
    // Verify if format is acceptable. Note that the returned
    // formats have been observed to not contain PFD_SUPPORT_OPENGL, ignore.
    initPixelFormatDescriptor(obtainedPfd);
    QOpenGLStaticContext::opengl32.describePixelFormat(hdc, pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), obtainedPfd);
    if (!isAcceptableFormat(additional, *obtainedPfd, true)) {
        qCDebug(lcQpaGl) << __FUNCTION__ << " obtained px #" << pixelFormat
            << " not acceptable=" << *obtainedPfd;
        pixelFormat = 0;
    }

#ifndef QT_NO_DEBUG_OUTPUT
    if (lcQpaGl().isDebugEnabled()) {
        QString message;
        QDebug nsp(&message);
        nsp << __FUNCTION__;
        if (sampleBuffersRequested)
            nsp << " samples=" << iAttributes[samplesValuePosition];
        nsp << " Attributes: " << Qt::hex << Qt::showbase;
        for (int ii = 0; ii < i; ++ii)
            nsp << iAttributes[ii] << ',';
        nsp << Qt::noshowbase << Qt::dec << "\n    obtained px #" << pixelFormat
            << " of " << numFormats << "\n    " << *obtainedPfd;
        qCDebug(lcQpaGl) << message;
    } // Debug
#endif

    return pixelFormat;
}

static QSurfaceFormat
    qSurfaceFormatFromHDC(const QOpenGLStaticContext &staticContext,
                          HDC hdc, int pixelFormat,
                          QWindowsOpenGLAdditionalFormat *additionalIn = nullptr)
{
    enum { attribSize = 42 };

    QSurfaceFormat result;
    result.setRenderableType(QSurfaceFormat::OpenGL);
    if (!staticContext.hasExtensions())
        return result;
    int iAttributes[attribSize];
    int iValues[attribSize];
    std::fill(iAttributes, iAttributes + attribSize, int(0));
    std::fill(iValues, iValues + attribSize, int(0));

    int i = 0;
    const bool hasSampleBuffers = testFlag(staticContext.extensions, QOpenGLStaticContext::SampleBuffers);
    const bool hasSrgbSupport = testFlag(staticContext.extensions, QOpenGLStaticContext::sRGBCapableFramebuffer);

    iAttributes[i++] = WGL_DOUBLE_BUFFER_ARB; // 0
    iAttributes[i++] = WGL_DEPTH_BITS_ARB; // 1
    iAttributes[i++] = WGL_PIXEL_TYPE_ARB; // 2
    iAttributes[i++] = WGL_RED_BITS_ARB; // 3
    iAttributes[i++] = WGL_GREEN_BITS_ARB; // 4
    iAttributes[i++] = WGL_BLUE_BITS_ARB; // 5
    iAttributes[i++] = WGL_ALPHA_BITS_ARB; // 6
    iAttributes[i++] = WGL_ACCUM_BITS_ARB; // 7
    iAttributes[i++] = WGL_STENCIL_BITS_ARB; // 8
    iAttributes[i++] = WGL_STEREO_ARB; // 9
    iAttributes[i++] = WGL_ACCELERATION_ARB; // 10
    iAttributes[i++] = WGL_NUMBER_OVERLAYS_ARB; // 11
    if (hasSampleBuffers) {
        iAttributes[i++] = WGL_SAMPLE_BUFFERS_ARB; // 12
        iAttributes[i++] = WGL_SAMPLES_ARB; // 13
    }
    if (hasSrgbSupport)
        iAttributes[i++] = WGL_FRAMEBUFFER_SRGB_CAPABLE_EXT; // 12 or 14

    if (!staticContext.wglGetPixelFormatAttribIVARB(hdc, pixelFormat, 0, i,
                                        iAttributes, iValues)) {
        qErrnoWarning("%s: wglGetPixelFormatAttribIVARB() failed for basic parameters.", __FUNCTION__);
        return result;
    }
    result.setSwapBehavior(iValues[0] ? QSurfaceFormat::DoubleBuffer : QSurfaceFormat::SingleBuffer);
    result.setDepthBufferSize(iValues[1]);
    result.setRedBufferSize(iValues[3]);
    result.setGreenBufferSize(iValues[4]);
    result.setBlueBufferSize(iValues[5]);
    result.setAlphaBufferSize(iValues[6]);
    result.setStencilBufferSize(iValues[8]);
    if (iValues[9])
        result.setOption(QSurfaceFormat::StereoBuffers);

    if (hasSampleBuffers) {
        result.setSamples(iValues[13]);
        if (hasSrgbSupport && iValues[14])
            result.setColorSpace(QSurfaceFormat::sRGBColorSpace);
    } else {
        if (hasSrgbSupport && iValues[12])
            result.setColorSpace(QSurfaceFormat::sRGBColorSpace);
    }
    if (additionalIn) {
        if (iValues[7])
            additionalIn->formatFlags |= QWindowsGLAccumBuffer;
        if (iValues[10] == WGL_FULL_ACCELERATION_ARB)
            additionalIn->formatFlags |= QWindowsGLDirectRendering;
        if (iValues[11])
            additionalIn->formatFlags |= QWindowsGLOverlay;
    }
    return result;
}

static HGLRC createContext(const QOpenGLStaticContext &staticContext,
                           HDC hdc,
                           const QSurfaceFormat &format,
                           const QWindowsOpenGLAdditionalFormat &,
                           HGLRC shared = nullptr)
{
    enum { attribSize = 11 };

    if (!staticContext.hasExtensions())
        return nullptr;
    int attributes[attribSize];
    int attribIndex = 0;
    std::fill(attributes, attributes + attribSize, int(0));

    // We limit the requested version by the version of the static context as
    // wglCreateContextAttribsARB fails and returns NULL if the requested context
    // version is not supported. This means that we will get the closest supported
    // context format that that which was requested and is supported by the driver
    const int requestedVersion = qMin((format.majorVersion() << 8) + format.minorVersion(),
                                      staticContext.defaultFormat.version);
    const int majorVersion = requestedVersion >> 8;
    const int minorVersion = requestedVersion & 0xFF;

    if (requestedVersion > 0x0101) {
        attributes[attribIndex++] = WGL_CONTEXT_MAJOR_VERSION_ARB;
        attributes[attribIndex++] = majorVersion;
        attributes[attribIndex++] = WGL_CONTEXT_MINOR_VERSION_ARB;
        attributes[attribIndex++] = minorVersion;
    }

    int flags = 0;
    if (format.testOption(QSurfaceFormat::DebugContext))
        flags |= WGL_CONTEXT_DEBUG_BIT_ARB;
    if (requestedVersion >= 0x0300) {
        if (!format.testOption(QSurfaceFormat::DeprecatedFunctions))
            flags |= WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
    }
    attributes[attribIndex++] = WGL_CONTEXT_FLAGS_ARB;
    attributes[attribIndex++] = flags;

    if (requestedVersion >= 0x0302) {
        switch (format.profile()) {
        case QSurfaceFormat::NoProfile:
            break;
        case QSurfaceFormat::CoreProfile:
            attributes[attribIndex++] = WGL_CONTEXT_PROFILE_MASK_ARB;
            attributes[attribIndex++] = WGL_CONTEXT_CORE_PROFILE_BIT_ARB;
            break;
        case QSurfaceFormat::CompatibilityProfile:
            attributes[attribIndex++] = WGL_CONTEXT_PROFILE_MASK_ARB;
            attributes[attribIndex++] = WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;
            break;
        }
    }

    if (format.testOption(QSurfaceFormat::ResetNotification)) {
        attributes[attribIndex++] = RESET_NOTIFICATION_STRATEGY_ARB;
        attributes[attribIndex++] = LOSE_CONTEXT_ON_RESET_ARB;
    }

    qCDebug(lcQpaGl) << __FUNCTION__ << "Creating context version"
        << majorVersion << '.' << minorVersion <<  attribIndex / 2 << "attributes";

    const HGLRC result =
        staticContext.wglCreateContextAttribsARB(hdc, shared, attributes);
    if (!result) {
        QString message;
        QDebug(&message).nospace() << __FUNCTION__ << ": wglCreateContextAttribsARB() failed (GL error code: 0x"
            << Qt::hex << staticContext.opengl32.glGetError() << Qt::dec << ") for format: " << format << ", shared context: " << shared;
        qErrnoWarning("%s", qPrintable(message));
    }
    return result;
}

} // namespace ARB

// Helpers for temporary contexts
static inline HWND createDummyGLWindow()
{
    return QWindowsContext::instance()->
        createDummyWindow(QStringLiteral("OpenGLDummyWindow"),
                          L"OpenGLDummyWindow", nullptr, WS_OVERLAPPED | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
}

// Create a dummy GL context (see QOpenGLTemporaryContext).
static inline HGLRC createDummyGLContext(HDC dc)
{
    if (!dc)
        return nullptr;
    PIXELFORMATDESCRIPTOR pixelFormDescriptor;
    initPixelFormatDescriptor(&pixelFormDescriptor);
    pixelFormDescriptor.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_GENERIC_FORMAT;
    pixelFormDescriptor.iPixelType = PFD_TYPE_RGBA;
    // Use the GDI variant, for the dummy this is fine, even when using something other than opengl32.dll.
    const int pixelFormat = ChoosePixelFormat(dc, &pixelFormDescriptor);
    if (!pixelFormat) {
        qErrnoWarning("%s: ChoosePixelFormat failed.", __FUNCTION__);
        return nullptr;
    }
    if (!QOpenGLStaticContext::opengl32.setPixelFormat(dc, pixelFormat, &pixelFormDescriptor)) {
        qErrnoWarning("%s: SetPixelFormat failed.", __FUNCTION__);
        return nullptr;
    }
    HGLRC rc = QOpenGLStaticContext::opengl32.wglCreateContext(dc);
    if (!rc) {
        qErrnoWarning("%s: wglCreateContext failed.", __FUNCTION__);
        return nullptr;
    }
    return rc;
}

static inline QOpenGLContextData currentOpenGLContextData()
{
    QOpenGLContextData result;
    result.hdc = QOpenGLStaticContext::opengl32.wglGetCurrentDC();
    result.renderingContext = QOpenGLStaticContext::opengl32.wglGetCurrentContext();
    return result;
}

static inline QOpenGLContextData createDummyWindowOpenGLContextData()
{
    QOpenGLContextData result;
    result.hwnd = createDummyGLWindow();
    result.hdc = GetDC(result.hwnd);
    result.renderingContext = createDummyGLContext(result.hdc);
    return result;
}

/*!
    \class QOpenGLContextFormat
    \brief Format options that are related to the context (not pixelformats)

    Provides utility function to retrieve from currently active
    context and to apply to a QSurfaceFormat.

    \internal
*/

QWindowsOpenGLContextFormat QWindowsOpenGLContextFormat::current()
{
    QWindowsOpenGLContextFormat result;
    const QByteArray version = QOpenGLStaticContext::getGlString(GL_VERSION);
    int major, minor;
    if (QPlatformOpenGLContext::parseOpenGLVersion(version, major, minor))
        result.version = (major << 8) + minor;
    else
        result.version = 0x0200;
    result.profile = QSurfaceFormat::NoProfile;
    if (result.version < 0x0300) {
        result.options |= QSurfaceFormat::DeprecatedFunctions;
        return result;
    }
    // v3 onwards
    GLint value = 0;
    QOpenGLStaticContext::opengl32.glGetIntegerv(GL_CONTEXT_FLAGS, &value);
    if (!(value & GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT))
        result.options |= QSurfaceFormat::DeprecatedFunctions;
    if (value & GL_CONTEXT_FLAG_DEBUG_BIT)
        result.options |= QSurfaceFormat::DebugContext;
    if (result.version < 0x0302)
        return result;
    // v3.2 onwards: Profiles
    value = 0;
    QOpenGLStaticContext::opengl32.glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &value);
    if (value & GL_CONTEXT_CORE_PROFILE_BIT)
        result.profile = QSurfaceFormat::CoreProfile;
    else if (value & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT)
        result.profile = QSurfaceFormat::CompatibilityProfile;
    return result;
}

void QWindowsOpenGLContextFormat::apply(QSurfaceFormat *format) const
{
    format->setMajorVersion(version >> 8);
    format->setMinorVersion(version & 0xFF);
    format->setProfile(profile);
    if (options & QSurfaceFormat::DebugContext)
        format->setOption(QSurfaceFormat::DebugContext);
    if (options & QSurfaceFormat::DeprecatedFunctions)
        format->setOption(QSurfaceFormat::DeprecatedFunctions);
}

/*!
    \class QOpenGLTemporaryContext
    \brief A temporary context that can be instantiated on the stack.

    Functions like wglGetProcAddress() or glGetString() only work if there
    is a current GL context.

    \internal
*/

class QOpenGLTemporaryContext
{
    Q_DISABLE_COPY_MOVE(QOpenGLTemporaryContext)
public:
    QOpenGLTemporaryContext();
    ~QOpenGLTemporaryContext();

private:
    const QOpenGLContextData m_previous;
    const QOpenGLContextData m_current;
};

QOpenGLTemporaryContext::QOpenGLTemporaryContext() :
    m_previous(currentOpenGLContextData()),
    m_current(createDummyWindowOpenGLContextData())
{
    QOpenGLStaticContext::opengl32.wglMakeCurrent(m_current.hdc, m_current.renderingContext);
}

QOpenGLTemporaryContext::~QOpenGLTemporaryContext()
{
    QOpenGLStaticContext::opengl32.wglMakeCurrent(m_previous.hdc, m_previous.renderingContext);
    ReleaseDC(m_current.hwnd, m_current.hdc);
    DestroyWindow(m_current.hwnd);
    QOpenGLStaticContext::opengl32.wglDeleteContext(m_current.renderingContext);
}

/*!
    \class QWindowsOpenGLAdditionalFormat
    \brief Additional format information that is not in QSurfaceFormat
*/

/*!
    \class QOpenGLStaticContext
    \brief Static Open GL context containing version information, extension function pointers, etc.

    Functions pending integration in the next version of OpenGL are post-fixed ARB.

    No WGL or OpenGL functions are called directly from the windows plugin. Instead, the
    static context loads opengl32.dll and resolves the necessary functions. This allows
    building the plugin without linking to opengl32 and enables QT_OPENGL_DYNAMIC builds
    where both the EGL and WGL (this) based implementation of the context are built.

    \note Initialization requires an active context (see create()).

    \sa QWindowsGLContext
    \internal
*/

#define SAMPLE_BUFFER_EXTENSION "GL_ARB_multisample"
#define ROBUSTNESS_EXTENSION "GL_ARB_robustness"

QOpenGLStaticContext::QOpenGLStaticContext() :
    vendor(QOpenGLStaticContext::getGlString(GL_VENDOR)),
    renderer(QOpenGLStaticContext::getGlString(GL_RENDERER)),
    extensionNames(QOpenGLStaticContext::getGlString(GL_EXTENSIONS)),
    extensions(0),
    defaultFormat(QWindowsOpenGLContextFormat::current()),
    wglGetPixelFormatAttribIVARB(reinterpret_cast<WglGetPixelFormatAttribIVARB>(
        reinterpret_cast<QFunctionPointer>(QOpenGLStaticContext::opengl32.wglGetProcAddress("wglGetPixelFormatAttribivARB")))),
    wglChoosePixelFormatARB(reinterpret_cast<WglChoosePixelFormatARB>(
        reinterpret_cast<QFunctionPointer>(QOpenGLStaticContext::opengl32.wglGetProcAddress("wglChoosePixelFormatARB")))),
    wglCreateContextAttribsARB(reinterpret_cast<WglCreateContextAttribsARB>(
        reinterpret_cast<QFunctionPointer>(QOpenGLStaticContext::opengl32.wglGetProcAddress("wglCreateContextAttribsARB")))),
    wglSwapInternalExt(reinterpret_cast<WglSwapInternalExt>(
        reinterpret_cast<QFunctionPointer>(QOpenGLStaticContext::opengl32.wglGetProcAddress("wglSwapIntervalEXT")))),
    wglGetSwapInternalExt(reinterpret_cast<WglGetSwapInternalExt>(
        reinterpret_cast<QFunctionPointer>(QOpenGLStaticContext::opengl32.wglGetProcAddress("wglGetSwapIntervalEXT")))),
    wglGetExtensionsStringARB(reinterpret_cast<WglGetExtensionsStringARB>(
        reinterpret_cast<QFunctionPointer>(QOpenGLStaticContext::opengl32.wglGetProcAddress("wglGetExtensionsStringARB"))))
{
    if (defaultFormat.version < 0x0300) {
        if (extensionNames.startsWith(SAMPLE_BUFFER_EXTENSION " ")
                || extensionNames.indexOf(" " SAMPLE_BUFFER_EXTENSION " ") != -1)
            extensions |= SampleBuffers;
        if (extensionNames.startsWith(ROBUSTNESS_EXTENSION " ")
                || extensionNames.indexOf(" " ROBUSTNESS_EXTENSION " ") != -1)
            extensions |= Robustness;
    } else {
        typedef const GLubyte * (APIENTRY *glGetStringi_t)(GLenum, GLuint);
        auto glGetStringi = reinterpret_cast<glGetStringi_t>(
            reinterpret_cast<QFunctionPointer>(QOpenGLStaticContext::opengl32.wglGetProcAddress("glGetStringi")));
        if (glGetStringi) {
            GLint n = 0;
            QOpenGLStaticContext::opengl32.glGetIntegerv(GL_NUM_EXTENSIONS, &n);
            for (GLint i = 0; i < n; ++i) {
                const char *p = reinterpret_cast<const char *>(glGetStringi(GL_EXTENSIONS, i));
                if (p) {
                    if (!strcmp(p, SAMPLE_BUFFER_EXTENSION))
                        extensions |= SampleBuffers;
                    else if (!strcmp(p, ROBUSTNESS_EXTENSION))
                        extensions |= Robustness;
                }
            }
        }
    }
}

QByteArray QOpenGLStaticContext::getGlString(unsigned int which)
{
    if (const GLubyte *s = opengl32.glGetString(which))
        return QByteArray(reinterpret_cast<const char*>(s));
    return QByteArray();
}

QOpenGLStaticContext *QOpenGLStaticContext::create(bool softwareRendering)
{
    if (!opengl32.init(softwareRendering)) {
        qWarning("Failed to load and resolve WGL/OpenGL functions");
        return nullptr;
    }

    // We need a current context for wglGetProcAdress()/getGLString() to work.
    QScopedPointer<QOpenGLTemporaryContext> temporaryContext;
    if (!QOpenGLStaticContext::opengl32.wglGetCurrentContext())
        temporaryContext.reset(new QOpenGLTemporaryContext);
    auto *result = new QOpenGLStaticContext;
    qCDebug(lcQpaGl) << __FUNCTION__ << *result;
    return result;
}

/*!
    \class QWindowsGLContext
    \brief Open GL context.

    An Open GL context for use with several windows.
    As opposed to other implementations, activating a GL context for
    a window requires a HDC allocated for it. The first time this
    HDC is created for the window, the pixel format must be applied,
    which will affect the window as well. The HDCs are stored in a list of
    QOpenGLContextData and are released in doneCurrent().

    \internal
*/

QWindowsGLContext::QWindowsGLContext(QOpenGLStaticContext *staticContext,
                                     QOpenGLContext *context) :
    m_staticContext(staticContext),
    m_context(context),
    m_renderingContext(nullptr),
    m_pixelFormat(0),
    m_extensionsUsed(false),
    m_swapInterval(-1),
    m_ownsContext(true),
    m_getGraphicsResetStatus(nullptr),
    m_lost(false)
{
    if (!m_staticContext) // Something went very wrong. Stop here, isValid() will return false.
        return;

    QVariant nativeHandle = context->nativeHandle();
    if (!nativeHandle.isNull()) {
        // Adopt and existing context.
        if (!nativeHandle.canConvert<QWGLNativeContext>()) {
            qWarning("QWindowsGLContext: Requires a QWGLNativeContext");
            return;
        }
        auto handle = nativeHandle.value<QWGLNativeContext>();
        HGLRC wglcontext = handle.context();
        HWND wnd = handle.window();
        if (!wglcontext || !wnd) {
            qWarning("QWindowsGLContext: No context and window given");
            return;
        }

        HDC dc = GetDC(wnd);
        // A window with an associated pixel format is mandatory.
        // When no SetPixelFormat() call has been made, the following will fail.
        m_pixelFormat = GetPixelFormat(dc);
        bool ok = m_pixelFormat != 0;
        if (!ok)
            qWarning("QWindowsGLContext: Failed to get pixel format");
        ok = DescribePixelFormat(dc, m_pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &m_obtainedPixelFormatDescriptor);
        if (!ok) {
            qWarning("QWindowsGLContext: Failed to describe pixel format");
        } else {
            QWindowsOpenGLAdditionalFormat obtainedAdditional;
            m_obtainedFormat = GDI::qSurfaceFormatFromPixelFormat(m_obtainedPixelFormatDescriptor, &obtainedAdditional);
            m_renderingContext = wglcontext;
            ok = updateObtainedParams(dc);
        }

        ReleaseDC(wnd, dc);

        if (ok)
            m_ownsContext = false;
        else
            m_renderingContext = nullptr;

        return;
    }

    QSurfaceFormat format = context->format();
    if (format.renderableType() == QSurfaceFormat::DefaultRenderableType)
        format.setRenderableType(QSurfaceFormat::OpenGL);
    if (format.renderableType() != QSurfaceFormat::OpenGL)
        return;

    // workaround for matrox driver:
    // make a cheap call to opengl to force loading of DLL
    static bool opengl32dll = false;
    if (!opengl32dll) {
        GLint params;
        staticContext->opengl32.glGetIntegerv(GL_DEPTH_BITS, &params);
        opengl32dll = true;
    }

    // SetPixelFormat (as of Windows 7) requires a real window.
    // Create a dummy one as we are not associated with a window yet.
    // Try to find a suitable pixel format using preferably ARB extensions
    // (default to GDI) and store that.
    HWND dummyWindow = nullptr;
    HDC hdc = nullptr;
    bool tryExtensions = false;
    int obtainedSwapInterval = -1;
    do {
        dummyWindow = createDummyGLWindow();
        if (!dummyWindow)
            break;
        hdc = GetDC(dummyWindow);
        if (!hdc)
            break;

        if (QWindowsContext::verbose > 1)
            describeFormats(hdc);
        // Preferably use direct rendering and ARB extensions (unless pixmap
        // or explicitly turned off on command line).
        const QWindowsOpenGLAdditionalFormat
            requestedAdditional(QWindowsGLDirectRendering);
        tryExtensions = m_staticContext->hasExtensions()
                && !testFlag(requestedAdditional.formatFlags, QWindowsGLRenderToPixmap)
                && !(QWindowsIntegration::instance()->options() & QWindowsIntegration::DisableArb);
        QWindowsOpenGLAdditionalFormat obtainedAdditional;
        if (tryExtensions) {
            if (m_staticContext->wglGetExtensionsStringARB) {
                const char *exts = m_staticContext->wglGetExtensionsStringARB(hdc);
                if (exts) {
                    qCDebug(lcQpaGl) << __FUNCTION__ << "WGL extensions:" << exts;
                    if (strstr(exts, "WGL_EXT_framebuffer_sRGB"))
                        m_staticContext->extensions |= QOpenGLStaticContext::sRGBCapableFramebuffer;
                }
            }
            m_pixelFormat =
                ARB::choosePixelFormat(hdc, *m_staticContext, format,
                                       requestedAdditional, &m_obtainedPixelFormatDescriptor);
            if (m_pixelFormat > 0) {
                m_obtainedFormat =
                    ARB::qSurfaceFormatFromHDC(*m_staticContext, hdc, m_pixelFormat,
                                                &obtainedAdditional);
                m_extensionsUsed = true;
            }
        } // tryExtensions
        if (!m_pixelFormat) { // Failed, try GDI
            m_pixelFormat = GDI::choosePixelFormat(hdc, format, requestedAdditional,
                                                   &m_obtainedPixelFormatDescriptor);
            if (m_pixelFormat)
                m_obtainedFormat =
                    GDI::qSurfaceFormatFromPixelFormat(m_obtainedPixelFormatDescriptor,
                                                       &obtainedAdditional);
        } // try GDI
        if (!m_pixelFormat) {
            qWarning("%s: Unable find a suitable pixel format.", __FUNCTION__);
            break;
        }
        if (!QOpenGLStaticContext::opengl32.setPixelFormat(hdc, m_pixelFormat, &m_obtainedPixelFormatDescriptor)) {
            qErrnoWarning("SetPixelFormat failed.");
            break;
        }
        // Create context with sharing, again preferably using ARB.
        HGLRC sharingRenderingContext = nullptr;
        if (const QPlatformOpenGLContext *sc = context->shareHandle())
            sharingRenderingContext = static_cast<const QWindowsGLContext *>(sc)->renderingContext();

        if (m_extensionsUsed)
            m_renderingContext =
                ARB::createContext(*m_staticContext, hdc,
                                   format,
                                   requestedAdditional,
                                   sharingRenderingContext);
        if (!m_renderingContext)
            m_renderingContext = GDI::createContext(hdc, sharingRenderingContext);

        if (!m_renderingContext) {
            qWarning("Unable to create a GL Context.");
            break;
        }

        // Query obtained parameters and apply swap interval.
        if (!updateObtainedParams(hdc, &obtainedSwapInterval))
            break;

    } while (false);

    // Make the HGLRC retrievable via QOpenGLContext::nativeHandle().
    // Do not provide the window since it is the dummy one and it is about to disappear.
    if (m_renderingContext)
        context->setNativeHandle(QVariant::fromValue<QWGLNativeContext>(QWGLNativeContext(m_renderingContext, nullptr)));

    if (hdc)
        ReleaseDC(dummyWindow, hdc);
    if (dummyWindow)
        DestroyWindow(dummyWindow);

    qCDebug(lcQpaGl) << __FUNCTION__ << this << (tryExtensions ? "ARB" : "GDI")
        << " requested: " << context->format()
        << "\n    obtained #" << m_pixelFormat << (m_extensionsUsed ? "ARB" : "GDI") << m_obtainedFormat
        << "\n    " << m_obtainedPixelFormatDescriptor << " swap interval: " << obtainedSwapInterval
        << "\n    default: " << m_staticContext->defaultFormat
        << "\n    HGLRC=" << m_renderingContext;
}

QWindowsGLContext::~QWindowsGLContext()
{
    if (m_renderingContext && m_ownsContext)
        QOpenGLStaticContext::opengl32.wglDeleteContext(m_renderingContext);
    releaseDCs();
}

bool QWindowsGLContext::updateObtainedParams(HDC hdc, int *obtainedSwapInterval)
{
    HGLRC prevContext = QOpenGLStaticContext::opengl32.wglGetCurrentContext();
    HDC prevSurface = QOpenGLStaticContext::opengl32.wglGetCurrentDC();

    if (!QOpenGLStaticContext::opengl32.wglMakeCurrent(hdc, m_renderingContext)) {
        qWarning("Failed to make context current.");
        return false;
    }

    QWindowsOpenGLContextFormat::current().apply(&m_obtainedFormat);

    if (m_staticContext->wglGetSwapInternalExt && obtainedSwapInterval)
        *obtainedSwapInterval = m_staticContext->wglGetSwapInternalExt();

    if (testFlag(m_staticContext->extensions, QOpenGLStaticContext::Robustness)) {
        GLint value = 0;
        QOpenGLStaticContext::opengl32.glGetIntegerv(RESET_NOTIFICATION_STRATEGY_ARB, &value);
        if (value == LOSE_CONTEXT_ON_RESET_ARB)
            m_obtainedFormat.setOption(QSurfaceFormat::ResetNotification);
        m_getGraphicsResetStatus = reinterpret_cast<GlGetGraphicsResetStatusArbType>(
            reinterpret_cast<QFunctionPointer>(QOpenGLStaticContext::opengl32.wglGetProcAddress("glGetGraphicsResetStatusARB")));
    }

    QOpenGLStaticContext::opengl32.wglMakeCurrent(prevSurface, prevContext);
    return true;
}

void QWindowsGLContext::releaseDCs()
{
    for (const auto &e : m_windowContexts)
        ReleaseDC(e.hwnd, e.hdc);
    m_windowContexts.clear();
}

static inline QWindowsWindow *glWindowOf(QPlatformSurface *s)
{
    return static_cast<QWindowsWindow *>(s);
}

static inline HWND handleOf(QPlatformSurface *s)
{
    return glWindowOf(s)->handle();
}

// Find a window in a context list.
static inline const QOpenGLContextData *
    findByHWND(const std::vector<QOpenGLContextData> &data, HWND hwnd)
{
    for (const auto &e : data) {
        if (e.hwnd == hwnd)
            return &e;
    }
    return nullptr;
}

void QWindowsGLContext::swapBuffers(QPlatformSurface *surface)
{
    if (QWindowsContext::verbose > 1)
        qCDebug(lcQpaGl) << __FUNCTION__ << surface;

    if (const QOpenGLContextData *contextData = findByHWND(m_windowContexts, handleOf(surface)))
        QOpenGLStaticContext::opengl32.swapBuffers(contextData->hdc);
    else
        qWarning("%s: Cannot find window %p", __FUNCTION__, handleOf(surface));
}

bool QWindowsGLContext::makeCurrent(QPlatformSurface *surface)
{
#ifdef DEBUG_GL
    if (QWindowsContext::verbose > 1)
        qCDebug(lcQpaGl) << __FUNCTION__ << this << m_windowContexts.size() << "contexts";
#endif // DEBUG_GL

    Q_ASSERT(surface->surface()->supportsOpenGL());

    // Do we already have a DC entry for that window?
    auto *window = static_cast<QWindowsWindow *>(surface);
    window->aboutToMakeCurrent();
    const HWND hwnd = window->handle();
    if (const QOpenGLContextData *contextData = findByHWND(m_windowContexts, hwnd)) {
        // Repeated calls to wglMakeCurrent when vsync is enabled in the driver will
        // often result in 100% cpuload. This check is cheap and avoids the problem.
        // This is reproducable on NVidia cards and Intel onboard chips.
        if (QOpenGLStaticContext::opengl32.wglGetCurrentContext() == contextData->renderingContext
                && QOpenGLStaticContext::opengl32.wglGetCurrentDC() == contextData->hdc) {
            return true;
        }
        return QOpenGLStaticContext::opengl32.wglMakeCurrent(contextData->hdc, contextData->renderingContext);
    }
    // Create a new entry.
    const QOpenGLContextData newContext(m_renderingContext, hwnd, GetDC(hwnd));
    if (!newContext.hdc)
        return false;
    // Initialize pixel format first time. This will apply to
    // the HWND as well and  must be done only once.
    if (!window->testFlag(QWindowsWindow::OpenGlPixelFormatInitialized)) {
        if (!QOpenGLStaticContext::opengl32.setPixelFormat(newContext.hdc, m_pixelFormat, &m_obtainedPixelFormatDescriptor)) {
            qErrnoWarning("%s: SetPixelFormat() failed", __FUNCTION__);
            ReleaseDC(newContext.hwnd, newContext.hdc);
            return false;
        }
        window->setFlag(QWindowsWindow::OpenGlPixelFormatInitialized);
        if (m_obtainedFormat.swapBehavior() == QSurfaceFormat::DoubleBuffer)
            window->setFlag(QWindowsWindow::OpenGLDoubleBuffered);
    }
    m_windowContexts.push_back(newContext);

    m_lost = false;
    bool success = QOpenGLStaticContext::opengl32.wglMakeCurrent(newContext.hdc, newContext.renderingContext);
    if (!success) {
        if (m_getGraphicsResetStatus && m_getGraphicsResetStatus()) {
            m_lost = true;
            qCDebug(lcQpaGl) << "makeCurrent(): context loss detected" << this;
            // Drop the surface. Will recreate on the next makeCurrent.
            window->invalidateSurface();
        }
    }

    // Set the swap interval
    if (m_staticContext->wglSwapInternalExt) {
        const int interval = surface->format().swapInterval();
        if (m_swapInterval != interval)
            m_swapInterval = interval;
        if (interval >= 0)
            m_staticContext->wglSwapInternalExt(interval);
    }

    return success;
}

void QWindowsGLContext::doneCurrent()
{
#ifdef DEBUG_GL
    if (QWindowsContext::verbose > 1)
        qCDebug(lcQpaGl) << __FUNCTION__ << this << m_windowContexts.size() << "contexts";
#endif // DEBUG_GL
    QOpenGLStaticContext::opengl32.wglMakeCurrent(nullptr, nullptr);
    releaseDCs();
}

QFunctionPointer QWindowsGLContext::getProcAddress(const char *procName)
{
    // Even though we use QFunctionPointer, it does not mean the function can be called.
    // It will need to be cast to the proper function type with the correct calling
    // convention. QFunctionPointer is nothing more than a glorified void* here.
    auto procAddress = reinterpret_cast<QFunctionPointer>(QOpenGLStaticContext::opengl32.wglGetProcAddress(procName));

    // We support AllGLFunctionsQueryable, which means this function must be able to
    // return a function pointer even for functions that are in GL.h and exported
    // normally from opengl32.dll. wglGetProcAddress() is not guaranteed to work for such
    // functions, however in QT_OPENGL_DYNAMIC builds QOpenGLFunctions will just blindly
    // call into here for _any_ OpenGL function.
    if (procAddress == nullptr || reinterpret_cast<quintptr>(procAddress) < 4u
        || procAddress == reinterpret_cast<QFunctionPointer>(-1)) {
        procAddress = QOpenGLStaticContext::opengl32.resolve(procName);
    }

    if (QWindowsContext::verbose > 1)
        qCDebug(lcQpaGl) << __FUNCTION__ <<  procName << QOpenGLStaticContext::opengl32.wglGetCurrentContext() << "returns" << procAddress;

    return reinterpret_cast<QFunctionPointer>(procAddress);
}

QT_END_NAMESPACE
