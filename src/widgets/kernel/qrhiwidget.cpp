// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qrhiwidget_p.h"
#include <private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>
#include <private/qwidgetrepaintmanager_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QRhiWidget
    \inmodule QtWidgets
    \since 6.7

    \brief The QRhiWidget class is a widget for rendering 3D graphics via an
    accelerated grapics API, such as Vulkan, Metal, or Direct 3D.

    QRhiWidget provides functionality for displaying 3D content rendered
    through the \l QRhi APIs within a QWidget-based application. In many ways
    it is the portable equivalent of \l QOpenGLWidget that is not tied to a
    single 3D graphics API, but rather can function with all the APIs QRhi
    supports (such as, Direct 3D 11/12, Vulkan, Metal, and OpenGL).

    QRhiWidget is expected to be subclassed. To render into the 2D texture that
    is implicitly created and managed by the QRhiWidget, subclasses should
    reimplement the virtual functions initialize() and render().

    The size of the texture will by default adapt to the size of the widget. If
    a fixed size is preferred, set a fixed size specified in pixels by calling
    setFixedColorBufferSize().

    In addition to the texture serving as the color buffer, a depth/stencil
    buffer and a render target binding these together is maintained implicitly
    as well.

    The QRhi for the widget's top-level window is configured to use a
    platform-specific backend and graphics API by default: Metal on macOS and
    iOS, Direct 3D 11 on Windows, OpenGL otherwise. Call setApi() to override
    this.

    \note A single widget window can only use one QRhi backend, and so one
    single 3D graphics API. If two QRhiWidget or QQuickWidget widgets in the
    window's widget hierarchy request different APIs, only one of them will
    function correctly.

    \note While QRhiWidget is a public Qt API, the QRhi family of classes in
    the Qt Gui module, including QRhi, QShader and QShaderDescription, offer
    limited compatibility guarantees. There are no source or binary
    compatibility guarantees for these classes, meaning the API is only
    guaranteed to work with the Qt version the application was developed
    against. Source incompatible changes are however aimed to be kept at a
    minimum and will only be made in minor releases (6.7, 6.8, and so on).
    \c{qrhiwidget.h} does not directly include any QRhi-related headers. To use
    those classes when implementing a QRhiWidget subclass, link to
    \c{Qt::GuiPrivate} (if using CMake), and include the appropriate headers
    with the \c rhi prefix, for example \c{#include <rhi/qrhi.h>}.

    An example of a simple QRhiWidget subclass rendering a triangle is the
    following:

    \snippet qrhiwidget/rhiwidgetintro.cpp 0

    This is a widget that continuously requests updates, throttled by the
    presentation rate (vsync, depending on the screen refresh rate). If
    rendering continuously is not desired, the update() call in render() should
    be removed, and rather issued only when updating the rendered content is
    necessary. For example, if the rotation of the cube should be tied to the
    value of a QSlider, then connecting the slider's value change signal to a
    slot or lambda that forwards the new value and calls update() is
    sufficient.

    The vertex and fragment shaders are provided as Vulkan-style GLSL and must
    be processed first by the Qt shader infrastructure first. This is achieved
    either by running the \c qsb command-line tool manually, or by using the
    \l{Qt Shader Tools Build System Integration}{qt_add_shaders()} function in
    CMake. The QRhiWidget implementation loads these pre-processed \c{.qsb}
    files that are shipped with the application. See \l{Qt Shader Tools} for
    more information about Qt's shader translation infrastructure.

    The source code for these shaders could be the following:

    \c{color.vert}

    \snippet qrhiwidget/rhiwidgetintro.vert 0

    \c{color.frag}

    \snippet qrhiwidget/rhiwidgetintro.frag 0

    The result is a widget that shows the following:

    \image qrhiwidget-intro.jpg

    For a complete, minimal, introductory example check out the \l{Simple RHI
    Widget Example}.

    For an example with more functionality and demonstration of further
    concepts, see the \l{Cube RHI Widget Example}.

    QRhiWidget always involves rendering into a backing texture, not
    directly to the window (the surface or layer provided by the windowing
    system for the native window). This allows properly compositing the content
    with the rest of the widget-based UI, and offering a simple and compact
    API, making it easy to get started. All this comes at the expense of
    additional resources and a potential effect on performance. This is often
    perfectly acceptable in practice, but advanced users should keep in mind
    the pros and cons of the different approaches. Refer to the \l{RHI Window
    Example} and compare it with the \l{Simple RHI Widget Example} for details
    about the two approaches.

    Reparenting a QRhiWidget into a widget hierarchy that belongs to a
    different window (top-level widget), or making the QRhiWidget itself a
    top-level (by setting the parent to \nullptr), involves changing the
    associated QRhi (and potentially destroying the old one) while the
    QRhiWidget continues to stay alive and well. To support this, robust
    QRhiWidget implementations are expected to reimplement the
    releaseResources() virtual function as well, and drop their QRhi resources
    just as they do in the destructor. The \l{Cube RHI Widget Example}
    demonstrates this in practice.

    While not a primary use case, QRhiWidget also allows incorporating
    rendering code that directly uses a 3D graphics API such as Vulkan, Metal,
    Direct 3D, or OpenGL. See \l QRhiCommandBuffer::beginExternal() for details
    on recording native commands within a QRhi render pass, as well as
    \l QRhiTexture::createFrom() for a way to wrap an existing native texture and
    then use it with QRhi in a subsequent render pass. Note however that the
    configurability of the underlying graphics API (its device or context
    features, layers, extensions, etc.) is going to be limited since
    QRhiWidget's primary goal is to provide an environment suitable for
    QRhi-based rendering code, not to enable arbitrary, potentially complex,
    foreign rendering engines.

    \since 6.7

    \sa QRhi, QShader, QOpenGLWidget, {Simple RHI Widget Example}, {Cube RHI Widget Example}
 */

/*!
    \enum QRhiWidget::Api
    Specifies the 3D API and QRhi backend to use

    \value Null
    \value OpenGL
    \value Metal
    \value Vulkan
    \value Direct3D11
    \value Direct3D12

    \sa QRhi
 */

/*!
    \enum QRhiWidget::TextureFormat
    Specifies the format of the texture to which the QRhiWidget renders.

    \value RGBA8 See QRhiTexture::RGBA8.
    \value RGBA16F See QRhiTexture::RGBA16F.
    \value RGBA32F See QRhiTexture::RGBA32F.
    \value RGB10A2 See QRhiTexture::RGB10A2.

    \sa QRhiTexture
 */

/*!
    Constructs a widget which is a child of \a parent, with widget flags set to \a f.
 */
QRhiWidget::QRhiWidget(QWidget *parent, Qt::WindowFlags f)
    : QWidget(*(new QRhiWidgetPrivate), parent, f)
{
    Q_D(QRhiWidget);
    d->init();
}

/*!
 *  \internal
 */
QRhiWidget::QRhiWidget(QRhiWidgetPrivate &dd, QWidget *parent, Qt::WindowFlags f)
    : QWidget(dd, parent, f)
{
    Q_D(QRhiWidget);
    d->init();
}

/*!
 *  \internal
 */
void QRhiWidgetPrivate::init()
{
    if (Q_UNLIKELY(!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::RhiBasedRendering)))
        qWarning("QRhiWidget: QRhi is not supported on this platform.");
    else
        setRenderToTexture();

    config.setEnabled(true);
#if defined(Q_OS_DARWIN)
    config.setApi(QPlatformBackingStoreRhiConfig::Metal);
#elif defined(Q_OS_WIN)
    config.setApi(QPlatformBackingStoreRhiConfig::D3D11);
#else
    config.setApi(QPlatformBackingStoreRhiConfig::OpenGL);
#endif
}

