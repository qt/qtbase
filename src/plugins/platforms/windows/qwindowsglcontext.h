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

#ifndef QWINDOWSGLCONTEXT_H
#define QWINDOWSGLCONTEXT_H

#include <QtCore/qt_windows.h>
#include "qwindowsopenglcontext.h"

#include <QtGui/qopenglcontext.h>

#include <vector>

QT_BEGIN_NAMESPACE
#ifndef QT_NO_OPENGL

class QDebug;

enum QWindowsGLFormatFlags
{
    QWindowsGLDirectRendering = 0x1,
    QWindowsGLOverlay = 0x2,
    QWindowsGLRenderToPixmap = 0x4,
    QWindowsGLAccumBuffer = 0x8
};

// Additional format information for Windows.
struct QWindowsOpenGLAdditionalFormat
{
    QWindowsOpenGLAdditionalFormat(unsigned formatFlagsIn = 0, unsigned pixmapDepthIn = 0) :
        formatFlags(formatFlagsIn), pixmapDepth(pixmapDepthIn) { }
    unsigned formatFlags; // QWindowsGLFormatFlags.
    unsigned pixmapDepth; // for QWindowsGLRenderToPixmap
};

// Per-window data for active OpenGL contexts.
struct QOpenGLContextData
{
    QOpenGLContextData(HGLRC r, HWND h, HDC d) : renderingContext(r), hwnd(h), hdc(d) {}
    QOpenGLContextData() {}

    HGLRC renderingContext = nullptr;
    HWND hwnd = nullptr;
    HDC hdc = nullptr;
};

class QOpenGLStaticContext;

struct QWindowsOpenGLContextFormat
{
    static QWindowsOpenGLContextFormat current();
    void apply(QSurfaceFormat *format) const;

    QSurfaceFormat::OpenGLContextProfile profile = QSurfaceFormat::NoProfile;
    int version = 0; //! majorVersion<<8 + minorVersion
    QSurfaceFormat::FormatOptions options;
};

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug d, const PIXELFORMATDESCRIPTOR &);
QDebug operator<<(QDebug d, const QWindowsOpenGLContextFormat &);
QDebug operator<<(QDebug d, const QOpenGLStaticContext &s);
#endif

struct QWindowsOpengl32DLL
{
    bool init(bool softwareRendering);
    void *moduleHandle() const { return m_lib; }
    bool moduleIsNotOpengl32() const { return m_nonOpengl32; }

    // Wrappers. Always use these instead of SwapBuffers/wglSwapBuffers/etc.
    BOOL swapBuffers(HDC dc);
    BOOL setPixelFormat(HDC dc, int pf, const PIXELFORMATDESCRIPTOR *pfd);
    int describePixelFormat(HDC dc, int pf, UINT size, PIXELFORMATDESCRIPTOR *pfd);

    // WGL
    HGLRC (WINAPI * wglCreateContext)(HDC dc);
    BOOL (WINAPI * wglDeleteContext)(HGLRC context);
    HGLRC (WINAPI * wglGetCurrentContext)();
    HDC (WINAPI * wglGetCurrentDC)();
    PROC (WINAPI * wglGetProcAddress)(LPCSTR name);
    BOOL (WINAPI * wglMakeCurrent)(HDC dc, HGLRC context);
    BOOL (WINAPI * wglShareLists)(HGLRC context1, HGLRC context2);

    // GL1+GLES2 common
    GLenum (APIENTRY * glGetError)();
    void (APIENTRY * glGetIntegerv)(GLenum pname, GLint* params);
    const GLubyte * (APIENTRY * glGetString)(GLenum name);

    QFunctionPointer resolve(const char *name);
private:
    HMODULE m_lib;
    bool m_nonOpengl32;

    // For Mesa llvmpipe shipped with a name other than opengl32.dll
    BOOL (WINAPI * wglSwapBuffers)(HDC dc);
    BOOL (WINAPI * wglSetPixelFormat)(HDC dc, int pf, const PIXELFORMATDESCRIPTOR *pfd);
    int (WINAPI * wglDescribePixelFormat)(HDC dc, int pf, UINT size, PIXELFORMATDESCRIPTOR *pfd);
};

