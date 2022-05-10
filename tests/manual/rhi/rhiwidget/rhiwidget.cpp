// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "rhiwidget_p.h"

#include <private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>
#include <private/qwidgetrepaintmanager_p.h>

/*!
    \class QRhiWidget
    \inmodule QtWidgets
    \since 6.x

    \brief The QRhiWidget class is a widget for rendering 3D graphics via an
    accelerated grapics API, such as Vulkan, Metal, or Direct 3D.

    QRhiWidget provides functionality for displaying 3D content rendered through
    the QRhi APIs within a QWidget-based application.

    QRhiWidget is expected to be subclassed. To render into the 2D texture that
    is implicitly created and managed by the QRhiWidget, subclasses should
    reimplement the virtual functions initialize() and render().

    The size of the texture will by default adapt to the size of the item. If a
    fixed size is preferred, set an explicit size specified in pixels by
    calling setExplicitSize().

    The QRhi for the widget's top-level window is configured to use a platform
    specific backend and graphics API by default: Metal on macOS and iOS,
    Direct 3D 11 on Windows, OpenGL otherwise. Call setApi() to override this.

    \note A single widget window can only use one QRhi backend, and so graphics
    API. If two QRhiWidget or QQuickWidget widgets in the window's widget
    hierarchy request different APIs, only one of them will function correctly.
 */

/*!
    Constructs a widget which is a child of \a parent, with widget flags set to \a f.
 */
QRhiWidget::QRhiWidget(QWidget *parent, Qt::WindowFlags f)
    : QWidget(*(new QRhiWidgetPrivate), parent, f)
{
    Q_D(QRhiWidget);
    if (Q_UNLIKELY(!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::RhiBasedRendering)))
        qWarning("QRhiWidget: QRhi is not supported on this platform.");
    else
        d->setRenderToTexture();

    d->config.setEnabled(true);
#if defined(Q_OS_DARWIN)
    d->config.setApi(QPlatformBackingStoreRhiConfig::Metal);
#elif defined(Q_OS_WIN)
    d->config.setApi(QPlatformBackingStoreRhiConfig::D3D11);
#else
    d->config.setApi(QPlatformBackingStoreRhiConfig::OpenGL);
#endif
}

/*!
    Destructor.
 */
QRhiWidget::~QRhiWidget()
{
    Q_D(QRhiWidget);
    // rhi resources must be destroyed here, cannot be left to the private dtor
    delete d->t;
    d->offscreenRenderer.reset();
}

/*!
    Handles resize events that are passed in the \a e event parameter. Calls
    the virtual function initialize().

    \note Avoid overriding this function in derived classes. If that is not
    feasible, make sure that QRhiWidget's implementation is invoked too.
    Otherwise the underlying texture object and related resources will not get
    resized properly and will lead to incorrect rendering.
 */
void QRhiWidget::resizeEvent(QResizeEvent *e)
{
    Q_D(QRhiWidget);

    if (e->size().isEmpty()) {
        d->noSize = true;
        return;
    }
    d->noSize = false;

    d->sendPaintEvent(QRect(QPoint(0, 0), size()));
}

/*!
    Handles paint events.

    Calling QWidget::update() will lead to sending a paint event \a e, and thus
    invoking this function. (NB this is asynchronous and will happen at some
    point after returning from update()). This function will then, after some
    preparation, call the virtual render() to update the contents of the
    QRhiWidget's associated texture. The widget's top-level window will then
    composite the texture with the rest of the window.
 */
void QRhiWidget::paintEvent(QPaintEvent *)
{
    Q_D(QRhiWidget);
    if (!updatesEnabled() || d->noSize)
        return;

    d->ensureRhi();
    if (!d->rhi) {
        qWarning("QRhiWidget: No QRhi");
        return;
    }

    const QSize prevSize = d->t ? d->t->pixelSize() : QSize();
    d->ensureTexture();
    if (!d->t)
        return;
    if (d->t->pixelSize() != prevSize)
        initialize(d->rhi, d->t);

    QRhiCommandBuffer *cb = nullptr;
    d->rhi->beginOffscreenFrame(&cb);
    render(cb);
    d->rhi->endOffscreenFrame();
}