/*!
    Destructor.
 */
QRhiWidget::~QRhiWidget()
{
    Q_D(QRhiWidget);

    if (d->rhi) {
        d->rhi->removeCleanupCallback(this);
        // rhi resources must be destroyed here, due to how QWidget teardown works;
        // it should not be left to the private object's destruction.
        d->resetRenderTargetObjects();
        d->resetColorBufferObjects();
        qDeleteAll(d->pendingDeletes);
    }

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
    invoking this function. The sending of the event is asynchronous and will
    happen at some point after returning from update(). This function will
    then, after some preparation, call the virtual render() to update the
    contents of the QRhiWidget's associated texture. The widget's top-level
    window will then composite the texture with the rest of the window.
 */
void QRhiWidget::paintEvent(QPaintEvent *)
{
    Q_D(QRhiWidget);
    if (!updatesEnabled() || d->noSize)
        return;

    d->ensureRhi();
    if (!d->rhi) {
        qWarning("QRhiWidget: No QRhi");
        emit renderFailed();
        return;
    }

    QRhiCommandBuffer *cb = nullptr;
    if (d->rhi->beginOffscreenFrame(&cb) != QRhi::FrameOpSuccess)
        return;

    bool needsInit = false;
    d->ensureTexture(&needsInit);
    if (d->colorTexture || d->msaaColorBuffer) {
        bool canRender = true;
        if (needsInit)
            canRender = d->invokeInitialize(cb);
        if (canRender)
            render(cb);
    }

    d->rhi->endOffscreenFrame();
}

/*!
  \reimp
*/
bool QRhiWidget::event(QEvent *e)
{
    Q_D(QRhiWidget);
    switch (e->type()) {
    case QEvent::WindowAboutToChangeInternal:
        // The QRhi will almost certainly change, prevent texture() from
        // returning the existing QRhiTexture in the meantime.
        d->textureInvalid = true;

        if (d->rhi && d->rhi != d->offscreenRenderer.rhi()) {
            // Drop the cleanup callback registered to the toplevel's rhi and
            // do the early-release, there may not be another chance to do
            // this, and the QRhi we have currently set may be destroyed by the
            // time we get to ensureRhi() again.
            d->rhi->removeCleanupCallback(this);
            releaseResources(); // notify the user code about the early-release
            d->releaseResources();
            // must _not_ null out d->rhi here, for proper interaction with ensureRhi()
        }

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

QWidgetPrivate::TextureData QRhiWidgetPrivate::texture() const
{
    // This is the only safe place to clear pendingDeletes, due to the
    // possibility of the texture returned in the previous invocation of this
    // function having been added to pendingDeletes, meaning the object then
    // needs to be valid until the next (this) invocation of this function.
    // (the exact object lifetime requirements depend on the
    // QWidget/RepaintManager internal implementation; for now avoid relying on
    // such details by clearing pendingDeletes only here, not in endCompose())
    qDeleteAll(pendingDeletes);
    pendingDeletes.clear();

    TextureData td;
    if (!textureInvalid)
        td.textureLeft = resolveTexture ? resolveTexture : colorTexture;
    return td;
}

QPlatformTextureList::Flags QRhiWidgetPrivate::textureListFlags()
{
    QPlatformTextureList::Flags flags = QWidgetPrivate::textureListFlags();
    if (mirrorVertically)
        flags |= QPlatformTextureList::MirrorVertically;
    return flags;
}

QPlatformBackingStoreRhiConfig QRhiWidgetPrivate::rhiConfig() const
{
    return config;
}

void QRhiWidgetPrivate::endCompose()
{
    // This function is called by QWidgetRepaintManager right after the
    // backingstore's QRhi-based flush returns. In practice that means after
    // the begin-endFrame() on the top-level window's swapchain.

    if (rhi) {
        Q_Q(QRhiWidget);
        emit q->frameSubmitted();
    }
}

// This is reimplemented to enable calling QWidget::grab() on the widget or an
// ancestor of it. At the same time, QRhiWidget provides its own
// grabFramebuffer() as well, mirroring QQuickWidget and QOpenGLWidget for
// consistency. In both types of grabs we end up in here.
QImage QRhiWidgetPrivate::grabFramebuffer()
{
    Q_Q(QRhiWidget);
    if (noSize)
        return QImage();

    ensureRhi();
    if (!rhi) {
        // The widget (and its parent chain, if any) may not be shown at
        // all, yet one may still want to use it for grabs. This is
        // ridiculous of course because the rendering infrastructure is
        // tied to the top-level widget that initializes upon expose, but
        // it has to be supported.
        offscreenRenderer.setConfig(config);
        // no window passed in, so no swapchain, but we get a functional QRhi which we own
        offscreenRenderer.create();
        rhi = offscreenRenderer.rhi();
        if (!rhi) {
            qWarning("QRhiWidget: Failed to create dedicated QRhi for grabbing");
            emit q->renderFailed();
            return QImage();
        }
    }

    QRhiCommandBuffer *cb = nullptr;
    if (rhi->beginOffscreenFrame(&cb) != QRhi::FrameOpSuccess)
        return QImage();

    QRhiReadbackResult readResult;
    bool readCompleted = false;
    bool needsInit = false;
    ensureTexture(&needsInit);

    if (colorTexture || msaaColorBuffer) {
        bool canRender = true;
        if (needsInit)
            canRender = invokeInitialize(cb);
        if (canRender)
            q->render(cb);

        QRhiResourceUpdateBatch *readbackBatch = rhi->nextResourceUpdateBatch();
        readResult.completed = [&readCompleted] { readCompleted = true; };
        readbackBatch->readBackTexture(resolveTexture ? resolveTexture : colorTexture, &readResult);
        cb->resourceUpdate(readbackBatch);
    }

    rhi->endOffscreenFrame();

    if (readCompleted) {
        QImage::Format imageFormat = QImage::Format_RGBA8888;
        switch (widgetTextureFormat) {
        case QRhiWidget::TextureFormat::RGBA8:
            break;
        case QRhiWidget::TextureFormat::RGBA16F:
            imageFormat = QImage::Format_RGBA16FPx4;
            break;
        case QRhiWidget::TextureFormat::RGBA32F:
            imageFormat = QImage::Format_RGBA32FPx4;
            break;
        case QRhiWidget::TextureFormat::RGB10A2:
            imageFormat = QImage::Format_BGR30;
            break;
        }
        QImage wrapperImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                            readResult.pixelSize.width(), readResult.pixelSize.height(),
                            imageFormat);
        QImage result;
        if (rhi->isYUpInFramebuffer())
            result = wrapperImage.flipped();
        else
            result = wrapperImage.copy();
        result.setDevicePixelRatio(q->devicePixelRatio());
        return result;
    } else {
        Q_UNREACHABLE();
    }

    return QImage();
}

void QRhiWidgetPrivate::resetColorBufferObjects()
{
    if (colorTexture) {
        pendingDeletes.append(colorTexture);
        colorTexture = nullptr;
    }
    if (msaaColorBuffer) {
        pendingDeletes.append(msaaColorBuffer);
        msaaColorBuffer = nullptr;
    }
    if (resolveTexture) {
        pendingDeletes.append(resolveTexture);
        resolveTexture = nullptr;
    }
}

void QRhiWidgetPrivate::resetRenderTargetObjects()
{
    if (renderTarget) {
        renderTarget->deleteLater();
        renderTarget = nullptr;
    }
    if (renderPassDescriptor) {
        renderPassDescriptor->deleteLater();
        renderPassDescriptor = nullptr;
    }
    if (depthStencilBuffer) {
        depthStencilBuffer->deleteLater();
        depthStencilBuffer = nullptr;
    }
}

void QRhiWidgetPrivate::releaseResources()
{
    resetRenderTargetObjects();
    resetColorBufferObjects();
    qDeleteAll(pendingDeletes);
    pendingDeletes.clear();
}

void QRhiWidgetPrivate::ensureRhi()
{
    Q_Q(QRhiWidget);
    QRhi *currentRhi = QWidgetPrivate::rhi();
    if (currentRhi && currentRhi->backend() != QBackingStoreRhiSupport::apiToRhiBackend(config.api())) {
        qWarning("The top-level window is already using another graphics API for composition, "
                 "'%s' is not compatible with this widget",
                 currentRhi->backendName());
        return;
    }

    // NB the rhi member may be an invalid object, the pointer can be used, but no deref
    if (currentRhi && rhi != currentRhi) {
        if (rhi) {
            // if previously we created our own but now get a QRhi from the
            // top-level, then drop what we have and start using the top-level's
            if (rhi == offscreenRenderer.rhi()) {
                q->releaseResources(); // notify the user code about the early-release
                releaseResources();
                offscreenRenderer.reset();
            } else {
                // rhi resources created by us all belong to the old rhi, drop them;
                // due to nulling out colorTexture this is also what ensures that
                // initialize() is going to be called again eventually
                resetRenderTargetObjects();
                resetColorBufferObjects();
            }
        }

        // Normally the widget gets destroyed before the QRhi (which is managed by
        // the top-level's backingstore). When reparenting between top-levels is
        // involved, that is not always the case. Therefore we use a per-widget rhi
        // cleanup callback to get notified when the QRhi is about to be destroyed
        // while the QRhiWidget is still around.
        currentRhi->addCleanupCallback(q, [q, this](QRhi *regRhi) {
            if (!QWidgetPrivate::get(q)->data.in_destructor && this->rhi == regRhi) {
                q->releaseResources(); // notify the user code about the early-release
                releaseResources();
                // must null out our ref, the QRhi object is going to be invalid
                this->rhi = nullptr;
            }
        });
    }

    rhi = currentRhi;
}

void QRhiWidgetPrivate::ensureTexture(bool *changed)
{
    Q_Q(QRhiWidget);

    QSize newSize = fixedSize;
    if (newSize.isEmpty())
        newSize = q->size() * q->devicePixelRatio();

    const int minTexSize = rhi->resourceLimit(QRhi::TextureSizeMin);
    const int maxTexSize = rhi->resourceLimit(QRhi::TextureSizeMax);
    newSize.setWidth(qMin(maxTexSize, qMax(minTexSize, newSize.width())));
    newSize.setHeight(qMin(maxTexSize, qMax(minTexSize, newSize.height())));

    if (colorTexture) {
        if (colorTexture->format() != rhiTextureFormat || colorTexture->sampleCount() != samples) {
            resetColorBufferObjects();
            // sample count change needs new depth-stencil, possibly a new
            // render target; format change needs new renderpassdescriptor;
            // therefore must drop the rest too
            resetRenderTargetObjects();
        }
    }

    if (msaaColorBuffer) {
        if (msaaColorBuffer->backingFormat() != rhiTextureFormat || msaaColorBuffer->sampleCount() != samples) {
            resetColorBufferObjects();
            // sample count change needs new depth-stencil, possibly a new
            // render target; format change needs new renderpassdescriptor;
            // therefore must drop the rest too
            resetRenderTargetObjects();
        }
    }

    if (!colorTexture && samples <= 1) {
        if (changed)
            *changed = true;
        if (!rhi->isTextureFormatSupported(rhiTextureFormat)) {
            qWarning("QRhiWidget: The requested texture format (%d) is not supported by the "
                     "underlying 3D graphics API implementation", int(rhiTextureFormat));
        }
        colorTexture = rhi->newTexture(rhiTextureFormat, newSize, samples, QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource);
        if (!colorTexture->create()) {
            qWarning("Failed to create backing texture for QRhiWidget");
            delete colorTexture;
            colorTexture = nullptr;
            return;
        }
    }

    if (samples > 1) {
        if (!msaaColorBuffer) {
            if (changed)
                *changed = true;
            if (!rhi->isFeatureSupported(QRhi::MultisampleRenderBuffer)) {
                qWarning("QRhiWidget: Multisample renderbuffers are reported as unsupported; "
                         "sample count %d will not work as expected", samples);
            }
            if (!rhi->isTextureFormatSupported(rhiTextureFormat)) {
                qWarning("QRhiWidget: The requested texture format (%d) is not supported by the "
                         "underlying 3D graphics API implementation", int(rhiTextureFormat));
            }
            msaaColorBuffer = rhi->newRenderBuffer(QRhiRenderBuffer::Color, newSize, samples, {}, rhiTextureFormat);
            if (!msaaColorBuffer->create()) {
                qWarning("Failed to create multisample color buffer for QRhiWidget");
                delete msaaColorBuffer;
                msaaColorBuffer = nullptr;
                return;
            }
        }
        if (!resolveTexture) {
            if (changed)
                *changed = true;
            resolveTexture = rhi->newTexture(rhiTextureFormat, newSize, 1, QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource);
            if (!resolveTexture->create()) {
                qWarning("Failed to create resolve texture for QRhiWidget");
                delete resolveTexture;
                resolveTexture = nullptr;
                return;
            }
        }
    } else if (resolveTexture) {
        resolveTexture->deleteLater();
        resolveTexture = nullptr;
    }

    if (colorTexture && colorTexture->pixelSize() != newSize) {
        if (changed)
            *changed = true;
        colorTexture->setPixelSize(newSize);
        if (!colorTexture->create())
            qWarning("Failed to rebuild texture for QRhiWidget after resizing");
    }

    if (msaaColorBuffer && msaaColorBuffer->pixelSize() != newSize) {
        if (changed)
            *changed = true;
        msaaColorBuffer->setPixelSize(newSize);
        if (!msaaColorBuffer->create())
            qWarning("Failed to rebuild multisample color buffer for QRhiWidget after resizing");
    }

    if (resolveTexture && resolveTexture->pixelSize() != newSize) {
        if (changed)
            *changed = true;
        resolveTexture->setPixelSize(newSize);
        if (!resolveTexture->create())
            qWarning("Failed to rebuild resolve texture for QRhiWidget after resizing");
    }

    textureInvalid = false;
}

bool QRhiWidgetPrivate::invokeInitialize(QRhiCommandBuffer *cb)
{
    Q_Q(QRhiWidget);
    if (!colorTexture && !msaaColorBuffer)
        return false;

    if (autoRenderTarget) {
        const QSize pixelSize = colorTexture ? colorTexture->pixelSize() : msaaColorBuffer->pixelSize();
        if (!depthStencilBuffer) {
            depthStencilBuffer = rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, pixelSize, samples);
            if (!depthStencilBuffer->create()) {
                qWarning("Failed to create depth-stencil buffer for QRhiWidget");
                resetRenderTargetObjects();
                return false;
            }
        } else if (depthStencilBuffer->pixelSize() != pixelSize) {
            depthStencilBuffer->setPixelSize(pixelSize);
            if (!depthStencilBuffer->create()) {
                qWarning("Failed to rebuild depth-stencil buffer for QRhiWidget with new size");
                return false;
            }
        }

        if (!renderTarget) {
            QRhiColorAttachment color0;
            if (colorTexture)
                color0.setTexture(colorTexture);
            else
                color0.setRenderBuffer(msaaColorBuffer);
            if (samples > 1)
                color0.setResolveTexture(resolveTexture);
            QRhiTextureRenderTargetDescription rtDesc(color0, depthStencilBuffer);
            renderTarget = rhi->newTextureRenderTarget(rtDesc);
            renderPassDescriptor = renderTarget->newCompatibleRenderPassDescriptor();
            renderTarget->setRenderPassDescriptor(renderPassDescriptor);
            if (!renderTarget->create()) {
                qWarning("Failed to create render target for QRhiWidget");
                resetRenderTargetObjects();
                return false;
            }
        }
    } else {
        resetRenderTargetObjects();
    }

    q->initialize(cb);

    return true;
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
        return Api::OpenGL;
    case QPlatformBackingStoreRhiConfig::Metal:
        return Api::Metal;
    case QPlatformBackingStoreRhiConfig::Vulkan:
        return Api::Vulkan;
    case QPlatformBackingStoreRhiConfig::D3D11:
        return Api::Direct3D11;
    case QPlatformBackingStoreRhiConfig::D3D12:
        return Api::Direct3D12;
    case QPlatformBackingStoreRhiConfig::Null:
        return Api::Null;
    }
    Q_UNREACHABLE_RETURN(Api::Null);
}

