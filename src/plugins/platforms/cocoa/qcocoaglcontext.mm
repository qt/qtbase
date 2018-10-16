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

#include "qcocoaglcontext.h"
#include "qcocoawindow.h"
#include "qcocoahelpers.h"
#include <qdebug.h>
#include <QtCore/private/qcore_mac_p.h>
#include <QtPlatformHeaders/qcocoanativecontext.h>
#include <dlfcn.h>

#import <AppKit/AppKit.h>

QT_BEGIN_NAMESPACE

static inline QByteArray getGlString(GLenum param)
{
    if (const GLubyte *s = glGetString(param))
        return QByteArray(reinterpret_cast<const char*>(s));
    return QByteArray();
}

#if !defined(GL_CONTEXT_FLAGS)
#define GL_CONTEXT_FLAGS 0x821E
#endif

#if !defined(GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT)
#define GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT 0x0001
#endif

#if !defined(GL_CONTEXT_PROFILE_MASK)
#define GL_CONTEXT_PROFILE_MASK 0x9126
#endif

#if !defined(GL_CONTEXT_CORE_PROFILE_BIT)
#define GL_CONTEXT_CORE_PROFILE_BIT 0x00000001
#endif

#if !defined(GL_CONTEXT_COMPATIBILITY_PROFILE_BIT)
#define GL_CONTEXT_COMPATIBILITY_PROFILE_BIT 0x00000002
#endif

static void updateFormatFromContext(QSurfaceFormat *format)
{
    Q_ASSERT(format);

    // Update the version, profile, and context bit of the format
    int major = 0, minor = 0;
    QByteArray versionString(getGlString(GL_VERSION));
    if (QPlatformOpenGLContext::parseOpenGLVersion(versionString, major, minor)) {
        format->setMajorVersion(major);
        format->setMinorVersion(minor);
    }

    format->setProfile(QSurfaceFormat::NoProfile);

    Q_ASSERT(format->renderableType() == QSurfaceFormat::OpenGL);
    if (format->version() < qMakePair(3, 0)) {
        format->setOption(QSurfaceFormat::DeprecatedFunctions);
        return;
    }

    // Version 3.0 onwards - check if it includes deprecated functionality
    GLint value = 0;
    glGetIntegerv(GL_CONTEXT_FLAGS, &value);
    if (!(value & GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT))
        format->setOption(QSurfaceFormat::DeprecatedFunctions);

    // Debug context option not supported on OS X

    if (format->version() < qMakePair(3, 2))
        return;

    // Version 3.2 and newer have a profile
    value = 0;
    glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &value);

    if (value & GL_CONTEXT_CORE_PROFILE_BIT)
        format->setProfile(QSurfaceFormat::CoreProfile);
    else if (value & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT)
        format->setProfile(QSurfaceFormat::CompatibilityProfile);
}

 // NSOpenGLContext is not re-entrant (https://openradar.appspot.com/37064579)
static QMutex s_contextMutex;

