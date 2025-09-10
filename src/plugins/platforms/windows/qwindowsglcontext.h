// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
    QWindowsOpenGLContext *createContext(HGLRC context, HWND window) override;
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

class QWindowsGLContext : public QWindowsOpenGLContext, public QNativeInterface::QWGLContext
{
public:
    explicit QWindowsGLContext(QOpenGLStaticContext *staticContext, QOpenGLContext *context);
    explicit QWindowsGLContext(QOpenGLStaticContext *staticContext, HGLRC context, HWND window);

    ~QWindowsGLContext() override;
    bool isSharing() const override { return context()->shareHandle(); }
    bool isValid() const override { return m_renderingContext && !m_lost; }
    QSurfaceFormat format() const override { return m_obtainedFormat; }

    void swapBuffers(QPlatformSurface *surface) override;

    bool makeCurrent(QPlatformSurface *surface) override;
    void doneCurrent() override;

    typedef void (*GL_Proc) ();

    QFunctionPointer getProcAddress(const char *procName) override;

    HGLRC renderingContext() const { return m_renderingContext; }

    HGLRC nativeContext() const override { return m_renderingContext; }

private:
    typedef GLenum (APIENTRY *GlGetGraphicsResetStatusArbType)();

    inline void releaseDCs();
    bool updateObtainedParams(HDC hdc, int *obtainedSwapInterval = nullptr);

    QOpenGLStaticContext *m_staticContext = nullptr;
    QSurfaceFormat m_obtainedFormat;
    HGLRC m_renderingContext = nullptr;
    std::vector<QOpenGLContextData> m_windowContexts;
    PIXELFORMATDESCRIPTOR m_obtainedPixelFormatDescriptor;
    int m_pixelFormat = 0;
    bool m_extensionsUsed = false;
    int m_swapInterval = -1;
    bool m_ownsContext = true;
    GlGetGraphicsResetStatusArbType m_getGraphicsResetStatus = nullptr;
    bool m_lost = false;
};
#endif
QT_END_NAMESPACE

#endif // QWINDOWSGLCONTEXT_H