/*!
    Sets the graphics API and QRhi backend to use to \a api.

    \warning This function must be called early enough, before the widget is
    added to a widget hierarchy and displayed on screen. For example, aim to
    call the function for the subclass constructor. If called too late, the
    function will have no effect.

    The default value depends on the platform: Metal on macOS and iOS, Direct
    3D 11 on Windows, OpenGL otherwise.

    The \a api can only be set once for the widget and its top-level window,
    once it is done and takes effect, the window can only use that API and QRhi
    backend to render. Attempting to set another value, or to add another
    QRhiWidget with a different \a api will not function as expected.

    \sa setColorBufferFormat(), setDebugLayerEnabled(), api()
 */
void QRhiWidget::setApi(Api api)
{
    Q_D(QRhiWidget);
    switch (api) {
    case Api::OpenGL:
        d->config.setApi(QPlatformBackingStoreRhiConfig::OpenGL);
        break;
    case Api::Metal:
        d->config.setApi(QPlatformBackingStoreRhiConfig::Metal);
        break;
    case Api::Vulkan:
        d->config.setApi(QPlatformBackingStoreRhiConfig::Vulkan);
        break;
    case Api::Direct3D11:
        d->config.setApi(QPlatformBackingStoreRhiConfig::D3D11);
        break;
    case Api::Direct3D12:
        d->config.setApi(QPlatformBackingStoreRhiConfig::D3D12);
        break;
    case Api::Null:
        d->config.setApi(QPlatformBackingStoreRhiConfig::Null);
        break;
    }
}

