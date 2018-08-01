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

static inline QByteArray getGlString(GLenum param)
{
    if (const GLubyte *s = glGetString(param))
        return QByteArray(reinterpret_cast<const char*>(s));
    return QByteArray();
}

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQpaOpenGLContext, "qt.qpa.openglcontext", QtWarningMsg);

QCocoaGLContext::QCocoaGLContext(QOpenGLContext *context)
    : QPlatformOpenGLContext(), m_format(context->format())
{
    QVariant nativeHandle = context->nativeHandle();
    if (!nativeHandle.isNull()) {
        if (!nativeHandle.canConvert<QCocoaNativeContext>()) {
            qCWarning(lcQpaOpenGLContext, "QOpenGLContext native handle must be a QCocoaNativeContext");
            return;
        }
        m_context = nativeHandle.value<QCocoaNativeContext>().context();
        if (!m_context) {
            qCWarning(lcQpaOpenGLContext, "QCocoaNativeContext's NSOpenGLContext can not be null");
            return;
        }

        [m_context retain];

        // Note: We have no way of knowing whether the NSOpenGLContext was created with the
        // share context as reported by the QOpenGLContext, but we just have to trust that
        // it was. It's okey, as the only thing we're using it for is to report isShared().
        if (QPlatformOpenGLContext *shareContext = context->shareHandle())
            m_shareContext = static_cast<QCocoaGLContext *>(shareContext)->nativeContext();

        updateSurfaceFormat();
        return;
    }

    // ----------- Default case, we own the NSOpenGLContext -----------

    // We only support OpenGL contexts under Cocoa
    if (m_format.renderableType() == QSurfaceFormat::DefaultRenderableType)
        m_format.setRenderableType(QSurfaceFormat::OpenGL);
    if (m_format.renderableType() != QSurfaceFormat::OpenGL)
        return;

    if (QPlatformOpenGLContext *shareContext = context->shareHandle()) {
        m_shareContext = static_cast<QCocoaGLContext *>(shareContext)->nativeContext();

        // Allow sharing between 3.2 Core and 4.1 Core profile versions in
        // cases where NSOpenGLContext creates a 4.1 context where a 3.2
        // context was requested. Due to the semantics of QSurfaceFormat
        // this 4.1 version can find its way onto the format for the new
        // context, even though it was at no point requested by the user.
        GLint shareContextRequestedProfile;
        [m_shareContext.pixelFormat getValues:&shareContextRequestedProfile
            forAttribute:NSOpenGLPFAOpenGLProfile forVirtualScreen:0];
        auto shareContextActualProfile = shareContext->format().version();

        if (shareContextRequestedProfile == NSOpenGLProfileVersion3_2Core
            && shareContextActualProfile >= qMakePair(4, 1)) {
            // There is a mismatch. Downgrade requested format to make the
            // NSOpenGLPFAOpenGLProfile attributes match. (NSOpenGLContext
            // will fail to create a new context if there is a mismatch).
            if (m_format.version() >= qMakePair(4, 1))
                m_format.setVersion(3, 2);
        }
    }

    // ------------------------- Create NSOpenGLContext -------------------------

    NSOpenGLPixelFormat *pixelFormat = [pixelFormatForSurfaceFormat(m_format) autorelease];
    m_context = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:m_shareContext];

    if (!m_context && m_shareContext) {
        qCWarning(lcQpaOpenGLContext, "Could not create NSOpenGLContext with shared context, "
            "falling back to unshared context.");
        m_context = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:nil];
        m_shareContext = nil;
    }

    if (!m_context) {
        qCWarning(lcQpaOpenGLContext, "Failed to create NSOpenGLContext");
        return;
    }

    // --------------------- Set NSOpenGLContext properties ---------------------

    const GLint interval = m_format.swapInterval() >= 0 ? m_format.swapInterval() : 1;
    [m_context setValues:&interval forParameter:NSOpenGLCPSwapInterval];

    if (m_format.alphaBufferSize() > 0) {
        int zeroOpacity = 0;
        [m_context setValues:&zeroOpacity forParameter:NSOpenGLCPSurfaceOpacity];
    }

    // OpenGL surfaces can be ordered either above(default) or below the NSWindow
    // FIXME: Promote to QSurfaceFormat option or property
    const GLint order = qt_mac_resolveOption(1, "QT_MAC_OPENGL_SURFACE_ORDER");
    [m_context setValues:&order forParameter:NSOpenGLCPSurfaceOrder];

    updateSurfaceFormat();
}

