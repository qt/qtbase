// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwaylandglcontext_p.h"

#include <QtWaylandClient/private/qwaylanddisplay_p.h>
#include <QtWaylandClient/private/qwaylandwindow_p.h>
#include <QtWaylandClient/private/qwaylandsubsurface_p.h>
#include <QtWaylandClient/private/qwaylandabstractdecoration_p.h>
#include <QtWaylandClient/private/qwaylandintegration_p.h>
#include "qwaylandeglwindow_p.h"

#include <QDebug>
#include <QtGui/private/qeglconvenience_p.h>
#include <QtGui/private/qopenglcontext_p.h>
#include <QtOpenGL/private/qopengltexturecache_p.h>
#include <QtGui/private/qguiapplication_p.h>

#include <qpa/qplatformopenglcontext.h>
#include <QtGui/QSurfaceFormat>
#include <QtOpenGL/QOpenGLShaderProgram>
#include <QtGui/QOpenGLFunctions>
#include <QOpenGLBuffer>

#include <QtCore/qmutex.h>

#include <dlfcn.h>

// Constants from EGL_KHR_create_context
#ifndef EGL_CONTEXT_MINOR_VERSION_KHR
#define EGL_CONTEXT_MINOR_VERSION_KHR 0x30FB
#endif
#ifndef EGL_CONTEXT_FLAGS_KHR
#define EGL_CONTEXT_FLAGS_KHR 0x30FC
#endif
#ifndef EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR
#define EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR 0x30FD
#endif
#ifndef EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR
#define EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR 0x00000001
#endif
#ifndef EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR
#define EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR 0x00000002
#endif
#ifndef EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR
#define EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR 0x00000001
#endif
#ifndef EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR
#define EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR 0x00000002
#endif

