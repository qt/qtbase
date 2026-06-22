// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qohosplatformbackingstoregl.h>

#include <QtGui/private/qhighdpiscaling_p.h>
#include <QtOpenGL/private/qopenglpaintdevice_p.h>
#include <QtGui/qopenglcontext.h>
#include <QtOpenGL/qopenglframebufferobject.h>
#include <QtGui/qopenglfunctions.h>
#include <QtOpenGL/qopengltextureblitter.h>
#include <memory>
#include <qohosplatformscreen.h>
#include <qohosplatformwindow.h>
#include <render/qohosview.h>

QT_BEGIN_NAMESPACE

namespace
{

QMatrix3x3 makeSourceTransform(const QRect &targetSubTexture, const QSizeF &srcTextureSize)
{
    auto bottomLeftOffset = QPointF(
        targetSubTexture.x(),
        srcTextureSize.height() - targetSubTexture.bottomRight().y() - 1.0f);
    auto normalizedOffset = QPointF(
        bottomLeftOffset.x() / srcTextureSize.width(),
        bottomLeftOffset.y() / srcTextureSize.height());

    QMatrix3x3 result;
    result(0, 2) = static_cast<float>(normalizedOffset.x());
    result(1, 2) = static_cast<float>(normalizedOffset.y());

    QSizeF textureScaleUv{
        targetSubTexture.width() / srcTextureSize.width(),
        targetSubTexture.height() / srcTextureSize.height()};

    result(0, 0) = static_cast<float>(textureScaleUv.width());
    result(1, 1) = static_cast<float>(textureScaleUv.height());

    return result;
}

// NOTE - We use std::unique_ptrs of those objects
// within the class.
// Those helper functions are explicitly created to avoid
// problems with calling unique_ptr<O>.release() unique_ptr<O>->release()
void unbindFrameBufferObject(QOpenGLFramebufferObject &fbo)
{
    fbo.release();
}

void unbindTextureBlitter(QOpenGLTextureBlitter &blitter)
{
    blitter.release();
}

void setViewportAndClearColorBuffer(QOpenGLContext &glCtx, const QSize &viewportSize, const QColor &clearColor)
{
    auto *glFuncs = glCtx.functions();
    glFuncs->glClearColor(
        static_cast<float>(clearColor.redF()),
        static_cast<float>(clearColor.greenF()),
        static_cast<float>(clearColor.blueF()),
        static_cast<float>(clearColor.alphaF()));
    glFuncs->glViewport(0, 0, viewportSize.width(), viewportSize.height());
    glFuncs->glClear(GL_COLOR_BUFFER_BIT);
}

void blitFramebufferToWindow(
    QOpenGLTextureBlitter &blitter,
    QOpenGLFramebufferObject &framebuffer,
    const QRect &srcRect)
{
    static const QMatrix4x4 identityMatrix = {};

    auto dstTransform = identityMatrix;
    auto srcTransform = makeSourceTransform(srcRect, framebuffer.size());

    blitter.bind();
    blitter.blit(framebuffer.texture(), dstTransform, srcTransform);
    unbindTextureBlitter(blitter);
}

struct RenderContextData
{
    QWindow *qWindow;
    QOhosPlatformWindow *platformWindow;
    QOhosView *view;
    QRect unscaledWindowGeometry;
    QSize surfaceResolution;

    static QOhosOptional<RenderContextData> tryCreateForQWindow(QWindow *window);
};

QOhosOptional<RenderContextData> RenderContextData::tryCreateForQWindow(QWindow *window)
{
    auto *platformWindow = QOhosPlatformWindow::fromQWindowOrNull(window);
    if (platformWindow == nullptr)
        return {};

    auto *view = platformWindow->ownedViewOrNull();
    if (view == nullptr)
        return {};

    auto optSurfaceResolution = view->surfaceResolution();
    return optSurfaceResolution.has_value()
        ? makeQOhosOptional(
            RenderContextData{
                .qWindow = window,
                .platformWindow = platformWindow,
                .view = view,
                .unscaledWindowGeometry = platformWindow->windowGeometry(),
                .surfaceResolution = optSurfaceResolution.value(),
            })
        : makeEmptyQOhosOptional();
}

class QOhosPlatformBackingStoreGL : public QPlatformBackingStore
{
public:
    explicit QOhosPlatformBackingStoreGL(QWindow *window);
    ~QOhosPlatformBackingStoreGL() override = default;

    QPaintDevice *paintDevice() override;
    void flush(
        QWindow *targetWindow, const QRegion &relToParentWindowRegion,
        const QPoint &relToRootWindowOffset) override;
    void resize(const QSize &size, const QRegion &staticContents) override;

    void beginPaint(const QRegion &) override;
    void endPaint() override;
    QImage toImage() const override;

private:
    void tryRecreatePaintDeviceIfNeeded();