NSOpenGLPixelFormat *QCocoaGLContext::pixelFormatForSurfaceFormat(const QSurfaceFormat &format)
{
    QVector<NSOpenGLPixelFormatAttribute> attrs;

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

    switch (format.swapBehavior()) {
    case QSurfaceFormat::SingleBuffer:
        break; // The NSOpenGLPixelFormat default, no attribute to set
    case QSurfaceFormat::DefaultSwapBehavior:
        // Technically this should be single-buffered, but we force double-buffered
        // FIXME: Why do we force double-buffered?
        Q_FALLTHROUGH();
    case QSurfaceFormat::DoubleBuffer:
        attrs.append(NSOpenGLPFADoubleBuffer);
        break;
    case QSurfaceFormat::TripleBuffer:
        attrs.append(NSOpenGLPFATripleBuffer);
        break;
    }

    if (format.depthBufferSize() > 0)
        attrs <<  NSOpenGLPFADepthSize << format.depthBufferSize();
    if (format.stencilBufferSize() > 0)
        attrs << NSOpenGLPFAStencilSize << format.stencilBufferSize();
    if (format.alphaBufferSize() > 0)
        attrs << NSOpenGLPFAAlphaSize << format.alphaBufferSize();
    if (format.redBufferSize() > 0 && format.greenBufferSize() > 0 && format.blueBufferSize() > 0) {
        const int colorSize = format.redBufferSize() + format.greenBufferSize() + format.blueBufferSize();
        attrs << NSOpenGLPFAColorSize << colorSize << NSOpenGLPFAMinimumPolicy;
    }

    if (format.samples() > 0) {
        attrs << NSOpenGLPFAMultisample
              << NSOpenGLPFASampleBuffers << NSOpenGLPixelFormatAttribute(1)
              << NSOpenGLPFASamples << NSOpenGLPixelFormatAttribute(format.samples());
    }

    if (format.stereo())
        attrs << NSOpenGLPFAStereo;

    // Allow rendering on GPUs without a connected display
    attrs << NSOpenGLPFAAllowOfflineRenderers;

    // FIXME: Pull this information out of the NSView
    QByteArray useLayer = qgetenv("QT_MAC_WANTS_LAYER");
    if (!useLayer.isEmpty() && useLayer.toInt() > 0) {
        // Disable the software rendering fallback. This makes compositing
        // OpenGL and raster NSViews using Core Animation layers possible.
        attrs << NSOpenGLPFANoRecovery;
    }

    attrs << 0; // 0-terminate array
    return [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs.constData()];
}

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

QCocoaGLContext::~QCocoaGLContext()
{
    if (m_currentWindow && m_currentWindow.data()->handle())
        static_cast<QCocoaWindow *>(m_currentWindow.data()->handle())->setCurrentContext(0);

    [m_context release];
}

bool QCocoaGLContext::makeCurrent(QPlatformSurface *surface)
{
    qCDebug(lcQpaOpenGLContext) << "Making" << m_context << "current"
        << "in" << QThread::currentThread() << "for" << surface;

    Q_ASSERT(surface->surface()->supportsOpenGL());

    [m_context makeCurrentContext];

    if (surface->surface()->surfaceClass() == QSurface::Offscreen)
        return true;

    QWindow *window = static_cast<QCocoaWindow *>(surface)->window();
    if (!setActiveWindow(window)) {
        qCDebug(lcQpaOpenGLContext) << "Failed to activate window, skipping makeCurrent";
        return false;
    }

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

bool QCocoaGLContext::setActiveWindow(QWindow *window)
{
    if (window == m_currentWindow.data())
        return true;

    Q_ASSERT(window->handle());
    QCocoaWindow *cocoaWindow = static_cast<QCocoaWindow *>(window->handle());
    NSView *view = cocoaWindow->view();

    if ((m_context.view = view) != view) {
        qCDebug(lcQpaOpenGLContext) << "Associating" << view << "with" << m_context << "failed";
        return false;
    }

    qCDebug(lcQpaOpenGLContext) << m_context << "now associated with" << m_context.view;

    if (m_currentWindow && m_currentWindow.data()->handle())
        static_cast<QCocoaWindow *>(m_currentWindow.data()->handle())->setCurrentContext(0);

    m_currentWindow = window;

    cocoaWindow->setCurrentContext(this);
    return true;
}

// NSOpenGLContext is not re-entrant (https://openradar.appspot.com/37064579)
static QMutex s_contextMutex;

void QCocoaGLContext::update()
{
    QMutexLocker locker(&s_contextMutex);
    qCInfo(lcQpaOpenGLContext) << "Updating" << m_context << "for" << m_context.view;
    [m_context update];
}

void QCocoaGLContext::swapBuffers(QPlatformSurface *surface)
{
    qCDebug(lcQpaOpenGLContext) << "Swapping" << m_context
        << "in" << QThread::currentThread() << "to" << surface;

    if (surface->surface()->surfaceClass() == QSurface::Offscreen)
        return; // Nothing to do

    QWindow *window = static_cast<QCocoaWindow *>(surface)->window();
    if (!setActiveWindow(window)) {
        qCWarning(lcQpaOpenGLContext) << "Failed to activate window, skipping swapBuffers";
        return;
    }

    QMutexLocker locker(&s_contextMutex);
    [m_context flushBuffer];
}

void QCocoaGLContext::doneCurrent()
{
    qCDebug(lcQpaOpenGLContext) << "Clearing current context"
        << [NSOpenGLContext currentContext] << "in" << QThread::currentThread();

    if (m_currentWindow && m_currentWindow.data()->handle())
        static_cast<QCocoaWindow *>(m_currentWindow.data()->handle())->setCurrentContext(nullptr);

    m_currentWindow.clear();

    [NSOpenGLContext clearCurrentContext];
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

QSurfaceFormat QCocoaGLContext::format() const
{
    return m_format;
}

bool QCocoaGLContext::isValid() const
{
    return m_context != nil;
}

bool QCocoaGLContext::isSharing() const
{
    return m_shareContext != nil;
}

NSOpenGLContext *QCocoaGLContext::nativeContext() const
{
    return m_context;
}

QFunctionPointer QCocoaGLContext::getProcAddress(const char *procName)
{
    return (QFunctionPointer)dlsym(RTLD_DEFAULT, procName);
}

QT_END_NAMESPACE