/*!
    \return true if a debug or validation layer will be requested if applicable
    to the graphics API in use.

    \sa setDebugLayerEnabled()
 */
bool QRhiWidget::isDebugLayerEnabled() const
{
    Q_D(const QRhiWidget);
    return d->config.isDebugLayerEnabled();
}

/*!
    Requests the debug or validation layer of the underlying graphics API
    when \a enable is true.

    \warning This function must be called early enough, before the widget is added
    to a widget hierarchy and displayed on screen. For example, aim to call the
    function for the subclass constructor. If called too late, the function
    will have no effect.

    Applicable for Vulkan and Direct 3D.

    By default this is disabled.

    \sa setApi(), isDebugLayerEnabled()
 */
void QRhiWidget::setDebugLayerEnabled(bool enable)
{
    Q_D(QRhiWidget);
    d->config.setDebugLayer(enable);
}

/*!
    \property QRhiWidget::colorBufferFormat

    This property controls the texture format of the texture (or renderbuffer)
    used as the color buffer. The default value is TextureFormat::RGBA8.
    QRhiWidget supports rendering to a subset of the formats supported by \l
    QRhiTexture. Only formats that are reported as supported from \l
    QRhi::isTextureFormatSupported() should be specified, rendering will not be
    functional otherwise.

    \note Setting a new format when the widget is already initialized and has
    rendered implies that all QRhiGraphicsPipeline objects created by the
    renderer may become unusable, if the associated QRhiRenderPassDescriptor is
    now incompatible due to the different texture format. Similarly to changing
    \l sampleCount dynamically, this means that initialize() or render()
    implementations must then take care of releasing the existing pipelines and
    creating new ones.
 */

