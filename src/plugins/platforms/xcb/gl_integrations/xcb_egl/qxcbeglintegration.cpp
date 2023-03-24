// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qxcbeglintegration.h"

#include "qxcbeglcontext.h"

#include <QtGui/QOffscreenSurface>
#include <QtGui/private/qeglconvenience_p.h>
#include <QtGui/private/qeglstreamconvenience_p.h>
#include <optional>

#include "qxcbeglnativeinterfacehandler.h"

QT_BEGIN_NAMESPACE

namespace {

struct VisualInfo
{
    xcb_visualtype_t visualType;
    uint8_t depth;
};

std::optional<VisualInfo> getVisualInfo(xcb_screen_t *screen,
                                        std::optional<xcb_visualid_t> requestedVisualId,
                                        std::optional<uint8_t> requestedDepth = std::nullopt)
{
    xcb_depth_iterator_t depthIterator = xcb_screen_allowed_depths_iterator(screen);

    while (depthIterator.rem) {
        xcb_depth_t *depth = depthIterator.data;
        xcb_visualtype_iterator_t visualTypeIterator = xcb_depth_visuals_iterator(depth);

        while (visualTypeIterator.rem) {
            xcb_visualtype_t *visualType = visualTypeIterator.data;
            if (requestedVisualId && visualType->visual_id != *requestedVisualId) {
                xcb_visualtype_next(&visualTypeIterator);
                continue;
            }

            if (requestedDepth && depth->depth != *requestedDepth) {
                xcb_visualtype_next(&visualTypeIterator);
                continue;
            }

            return VisualInfo{ *visualType, depth->depth };
        }

        xcb_depth_next(&depthIterator);
    }

    return std::nullopt;
}

} // namespace

QXcbEglIntegration::QXcbEglIntegration()
    : m_connection(nullptr)
    , m_egl_display(EGL_NO_DISPLAY)
    , m_using_platform_display(false)
{
    qCDebug(lcQpaGl) << "Xcb EGL gl-integration created";
}

QXcbEglIntegration::~QXcbEglIntegration()
{
    if (m_egl_display != EGL_NO_DISPLAY)
        eglTerminate(m_egl_display);
}

bool QXcbEglIntegration::initialize(QXcbConnection *connection)
{
    m_connection = connection;

    const char *extensions = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);

#if QT_CONFIG(xcb_xlib)
    if (extensions && strstr(extensions, "EGL_EXT_platform_x11")) {
        QEGLStreamConvenience streamFuncs;
        m_egl_display = streamFuncs.get_platform_display(EGL_PLATFORM_X11_KHR,
                                                         m_connection->xlib_display(),
                                                         nullptr);
        m_using_platform_display = true;
    }

#if QT_CONFIG(egl_x11)
    if (!m_egl_display)
        m_egl_display = eglGetDisplay(reinterpret_cast<EGLNativeDisplayType>(m_connection->xlib_display()));
#endif
#else
    if (extensions && (strstr(extensions, "EGL_EXT_platform_xcb") || strstr(extensions, "EGL_MESA_platform_xcb"))) {
        QEGLStreamConvenience streamFuncs;
        m_egl_display = streamFuncs.get_platform_display(EGL_PLATFORM_XCB_KHR,
                                                         reinterpret_cast<void *>(connection->xcb_connection()),
                                                         nullptr);
        m_using_platform_display = true;
    }
#endif

    EGLint major, minor;
    bool success = eglInitialize(m_egl_display, &major, &minor);
#if QT_CONFIG(egl_x11)
    if (!success) {
        m_egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        qCDebug(lcQpaGl) << "Xcb EGL gl-integration retrying with display" << m_egl_display;
        success = eglInitialize(m_egl_display, &major, &minor);
    }
#endif

    m_native_interface_handler.reset(new QXcbEglNativeInterfaceHandler(connection->nativeInterface()));

    if (success)
        qCDebug(lcQpaGl) << "Xcb EGL gl-integration successfully initialized";
    else
        qCWarning(lcQpaGl) << "Xcb EGL gl-integration initialize failed";

    return success;
}

QXcbWindow *QXcbEglIntegration::createWindow(QWindow *window) const
{
    return new QXcbEglWindow(window, const_cast<QXcbEglIntegration *>(this));
}

QPlatformOpenGLContext *QXcbEglIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    QXcbScreen *screen = static_cast<QXcbScreen *>(context->screen()->handle());
    QXcbEglContext *platformContext = new QXcbEglContext(screen->surfaceFormatFor(context->format()),
                                                         context->shareHandle(),
                                                         eglDisplay());
    return platformContext;
}

QOpenGLContext *QXcbEglIntegration::createOpenGLContext(EGLContext context, EGLDisplay display, QOpenGLContext *shareContext) const
{
    return QEGLPlatformContext::createFrom<QXcbEglContext>(context, display, eglDisplay(), shareContext);
}