/*!
  \reimp
*/
bool QRhiWidget::event(QEvent *e)
{
    Q_D(QRhiWidget);
    switch (e->type()) {
    case QEvent::WindowChangeInternal:
        // the QRhi will almost certainly change, prevent texture() from
        // returning the existing QRhiTexture in the meantime
        d->textureInvalid = true;
        break;
    case QEvent::Show:
        if (isVisible())
            d->sendPaintEvent(QRect(QPoint(0, 0), size()));
        break;
    default:
        break;
    }
    return QWidget::event(e);
}

QPlatformBackingStoreRhiConfig QRhiWidgetPrivate::rhiConfig() const
{
    return config;
}

void QRhiWidgetPrivate::ensureRhi()
{
    Q_Q(QRhiWidget);
    // the QRhi and infrastructure belongs to the top-level widget, not to this widget
    QWidget *tlw = q->window();
    QWidgetPrivate *wd = get(tlw);

    QRhi *currentRhi = nullptr;
    if (QWidgetRepaintManager *repaintManager = wd->maybeRepaintManager())
        currentRhi = repaintManager->rhi();

    if (currentRhi && currentRhi->backend() != QBackingStoreRhiSupport::apiToRhiBackend(config.api())) {
        qWarning("The top-level window is already using another graphics API for composition, "
                 "'%s' is not compatible with this widget",
                 currentRhi->backendName());
        return;
    }

    if (currentRhi && rhi && rhi != currentRhi) {
        // the texture belongs to the old rhi, drop it, this will also lead to
        // initialize() being called again
        delete t;
        t = nullptr;
        // if previously we created our own but now get a QRhi from the
        // top-level, then drop what we have and start using the top-level's
        if (rhi == offscreenRenderer.rhi())
            offscreenRenderer.reset();
    }

    rhi = currentRhi;
}

void QRhiWidgetPrivate::ensureTexture()
{
    Q_Q(QRhiWidget);

    QSize newSize = explicitSize;
    if (newSize.isEmpty())
        newSize = q->size() * q->devicePixelRatio();

    if (!t) {
        if (!rhi->isTextureFormatSupported(format))
            qWarning("QRhiWidget: The requested texture format is not supported by the graphics API implementation");
        t = rhi->newTexture(format, newSize, 1, QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource);
        if (!t->create()) {
            qWarning("Failed to create backing texture for QRhiWidget");
            delete t;
            t = nullptr;
            return;
        }
    }

    if (t->pixelSize() != newSize) {
        t->setPixelSize(newSize);
        if (!t->create())
            qWarning("Failed to rebuild texture for QRhiWidget after resizing");
    }

    textureInvalid = false;
}

/*!
    \return the currently set graphics API (QRhi backend).

    \sa setApi()
 */
QRhiWidget::Api QRhiWidget::api() const
{
    Q_D(const QRhiWidget);
    switch (d->config.api()) {
    case QPlatformBackingStoreRhiConfig::OpenGL:
        return OpenGL;
    case QPlatformBackingStoreRhiConfig::Metal:
        return Metal;
    case QPlatformBackingStoreRhiConfig::Vulkan:
        return Vulkan;
    case QPlatformBackingStoreRhiConfig::D3D11:
        return D3D11;
    default:
        return Null;
    }
}

/*!
    Sets the graphics API and QRhi backend to use to \a api.

    The default value depends on the platform: Metal on macOS and iOS, Direct
    3D 11 on Windows, OpenGL otherwise.

    \note This function must be called early enough, before the widget is added
    to a widget hierarchy and displayed on screen. For example, aim to call the
    function for the subclass constructor. If called too late, the function
    will have no effect.

    The \a api can only be set once for the widget and its top-level window,
    once it is done and takes effect, the window can only use that API and QRhi
    backend to render. Attempting to set another value, or to add another
    QRhiWidget with a different \a api will not function as expected.

    \sa setTextureFormat(), setDebugLayer(), api()
 */
void QRhiWidget::setApi(Api api)
{
    Q_D(QRhiWidget);
    switch (api) {
    case OpenGL:
        d->config.setApi(QPlatformBackingStoreRhiConfig::OpenGL);
        break;
    case Metal:
        d->config.setApi(QPlatformBackingStoreRhiConfig::Metal);
        break;
    case Vulkan:
        d->config.setApi(QPlatformBackingStoreRhiConfig::Vulkan);
        break;
    case D3D11:
        d->config.setApi(QPlatformBackingStoreRhiConfig::D3D11);
        break;
    default:
        d->config.setApi(QPlatformBackingStoreRhiConfig::Null);
        break;
    }
}