QRhiWidget::TextureFormat QRhiWidget::colorBufferFormat() const
{
    Q_D(const QRhiWidget);
    return d->widgetTextureFormat;
}

void QRhiWidget::setColorBufferFormat(TextureFormat format)
{
    Q_D(QRhiWidget);
    if (d->widgetTextureFormat != format) {
        d->widgetTextureFormat = format;
        switch (format) {
        case TextureFormat::RGBA8:
            d->rhiTextureFormat = QRhiTexture::RGBA8;
            break;
        case TextureFormat::RGBA16F:
            d->rhiTextureFormat = QRhiTexture::RGBA16F;
            break;
        case TextureFormat::RGBA32F:
            d->rhiTextureFormat = QRhiTexture::RGBA32F;
            break;
        case TextureFormat::RGB10A2:
            d->rhiTextureFormat = QRhiTexture::RGB10A2;
            break;
        }
        emit colorBufferFormatChanged(format);
        update();
    }
}

/*!
    \property QRhiWidget::sampleCount

    This property controls for sample count for multisample antialiasing.
    By default the value is \c 1 which means MSAA is disabled.

    Valid values are 1, 4, 8, and sometimes 16 and 32.
    \l QRhi::supportedSampleCounts() can be used to query the supported sample
    counts at run time, but typically applications should request 1 (no MSAA),
    4x (normal MSAA) or 8x (high MSAA).

    \note Setting a new value implies that all QRhiGraphicsPipeline objects
    created by the renderer must use the same sample count from then on.
    Existing QRhiGraphicsPipeline objects created with a different sample count
    must not be used anymore. When the value changes, all color and
    depth-stencil buffers are destroyed and recreated automatically, and
    initialize() is invoked again. However, when
    \l autoRenderTarget is \c false, it will be up to the application to
    manage this with regards to the depth-stencil buffer or additional color
    buffers.

    Changing the sample count from the default 1 to a higher value implies that
    colorTexture() becomes \nullptr and msaaColorBuffer() starts returning a
    valid object. Switching back to 1 (or 0), implies the opposite: in the next
    call to initialize() msaaColorBuffer() is going to return \nullptr, whereas
    colorTexture() becomes once again valid. In addition, resolveTexture()
    returns a valid (non-multisample) QRhiTexture whenever the sample count is
    greater than 1 (i.e., MSAA is in use).

    \sa msaaColorBuffer(), resolveTexture()
 */

