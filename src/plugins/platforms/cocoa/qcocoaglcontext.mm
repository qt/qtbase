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
#include "qcocoascreen.h"

#include <qdebug.h>
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
    : QPlatformOpenGLContext()
    , m_format(context->format())
{
}

void QCocoaGLContext::initialize()
{
    QVariant nativeHandle = context()->nativeHandle();
    if (!nativeHandle.isNull()) {
        if (!nativeHandle.canConvert<QCocoaNativeContext>()) {
            qCWarning(lcQpaOpenGLContext, "QOpenGLContext native handle must be a QCocoaNativeContext");
            return;
        }
        m_context = nativeHandle.value<QCocoaNativeContext>().context();
        if (!m_context) {
            qCWarning(lcQpaOpenGLContext, "QCocoaNativeContext's NSOpenGLContext cannot be null");
            return;
        }

        [m_context retain];

        // Note: We have no way of knowing whether the NSOpenGLContext was created with the
        // share context as reported by the QOpenGLContext, but we just have to trust that
        // it was. It's okey, as the only thing we're using it for is to report isShared().
        if (QPlatformOpenGLContext *shareContext = context()->shareHandle())
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

    if (QPlatformOpenGLContext *shareContext = context()->shareHandle()) {
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

    // The native handle should reflect the underlying context, even if we created it
    context()->setNativeHandle(QVariant::fromValue<QCocoaNativeContext>(m_context));

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

    qCDebug(lcQpaOpenGLContext).verbosity(3) << "Created" << this << "based on requested" << context()->format();
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

    auto rbz = format.redBufferSize();
    auto gbz = format.greenBufferSize();
    auto bbz = format.blueBufferSize();
    if (rbz > 0 || gbz > 0 || bbz > 0) {
        auto fallbackSize = qMax(rbz, qMax(gbz, bbz));
        auto colorSize = (rbz > 0 ? rbz : fallbackSize)
                       + (gbz > 0 ? gbz : fallbackSize)
                       + (bbz > 0 ? bbz : fallbackSize);
        attrs << NSOpenGLPFAColorSize << colorSize << NSOpenGLPFAMinimumPolicy;
    }

    if (format.samples() > 0) {
        attrs << NSOpenGLPFAMultisample
              << NSOpenGLPFASampleBuffers << NSOpenGLPixelFormatAttribute(1)
              << NSOpenGLPFASamples << NSOpenGLPixelFormatAttribute(format.samples());
    }

    //Workaround for problems with Chromium and offline renderers on the lat 2013 MacPros.
    //FIXME: Think if this could be solved via QSurfaceFormat in the future.
    static bool offlineRenderersAllowed = qEnvironmentVariableIsEmpty("QT_MAC_PRO_WEBENGINE_WORKAROUND");
    if (offlineRenderersAllowed) {
        // Allow rendering on GPUs without a connected display
        attrs << NSOpenGLPFAAllowOfflineRenderers;
    }

    if (qGuiApp->testAttribute(Qt::AA_UseSoftwareOpenGL)) {
        // kCGLRendererGenericFloatID is the modern software renderer on macOS,
        // as opposed to kCGLRendererGenericID, which is deprecated.
        attrs << NSOpenGLPFARendererID << kCGLRendererGenericFloatID;
    }

    attrs << 0; // 0-terminate array
    return [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs.constData()];
}

/*!
    Updates the surface format of this context based on properties of
    the native context and GL state, so that the result of creating
    the context is reflected back in QOpenGLContext.
*/
void QCocoaGLContext::updateSurfaceFormat()
{
    NSOpenGLContext *oldContext = [NSOpenGLContext currentContext];
    [m_context makeCurrentContext];

    // --------------------- Query GL state ---------------------

    int major = 0, minor = 0;
    QByteArray versionString(getGlString(GL_VERSION));
    if (QPlatformOpenGLContext::parseOpenGLVersion(versionString, major, minor)) {
        m_format.setMajorVersion(major);
        m_format.setMinorVersion(minor);
    }

    m_format.setProfile(QSurfaceFormat::NoProfile);
    if (m_format.version() >= qMakePair(3, 2)) {
        GLint value = 0;
        glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &value);
        if (value & GL_CONTEXT_CORE_PROFILE_BIT)
            m_format.setProfile(QSurfaceFormat::CoreProfile);
        else if (value & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT)
            m_format.setProfile(QSurfaceFormat::CompatibilityProfile);
    }

    m_format.setOption(QSurfaceFormat::DeprecatedFunctions, [&]() {
        if (m_format.version() < qMakePair(3, 0)) {
            return true;
        } else {
            GLint value = 0;
            glGetIntegerv(GL_CONTEXT_FLAGS, &value);
            return !(value & GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT);
        }
    }());

    // Debug contexts not supported on macOS
    m_format.setOption(QSurfaceFormat::DebugContext, false);

    // Nor are stereo buffers (deprecated in macOS 10.12)
    m_format.setOption(QSurfaceFormat::StereoBuffers, false);

    // ------------------ Query the pixel format ------------------

    NSOpenGLPixelFormat *pixelFormat = m_context.pixelFormat;

    GLint virtualScreen = [&, this]() {
        auto *platformScreen = static_cast<QCocoaScreen*>(context()->screen()->handle());
        auto displayId = platformScreen->nativeScreen().qt_displayId;
        auto requestedDisplay = CGDisplayIDToOpenGLDisplayMask(displayId);
        for (int i = 0; i < pixelFormat.numberOfVirtualScreens; ++i) {
            GLint supportedDisplays;
            [pixelFormat getValues:&supportedDisplays forAttribute:NSOpenGLPFAScreenMask forVirtualScreen:i];
            // Note: The mask returned for NSOpenGLPFAScreenMask is a bit mask of
            // physical displays that the renderer can drive, while the one returned
            // from CGDisplayIDToOpenGLDisplayMask has a single bit set, representing
            // that particular display.
            if (requestedDisplay & supportedDisplays)
                return i;
        }
        qCWarning(lcQpaOpenGLContext) << "Could not find virtual screen for"
                    << platformScreen << "with displayId" << displayId;
        return 0;
    }();

    auto pixelFormatAttribute = [&](NSOpenGLPixelFormatAttribute attribute) {
        int value = 0;
        [pixelFormat getValues:&value forAttribute:attribute forVirtualScreen:virtualScreen];
        return value;
    };

    int colorSize = pixelFormatAttribute(NSOpenGLPFAColorSize);
    colorSize /= 4; // The attribute includes the alpha component
    m_format.setRedBufferSize(colorSize);
    m_format.setGreenBufferSize(colorSize);
    m_format.setBlueBufferSize(colorSize);

    // Surfaces on macOS always have an alpha channel, but unless the user requested
    // one via setAlphaBufferSize(), which triggered setting NSOpenGLCPSurfaceOpacity
    // to make the surface non-opaque, we don't want to report back the actual alpha
    // size, as that will make the user believe the alpha channel can be used for
    // something useful, when in reality it can't, due to the surface being opaque.
    if (m_format.alphaBufferSize() > 0)
        m_format.setAlphaBufferSize(pixelFormatAttribute(NSOpenGLPFAAlphaSize));

    m_format.setDepthBufferSize(pixelFormatAttribute(NSOpenGLPFADepthSize));
    m_format.setStencilBufferSize(pixelFormatAttribute(NSOpenGLPFAStencilSize));
    m_format.setSamples(pixelFormatAttribute(NSOpenGLPFASamples));

    if (pixelFormatAttribute(NSOpenGLPFATripleBuffer))
        m_format.setSwapBehavior(QSurfaceFormat::TripleBuffer);
    else if (pixelFormatAttribute(NSOpenGLPFADoubleBuffer))
        m_format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    else
        m_format.setSwapBehavior(QSurfaceFormat::SingleBuffer);

    // ------------------- Query the context -------------------

    auto glContextParameter = [&](NSOpenGLContextParameter parameter) {
        int value = 0;
        [m_context getValues:&value forParameter:parameter];
        return value;
    };

    m_format.setSwapInterval(glContextParameter(NSOpenGLCPSwapInterval));

    if (oldContext)
        [oldContext makeCurrentContext];
    else
        [NSOpenGLContext clearCurrentContext];
}

QCocoaGLContext::~QCocoaGLContext()
{
    [m_context release];
}

bool QCocoaGLContext::makeCurrent(QPlatformSurface *surface)
{
    QMacAutoReleasePool pool;

    qCDebug(lcQpaOpenGLContext) << "Making" << this << "current"
        << "in" << QThread::currentThread() << "for" << surface;

    Q_ASSERT(surface->surface()->supportsOpenGL());

    if (!setDrawable(surface))
        return false;

    [m_context makeCurrentContext];

    if (surface->surface()->surfaceClass() == QSurface::Window) {
        if (m_needsUpdate.fetchAndStoreRelaxed(false))
            update();
    }

    return true;
}

/*!
    Sets the drawable object of the NSOpenGLContext, which is the
    frame buffer that is the target of OpenGL drawing operations.
*/
bool QCocoaGLContext::setDrawable(QPlatformSurface *surface)
{
    // Make sure any surfaces released during this process are deallocated
    // straight away, otherwise we may run out of surfaces when spinning a
    // render-loop that doesn't return to one of the outer pools.
    QMacAutoReleasePool pool;

    if (!surface || surface->surface()->surfaceClass() == QSurface::Offscreen) {
        // Clear the current drawable and reset the active window, so that GL
        // commands that don't target a specific FBO will not end up stomping
        // on the previously set drawable.
        qCDebug(lcQpaOpenGLContext) << "Clearing current drawable" << m_context.view << "for" << m_context;
        [m_context clearDrawable];
        return true;
    }

    Q_ASSERT(surface->surface()->surfaceClass() == QSurface::Window);
    auto *cocoaWindow = static_cast<QCocoaWindow *>(surface);
    QNSView *view = qnsview_cast(cocoaWindow->view());

    if (view == m_context.view)
        return true;

    prepareDrawable(cocoaWindow);

    // Setting the drawable may happen on a separate thread as a result of
    // a call to makeCurrent, so we need to set up the observers before we
    // associate the view with the context. That way we will guarantee that
    // as long as the view is the drawable of the context we will know about
    // any updates to the view that require surface invalidation.

    auto updateCallback = [this, view]() {
        Q_ASSERT(QThread::currentThread() == qApp->thread());
        if (m_context.view != view)
            return;
        m_needsUpdate = true;
    };

    m_updateObservers.clear();

    if (view.layer) {
        m_updateObservers.append(QMacNotificationObserver(view, NSViewFrameDidChangeNotification, updateCallback));
        m_updateObservers.append(QMacNotificationObserver(view.window, NSWindowDidChangeScreenNotification, updateCallback));
    } else {
        m_updateObservers.append(QMacNotificationObserver(view, NSViewGlobalFrameDidChangeNotification, updateCallback));
    }

    m_updateObservers.append(QMacNotificationObserver([NSApplication sharedApplication],
        NSApplicationDidChangeScreenParametersNotification, updateCallback));

    // If any of the observers fire at this point it's fine. We check the
    // view association (atomically) in the update callback, and skip the
    // update if we haven't associated yet. Setting the drawable below will
    // have the same effect as an update.

    // Now we are ready to associate the view with the context
    m_context.view = view;
    if (m_context.view != view) {
        qCInfo(lcQpaOpenGLContext) << "Failed to set" << view << "as drawable for" << m_context;
        m_updateObservers.clear();
        return false;
    }

    qCInfo(lcQpaOpenGLContext) << "Set drawable for" << m_context << "to" << m_context.view;
    return true;
}

void QCocoaGLContext::prepareDrawable(QCocoaWindow *platformWindow)
{
    // We generally want high-DPI GL surfaces, unless the user has explicitly disabled them
    bool prefersBestResolutionOpenGLSurface = qt_mac_resolveOption(YES,
        platformWindow->window(), "_q_mac_wantsBestResolutionOpenGLSurface",
        "QT_MAC_WANTS_BEST_RESOLUTION_OPENGL_SURFACE");

    auto *view = platformWindow->view();

    // The only case we have to opt out ourselves is when using the Apple software renderer
    // in combination with surface-backed views, as these together do not support high-DPI.
    if (prefersBestResolutionOpenGLSurface) {
        int rendererID = 0;
        [m_context getValues:&rendererID forParameter:NSOpenGLContextParameterCurrentRendererID];
        bool isSoftwareRenderer = (rendererID & kCGLRendererIDMatchingMask) == kCGLRendererGenericFloatID;
        if (isSoftwareRenderer && !view.layer) {
            qCInfo(lcQpaOpenGLContext) << "Disabling high resolution GL surface due to software renderer";
            prefersBestResolutionOpenGLSurface = false;
        }
    }

    view.wantsBestResolutionOpenGLSurface = prefersBestResolutionOpenGLSurface;
}

// NSOpenGLContext is not re-entrant. Even when using separate contexts per thread,
// view, and window, calls into the API will still deadlock. For more information
// see https://openradar.appspot.com/37064579
static QMutex s_reentrancyMutex;

void QCocoaGLContext::update()
{
    // Make sure any surfaces released during this process are deallocated
    // straight away, otherwise we may run out of surfaces when spinning a
    // render-loop that doesn't return to one of the outer pools.
    QMacAutoReleasePool pool;

    QMutexLocker locker(&s_reentrancyMutex);
    qCInfo(lcQpaOpenGLContext) << "Updating" << m_context << "for" << m_context.view;
    [m_context update];
}

void QCocoaGLContext::swapBuffers(QPlatformSurface *surface)
{
    QMacAutoReleasePool pool;

    qCDebug(lcQpaOpenGLContext) << "Swapping" << m_context
        << "in" << QThread::currentThread() << "to" << surface;

    if (surface->surface()->surfaceClass() == QSurface::Offscreen)
        return; // Nothing to do

    if (!setDrawable(surface)) {
        qCWarning(lcQpaOpenGLContext) << "Can't flush" << m_context
            << "without" << surface << "as drawable";
        return;
    }

    if (m_context.view.layer) {
        // Flushing an NSOpenGLContext will hit the screen immediately, ignoring
        // any Core Animation transactions in place. This may result in major
        // visual artifacts if the flush happens out of sync with the size
        // of the layer, view, and window reflected by other parts of the UI,
        // e.g. if the application flushes in the resize event or a timer during
        // window resizing, instead of in the expose event.
        auto *cocoaWindow = static_cast<QCocoaWindow *>(surface);
        if (cocoaWindow->geometry().size() != cocoaWindow->m_exposedRect.size()) {
            qCInfo(lcQpaOpenGLContext) << "Window exposed size does not match geometry (yet)."
                << "Skipping flush to avoid visual artifacts.";
            return;
        }
    }

    QMutexLocker locker(&s_reentrancyMutex);
    [m_context flushBuffer];
}

void QCocoaGLContext::doneCurrent()
{
    QMacAutoReleasePool pool;

    qCDebug(lcQpaOpenGLContext) << "Clearing current context"
        << [NSOpenGLContext currentContext] << "in" << QThread::currentThread();

    // Note: We do not need to clear the current drawable here.
    // As long as there is no current context, GL calls will
    // do nothing.

    [NSOpenGLContext clearCurrentContext];
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
    return (QFunctionPointer)dlsym(RTLD_NEXT, procName);
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const QCocoaGLContext *context)
{
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug << "QCocoaGLContext(" << (const void *)context;
    if (context) {
        if (debug.verbosity() > QDebug::DefaultVerbosity)
            debug << ", " << context->format();
        debug << ", " << context->nativeContext();
    }
    debug << ')';
    return debug;
}
#endif // !QT_NO_DEBUG_STREAM

QT_END_NAMESPACE