// Constants for OpenGL which are not available in the ES headers.
#ifndef GL_CONTEXT_FLAGS
#define GL_CONTEXT_FLAGS 0x821E
#endif
#ifndef GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT
#define GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT 0x0001
#endif
#ifndef GL_CONTEXT_FLAG_DEBUG_BIT
#define GL_CONTEXT_FLAG_DEBUG_BIT 0x00000002
#endif
#ifndef GL_CONTEXT_PROFILE_MASK
#define GL_CONTEXT_PROFILE_MASK 0x9126
#endif
#ifndef GL_CONTEXT_CORE_PROFILE_BIT
#define GL_CONTEXT_CORE_PROFILE_BIT 0x00000001
#endif
#ifndef GL_CONTEXT_COMPATIBILITY_PROFILE_BIT
#define GL_CONTEXT_COMPATIBILITY_PROFILE_BIT 0x00000002
#endif

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class DecorationsBlitter : public QOpenGLFunctions
{
public:
    DecorationsBlitter(QWaylandGLContext *context)
        : m_context(context)
    {
        initializeOpenGLFunctions();
        m_blitProgram = new QOpenGLShaderProgram();
        m_blitProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, "attribute vec4 position;\n\
                                                                    attribute vec4 texCoords;\n\
                                                                    varying vec2 outTexCoords;\n\
                                                                    void main()\n\
                                                                    {\n\
                                                                        gl_Position = position;\n\
                                                                        outTexCoords = texCoords.xy;\n\
                                                                    }");
        m_blitProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, "varying highp vec2 outTexCoords;\n\
                                                                        uniform sampler2D texture;\n\
                                                                        void main()\n\
                                                                        {\n\
                                                                            gl_FragColor = texture2D(texture, outTexCoords);\n\
                                                                        }");

        m_blitProgram->bindAttributeLocation("position", 0);
        m_blitProgram->bindAttributeLocation("texCoords", 1);

        if (!m_blitProgram->link()) {
            qDebug() << "Shader Program link failed.";
            qDebug() << m_blitProgram->log();
        }

        m_blitProgram->bind();
        m_blitProgram->enableAttributeArray(0);
        m_blitProgram->enableAttributeArray(1);

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glDisable(GL_CULL_FACE);
        glDisable(GL_SCISSOR_TEST);
        glDepthMask(GL_FALSE);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

        m_buffer.create();
        m_buffer.bind();

        static const GLfloat squareVertices[] = {
            -1.f, -1.f,
            1.0f, -1.f,
            -1.f,  1.0f,
            1.0f,  1.0f
        };
        static const GLfloat inverseSquareVertices[] = {
            -1.f, 1.f,
            1.f, 1.f,
            -1.f, -1.f,
            1.f, -1.f
        };
        static const GLfloat textureVertices[] = {
            0.0f,  0.0f,
            1.0f,  0.0f,
            0.0f,  1.0f,
            1.0f,  1.0f,
        };

        m_squareVerticesOffset = 0;
        m_inverseSquareVerticesOffset = sizeof(squareVertices);
        m_textureVerticesOffset = sizeof(squareVertices) + sizeof(textureVertices);

        m_buffer.allocate(sizeof(squareVertices) + sizeof(inverseSquareVertices) + sizeof(textureVertices));
        m_buffer.write(m_squareVerticesOffset, squareVertices, sizeof(squareVertices));
        m_buffer.write(m_inverseSquareVerticesOffset, inverseSquareVertices, sizeof(inverseSquareVertices));
        m_buffer.write(m_textureVerticesOffset, textureVertices, sizeof(textureVertices));

        m_blitProgram->setAttributeBuffer(1, GL_FLOAT, m_textureVerticesOffset, 2);

        m_textureWrap = m_context->context()->functions()->hasOpenGLFeature(QOpenGLFunctions::NPOTTextureRepeat) ? GL_REPEAT : GL_CLAMP_TO_EDGE;
    }
    ~DecorationsBlitter()
    {
        delete m_blitProgram;
    }
    void blit(QWaylandEglWindow *window)
    {
        QOpenGLTextureCache *cache = QOpenGLTextureCache::cacheForContext(m_context->context());

        QSize surfaceSize = window->surfaceSize();
        qreal scale = window->scale() ;
        glViewport(0, 0, surfaceSize.width() * scale, surfaceSize.height() * scale);

        //Draw Decoration
        if (auto *decoration = window->decoration()) {
            m_blitProgram->setAttributeBuffer(0, GL_FLOAT, m_inverseSquareVerticesOffset, 2);
            QImage decorationImage = decoration->contentImage();
            cache->bindTexture(m_context->context(), decorationImage);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, m_textureWrap);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, m_textureWrap);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }

        //Draw Content
        m_blitProgram->setAttributeBuffer(0, GL_FLOAT, m_squareVerticesOffset, 2);
        glBindTexture(GL_TEXTURE_2D, window->contentTexture());
        QRect r = window->contentsRect();
        glViewport(r.x() * scale, r.y() * scale, r.width() * scale, r.height() * scale);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    QOpenGLShaderProgram *m_blitProgram = nullptr;
    QWaylandGLContext *m_context = nullptr;
    QOpenGLBuffer m_buffer;
    int m_squareVerticesOffset;
    int m_inverseSquareVerticesOffset;
    int m_textureVerticesOffset;
    int m_textureWrap;
};

QWaylandGLContext::QWaylandGLContext()
    : QEGLPlatformContext(),
      m_api(EGL_OPENGL_ES_API)
{
}

QWaylandGLContext::QWaylandGLContext(EGLDisplay eglDisplay, QWaylandDisplay *display,
                                     const QSurfaceFormat &fmt, QPlatformOpenGLContext *share)
    : QEGLPlatformContext(fmt, share, eglDisplay)
    , m_display(display)
{
    m_reconnectionWatcher = QObject::connect(m_display, &QWaylandDisplay::connected,
                                             m_display, [this] { invalidateContext(); });

    switch (format().renderableType()) {
    case QSurfaceFormat::OpenVG:
        m_api = EGL_OPENVG_API;
        break;
#ifdef EGL_VERSION_1_4
    case QSurfaceFormat::OpenGL:
        m_api = EGL_OPENGL_API;
        break;
#endif // EGL_VERSION_1_4
    default:
        m_api = EGL_OPENGL_ES_API;
        break;
    }

    if (m_display->supportsWindowDecoration()) {
        // Create an EGL context for the decorations blitter. By using a dedicated context we are free to
        // change its state and we also use OpenGL ES 2 API independently to what the app is using to draw.
        QList<EGLint> eglDecorationsContextAttrs = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
        m_decorationsContext = eglCreateContext(eglDisplay, eglConfig(), eglContext(),
                                                eglDecorationsContextAttrs.constData());
        if (m_decorationsContext == EGL_NO_CONTEXT)
            qWarning("QWaylandGLContext: Failed to create the decorations EGLContext. Decorations will not be drawn.");
    }

    EGLint a = EGL_MIN_SWAP_INTERVAL;
    EGLint b = EGL_MAX_SWAP_INTERVAL;
    if (!eglGetConfigAttrib(eglDisplay, eglConfig(), a, &a)
        || !eglGetConfigAttrib(eglDisplay, eglConfig(), b, &b) || a > 0) {
        m_supportNonBlockingSwap = false;
    }
    {
        bool ok;
        int supportNonBlockingSwap = qEnvironmentVariableIntValue("QT_WAYLAND_FORCE_NONBLOCKING_SWAP_SUPPORT", &ok);
        if (ok)
            m_supportNonBlockingSwap = supportNonBlockingSwap != 0;
    }
    if (!m_supportNonBlockingSwap) {
        qCWarning(lcQpaWayland) << "Non-blocking swap buffers not supported."
                               << "Subsurface rendering can be affected."
                               << "It may also cause the event loop to freeze in some situations";
    }
}