int QRhiWidget::sampleCount() const
{
    Q_D(const QRhiWidget);
    return d->samples;
}

void QRhiWidget::setSampleCount(int samples)
{
    Q_D(QRhiWidget);
    if (d->samples != samples) {
        d->samples = samples;
        emit sampleCountChanged(samples);
        update();
    }
}

/*!
    \property QRhiWidget::fixedColorBufferSize

    The fixed size, in pixels, of the QRhiWidget's associated texture. Relevant
    when a fixed texture size is desired that does not depend on the widget's
    size. This size has no effect on the geometry of the widget (its size and
    placement within the top-level window), which means the texture's content
    will appear stretched (scaled up) or scaled down onto the widget's area.

    For example, setting a size that is exactly twice the widget's (pixel) size
    effectively performs 2x supersampling (rendering at twice the resolution
    and then implicitly scaling down when texturing the quad corresponding to
    the widget in the window).

    By default the value is a null QSize. A null or empty QSize means that the
    texture's size follows the QRhiWidget's size. (\c{texture size} = \c{widget
    size} * \c{device pixel ratio}).
 */

QSize QRhiWidget::fixedColorBufferSize() const
{
    Q_D(const QRhiWidget);
    return d->fixedSize;
}

void QRhiWidget::setFixedColorBufferSize(QSize pixelSize)
{
    Q_D(QRhiWidget);
    if (d->fixedSize != pixelSize) {
        d->fixedSize = pixelSize;
        emit fixedColorBufferSizeChanged(pixelSize);
        update();
    }
}

/*!
    \property QRhiWidget::mirrorVertically

    When enabled, flips the image around the X axis when compositing the
    QRhiWidget's backing texture with the rest of the widget content in the
    top-level window.

    The default value is \c false.
 */

bool QRhiWidget::isMirrorVerticallyEnabled() const
{
    Q_D(const QRhiWidget);
    return d->mirrorVertically;
}

void QRhiWidget::setMirrorVertically(bool enabled)
{
    Q_D(QRhiWidget);
    if (d->mirrorVertically != enabled) {
        d->mirrorVertically = enabled;
        emit mirrorVerticallyChanged(enabled);
        update();
    }
}

/*!
    \property QRhiWidget::autoRenderTarget

    The current setting for automatic depth-stencil buffer and render
    target maintenance.

    By default the value is \c true.
 */
bool QRhiWidget::isAutoRenderTargetEnabled() const
{
    Q_D(const QRhiWidget);
    return d->autoRenderTarget;
}

/*!
    Controls if a depth-stencil QRhiRenderBuffer and a QRhiTextureRenderTarget
    is created and maintained automatically by the widget. The default value is
    \c true.

    In automatic mode, the size and sample count of the depth-stencil buffer
    follows the color buffer texture's settings. In non-automatic mode,
    renderTarget() and depthStencilBuffer() always return \nullptr and it is
    then up to the application's implementation of initialize() to take care of
    setting up and managing these objects.

    Call this function with \a enabled set to \c false early on, for example in
    the derived class' constructor, to disable the automatic mode.
 */
void QRhiWidget::setAutoRenderTarget(bool enabled)
{
    Q_D(QRhiWidget);
    if (d->autoRenderTarget != enabled) {
        d->autoRenderTarget = enabled;
        update();
    }
}

/*!
    Renders a new frame, reads the contents of the texture back, and returns it
    as a QImage.

    When an error occurs, a null QImage is returned.

    The returned QImage will have a format of QImage::Format_RGBA8888,
    QImage::Format_RGBA16FPx4, QImage::Format_RGBA32FPx4, or
    QImage::Format_BGR30, depending on colorBufferFormat().

    QRhiWidget does not know the renderer's approach to blending and
    composition, and therefore cannot know if the output has alpha
    premultiplied in the RGB color values. Thus \c{_Premultiplied} QImage
    formats are never used for the returned QImage, even when it would be
    appropriate. It is up to the caller to reinterpret the resulting data as it
    sees fit.

    \note This function can also be called when the QRhiWidget is not added to
    a widget hierarchy belonging to an on-screen top-level window. This allows
    generating an image from a 3D rendering off-screen.

    The function is named grabFramebuffer() for consistency with QOpenGLWidget
    and QQuickWidget. It is not the only way to get CPU-side image data out of
    the QRhiWidget's content: calling \l QWidget::grab() on a QRhiWidget, or an
    ancestor of it, is functional as well (returning a QPixmap). Besides
    working directly with QImage, another advantage of grabFramebuffer() is
    that it may be slightly more performant, simply because it does not have to
    go through the rest of QWidget infrastructure but can right away trigger
    rendering a new frame and then do the readback.

    \sa setColorBufferFormat()
 */