QCocoaGLContext::QCocoaGLContext(const QSurfaceFormat &format, QPlatformOpenGLContext *share,
                                 const QVariant &nativeHandle)
    : m_context(nil),
      m_shareContext(nil),
      m_format(format),
      m_didCheckForSoftwareContext(false)
{
    if (!nativeHandle.isNull()) {
        if (!nativeHandle.canConvert<QCocoaNativeContext>()) {
            qWarning("QCocoaGLContext: Requires a QCocoaNativeContext");
            return;
        }
        QCocoaNativeContext handle = nativeHandle.value<QCocoaNativeContext>();
        NSOpenGLContext *context = handle.context();
        if (!context) {
            qWarning("QCocoaGLContext: No NSOpenGLContext given");
            return;
        }
        m_context = context;
        [m_context retain];
        m_shareContext = share ? static_cast<QCocoaGLContext *>(share)->nsOpenGLContext() : nil;
        // OpenGL surfaces can be ordered either above(default) or below the NSWindow.
        const GLint order = qt_mac_resolveOption(1, "QT_MAC_OPENGL_SURFACE_ORDER");
        [m_context setValues:&order forParameter:NSOpenGLCPSurfaceOrder];
        updateSurfaceFormat();
        return;
    }

    // we only support OpenGL contexts under Cocoa
    if (m_format.renderableType() == QSurfaceFormat::DefaultRenderableType)
        m_format.setRenderableType(QSurfaceFormat::OpenGL);
    if (m_format.renderableType() != QSurfaceFormat::OpenGL)
        return;

    QMacAutoReleasePool pool; // For the SG Canvas render thread

    m_shareContext = share ? static_cast<QCocoaGLContext *>(share)->nsOpenGLContext() : nil;

    if (m_shareContext) {
        // Allow sharing between 3.2 Core and 4.1 Core profile versions in
        // cases where NSOpenGLContext creates a 4.1 context where a 3.2
        // context was requested. Due to the semantics of QSurfaceFormat
        // this 4.1 version can find its way onto the format for the new
        // context, even though it was at no point requested by the user.
        GLint shareContextRequestedProfile;
        [m_shareContext.pixelFormat getValues:&shareContextRequestedProfile
            forAttribute:NSOpenGLPFAOpenGLProfile forVirtualScreen:0];
        auto shareContextActualProfile = share->format().version();

        if (shareContextRequestedProfile == NSOpenGLProfileVersion3_2Core &&
            shareContextActualProfile >= qMakePair(4, 1)) {

            // There is a mismatch, downgrade requested format to make the
            // NSOpenGLPFAOpenGLProfile attributes match. (NSOpenGLContext will
            // fail to create a new context if there is a mismatch).
            if (m_format.version() >= qMakePair(4, 1))
                m_format.setVersion(3, 2);
        }
    }

    // create native context for the requested pixel format and share
    NSOpenGLPixelFormat *pixelFormat = createNSOpenGLPixelFormat(m_format);
    m_context = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:m_shareContext];

    // retry without sharing on context creation failure.
    if (!m_context && m_shareContext) {
        m_shareContext = nil;
        m_context = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:nil];
        if (m_context)
            qWarning("QCocoaGLContext: Falling back to unshared context.");
    }

    // give up if we still did not get a native context
    [pixelFormat release];
    if (!m_context) {
        qWarning("QCocoaGLContext: Failed to create context.");
        return;
    }

    const GLint interval = format.swapInterval() >= 0 ? format.swapInterval() : 1;
    [m_context setValues:&interval forParameter:NSOpenGLCPSwapInterval];

    if (format.alphaBufferSize() > 0) {
        int zeroOpacity = 0;
        [m_context setValues:&zeroOpacity forParameter:NSOpenGLCPSurfaceOpacity];
    }


    // OpenGL surfaces can be ordered either above(default) or below the NSWindow.
    const GLint order = qt_mac_resolveOption(1, "QT_MAC_OPENGL_SURFACE_ORDER");
    [m_context setValues:&order forParameter:NSOpenGLCPSurfaceOrder];

    updateSurfaceFormat();
}

QCocoaGLContext::~QCocoaGLContext()
{
    if (m_currentWindow && m_currentWindow.data()->handle())
        static_cast<QCocoaWindow *>(m_currentWindow.data()->handle())->setCurrentContext(0);

    [m_context release];
}

QVariant QCocoaGLContext::nativeHandle() const
{
    return QVariant::fromValue<QCocoaNativeContext>(QCocoaNativeContext(m_context));
}

QSurfaceFormat QCocoaGLContext::format() const
{
    return m_format;
}

void QCocoaGLContext::windowWasHidden()
{
    // If the window is hidden, we need to unset the m_currentWindow
    // variable so that succeeding makeCurrent's will not abort prematurely
    // because of the optimization in setActiveWindow.
    // Doing a full doneCurrent here is not preferable, because the GL context
    // might be rendering in a different thread at this time.
    m_currentWindow.clear();
}

void QCocoaGLContext::swapBuffers(QPlatformSurface *surface)
{
    if (surface->surface()->surfaceClass() == QSurface::Offscreen)
        return; // Nothing to do

    QWindow *window = static_cast<QCocoaWindow *>(surface)->window();
    setActiveWindow(window);

    QMutexLocker locker(&s_contextMutex);
    [m_context flushBuffer];
}

bool QCocoaGLContext::makeCurrent(QPlatformSurface *surface)
{
    Q_ASSERT(surface->surface()->supportsOpenGL());

    QMacAutoReleasePool pool;
    [m_context makeCurrentContext];

    if (surface->surface()->surfaceClass() == QSurface::Offscreen)
        return true;

    QWindow *window = static_cast<QCocoaWindow *>(surface)->window();
    setActiveWindow(window);

    // Disable high-resolution surfaces when using the software renderer, which has the
    // problem that the system silently falls back to a to using a low-resolution buffer
    // when a high-resolution buffer is requested. This is not detectable using the NSWindow
    // convertSizeToBacking and backingScaleFactor APIs. A typical result of this is that Qt
    // will display a quarter of the window content when running in a virtual machine.
    if (!m_didCheckForSoftwareContext) {
        m_didCheckForSoftwareContext = true;

        const GLubyte* renderer = glGetString(GL_RENDERER);
        if (qstrcmp((const char *)renderer, "Apple Software Renderer") == 0) {
            NSView *view = static_cast<QCocoaWindow *>(surface)->m_view;
            [view setWantsBestResolutionOpenGLSurface:NO];
        }
    }

    update();
    return true;
}