EGLSurface QWaylandGLContext::createTemporaryOffscreenSurface()
{
    m_wlSurface = m_display->createSurface(nullptr);
    m_eglWindow = wl_egl_window_create(m_wlSurface, 1, 1);
#if QT_CONFIG(egl_extension_platform_wayland)
    EGLSurface eglSurface =
            eglCreatePlatformWindowSurface(eglDisplay(), eglConfig(), m_eglWindow, nullptr);
#else
    EGLSurface eglSurface = eglCreateWindowSurface(eglDisplay(), eglConfig(), m_eglWindow, nullptr);
#endif
    return eglSurface;
}

void QWaylandGLContext::destroyTemporaryOffscreenSurface(EGLSurface eglSurface)
{
    eglDestroySurface(eglDisplay(), eglSurface);
    wl_egl_window_destroy(m_eglWindow);
    m_eglWindow = nullptr;
    wl_surface_destroy(m_wlSurface);
    m_wlSurface = nullptr;
}

void QWaylandGLContext::runGLChecks()
{
    bool ok;
    const int doneCurrentWorkAround = qEnvironmentVariableIntValue("QT_WAYLAND_ENABLE_DONECURRENT_WORKAROUND", &ok);
    if (ok) {
        m_doneCurrentWorkAround = doneCurrentWorkAround != 0;
        if (m_doneCurrentWorkAround)
            qCDebug(lcQpaWayland) << "Enabling doneCurrent() workaround on request.";
        else
            qCDebug(lcQpaWayland) << "Disabling doneCurrent() workaround on request.";

    } else {
        // Note that even though there is an EGL context current here,
        // QOpenGLContext and QOpenGLFunctions are not yet usable at this stage.
        const char *renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
        if (renderer && strstr(renderer, "Mali")) {
            qCDebug(lcQpaWayland) << "Enabling doneCurrent() workaround for Mali GPU."
                                  << "Set QT_WAYLAND_ENABLE_DONECURRENT_WORKAROUND=0 to disable.";
            m_doneCurrentWorkAround = true;
        }
    }

    QEGLPlatformContext::runGLChecks();
}

QWaylandGLContext::~QWaylandGLContext()
{
    QObject::disconnect(m_reconnectionWatcher);
    delete m_blitter;
    m_blitter = nullptr;
    if (m_decorationsContext != EGL_NO_CONTEXT)
        eglDestroyContext(eglDisplay(), m_decorationsContext);
}

void QWaylandGLContext::beginFrame()
{
    Q_ASSERT(m_currentWindow != nullptr);
    if (m_supportNonBlockingSwap)
        m_currentWindow->beginFrame();
}

void QWaylandGLContext::endFrame()
{
    Q_ASSERT(m_currentWindow != nullptr);
    if (m_doneCurrentWorkAround) {
        doneCurrent();
        QOpenGLContextPrivate::setCurrentContext(nullptr);
    }

    if (m_supportNonBlockingSwap)
        m_currentWindow->endFrame();
}