QImage QRhiWidget::grabFramebuffer() const
{
    return const_cast<QRhiWidgetPrivate *>(d_func())->grabFramebuffer();
}

/*!
    Called when the widget is initialized for the first time, when the
    associated texture's size, format, or sample count changes, or when the
    QRhi and texture change for any reason. The function is expected to
    maintain (create if not yet created, adjust and rebuild if the size has
    changed) the graphics resources used by the rendering code in render().

    To query the QRhi, QRhiTexture, and other related objects, call rhi(),
    colorTexture(), depthStencilBuffer(), and renderTarget().

    When the widget size changes, the QRhi object, the color buffer texture,
    and the depth stencil buffer objects are all the same instances (so the
    getters return the same pointers) as before, but the color and
    depth/stencil buffers will likely have been rebuilt, meaning the
    \l{QRhiTexture::pixelSize()}{size} and the underlying native texture
    resource may be different than in the last invocation.

    Reimplementations should also be prepared that the QRhi object and the
    color buffer texture may change between invocations of this function. One
    special case where the objects will be different is when performing a
    grabFramebuffer() with a widget that is not yet shown, and then making the
    widget visible on-screen within a top-level widget. There the grab will
    happen with a dedicated QRhi that is then replaced with the top-level
    window's associated QRhi in subsequent initialize() and render()
    invocations. Another, more common case is when the widget is reparented so
    that it belongs to a new top-level window. In this case the QRhi and all
    related resources managed by the QRhiWidget will be different instances
    than before in the subsequent call to this function. Is is then important
    that all existing QRhi resources previously created by the subclass are
    destroyed because they belong to the previous QRhi that should not be used
    by the widget anymore.

    When \l autoRenderTarget is \c true, which is the default, a
    depth-stencil QRhiRenderBuffer and a QRhiTextureRenderTarget associated
    with colorTexture() (or msaaColorBuffer()) and the depth-stencil buffer are
    created and managed automatically. Reimplementations of initialize() and
    render() can query those objects via depthStencilBuffer() and
    renderTarget(). When \l autoRenderTarget is set to \c false, these
    objects are no longer created and managed automatically. Rather, it will be
    up the the initialize() implementation to create buffers and set up the
    render target as it sees fit. When manually managing additional color or
    depth-stencil attachments for the render target, their size and sample
    count must always follow the size and sample count of colorTexture() /
    msaaColorBuffer(), otherwise rendering or 3D API validation errors may
    occur.

    The subclass-created graphics resources are expected to be released in the
    destructor implementation of the subclass.

    \a cb is the QRhiCommandBuffer for the current frame of the widget. The
    function is called with a frame being recorded, but without an active
    render pass. The command buffer is provided primarily to allow enqueuing
    \l{QRhiCommandBuffer::resourceUpdate()}{resource updates} without deferring
    to render().

    \sa render()
 */
void QRhiWidget::initialize(QRhiCommandBuffer *cb)
{
    Q_UNUSED(cb);
}

/*!
    Called when the widget contents (i.e. the contents of the texture) need
    updating.

    There is always at least one call to initialize() before this function is
    called.

    To request updates, call QWidget::update(). Calling update() from within
    render() will lead to updating continuously, throttled by vsync.

    \a cb is the QRhiCommandBuffer for the current frame of the widget. The
    function is called with a frame being recorded, but without an active
    render pass.

    \sa initialize()
 */
void QRhiWidget::render(QRhiCommandBuffer *cb)
{
    Q_UNUSED(cb);
}

/*!
    Called when the need to early-release the graphics resources arises.

    This normally does not happen for a QRhiWidget that is added to a top-level
    widget's child hierarchy and it then stays there for the rest of its and
    the top-level's lifetime. Thus in many cases there is no need to
    reimplement this function, e.g. because the application only ever has a
    single top-level widget (native window). However, when reparenting of the
    widget (or an ancestor of it) is involved, reimplementing this function
    will become necessary in robust, well-written QRhiWidget subclasses.

    When this function is called, the implementation is expected to destroy all
    QRhi resources (QRhiBuffer, QRhiTexture, etc. objects), similarly to how it
    is expected to do this in the destructor. Nulling out, using a smart
    pointer, or setting a \c{resources-invalid} flag is going to be required as
    well, because initialize() will eventually get called afterwards. Note
    however that deferring the releasing of resources to the subsequent
    initialize() is wrong. If this function is called, the resource must be
    dropped before returning. Also note that implementing this function does
    not replace the class destructor (or smart pointers): the graphics
    resources must still be released in both.

    See the \l{Cube RHI Widget Example} for an example of this in action. There
    the button that toggles the QRhiWidget between being a child widget (due to
    having a parent widget) and being a top-level widget (due to having no
    parent widget), will trigger invoking this function since the associated
    top-level widget, native window, and QRhi all change during the lifetime of
    the QRhiWidget, with the previously used QRhi getting destroyed which
    implies an early-release of the associated resources managed by the
    still-alive QRhiWidget.

    Another case when this function is called is when grabFramebuffer() is used
    with a QRhiWidget that is not added to a visible window, i.e. the rendering
    is performed offscreen. If later on this QRhiWidget is made visible, or
    added to a visible widget hierarchy, the associated QRhi will change from
    the temporary one used for offscreen rendering to the window's dedicated
    one, thus triggering this function as well.

    \sa initialize()
 */
void QRhiWidget::releaseResources()
{
}

/*!
    \return the current QRhi object.

    Must only be called from initialize() and render().
 */
QRhi *QRhiWidget::rhi() const
{
    Q_D(const QRhiWidget);
    return d->rhi;
}