class QOpenGLStaticContext : public QWindowsStaticOpenGLContext
{
    Q_DISABLE_COPY_MOVE(QOpenGLStaticContext)
    QOpenGLStaticContext();
public:
    enum Extensions
    {
        SampleBuffers = 0x1,
        sRGBCapableFramebuffer = 0x2,
        Robustness = 0x4,
    };

    typedef bool
        (APIENTRY *WglGetPixelFormatAttribIVARB)
            (HDC hdc, int iPixelFormat, int iLayerPlane,
             uint nAttributes, const int *piAttributes, int *piValues);

    typedef bool
        (APIENTRY *WglChoosePixelFormatARB)(HDC hdc, const int *piAttribList,
            const float *pfAttribFList, uint nMaxFormats, int *piFormats,
            UINT *nNumFormats);

    typedef HGLRC
        (APIENTRY *WglCreateContextAttribsARB)(HDC, HGLRC, const int *);

    typedef BOOL
        (APIENTRY *WglSwapInternalExt)(int interval);
    typedef int
        (APIENTRY *WglGetSwapInternalExt)(void);

    typedef const char *
        (APIENTRY *WglGetExtensionsStringARB)(HDC);

    bool hasExtensions() const
        { return wglGetPixelFormatAttribIVARB && wglChoosePixelFormatARB && wglCreateContextAttribsARB; }

    static QOpenGLStaticContext *create(bool softwareRendering = false);
    static QByteArray getGlString(unsigned int which);

    QWindowsOpenGLContext *createContext(QOpenGLContext *context) override;
    void *moduleHandle() const override { return opengl32.moduleHandle(); }
    QOpenGLContext::OpenGLModuleType moduleType() const override
    { return QOpenGLContext::LibGL; }

    // For a regular opengl32.dll report the ThreadedOpenGL capability.
    // For others, which are likely to be software-only, don't.
    bool supportsThreadedOpenGL() const override
    { return !opengl32.moduleIsNotOpengl32(); }

    const QByteArray vendor;
    const QByteArray renderer;
    const QByteArray extensionNames;
    unsigned extensions;
    const QWindowsOpenGLContextFormat defaultFormat;

    WglGetPixelFormatAttribIVARB wglGetPixelFormatAttribIVARB;
    WglChoosePixelFormatARB wglChoosePixelFormatARB;
    WglCreateContextAttribsARB wglCreateContextAttribsARB;
    WglSwapInternalExt wglSwapInternalExt;
    WglGetSwapInternalExt wglGetSwapInternalExt;
    WglGetExtensionsStringARB wglGetExtensionsStringARB;

    static QWindowsOpengl32DLL opengl32;
};

class QWindowsGLContext : public QWindowsOpenGLContext
{
public:
    explicit QWindowsGLContext(QOpenGLStaticContext *staticContext, QOpenGLContext *context);
    ~QWindowsGLContext() override;
    bool isSharing() const override { return m_context->shareHandle(); }
    bool isValid() const override { return m_renderingContext && !m_lost; }
    QSurfaceFormat format() const override { return m_obtainedFormat; }

    void swapBuffers(QPlatformSurface *surface) override;

    bool makeCurrent(QPlatformSurface *surface) override;
    void doneCurrent() override;

    typedef void (*GL_Proc) ();

    QFunctionPointer getProcAddress(const char *procName) override;

    HGLRC renderingContext() const { return m_renderingContext; }

    void *nativeContext() const override { return m_renderingContext; }

private:
    typedef GLenum (APIENTRY *GlGetGraphicsResetStatusArbType)();

    inline void releaseDCs();
    bool updateObtainedParams(HDC hdc, int *obtainedSwapInterval = nullptr);

    QOpenGLStaticContext *m_staticContext;
    QOpenGLContext *m_context;
    QSurfaceFormat m_obtainedFormat;
    HGLRC m_renderingContext;
    std::vector<QOpenGLContextData> m_windowContexts;
    PIXELFORMATDESCRIPTOR m_obtainedPixelFormatDescriptor;
    int m_pixelFormat;
    bool m_extensionsUsed;
    int m_swapInterval;
    bool m_ownsContext;
    GlGetGraphicsResetStatusArbType m_getGraphicsResetStatus;
    bool m_lost;
};
#endif
QT_END_NAMESPACE

#endif // QWINDOWSGLCONTEXT_H