bool QWaylandGLContext::makeCurrent(QPlatformSurface *surface)
{
    if (!isValid()) {
        return false;
    }

    // in QWaylandGLContext() we called eglBindAPI with the correct value. However,
    // eglBindAPI's documentation says:
    // "eglBindAPI defines the current rendering API for EGL in the thread it is called from"
    // Since makeCurrent() can be called from a different thread than the one we created the
    // context in make sure to call eglBindAPI in the correct thread.
    if (eglQueryAPI() != m_api) {
        eglBindAPI(m_api);
    }

    m_currentWindow = static_cast<QWaylandEglWindow *>(surface);

    QMutexLocker lock(m_currentWindow->eglSurfaceLock());
    EGLSurface eglSurface = m_currentWindow->eglSurface();

    if (!m_currentWindow->needToUpdateContentFBO() && (eglSurface != EGL_NO_SURFACE)) {
        if (!eglMakeCurrent(eglDisplay(), eglSurface, eglSurface, eglContext())) {
            qWarning("QWaylandGLContext::makeCurrent: eglError: %#x, this: %p \n", eglGetError(), this);
            return false;
        }
        return true;
    }

    if (eglSurface == EGL_NO_SURFACE) {
        m_currentWindow->updateSurface(true);
        eglSurface = m_currentWindow->eglSurface();
    }

    if (!eglMakeCurrent(eglDisplay(), eglSurface, eglSurface, eglContext())) {
        qWarning("QWaylandGLContext::makeCurrent: eglError: %#x, this: %p \n", eglGetError(), this);
        return false;
    }

    //### setCurrentContext will be called in QOpenGLContext::makeCurrent after this function
    // returns, but that's too late, as we need a current context in order to bind the content FBO.
    QOpenGLContextPrivate::setCurrentContext(context());
    m_currentWindow->bindContentFBO();

    return true;
}

void QWaylandGLContext::doneCurrent()
{
    eglMakeCurrent(eglDisplay(), EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

void QWaylandGLContext::swapBuffers(QPlatformSurface *surface)
{
    QWaylandEglWindow *window = static_cast<QWaylandEglWindow *>(surface);

    EGLSurface eglSurface = window->eglSurface();

    if (window->decoration()) {
        if (m_api != EGL_OPENGL_ES_API)
            eglBindAPI(EGL_OPENGL_ES_API);

        // save the current EGL content and surface to set it again after the blitter is done
        EGLDisplay currentDisplay = eglGetCurrentDisplay();
        EGLContext currentContext = eglGetCurrentContext();
        EGLSurface currentSurfaceDraw = eglGetCurrentSurface(EGL_DRAW);
        EGLSurface currentSurfaceRead = eglGetCurrentSurface(EGL_READ);
        eglMakeCurrent(eglDisplay(), eglSurface, eglSurface, m_decorationsContext);

        if (!m_blitter)
            m_blitter = new DecorationsBlitter(this);
        m_blitter->blit(window);

        if (m_api != EGL_OPENGL_ES_API)
            eglBindAPI(m_api);
        eglMakeCurrent(currentDisplay, currentSurfaceDraw, currentSurfaceRead, currentContext);
    }

    int swapInterval = m_supportNonBlockingSwap ? 0 : format().swapInterval();
    eglSwapInterval(eglDisplay(), swapInterval);
    if (swapInterval == 0 && format().swapInterval() > 0) {
        // Emulating a blocking swap
        glFlush(); // Flush before waiting so we can swap more quickly when the frame event arrives
        window->waitForFrameSync(100);
    }
    window->handleUpdate();
    if (!eglSwapBuffers(eglDisplay(), eglSurface))
        qCWarning(lcQpaWayland, "eglSwapBuffers failed with %#x, surface: %p", eglGetError(), eglSurface);
}

GLuint QWaylandGLContext::defaultFramebufferObject(QPlatformSurface *surface) const
{
    return static_cast<QWaylandEglWindow *>(surface)->contentFBO();
}

QFunctionPointer QWaylandGLContext::getProcAddress(const char *procName)
{
    QFunctionPointer proc = (QFunctionPointer) eglGetProcAddress(procName);
    if (!proc)
        proc = (QFunctionPointer) dlsym(RTLD_DEFAULT, procName);
    return proc;
}

EGLSurface QWaylandGLContext::eglSurfaceForPlatformSurface(QPlatformSurface *surface)
{
    return static_cast<QWaylandEglWindow *>(surface)->eglSurface();
}

}

QT_END_NAMESPACE