/*!
    \return true if a debug or validation layer will be requested if applicable
    to the graphics API in use.

    \sa setDebugLayer()
 */
bool QRhiWidget::isDebugLayerEnabled() const
{
    Q_D(const QRhiWidget);
    return d->config.isDebugLayerEnabled();
}

/*!
    Requests the debug or validation layer of the underlying graphics API
    when \a enable is true.

    Applicable for Vulkan and Direct 3D.

    \note This function must be called early enough, before the widget is added
    to a widget hierarchy and displayed on screen. For example, aim to call the
    function for the subclass constructor. If called too late, the function
    will have no effect.

    By default this is disabled.

    \sa setApi(), isDebugLayerEnabled()
 */
void QRhiWidget::setDebugLayer(bool enable)
{
    Q_D(QRhiWidget);
    d->config.setDebugLayer(enable);
}

/*!
    \return the currently set texture format.

    The default value is QRhiTexture::RGBA8.

    \sa setTextureFormat()
 */
QRhiTexture::Format QRhiWidget::textureFormat() const
{
    Q_D(const QRhiWidget);
    return d->format;
}

/*!
    Sets the associated texture's \a format.

    The default value is QRhiTexture::RGBA8. Only formats that are reported as
    supported from QRhi::isTextureFormatSupported() should be specified,
    rendering will not be functional otherwise.

    \note This function must be called early enough, before the widget is added
    to a widget hierarchy and displayed on screen. For example, aim to call the
    function for the subclass constructor. If called too late, the function
    will have no effect.

    \sa setApi(), textureFormat()
 */
void QRhiWidget::setTextureFormat(QRhiTexture::Format format)
{
    Q_D(QRhiWidget);
    d->format = format;
}

/*!
    \property QRhiWidget::explicitSize

    The fixed size (in pixels) of the QRhiWidget's associated texture.

    Only relevant when a fixed texture size is desired that does not depend on
    the widget's size.

    By default the value is a null QSize. A null or empty QSize means that the
    texture's size follows the QRhiWidget's size. (\c{texture size} = \c{widget
    size} * \c{device pixel ratio}).
 */

QSize QRhiWidget::explicitSize() const
{
    Q_D(const QRhiWidget);
    return d->explicitSize;
}

void QRhiWidget::setExplicitSize(const QSize &pixelSize)
{
    Q_D(QRhiWidget);
    if (d->explicitSize != pixelSize) {
        d->explicitSize = pixelSize;
        emit explicitSizeChanged(pixelSize);
        update();
    }
}

/*!
    Renders a new frame, reads the contents of the texture back, and returns it
    as a QImage.

    When an error occurs, a null QImage is returned.

    \note This function only supports reading back QRhiTexture::RGBA8 textures
    at the moment. For other formats, the implementer of render() should
    implement their own readback logic as they see fit.

    The returned QImage will have a format of QImage::Format_RGBA8888.
    QRhiWidget does not know the renderer's approach to blending and
    composition, and therefore cannot know if the output has alpha
    premultiplied.

    This function can also be called when the QRhiWidget is not added to a
    widget hierarchy belonging to an on-screen top-level window. This allows
    generating an image from a 3D rendering off-screen.

    \sa setTextureFormat()
 */
QImage QRhiWidget::grabTexture()
{
    Q_D(QRhiWidget);
    if (d->noSize)
        return QImage();

    if (d->format != QRhiTexture::RGBA8) {
        qWarning("QRhiWidget::grabTexture() only supports RGBA8 textures");
        return QImage();
    }

    d->ensureRhi();
    if (!d->rhi) {
        // The widget (and its parent chain, if any) may not be shown at
        // all, yet one may still want to use it for grabs. This is
        // ridiculous of course because the rendering infrastructure is
        // tied to the top-level widget that initializes upon expose, but
        // it has to be supported.
        d->offscreenRenderer.setConfig(d->config);
        // no window passed in, so no swapchain, but we get a functional QRhi which we own
        d->offscreenRenderer.create();
        d->rhi = d->offscreenRenderer.rhi();
        if (!d->rhi) {
            qWarning("QRhiWidget: Failed to create dedicated QRhi for grabbing");
            return QImage();
        }
    }

    const QSize prevSize = d->t ? d->t->pixelSize() : QSize();
    d->ensureTexture();
    if (!d->t)
        return QImage();
    if (d->t->pixelSize() != prevSize)
        initialize(d->rhi, d->t);

    QRhiReadbackResult readResult;
    bool readCompleted = false;
    readResult.completed = [&readCompleted] { readCompleted = true; };

    QRhiCommandBuffer *cb = nullptr;
    d->rhi->beginOffscreenFrame(&cb);
    render(cb);
    QRhiResourceUpdateBatch *readbackBatch = d->rhi->nextResourceUpdateBatch();
    readbackBatch->readBackTexture(d->t, &readResult);
    cb->resourceUpdate(readbackBatch);
    d->rhi->endOffscreenFrame();

    if (readCompleted) {
        QImage wrapperImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                            readResult.pixelSize.width(), readResult.pixelSize.height(),
                            QImage::Format_RGBA8888);
        QImage result = wrapperImage.copy();
        result.setDevicePixelRatio(devicePixelRatio());
        return result;
    } else {
        Q_UNREACHABLE();
    }

    return QImage();
}