/*!
    \return the texture serving as the color buffer for the widget.

    Must only be called from initialize() and render().

    Unlike the depth-stencil buffer and the QRhiRenderTarget, this texture is
    always available and is managed by the QRhiWidget, independent of the value
    of \l autoRenderTarget.

    \note When \l sampleCount is larger than 1, and so multisample antialiasing
    is enabled, the return value is \nullptr. Instead, query the
    \l QRhiRenderBuffer by calling msaaColorBuffer().

    \note The backing texture size and sample count can also be queried via the
    QRhiRenderTarget returned from renderTarget(). This can be more convenient
    and compact than querying from the QRhiTexture or QRhiRenderBuffer, because
    it works regardless of multisampling is in use or not.

    \sa msaaColorBuffer(), depthStencilBuffer(), renderTarget(), resolveTexture()
 */
QRhiTexture *QRhiWidget::colorTexture() const
{
    Q_D(const QRhiWidget);
    return d->colorTexture;
}

/*!
    \return the renderbuffer serving as the multisample color buffer for the widget.

    Must only be called from initialize() and render().

    When \l sampleCount is larger than 1, and so multisample antialising is
    enabled, the returned QRhiRenderBuffer has a matching sample count and
    serves as the color buffer. Graphics pipelines used to render into this
    buffer must be created with the same sample count, and the depth-stencil
    buffer's sample count must match as well. The multisample content is
    expected to be resolved into the texture returned from resolveTexture().
    When \l autoRenderTarget is
    \c true, renderTarget() is set up automatically to do this, by setting up
    msaaColorBuffer() as the \l{QRhiColorAttachment::renderBuffer()}{renderbuffer} of
    color attachment 0 and resolveTexture() as its
    \l{QRhiColorAttachment::resolveTexture()}{resolveTexture}.

    When MSAA is not in use, the return value is \nullptr. Use colorTexture()
    instead then.

    Depending on the underlying 3D graphics API, there may be no practical
    difference between multisample textures and color renderbuffers with a
    sample count larger than 1 (QRhi may just map both to the same native
    resource type). Some older APIs however may differentiate between textures
    and renderbuffers. In order to support OpenGL ES 3.0, where multisample
    renderbuffers are available, but multisample textures are not, QRhiWidget
    always performs MSAA by using a multisample QRhiRenderBuffer as the color
    attachment (and never a multisample QRhiTexture).

    \note The backing texture size and sample count can also be queried via the
    QRhiRenderTarget returned from renderTarget(). This can be more convenient
    and compact than querying from the QRhiTexture or QRhiRenderBuffer, because
    it works regardless of multisampling is in use or not.

    \sa colorTexture(), depthStencilBuffer(), renderTarget(), resolveTexture()
 */
QRhiRenderBuffer *QRhiWidget::msaaColorBuffer() const
{
    Q_D(const QRhiWidget);
    return d->msaaColorBuffer;
}

/*!
    \return the non-multisample texture to which the multisample content is resolved.

    The result is \nullptr when multisample antialiasing is not enabled.

    Must only be called from initialize() and render().

    With MSAA enabled, this is the texture that gets composited with the rest
    of the QWidget content on-screen. However, the QRhiWidget's rendering must
    target the (multisample) QRhiRenderBuffer returned from
    msaaColorBuffer(). When
    \l autoRenderTarget is \c true, this is taken care of by the
    QRhiRenderTarget returned from renderTarget(). Otherwise, it is up to the
    subclass code to correctly configure a render target object with both the
    color buffer and resolve textures.

    \sa colorTexture()
 */
QRhiTexture *QRhiWidget::resolveTexture() const
{
    Q_D(const QRhiWidget);
    return d->resolveTexture;
}

/*!
    \return the depth-stencil buffer used by the widget's rendering.

    Must only be called from initialize() and render().

    Available only when \l autoRenderTarget is \c true. Otherwise the
    returned value is \nullptr and it is up the reimplementation of
    initialize() to create and manage a depth-stencil buffer and a
    QRhiTextureRenderTarget.

    \sa colorTexture(), renderTarget()
 */
QRhiRenderBuffer *QRhiWidget::depthStencilBuffer() const
{
    Q_D(const QRhiWidget);
    return d->depthStencilBuffer;
}

/*!
    \return the render target object that must be used with
    \l QRhiCommandBuffer::beginPass() in reimplementations of render().

    Must only be called from initialize() and render().

    Available only when \l autoRenderTarget is \c true. Otherwise the
    returned value is \nullptr and it is up the reimplementation of
    initialize() to create and manage a depth-stencil buffer and a
    QRhiTextureRenderTarget.

    When creating \l{QRhiGraphicsPipeline}{graphics pipelines}, a
    QRhiRenderPassDescriptor is needed. This can be queried from the returned
    QRhiTextureRenderTarget by calling
    \l{QRhiTextureRenderTarget::renderPassDescriptor()}{renderPassDescriptor()}.

    \sa colorTexture(), depthStencilBuffer()
 */
QRhiRenderTarget *QRhiWidget::renderTarget() const
{
    Q_D(const QRhiWidget);
    return d->renderTarget;
}

/*!
    \fn void QRhiWidget::frameSubmitted()

    This signal is emitted after the widget's top-level window has finished
    composition and has \l{QRhi::endFrame()}{submitted a frame}.
*/

/*!
    \fn void QRhiWidget::renderFailed()

    This signal is emitted whenever the widget is supposed to render to its
    backing texture (either due to a \l{QWidget::update()}{widget update} or
    due to a call to grabFramebuffer()), but there is no \l QRhi for the widget to
    use, likely due to issues related to graphics configuration.

    This signal may be emitted multiple times when a problem arises. Do not
    assume it is emitted only once. Connect with Qt::SingleShotConnection if
    the error handling code is to be notified only once.
*/

QT_END_NAMESPACE