QPlatformOffscreenSurface *QXcbEglIntegration::createPlatformOffscreenSurface(QOffscreenSurface *surface) const
{
    QXcbScreen *screen = static_cast<QXcbScreen *>(surface->screen()->handle());
    return new QEGLPbuffer(eglDisplay(), screen->surfaceFormatFor(surface->requestedFormat()), surface);
}

xcb_visualid_t QXcbEglIntegration::getCompatibleVisualId(xcb_screen_t *screen, EGLConfig config) const
{
    xcb_visualid_t visualId = 0;
    EGLint eglValue = 0;

    EGLint configRedSize = 0;
    eglGetConfigAttrib(eglDisplay(), config, EGL_RED_SIZE, &configRedSize);

    EGLint configGreenSize = 0;
    eglGetConfigAttrib(eglDisplay(), config, EGL_GREEN_SIZE, &configGreenSize);

    EGLint configBlueSize = 0;
    eglGetConfigAttrib(eglDisplay(), config, EGL_BLUE_SIZE, &configBlueSize);

    EGLint configAlphaSize = 0;
    eglGetConfigAttrib(eglDisplay(), config, EGL_ALPHA_SIZE, &configAlphaSize);

    eglGetConfigAttrib(eglDisplay(), config, EGL_CONFIG_ID, &eglValue);
    int configId = eglValue;

    // See if EGL provided a valid VisualID:
    eglGetConfigAttrib(eglDisplay(), config, EGL_NATIVE_VISUAL_ID, &eglValue);
    visualId = eglValue;
    if (visualId) {
        // EGL has suggested a visual id, so get the rest of the visual info for that id:
        std::optional<VisualInfo> chosenVisualInfo = getVisualInfo(screen, visualId);
        if (chosenVisualInfo) {
            // Skip size checks if implementation supports non-matching visual
            // and config (QTBUG-9444).
            if (q_hasEglExtension(eglDisplay(), "EGL_NV_post_convert_rounding"))
                return visualId;
            // Skip also for i.MX6 where 565 visuals are suggested for the default 444 configs and it works just fine.
            const char *vendor = eglQueryString(eglDisplay(), EGL_VENDOR);
            if (vendor && strstr(vendor, "Vivante"))
                return visualId;

            int visualRedSize = qPopulationCount(chosenVisualInfo->visualType.red_mask);
            int visualGreenSize = qPopulationCount(chosenVisualInfo->visualType.green_mask);
            int visualBlueSize = qPopulationCount(chosenVisualInfo->visualType.blue_mask);
            int visualAlphaSize = chosenVisualInfo->depth - visualRedSize - visualBlueSize - visualGreenSize;

            const bool visualMatchesConfig = visualRedSize >= configRedSize
                && visualGreenSize >= configGreenSize
                && visualBlueSize >= configBlueSize
                && visualAlphaSize >= configAlphaSize;

            // In some cases EGL tends to suggest a 24-bit visual for 8888
            // configs. In such a case we have to fall back to getVisualInfo.
            if (!visualMatchesConfig) {
                visualId = 0;
                qCDebug(lcQpaGl,
                        "EGL suggested using X Visual ID %d (%d %d %d %d depth %d) for EGL config %d"
                        "(%d %d %d %d), but this is incompatible",
                        visualId, visualRedSize, visualGreenSize, visualBlueSize, visualAlphaSize, chosenVisualInfo->depth,
                        configId, configRedSize, configGreenSize, configBlueSize, configAlphaSize);
            }
        } else {
            qCDebug(lcQpaGl, "EGL suggested using X Visual ID %d for EGL config %d, but that isn't a valid ID",
                    visualId, configId);
            visualId = 0;
        }
    }
    else
        qCDebug(lcQpaGl, "EGL did not suggest a VisualID (EGL_NATIVE_VISUAL_ID was zero) for EGLConfig %d", configId);

    if (visualId) {
        qCDebug(lcQpaGl, configAlphaSize > 0
                ? "Using ARGB Visual ID %d provided by EGL for config %d"
                : "Using Opaque Visual ID %d provided by EGL for config %d", visualId, configId);
        return visualId;
    }

    // Finally, try to use getVisualInfo and only use the bit depths to match on:
    if (!visualId) {
        uint8_t depth = configRedSize + configGreenSize + configBlueSize + configAlphaSize;
        std::optional<VisualInfo> matchingVisual = getVisualInfo(screen, std::nullopt, depth);
        if (!matchingVisual) {
            // Try again without taking the alpha channel into account:
            depth = configRedSize + configGreenSize + configBlueSize;
            matchingVisual = getVisualInfo(screen, std::nullopt, depth);
        }

        if (matchingVisual)
            visualId = matchingVisual->visualType.visual_id;
    }

    if (visualId) {
        qCDebug(lcQpaGl, "Using Visual ID %d provided by getVisualInfo for EGL config %d", visualId, configId);
        return visualId;
    }

    qWarning("Unable to find an X11 visual which matches EGL config %d", configId);
    return 0;
}

QT_END_NAMESPACE