void QCocoaGLContext::setActiveWindow(QWindow *window)
{
    if (window == m_currentWindow.data())
        return;

    if (m_currentWindow && m_currentWindow.data()->handle())
        static_cast<QCocoaWindow *>(m_currentWindow.data()->handle())->setCurrentContext(0);

    Q_ASSERT(window->handle());

    m_currentWindow = window;

    QCocoaWindow *cocoaWindow = static_cast<QCocoaWindow *>(window->handle());
    cocoaWindow->setCurrentContext(this);

    Q_ASSERT(!cocoaWindow->isForeignWindow());
    [qnsview_cast(cocoaWindow->view()) setQCocoaGLContext:this];
}

void QCocoaGLContext::updateSurfaceFormat()
{
    // At present it is impossible to turn an option off on a QSurfaceFormat (see
    // https://codereview.qt-project.org/#change,70599). So we have to populate
    // the actual surface format from scratch
    QSurfaceFormat requestedFormat = m_format;
    m_format = QSurfaceFormat();
    m_format.setRenderableType(QSurfaceFormat::OpenGL);

    // CoreGL doesn't require a drawable to make the context current
    CGLContextObj oldContext = CGLGetCurrentContext();
    CGLContextObj ctx = static_cast<CGLContextObj>([m_context CGLContextObj]);
    CGLSetCurrentContext(ctx);

    // Get the data that OpenGL provides
    updateFormatFromContext(&m_format);

    // Get the data contained within the pixel format
    CGLPixelFormatObj cglPixelFormat = static_cast<CGLPixelFormatObj>(CGLGetPixelFormat(ctx));
    NSOpenGLPixelFormat *pixelFormat = [[NSOpenGLPixelFormat alloc] initWithCGLPixelFormatObj:cglPixelFormat];

    int colorSize = -1;
    [pixelFormat getValues:&colorSize forAttribute:NSOpenGLPFAColorSize forVirtualScreen:0];
    if (colorSize > 0) {
        // This seems to return the total color buffer depth, including alpha
        m_format.setRedBufferSize(colorSize / 4);
        m_format.setGreenBufferSize(colorSize / 4);
        m_format.setBlueBufferSize(colorSize / 4);
    }

    // The pixel format always seems to return 8 for alpha. However, the framebuffer only
    // seems to have alpha enabled if we requested it explicitly. I can't find any other
    // attribute to check explicitly for this so we use our best guess for alpha.
    int alphaSize = -1;
    [pixelFormat getValues:&alphaSize forAttribute:NSOpenGLPFAAlphaSize forVirtualScreen:0];
    if (alphaSize > 0 && requestedFormat.alphaBufferSize() > 0)
        m_format.setAlphaBufferSize(alphaSize);

    int depthSize = -1;
    [pixelFormat getValues:&depthSize forAttribute:NSOpenGLPFADepthSize forVirtualScreen:0];
    if (depthSize > 0)
        m_format.setDepthBufferSize(depthSize);

    int stencilSize = -1;
    [pixelFormat getValues:&stencilSize forAttribute:NSOpenGLPFAStencilSize forVirtualScreen:0];
    if (stencilSize > 0)
        m_format.setStencilBufferSize(stencilSize);

    int samples = -1;
    [pixelFormat getValues:&samples forAttribute:NSOpenGLPFASamples forVirtualScreen:0];
    if (samples > 0)
        m_format.setSamples(samples);

    int doubleBuffered = -1;
    int tripleBuffered = -1;
    [pixelFormat getValues:&doubleBuffered forAttribute:NSOpenGLPFADoubleBuffer forVirtualScreen:0];
    [pixelFormat getValues:&tripleBuffered forAttribute:NSOpenGLPFATripleBuffer forVirtualScreen:0];

    if (tripleBuffered == 1)
        m_format.setSwapBehavior(QSurfaceFormat::TripleBuffer);
    else if (doubleBuffered == 1)
        m_format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    else
        m_format.setSwapBehavior(QSurfaceFormat::SingleBuffer);

    int steroBuffers = -1;
    [pixelFormat getValues:&steroBuffers forAttribute:NSOpenGLPFAStereo forVirtualScreen:0];
    if (steroBuffers == 1)
        m_format.setOption(QSurfaceFormat::StereoBuffers);

    [pixelFormat release];

    GLint swapInterval = -1;
    [m_context getValues:&swapInterval forParameter:NSOpenGLCPSwapInterval];
    if (swapInterval >= 0)
        m_format.setSwapInterval(swapInterval);

    // Restore the original context
    CGLSetCurrentContext(oldContext);
}