    std::unique_ptr<QPaintDevice> m_dummyPaintDevice;
    std::unique_ptr<QOpenGLPaintDevice> m_paintDevice;
    std::unique_ptr<QOpenGLContext> m_glContext;
    std::unique_ptr<QOpenGLFramebufferObject> m_framebuffer;
    std::unique_ptr<QOpenGLTextureBlitter> m_blitter;
    QOhosOptional<QSize> m_pendingResizeRequest;
};


QOhosPlatformBackingStoreGL::QOhosPlatformBackingStoreGL(QWindow *window)
    : QPlatformBackingStore(window)
    , m_dummyPaintDevice(std::make_unique<QImage>())
{
}

QPaintDevice *QOhosPlatformBackingStoreGL::paintDevice()
{
    return m_paintDevice ? m_paintDevice.get() : m_dummyPaintDevice.get();
}

void QOhosPlatformBackingStoreGL::flush(
    QWindow *targetWindow, const QRegion &relToParentWindowRegion,
    const QPoint &relToRootWindowOffset)
{
    Q_UNUSED(relToParentWindowRegion);

    auto optSrcWindowCtxData = RenderContextData::tryCreateForQWindow(window());
    auto optDstWidnowCtxData = RenderContextData::tryCreateForQWindow(targetWindow);

    if (!optSrcWindowCtxData.has_value() || !optDstWidnowCtxData.has_value())
        return;

    if (!m_glContext)
        return;

    auto dstWindowCtxData = optDstWidnowCtxData.value();

    unbindFrameBufferObject(*m_framebuffer);
    m_glContext->makeCurrent(dstWindowCtxData.qWindow);

    if (!m_blitter) {
        m_blitter = std::make_unique<QOpenGLTextureBlitter>();
        m_blitter->create();
    }

    auto dstSize = dstWindowCtxData.unscaledWindowGeometry.size();

    auto srcRect = QRect(
        relToRootWindowOffset, dstWindowCtxData.platformWindow->windowGeometry().size());

    setViewportAndClearColorBuffer(*m_glContext, dstSize, QColor(Qt::transparent));

    blitFramebufferToWindow(*m_blitter, *m_framebuffer, srcRect);

    m_glContext->swapBuffers(dstWindowCtxData.qWindow);
}

void QOhosPlatformBackingStoreGL::resize(const QSize &targetSize, const QRegion &)
{
    m_pendingResizeRequest = targetSize;
}

void QOhosPlatformBackingStoreGL::tryRecreatePaintDeviceIfNeeded()
{
    auto optRootWindowCtxData = RenderContextData::tryCreateForQWindow(window());
    if (!optRootWindowCtxData.has_value())
        return;

    auto rootWindowCtxData = optRootWindowCtxData.value();

    if (!m_glContext) {
        m_glContext = std::make_unique<QOpenGLContext>(rootWindowCtxData.qWindow);
        m_glContext->setFormat(rootWindowCtxData.qWindow->format());
        m_glContext->create();
    }

    m_glContext->makeCurrent(rootWindowCtxData.qWindow);

    auto targetFramebufferResolution = m_pendingResizeRequest.value_or(
        rootWindowCtxData.platformWindow->windowGeometry().size());
    if (!m_framebuffer || m_framebuffer->size() != targetFramebufferResolution) {
        m_framebuffer = std::make_unique<QOpenGLFramebufferObject>(targetFramebufferResolution);
        m_paintDevice = std::make_unique<QOpenGLPaintDevice>(targetFramebufferResolution);
        if (QHighDpiScaling::isActive()) {
            auto pixelDensity = static_cast<QOhosPlatformScreen *>(rootWindowCtxData.platformWindow->screen())->pixelScalingCoefficient();
            m_paintDevice->setDevicePixelRatio(pixelDensity);
        }

        m_framebuffer->bind();
        setViewportAndClearColorBuffer(*m_glContext, targetFramebufferResolution, QColor(Qt::transparent));
    }
}

void QOhosPlatformBackingStoreGL::beginPaint(const QRegion &)
{
    if (!m_framebuffer || m_pendingResizeRequest.has_value())
        tryRecreatePaintDeviceIfNeeded();

    if (!m_framebuffer)
        return;

    m_pendingResizeRequest = makeEmptyQOhosOptional();

    m_glContext->makeCurrent(window());
    m_framebuffer->bind();
}

void QOhosPlatformBackingStoreGL::endPaint()
{
}


QImage QOhosPlatformBackingStoreGL::toImage() const
{
    return m_framebuffer
        ? m_framebuffer->toImage()
        : QImage();
}

}

std::unique_ptr<QPlatformBackingStore> makeGlOhosPlatformBackingStore(QWindow *window)
{
    return std::make_unique<QOhosPlatformBackingStoreGL>(window);
}

QT_END_NAMESPACE