/*!
    Called when the widget is initialized, when the associated texture's size
    changes, or when the QRhi and texture change for some reason.

    The implementation should be prepared that both \a rhi and \a outputTexture
    can change between invocations of this function, although this is not
    always going to happen in practice. When the widget size changes, this
    function is called with the same \a rhi and \a outputTexture as before, but
    \a outputTexture may have been rebuilt, meaning its
    \l{QRhiTexture::pixelSize()}{size} and the underlying native texture
    resource may be different than in the last invocation.

    One special case where the objects will be different is when performing a
    grabTexture() with a widget that is not yet shown, and then making the
    widget visible on-screen within a top-level widget. There the grab will
    happen with a dedicated QRhi that is then replaced with the top-level
    window's associated QRhi in subsequent initialize() and render()
    invocations.

    Another, more common case is when the widget is reparented so that it
    belongs to a new top-level window. In this case \a rhi and \a outputTexture
    will definitely be different in the subsequent call to this function. Is is
    then important that all existing QRhi resources are destroyed because they
    belong to the previous QRhi that should not be used by the widget anymore.

    Implementations will typically create or rebuild a QRhiTextureRenderTarget
    in order to allow the subsequent render() call to render into the texture.
    When a depth buffer is necessary create a QRhiRenderBuffer as well. The
    size if this must follow the size of \a outputTexture. A compact and
    efficient way for this is the following:

    \code
    if (m_rhi != rhi) {
        // reset all resources (incl. m_ds, m_rt, m_rp)
    } else if (m_output != outputTexture) {
        // reset m_rt and m_rp
    }
    m_rhi = rhi;
    m_output = outputTexture;
    if (!m_ds) {
        // no depth-stencil buffer yet, create one
        m_ds = m_rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, m_output->pixelSize());
        m_ds->create();
    } else if (m_ds->pixelSize() != m_output->pixelSize()) {
        // the size has changed, update the size and rebuild
        m_ds->setPixelSize(m_output->pixelSize());
        m_ds->create();
    }
    if (!m_rt) {
        m_rt = m_rhi->newTextureRenderTarget({ { m_output }, m_ds });
        m_rp = m_rt->newCompatibleRenderPassDescriptor();
        m_rt->setRenderPassDescriptor(m_rp);
        m_rt->create();
    }
    \endcode

    The above snippet is also prepared for \a rhi and \a outputTexture changing
    between invocations, via the checks at the beginning of the function.

    The created resources are expected to be released in the destructor
    implementation of the subclass. \a rhi and \a outputTexture are not owned
    by, and are guaranteed to outlive the QRhiWidget.

    \sa render()
 */
void QRhiWidget::initialize(QRhi *rhi, QRhiTexture *outputTexture)
{
    Q_UNUSED(rhi);
    Q_UNUSED(outputTexture);
}

/*!
    Called when the widget contents (i.e. the contents of the texture) need
    updating.

    There is always at least one call to initialize() before this function is
    called.

    To request updates, call QWidget::update(). Calling update() from within
    render() will lead to updating continuously, throttled by vsync.

    \a cb is the QRhiCommandBuffer for the current frame of the Qt Quick
    scenegraph. The function is called with a frame being recorded, but without
    an active render pass.

    \sa initialize()
 */
void QRhiWidget::render(QRhiCommandBuffer *cb)
{
    Q_UNUSED(cb);
}