void QCocoaGLContext::doneCurrent()
{
    if (m_currentWindow && m_currentWindow.data()->handle())
        static_cast<QCocoaWindow *>(m_currentWindow.data()->handle())->setCurrentContext(0);

    m_currentWindow.clear();

    [NSOpenGLContext clearCurrentContext];
}

QFunctionPointer QCocoaGLContext::getProcAddress(const char *procName)
{
    return (QFunctionPointer)dlsym(RTLD_DEFAULT, procName);
}

void QCocoaGLContext::update()
{
    QMutexLocker locker(&s_contextMutex);
    [m_context update];
}

NSOpenGLPixelFormat *QCocoaGLContext::createNSOpenGLPixelFormat(const QSurfaceFormat &format)
{
    QVector<NSOpenGLPixelFormatAttribute> attrs;

    if (format.swapBehavior() == QSurfaceFormat::DoubleBuffer
        || format.swapBehavior() == QSurfaceFormat::DefaultSwapBehavior)
        attrs.append(NSOpenGLPFADoubleBuffer);
    else if (format.swapBehavior() == QSurfaceFormat::TripleBuffer)
        attrs.append(NSOpenGLPFATripleBuffer);


    // Select OpenGL profile
    attrs << NSOpenGLPFAOpenGLProfile;
    if (format.profile() == QSurfaceFormat::CoreProfile) {
        if (format.version() >= qMakePair(4, 1))
            attrs << NSOpenGLProfileVersion4_1Core;
        else if (format.version() >= qMakePair(3, 2))
            attrs << NSOpenGLProfileVersion3_2Core;
        else
            attrs << NSOpenGLProfileVersionLegacy;
    } else {
        attrs << NSOpenGLProfileVersionLegacy;
    }

    if (format.depthBufferSize() > 0)
        attrs <<  NSOpenGLPFADepthSize << format.depthBufferSize();
    if (format.stencilBufferSize() > 0)
        attrs << NSOpenGLPFAStencilSize << format.stencilBufferSize();
    if (format.alphaBufferSize() > 0)
        attrs << NSOpenGLPFAAlphaSize << format.alphaBufferSize();
    if ((format.redBufferSize() > 0) &&
        (format.greenBufferSize() > 0) &&
        (format.blueBufferSize() > 0)) {
        const int colorSize = format.redBufferSize() +
                              format.greenBufferSize() +
                              format.blueBufferSize();
        attrs << NSOpenGLPFAColorSize << colorSize << NSOpenGLPFAMinimumPolicy;
    }

    if (format.samples() > 0) {
        attrs << NSOpenGLPFAMultisample
              << NSOpenGLPFASampleBuffers << (NSOpenGLPixelFormatAttribute) 1
              << NSOpenGLPFASamples << (NSOpenGLPixelFormatAttribute) format.samples();
    }

    if (format.stereo())
        attrs << NSOpenGLPFAStereo;

    //Workaround for problems with Chromium and offline renderers on the lat 2013 MacPros.
    //FIXME: Think if this could be solved via QSurfaceFormat in the future.
    static bool offlineRenderersAllowed = qEnvironmentVariableIsEmpty("QT_MAC_PRO_WEBENGINE_WORKAROUND");
    if (offlineRenderersAllowed) {
        // Allow rendering on GPUs without a connected display
        attrs << NSOpenGLPFAAllowOfflineRenderers;
    }

    QByteArray useLayer = qgetenv("QT_MAC_WANTS_LAYER");
    if (!useLayer.isEmpty() && useLayer.toInt() > 0) {
        // Disable the software rendering fallback. This makes compositing
        // OpenGL and raster NSViews using Core Animation layers possible.
        attrs << NSOpenGLPFANoRecovery;
    }

    attrs << 0;

    return [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs.constData()];
}

NSOpenGLContext *QCocoaGLContext::nsOpenGLContext() const
{
    return m_context;
}

bool QCocoaGLContext::isValid() const
{
    return m_context != nil;
}

bool QCocoaGLContext::isSharing() const
{
    return m_shareContext != nil;
}

QT_END_NAMESPACE

