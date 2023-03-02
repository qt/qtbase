// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qopenglwidget.h"
#include <QtGui/QOpenGLContext>
#include <QtGui/QOffscreenSurface>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QWindow>
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>
#include <QtGui/qpa/qplatformwindow.h>
#include <QtGui/qpa/qplatformintegration.h>
#include <QtOpenGL/QOpenGLFramebufferObject>
#include <QtOpenGL/QOpenGLPaintDevice>

#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qopenglextensions_p.h>
#include <QtGui/private/qfont_p.h>
#include <QtGui/private/qopenglcontext_p.h>
#include <QtOpenGL/private/qopenglframebufferobject_p.h>
#include <QtOpenGL/private/qopenglpaintdevice_p.h>

#include <QtWidgets/private/qwidget_p.h>
#include <QtWidgets/private/qwidgetrepaintmanager_p.h>

#include <rhi/qrhi.h>

QT_BEGIN_NAMESPACE

/*!
  \class QOpenGLWidget
  \inmodule QtOpenGLWidgets
  \since 5.4

  \brief The QOpenGLWidget class is a widget for rendering OpenGL graphics.

  QOpenGLWidget provides functionality for displaying OpenGL graphics
  integrated into a Qt application. It is very simple to use: Make
  your class inherit from it and use the subclass like any other
  QWidget, except that you have the choice between using QPainter and
  standard OpenGL rendering commands.

  QOpenGLWidget provides three convenient virtual functions that you
  can reimplement in your subclass to perform the typical OpenGL
  tasks:

  \list
  \li paintGL() - Renders the OpenGL scene. Gets called whenever the widget
  needs to be updated.
  \li resizeGL() - Sets up the OpenGL viewport, projection, etc. Gets
  called whenever the widget has been resized (and also when it
  is shown for the first time because all newly created widgets get a
  resize event automatically).
  \li initializeGL() - Sets up the OpenGL resources and state. Gets called
  once before the first time resizeGL() or paintGL() is called.
  \endlist

  If you need to trigger a repaint from places other than paintGL() (a
  typical example is when using \l{QTimer}{timers} to animate scenes),
  you should call the widget's update() function to schedule an update.

  Your widget's OpenGL rendering context is made current when
  paintGL(), resizeGL(), or initializeGL() is called. If you need to
  call the standard OpenGL API functions from other places (e.g. in
  your widget's constructor or in your own paint functions), you
  must call makeCurrent() first.

  All rendering happens into an OpenGL framebuffer
  object. makeCurrent() ensure that it is bound in the context. Keep
  this in mind when creating and binding additional framebuffer
  objects in the rendering code in paintGL(). Never re-bind the
  framebuffer with ID 0. Instead, call defaultFramebufferObject() to
  get the ID that should be bound.

  QOpenGLWidget allows using different OpenGL versions and profiles
  when the platform supports it. Just set the requested format via
  setFormat(). Keep in mind however that having multiple QOpenGLWidget
  instances in the same window requires that they all use the same
  format, or at least formats that do not make the contexts
  non-sharable. To overcome this issue, prefer using
  QSurfaceFormat::setDefaultFormat() instead of setFormat().

  \note Calling QSurfaceFormat::setDefaultFormat() before constructing
  the QApplication instance is mandatory on some platforms (for example,
  \macos) when an OpenGL core profile context is requested. This is to
  ensure that resource sharing between contexts stays functional as all
  internal contexts are created using the correct version and profile.

  \section1 Painting Techniques

  As described above, subclass QOpenGLWidget to render pure 3D content in the
  following way:

  \list

  \li Reimplement the initializeGL() and resizeGL() functions to
  set up the OpenGL state and provide a perspective transformation.

  \li Reimplement paintGL() to paint the 3D scene, calling only
  OpenGL functions.

  \endlist

  It is also possible to draw 2D graphics onto a QOpenGLWidget subclass using QPainter:

  \list

  \li In paintGL(), instead of issuing OpenGL commands, construct a QPainter
      object for use on the widget.

  \li Draw primitives using QPainter's member functions.

  \li Direct OpenGL commands can still be issued. However, you must make sure
  these are enclosed by a call to the painter's beginNativePainting() and
  endNativePainting().

  \endlist

  When performing drawing using QPainter only, it is also possible to perform
  the painting like it is done for ordinary widgets: by reimplementing paintEvent().

  \list

  \li Reimplement the paintEvent() function.

  \li Construct a QPainter object targeting the widget. Either pass the widget to the
  constructor or the QPainter::begin() function.

  \li Draw primitives using QPainter's member functions.

  \li Painting finishes then the QPainter instance is destroyed. Alternatively,
  call QPainter::end() explicitly.

  \endlist

  \section1 OpenGL Function Calls, Headers and QOpenGLFunctions

  When making OpenGL function calls, it is strongly recommended to avoid calling
  the functions directly. Instead, prefer using QOpenGLFunctions (when making
  portable applications) or the versioned variants (for example,
  QOpenGLFunctions_3_2_Core and similar, when targeting modern, desktop-only
  OpenGL). This way the application will work correctly in all Qt build
  configurations, including the ones that perform dynamic OpenGL implementation
  loading which means applications are not directly linking to an GL
  implementation and thus direct function calls are not feasible.

  In paintGL() the current context is always accessible by calling
  QOpenGLContext::currentContext(). From this context an already initialized,
  ready-to-be-used QOpenGLFunctions instance is retrievable by calling
  QOpenGLContext::functions(). An alternative to prefixing every GL call is to
  inherit from QOpenGLFunctions and call
  QOpenGLFunctions::initializeOpenGLFunctions() in initializeGL().

  As for the OpenGL headers, note that in most cases there will be no need to
  directly include any headers like GL.h. The OpenGL-related Qt headers will
  include qopengl.h which will in turn include an appropriate header for the
  system. This might be an OpenGL ES 3.x or 2.0 header, the highest version that
  is available, or a system-provided gl.h. In addition, a copy of the extension
  headers (called glext.h on some systems) is provided as part of Qt both for
  OpenGL and OpenGL ES. These will get included automatically on platforms where
  feasible. This means that constants and function pointer typedefs from ARB,
  EXT, OES extensions are automatically available.

  \section1 Code Examples

  To get started, the simplest QOpenGLWidget subclass could look like the following:

  \snippet code/doc_gui_widgets_qopenglwidget.cpp 0

  Alternatively, the prefixing of each and every OpenGL call can be avoided by deriving
  from QOpenGLFunctions instead:

  \snippet code/doc_gui_widgets_qopenglwidget.cpp 1

  To get a context compatible with a given OpenGL version or profile, or to
  request depth and stencil buffers, call setFormat():

  \snippet code/doc_gui_widgets_qopenglwidget.cpp 2

  \note It is up to the application to ensure depth and stencil buffers are
  requested from the underlying windowing system interface. Without requesting
  a non-zero depth buffer size there is no guarantee that a depth buffer will
  be available, and as a result depth testing related OpenGL operations may
  fail to function as expected. Commonly used depth and stencil buffer size
  requests are 24 and 8, respectively.

  With OpenGL 3.0+ contexts, when portability is not important, the versioned
  QOpenGLFunctions variants give easy access to all the modern OpenGL functions
  available in a given version:

  \snippet code/doc_gui_widgets_qopenglwidget.cpp 3

  As described above, it is simpler and more robust to set the requested format
  globally so that it applies to all windows and contexts during the lifetime of
  the application. Below is an example of this:

  \snippet code/doc_gui_widgets_qopenglwidget.cpp 6

  \section1 Multisampling

  To enable multisampling, set the number of requested samples on the
  QSurfaceFormat that is passed to setFormat(). On systems that do not support
  it the request may get ignored.

  Multisampling support requires support for multisampled renderbuffers and
  framebuffer blits. On OpenGL ES 2.0 implementations it is likely that these
  will not be present. This means that multisampling will not be available. With
  modern OpenGL versions and OpenGL ES 3.0 and up this is usually not a problem
  anymore.

  \section1 Threading

  Performing offscreen rendering on worker threads, for example to generate
  textures that are then used in the GUI/main thread in paintGL(), are supported
  by exposing the widget's QOpenGLContext so that additional contexts sharing
  with it can be created on each thread.

  Drawing directly to the QOpenGLWidget's framebuffer outside the GUI/main
  thread is possible by reimplementing paintEvent() to do nothing. The context's
  thread affinity has to be changed via QObject::moveToThread(). After that,
  makeCurrent() and doneCurrent() are usable on the worker thread. Be careful to
  move the context back to the GUI/main thread afterwards.

  Triggering a buffer swap just for the QOpenGLWidget is not possible since there
  is no real, onscreen native surface for it. It is up to the widget stack to
  manage composition and buffer swaps on the gui thread. When a thread is done
  updating the framebuffer, call update() \b{on the GUI/main thread} to
  schedule composition.

  Extra care has to be taken to avoid using the framebuffer when the GUI/main
  thread is performing compositing. The signals aboutToCompose() and
  frameSwapped() will be emitted when the composition is starting and
  ending. They are emitted on the GUI/main thread. This means that by using a
  direct connection aboutToCompose() can block the GUI/main thread until the
  worker thread has finished its rendering. After that, the worker thread must
  perform no further rendering until the frameSwapped() signal is emitted. If
  this is not acceptable, the worker thread has to implement a double buffering
  mechanism. This involves drawing using an alternative render target, that is
  fully controlled by the thread, e.g. an additional framebuffer object, and
  blitting to the QOpenGLWidget's framebuffer at a suitable time.

  \section1 Context Sharing

  When multiple QOpenGLWidgets are added as children to the same top-level
  widget, their contexts will share with each other. This does not apply for
  QOpenGLWidget instances that belong to different windows.

  This means that all QOpenGLWidgets in the same window can access each other's
  sharable resources, like textures, and there is no need for an extra "global
  share" context.

  To set up sharing between QOpenGLWidget instances belonging to different
  windows, set the Qt::AA_ShareOpenGLContexts application attribute before
  instantiating QApplication. This will trigger sharing between all
  QOpenGLWidget instances without any further steps.

  Creating extra QOpenGLContext instances that share resources like textures
  with the QOpenGLWidget's context is also possible. Simply pass the pointer
  returned from context() to QOpenGLContext::setShareContext() before calling
  QOpenGLContext::create(). The resulting context can also be used on a
  different thread, allowing threaded generation of textures and asynchronous
  texture uploads.

  Note that QOpenGLWidget expects a standard conformant implementation of
  resource sharing when it comes to the underlying graphics drivers. For
  example, some drivers, in particular for mobile and embedded hardware, have
  issues with setting up sharing between an existing context and others that are
  created later. Some other drivers may behave in unexpected ways when trying to
  utilize shared resources between different threads.

  \section1 Resource Initialization and Cleanup

  The QOpenGLWidget's associated OpenGL context is guaranteed to be current
  whenever initializeGL() and paintGL() are invoked. Do not attempt to create
  OpenGL resources before initializeGL() is called. For example, attempting to
  compile shaders, initialize vertex buffer objects or upload texture data will
  fail when done in a subclass's constructor. These operations must be deferred
  to initializeGL(). Some of Qt's OpenGL helper classes, like QOpenGLBuffer or
  QOpenGLVertexArrayObject, have a matching deferred behavior: they can be
  instantiated without a context, but all initialization is deferred until a
  create(), or similar, call. This means that they can be used as normal
  (non-pointer) member variables in a QOpenGLWidget subclass, but the create()
  or similar function can only be called from initializeGL(). Be aware however
  that not all classes are designed like this. When in doubt, make the member
  variable a pointer and create and destroy the instance dynamically in
  initializeGL() and the destructor, respectively.

  Releasing the resources also needs the context to be current. Therefore
  destructors that perform such cleanup are expected to call makeCurrent()
  before moving on to destroy any OpenGL resources or wrappers. Avoid deferred
  deletion via \l{QObject::deleteLater()}{deleteLater()} or the parenting
  mechanism of QObject. There is no guarantee the correct context will be
  current at the time the instance in question is really destroyed.

  A typical subclass will therefore often look like the following when it comes
  to resource initialization and destruction:

  \snippet code/doc_gui_widgets_qopenglwidget.cpp 4

  This works for most cases, but not fully ideal as a generic solution. When
  the widget is reparented so that it ends up in an entirely different
  top-level window, something more is needed: by connecting to the
  \l{QOpenGLContext::aboutToBeDestroyed()}{aboutToBeDestroyed()} signal of
  QOpenGLContext, cleanup can be performed whenever the OpenGL context is about
  to be released.

  \note For widgets that change their associated top-level window multiple
  times during their lifetime, a combined cleanup approach, as demonstrated in
  the code snippet below, is essential. Whenever the widget or a parent of it
  gets reparented so that the top-level window becomes different, the widget's
  associated context is destroyed and a new one is created. This is then
  followed by a call to initializeGL() where all OpenGL resources must get
  reinitialized. Due to this the only option to perform proper cleanup is to
  connect to the context's aboutToBeDestroyed() signal. Note that the context
  in question may not be the current one when the signal gets emitted.
  Therefore it is good practice to call makeCurrent() in the connected slot.
  Additionally, the same cleanup steps must be performed from the derived
  class' destructor, since the slot or lambda connected to the signal may not
  invoked when the widget is being destroyed.

  \snippet code/doc_gui_widgets_qopenglwidget.cpp 5

  \note When Qt::AA_ShareOpenGLContexts is set, the widget's context never
  changes, not even when reparenting because the widget's associated texture is
  going to be accessible also from the new top-level's context. Therefore,
  acting on the aboutToBeDestroyed() signal of the context is not mandatory
  with this flag set.

  Proper cleanup is especially important due to context sharing. Even though
  each QOpenGLWidget's associated context is destroyed together with the
  QOpenGLWidget, the sharable resources in that context, like textures, will
  stay valid until the top-level window, in which the QOpenGLWidget lived, is
  destroyed. Additionally, settings like Qt::AA_ShareOpenGLContexts and some Qt
  modules may trigger an even wider scope for sharing contexts, potentially
  leading to keeping the resources in question alive for the entire lifetime of
  the application. Therefore the safest and most robust is always to perform
  explicit cleanup for all resources and resource wrappers used in the
  QOpenGLWidget.

  \section1 Limitations and Other Considerations

  Putting other widgets underneath and making the QOpenGLWidget transparent will
  not lead to the expected results: The widgets underneath will not be
  visible. This is because in practice the QOpenGLWidget is drawn before all
  other regular, non-OpenGL widgets, and so see-through type of solutions are
  not feasible. Other type of layouts, like having widgets on top of the
  QOpenGLWidget, will function as expected.

  When absolutely necessary, this limitation can be overcome by setting the
  Qt::WA_AlwaysStackOnTop attribute on the QOpenGLWidget. Be aware however that
  this breaks stacking order, for example it will not be possible to have other
  widgets on top of the QOpenGLWidget, so it should only be used in situations
  where a semi-transparent QOpenGLWidget with other widgets visible underneath
  is required.

  Note that this does not apply when there are no other widgets underneath and
  the intention is to have a semi-transparent window. In that case the
  traditional approach of setting Qt::WA_TranslucentBackground
  on the top-level window is sufficient. Note that if the transparent areas are
  only desired in the QOpenGLWidget, then Qt::WA_NoSystemBackground will need
  to be turned back to \c false after enabling Qt::WA_TranslucentBackground.
  Additionally, requesting an alpha channel for the QOpenGLWidget's context via
  setFormat() may be necessary too, depending on the system.

  QOpenGLWidget supports multiple update behaviors, just like QOpenGLWindow. In
  preserved mode the rendered content from the previous paintGL() call is
  available in the next one, allowing incremental rendering. In non-preserved
  mode the content is lost and paintGL() implementations are expected to redraw
  everything in the view.

  Before Qt 5.5 the default behavior of QOpenGLWidget was to preserve the
  rendered contents between paintGL() calls. Since Qt 5.5 the default behavior
  is non-preserved because this provides better performance and the majority of
  applications have no need for the previous content. This also resembles the
  semantics of an OpenGL-based QWindow and matches the default behavior of
  QOpenGLWindow in that the color and ancillary buffers are invalidated for
  each frame. To restore the preserved behavior, call setUpdateBehavior() with
  \c PartialUpdate.

  \note When dynamically adding a QOpenGLWidget into a widget hierarchy, e.g.
  by parenting a new QOpenGLWidget to a widget where the corresponding
  top-level widget is already shown on screen, the associated native window may
  get implicitly destroyed and recreated if the QOpenGLWidget is the first of
  its kind within its window. This is because the window type changes from
  \l{QSurface::RasterSurface}{RasterSurface} to
  \l{QSurface::OpenGLSurface}{OpenGLSurface} and that has platform-specific
  implications. This behavior is new in Qt 6.4.

  Once a QOpenGLWidget is added to a widget hierarchy, the contents of the
  top-level window is flushed via OpenGL-based rendering. Widgets other than
  the QOpenGLWidget continue to draw their content using a software-based
  painter, but the final composition is done through the 3D API.

  \note Displaying a QOpenGLWidget requires an alpha channel in the associated
  top-level window's backing store due to the way composition with other
  QWidget-based content works. If there is no alpha channel, the content
  rendered by the QOpenGLWidget will not be visible. This can become
  particularly relevant on Linux/X11 in remote display setups (such as, with
  Xvnc), when using a color depth lower than 24. For example, a color depth of
  16 will typically map to using a backing store image with the format
  QImage::Format_RGB16 (RGB565), leaving no room for an alpha
  channel. Therefore, if experiencing problems with getting the contents of a
  QOpenGLWidget composited correctly with other the widgets in the window, make
  sure the server (such as, vncserver) is configured with a 24 or 32 bit depth
  instead of 16.

  \section1 Alternatives

  Adding a QOpenGLWidget into a window turns on OpenGL-based
  compositing for the entire window.  In some special cases this may
  not be ideal, and the old QGLWidget-style behavior with a separate,
  native child window is desired. Desktop applications that understand
  the limitations of this approach (for example when it comes to
  overlaps, transparency, scroll views and MDI areas), can use
  QOpenGLWindow with QWidget::createWindowContainer(). This is a
  modern alternative to QGLWidget and is faster than QOpenGLWidget due
  to the lack of the additional composition step. It is strongly
  recommended to limit the usage of this approach to cases where there
  is no other choice. Note that this option is not suitable for most
  embedded and mobile platforms, and it is known to have issues on
  certain desktop platforms (e.g. \macos) too. The stable,
  cross-platform solution is always QOpenGLWidget.


  \section1 Stereoscopic rendering

  Starting from 6.5 QOpenGLWidget has support for stereoscopic rendering.
  To enable it, set the QSurfaceFormat::StereoBuffers flag
  globally before the window is created, using QSurfaceFormat::SetDefaultFormat().

  \note Using setFormat() will not necessarily work because of how the flag is
  handled internally.

  This will trigger paintGL() to be called twice each frame,
  once for each QOpenGLWidget::TargetBuffer. In paintGL(), call
  currentTargetBuffer() to query which one is currently being drawn to.

  \note For more control over the left and right color buffers, consider using
  QOpenGLWindow + QWidget::createWindowContainer() instead.

  \note This type of 3D rendering has certain hardware requirements,
  like the graphics card needs to be setup with stereo support.

  \e{OpenGL is a trademark of Silicon Graphics, Inc. in the United States and other
  countries.}

  \sa QOpenGLFunctions, QOpenGLWindow, Qt::AA_ShareOpenGLContexts, UpdateBehavior
*/

/*!
    \fn void QOpenGLWidget::aboutToCompose()

    This signal is emitted when the widget's top-level window is about to begin
    composing the textures of its QOpenGLWidget children and the other widgets.
*/

/*!
    \fn void QOpenGLWidget::frameSwapped()

    This signal is emitted after the widget's top-level window has finished
    composition and returned from its potentially blocking
    QOpenGLContext::swapBuffers() call.
*/

/*!
    \fn void QOpenGLWidget::aboutToResize()

    This signal is emitted when the widget's size is changed and therefore the
    framebuffer object is going to be recreated.
*/

/*!
    \fn void QOpenGLWidget::resized()

    This signal is emitted right after the framebuffer object has been recreated
    due to resizing the widget.
*/

/*!
    \enum QOpenGLWidget::TargetBuffer
    \since 6.5

    Specifies the buffer to use when stereoscopic rendering is enabled, which is
    toggled by setting \l QSurfaceFormat::StereoBuffers.

    \note LeftBuffer is always the default and used as fallback value when
    stereoscopic rendering is disabled or not supported by the graphics driver.

    \value LeftBuffer
    \value RightBuffer
 */

/*!
    \enum QOpenGLWidget::UpdateBehavior
    \since 5.5

    This enum describes the update semantics of QOpenGLWidget.

    \value NoPartialUpdate QOpenGLWidget will discard the
    contents of the color buffer and the ancillary buffers after the
    QOpenGLWidget is rendered to screen. This is the same behavior that can be
    expected by calling QOpenGLContext::swapBuffers with a default opengl
    enabled QWindow as the argument. NoPartialUpdate can have some performance
    benefits on certain hardware architectures common in the mobile and
    embedded space when a framebuffer object is used as the rendering target.
    The framebuffer object is invalidated between frames with
    glDiscardFramebufferEXT if supported or a glClear. Please see the
    documentation of EXT_discard_framebuffer for more information:
    https://www.khronos.org/registry/gles/extensions/EXT/EXT_discard_framebuffer.txt

    \value PartialUpdate The framebuffer objects color buffer and ancillary
    buffers are not invalidated between frames.

    \sa updateBehavior(), setUpdateBehavior()
*/

class QOpenGLWidgetPaintDevicePrivate : public QOpenGLPaintDevicePrivate
{
public:
    explicit QOpenGLWidgetPaintDevicePrivate(QOpenGLWidget *widget)
        : QOpenGLPaintDevicePrivate(QSize()),
          w(widget) { }

    void beginPaint() override;
    void endPaint() override;

    QOpenGLWidget *w;
};

class QOpenGLWidgetPaintDevice : public QOpenGLPaintDevice
{
public:
    explicit QOpenGLWidgetPaintDevice(QOpenGLWidget *widget)
        : QOpenGLPaintDevice(*new QOpenGLWidgetPaintDevicePrivate(widget)) { }
    void ensureActiveTarget() override;
};

class QOpenGLWidgetPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QOpenGLWidget)
public:
    QOpenGLWidgetPrivate() = default;

    void reset();
    void resetRhiDependentResources();
    void recreateFbos();
    void ensureRhiDependentResources();

    QWidgetPrivate::TextureData texture() const override;
    QPlatformTextureList::Flags textureListFlags() override;

    QPlatformBackingStoreRhiConfig rhiConfig() const override { return { QPlatformBackingStoreRhiConfig::OpenGL }; }

    void initialize();
    void render();

    void invalidateFbo();

    void destroyFbos();

    bool setCurrentTargetBuffer(QOpenGLWidget::TargetBuffer targetBuffer);
    QImage grabFramebuffer(QOpenGLWidget::TargetBuffer targetBuffer);
    QImage grabFramebuffer() override;
    void beginBackingStorePainting() override { inBackingStorePaint = true; }
    void endBackingStorePainting() override { inBackingStorePaint = false; }
    void beginCompose() override;
    void endCompose() override;
    void initializeViewportFramebuffer() override;
    bool isStereoEnabled() override;
    bool toggleStereoTargetBuffer() override;
    void resizeViewportFramebuffer() override;
    void resolveSamples() override;

    void resolveSamplesForBuffer(QOpenGLWidget::TargetBuffer targetBuffer);

    QOpenGLContext *context = nullptr;
    QRhiTexture *wrapperTextures[2] = {};
    QOpenGLFramebufferObject *fbos[2] = {};
    QOpenGLFramebufferObject *resolvedFbos[2] = {};
    QOffscreenSurface *surface = nullptr;
    QOpenGLPaintDevice *paintDevice = nullptr;
    int requestedSamples = 0;
    GLenum textureFormat = 0;
    QSurfaceFormat requestedFormat = QSurfaceFormat::defaultFormat();
    QOpenGLWidget::UpdateBehavior updateBehavior = QOpenGLWidget::NoPartialUpdate;
    bool initialized = false;
    bool fakeHidden = false;
    bool inBackingStorePaint = false;
    bool hasBeenComposed = false;
    bool flushPending = false;
    bool inPaintGL = false;
    QOpenGLWidget::TargetBuffer currentTargetBuffer = QOpenGLWidget::LeftBuffer;
};

void QOpenGLWidgetPaintDevicePrivate::beginPaint()
{
    // NB! autoFillBackground is and must be false by default. Otherwise we would clear on
    // every QPainter begin() which is not desirable. This is only for legacy use cases,
    // like using QOpenGLWidget as the viewport of a graphics view, that expect clearing
    // with the palette's background color.
    if (w->autoFillBackground()) {
        QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
        if (w->format().hasAlpha()) {
            f->glClearColor(0, 0, 0, 0);
        } else {
            QColor c = w->palette().brush(w->backgroundRole()).color();
            float alpha = c.alphaF();
            f->glClearColor(c.redF() * alpha, c.greenF() * alpha, c.blueF() * alpha, alpha);
        }
        f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }
}

void QOpenGLWidgetPaintDevicePrivate::endPaint()
{
    QOpenGLWidgetPrivate *wd = static_cast<QOpenGLWidgetPrivate *>(QWidgetPrivate::get(w));
    if (!wd->initialized)
        return;

    if (!wd->inPaintGL)
        QOpenGLContextPrivate::get(wd->context)->defaultFboRedirect = 0;
}

void QOpenGLWidgetPaintDevice::ensureActiveTarget()
{
    QOpenGLWidgetPaintDevicePrivate *d = static_cast<QOpenGLWidgetPaintDevicePrivate *>(d_ptr.data());
    QOpenGLWidgetPrivate *wd = static_cast<QOpenGLWidgetPrivate *>(QWidgetPrivate::get(d->w));
    if (!wd->initialized)
        return;

    if (QOpenGLContext::currentContext() != wd->context)
        d->w->makeCurrent();
    else
        wd->fbos[wd->currentTargetBuffer]->bind();


    if (!wd->inPaintGL)
        QOpenGLContextPrivate::get(wd->context)->defaultFboRedirect = wd->fbos[wd->currentTargetBuffer]->handle();

    // When used as a viewport, drawing is done via opening a QPainter on the widget
    // without going through paintEvent(). We will have to make sure a glFlush() is done
    // before the texture is accessed also in this case.
    wd->flushPending = true;
}

QWidgetPrivate::TextureData QOpenGLWidgetPrivate::texture() const
{
    return { wrapperTextures[QOpenGLWidget::LeftBuffer], wrapperTextures[QOpenGLWidget::RightBuffer] };
}

#ifndef GL_SRGB
#define GL_SRGB 0x8C40
#endif
#ifndef GL_SRGB8
#define GL_SRGB8 0x8C41
#endif
#ifndef GL_SRGB_ALPHA
#define GL_SRGB_ALPHA 0x8C42
#endif
#ifndef GL_SRGB8_ALPHA8
#define GL_SRGB8_ALPHA8 0x8C43
#endif

QPlatformTextureList::Flags QOpenGLWidgetPrivate::textureListFlags()
{
    QPlatformTextureList::Flags flags = QWidgetPrivate::textureListFlags();
    switch (textureFormat) {
    case GL_SRGB:
    case GL_SRGB8:
    case GL_SRGB_ALPHA:
    case GL_SRGB8_ALPHA8:
        flags |= QPlatformTextureList::TextureIsSrgb;
        break;
    default:
        break;
    }
    return flags;
}

void QOpenGLWidgetPrivate::reset()
{
    Q_Q(QOpenGLWidget);

    // Destroy the OpenGL resources first. These need the context to be current.
    if (initialized)
        q->makeCurrent();

    delete paintDevice;
    paintDevice = nullptr;

    destroyFbos();

    if (initialized)
        q->doneCurrent();

    // Delete the context first, then the surface. Slots connected to
    // the context's aboutToBeDestroyed() may still call makeCurrent()
    // to perform some cleanup.
    delete context;
    context = nullptr;
    delete surface;
    surface = nullptr;
    initialized = fakeHidden = inBackingStorePaint = false;
}

void QOpenGLWidgetPrivate::resetRhiDependentResources()
{
    // QRhi resource created from the QRhi. These must be released whenever the
    // widget gets associated with a different QRhi, even when all OpenGL
    // contexts share resources.

    delete wrapperTextures[0];
    wrapperTextures[0] = nullptr;

    if (isStereoEnabled()) {
        delete wrapperTextures[1];
        wrapperTextures[1] = nullptr;
    }
}

void QOpenGLWidgetPrivate::recreateFbos()
{
    Q_Q(QOpenGLWidget);

    emit q->aboutToResize();

    context->makeCurrent(surface);

    destroyFbos();

    int samples = requestedSamples;
    QOpenGLExtensions *extfuncs = static_cast<QOpenGLExtensions *>(context->functions());
    if (!extfuncs->hasOpenGLExtension(QOpenGLExtensions::FramebufferMultisample))
        samples = 0;

    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(samples);
    if (textureFormat)
        format.setInternalTextureFormat(textureFormat);

    const QSize deviceSize = q->size() * q->devicePixelRatio();
    fbos[QOpenGLWidget::LeftBuffer] = new QOpenGLFramebufferObject(deviceSize, format);
    if (samples > 0)
        resolvedFbos[QOpenGLWidget::LeftBuffer] = new QOpenGLFramebufferObject(deviceSize);

    const bool stereo = isStereoEnabled();

    if (stereo) {
        fbos[QOpenGLWidget::RightBuffer] = new QOpenGLFramebufferObject(deviceSize, format);
        if (samples > 0)
            resolvedFbos[QOpenGLWidget::RightBuffer] = new QOpenGLFramebufferObject(deviceSize);
    }

    textureFormat = fbos[QOpenGLWidget::LeftBuffer]->format().internalTextureFormat();

    currentTargetBuffer = QOpenGLWidget::LeftBuffer;
    fbos[currentTargetBuffer]->bind();
    context->functions()->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    ensureRhiDependentResources();

    if (stereo) {
        currentTargetBuffer = QOpenGLWidget::RightBuffer;
        fbos[currentTargetBuffer]->bind();
        context->functions()->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        ensureRhiDependentResources();
        currentTargetBuffer = QOpenGLWidget::LeftBuffer;
    }

    flushPending = true; // Make sure the FBO is initialized before use

    paintDevice->setSize(deviceSize);
    paintDevice->setDevicePixelRatio(q->devicePixelRatio());

    emit q->resized();
}

void QOpenGLWidgetPrivate::ensureRhiDependentResources()
{
    Q_Q(QOpenGLWidget);

    QRhi *rhi = nullptr;
    if (QWidgetRepaintManager *repaintManager = QWidgetPrivate::get(q->window())->maybeRepaintManager())
        rhi = repaintManager->rhi();

    // If there is no rhi, because we are completely offscreen, then there's no wrapperTexture either
    if (rhi && rhi->backend() == QRhi::OpenGLES2) {
        const QSize deviceSize = q->size() * q->devicePixelRatio();
        if (!wrapperTextures[currentTargetBuffer] || wrapperTextures[currentTargetBuffer]->pixelSize() != deviceSize) {
            const uint textureId = resolvedFbos[currentTargetBuffer] ?
                        resolvedFbos[currentTargetBuffer]->texture()
                      : (fbos[currentTargetBuffer] ? fbos[currentTargetBuffer]->texture() : 0);
            if (!wrapperTextures[currentTargetBuffer])
                wrapperTextures[currentTargetBuffer] = rhi->newTexture(QRhiTexture::RGBA8, deviceSize, 1, QRhiTexture::RenderTarget);
            else
                wrapperTextures[currentTargetBuffer]->setPixelSize(deviceSize);
            if (!wrapperTextures[currentTargetBuffer]->createFrom({textureId, 0 }))
                qWarning("QOpenGLWidget: Failed to create wrapper texture");
        }
    }
}

void QOpenGLWidgetPrivate::beginCompose()
{
    Q_Q(QOpenGLWidget);
    if (flushPending) {
        flushPending = false;
        q->makeCurrent();
        static_cast<QOpenGLExtensions *>(context->functions())->flushShared();
    }
    hasBeenComposed = true;
    emit q->aboutToCompose();
}

void QOpenGLWidgetPrivate::endCompose()
{
    Q_Q(QOpenGLWidget);
    emit q->frameSwapped();
}

void QOpenGLWidgetPrivate::initialize()
{
    Q_Q(QOpenGLWidget);
    if (initialized)
        return;

    // If no global shared context get our toplevel's context with which we
    // will share in order to make the texture usable by the underlying window's backingstore.
    QWidget *tlw = q->window();
    QWidgetPrivate *tlwd = get(tlw);

    // Do not include the sample count. Requesting a multisampled context is not necessary
    // since we render into an FBO, never to an actual surface. What's more, attempting to
    // create a pbuffer with a multisampled config crashes certain implementations. Just
    // avoid the entire hassle, the result is the same.
    requestedSamples = requestedFormat.samples();
    requestedFormat.setSamples(0);

    QRhi *rhi = nullptr;
    if (QWidgetRepaintManager *repaintManager = tlwd->maybeRepaintManager())
        rhi = repaintManager->rhi();

    // Could be that something else already initialized the window with some
    // other graphics API for the QRhi, that's not good.
    if (rhi && rhi->backend() != QRhi::OpenGLES2) {
        qWarning("The top-level window is not using OpenGL for composition, '%s' is not compatible with QOpenGLWidget",
                 rhi->backendName());
        return;
    }

    // If rhi or contextFromRhi is null, showing content on-screen will not work.
    // However, offscreen rendering and grabFramebuffer() will stay fully functional.

    QOpenGLContext *contextFromRhi = rhi ? static_cast<const QRhiGles2NativeHandles *>(rhi->nativeHandles())->context : nullptr;

    context = new QOpenGLContext;
    context->setFormat(requestedFormat);
    if (contextFromRhi) {
        context->setShareContext(contextFromRhi);
        context->setScreen(contextFromRhi->screen());
    }
    if (Q_UNLIKELY(!context->create())) {
        qWarning("QOpenGLWidget: Failed to create context");
        return;
    }

    surface = new QOffscreenSurface;
    surface->setFormat(context->format());
    surface->setScreen(context->screen());
    surface->create();

    if (Q_UNLIKELY(!context->makeCurrent(surface))) {
        qWarning("QOpenGLWidget: Failed to make context current");
        return;
    }

    // Propagate settings that make sense only for the tlw. Note that this only
    // makes sense for properties that get picked up even after the native
    // window is created.
    if (tlw->windowHandle()) {
        QSurfaceFormat tlwFormat = tlw->windowHandle()->format();
        if (requestedFormat.swapInterval() != tlwFormat.swapInterval()) {
            // Most platforms will pick up the changed swap interval on the next
            // makeCurrent or swapBuffers.
            tlwFormat.setSwapInterval(requestedFormat.swapInterval());
            tlw->windowHandle()->setFormat(tlwFormat);
        }
        if (requestedFormat.swapBehavior() != tlwFormat.swapBehavior()) {
            tlwFormat.setSwapBehavior(requestedFormat.swapBehavior());
            tlw->windowHandle()->setFormat(tlwFormat);
        }
    }

    paintDevice = new QOpenGLWidgetPaintDevice(q);
    paintDevice->setSize(q->size() * q->devicePixelRatio());
    paintDevice->setDevicePixelRatio(q->devicePixelRatio());

    initialized = true;

    q->initializeGL();
}

void QOpenGLWidgetPrivate::resolveSamples()
{
    resolveSamplesForBuffer(QOpenGLWidget::LeftBuffer);
    resolveSamplesForBuffer(QOpenGLWidget::RightBuffer);
}

void QOpenGLWidgetPrivate::resolveSamplesForBuffer(QOpenGLWidget::TargetBuffer targetBuffer)
{
    Q_Q(QOpenGLWidget);
    if (resolvedFbos[targetBuffer]) {
        q->makeCurrent(targetBuffer);
        QRect rect(QPoint(0, 0), fbos[targetBuffer]->size());
        QOpenGLFramebufferObject::blitFramebuffer(resolvedFbos[targetBuffer], rect, fbos[targetBuffer], rect);
        flushPending = true;
    }
}

void QOpenGLWidgetPrivate::render()
{
    Q_Q(QOpenGLWidget);

    if (fakeHidden || !initialized)
        return;

    setCurrentTargetBuffer(QOpenGLWidget::LeftBuffer);

    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    if (!ctx) {
        qWarning("QOpenGLWidget: No current context, cannot render");
        return;
    }

    if (!fbos[QOpenGLWidget::LeftBuffer]) {
        qWarning("QOpenGLWidget: No fbo, cannot render");
        return;
    }

    const bool stereo = isStereoEnabled();
    if (stereo) {
        static bool warningGiven = false;
        if (!fbos[QOpenGLWidget::RightBuffer] && !warningGiven) {
            qWarning("QOpenGLWidget: Stereo is enabled, but no right buffer. Using only left buffer");
            warningGiven = true;
        }
    }

    if (updateBehavior == QOpenGLWidget::NoPartialUpdate && hasBeenComposed) {
        invalidateFbo();

        if (stereo && fbos[QOpenGLWidget::RightBuffer]) {
            setCurrentTargetBuffer(QOpenGLWidget::RightBuffer);
            invalidateFbo();
            setCurrentTargetBuffer(QOpenGLWidget::LeftBuffer);
        }

        hasBeenComposed = false;
    }

    QOpenGLFunctions *f = ctx->functions();
    f->glViewport(0, 0, q->width() * q->devicePixelRatio(), q->height() * q->devicePixelRatio());
    inPaintGL = true;

#ifdef Q_OS_WASM
    f->glDepthMask(GL_TRUE);
#endif

    QOpenGLContextPrivate::get(ctx)->defaultFboRedirect = fbos[currentTargetBuffer]->handle();
    q->paintGL();

    if (stereo && fbos[QOpenGLWidget::RightBuffer]) {
        setCurrentTargetBuffer(QOpenGLWidget::RightBuffer);
        QOpenGLContextPrivate::get(ctx)->defaultFboRedirect = fbos[currentTargetBuffer]->handle();
        q->paintGL();
    }
    QOpenGLContextPrivate::get(ctx)->defaultFboRedirect = 0;

    inPaintGL = false;
    flushPending = true;
}

void QOpenGLWidgetPrivate::invalidateFbo()
{
    QOpenGLExtensions *f = static_cast<QOpenGLExtensions *>(QOpenGLContext::currentContext()->functions());
    if (f->hasOpenGLExtension(QOpenGLExtensions::DiscardFramebuffer)) {
        const int gl_color_attachment0 = 0x8CE0;  // GL_COLOR_ATTACHMENT0
        const int gl_depth_attachment = 0x8D00;   // GL_DEPTH_ATTACHMENT
        const int gl_stencil_attachment = 0x8D20; // GL_STENCIL_ATTACHMENT
#ifdef Q_OS_WASM
        // webgl does not allow separate depth and stencil attachments
        // QTBUG-69913
        const int gl_depth_stencil_attachment = 0x821A; // GL_DEPTH_STENCIL_ATTACHMENT

        const GLenum attachments[] = {
            gl_color_attachment0, gl_depth_attachment, gl_stencil_attachment, gl_depth_stencil_attachment
        };
#else
        const GLenum attachments[] = {
            gl_color_attachment0, gl_depth_attachment, gl_stencil_attachment
        };
#endif
        f->glDiscardFramebufferEXT(GL_FRAMEBUFFER, sizeof attachments / sizeof *attachments, attachments);
    } else {
        f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }
}

void QOpenGLWidgetPrivate::destroyFbos()
{
    delete fbos[QOpenGLWidget::LeftBuffer];
    fbos[QOpenGLWidget::LeftBuffer] = nullptr;
    delete resolvedFbos[QOpenGLWidget::LeftBuffer];
    resolvedFbos[QOpenGLWidget::LeftBuffer] = nullptr;

    delete fbos[QOpenGLWidget::RightBuffer];
    fbos[QOpenGLWidget::RightBuffer] = nullptr;
    delete resolvedFbos[QOpenGLWidget::RightBuffer];
    resolvedFbos[QOpenGLWidget::RightBuffer] = nullptr;

    resetRhiDependentResources();
}

QImage QOpenGLWidgetPrivate::grabFramebuffer()
{
    return grabFramebuffer(QOpenGLWidget::LeftBuffer);
}

QImage QOpenGLWidgetPrivate::grabFramebuffer(QOpenGLWidget::TargetBuffer targetBuffer)
{
    Q_Q(QOpenGLWidget);

    initialize();
    if (!initialized)
        return QImage();

    // The second fbo is only created when stereoscopic rendering is enabled
    // Just use the default one if not.
    if (targetBuffer == QOpenGLWidget::RightBuffer && !isStereoEnabled())
        targetBuffer = QOpenGLWidget::LeftBuffer;

    if (!fbos[targetBuffer]) // could be completely offscreen, without ever getting a resize event
        recreateFbos();

    if (!inPaintGL)
        render();

    setCurrentTargetBuffer(targetBuffer);
    if (resolvedFbos[targetBuffer]) {
        resolveSamplesForBuffer(targetBuffer);
        resolvedFbos[targetBuffer]->bind();
    }

    const bool hasAlpha = q->format().hasAlpha();
    QImage res = qt_gl_read_framebuffer(q->size() * q->devicePixelRatio(), hasAlpha, hasAlpha);
    res.setDevicePixelRatio(q->devicePixelRatio());

    // While we give no guarantees of what is going to be left bound, prefer the
    // multisample fbo instead of the resolved one. Clients may continue to
    // render straight after calling this function.
    if (resolvedFbos[targetBuffer]) {
        setCurrentTargetBuffer(targetBuffer);
    }

    return res;
}

void QOpenGLWidgetPrivate::initializeViewportFramebuffer()
{
    Q_Q(QOpenGLWidget);
    // Legacy behavior for compatibility with QGLWidget when used as a graphics view
    // viewport: enable clearing on each painter begin.
    q->setAutoFillBackground(true);
}

bool QOpenGLWidgetPrivate::isStereoEnabled()
{
    Q_Q(QOpenGLWidget);
    // Note that because this internally might use the requested format,
    // then this can return a false positive on hardware where
    // steroscopic rendering is not supported.
    return q->format().stereo();
}

bool QOpenGLWidgetPrivate::toggleStereoTargetBuffer()
{
    return setCurrentTargetBuffer(currentTargetBuffer == QOpenGLWidget::LeftBuffer ?
                               QOpenGLWidget::RightBuffer :
                               QOpenGLWidget::LeftBuffer);
}

void QOpenGLWidgetPrivate::resizeViewportFramebuffer()
{
    Q_Q(QOpenGLWidget);
    if (!initialized)
        return;

    if (!fbos[currentTargetBuffer] || q->size() * q->devicePixelRatio() != fbos[currentTargetBuffer]->size()) {
        recreateFbos();
        q->update();
    }
}

/*!
  Constructs a widget which is a child of \a parent, with widget flags set to \a f.
 */
QOpenGLWidget::QOpenGLWidget(QWidget *parent, Qt::WindowFlags f)
    : QWidget(*(new QOpenGLWidgetPrivate), parent, f)
{
    Q_D(QOpenGLWidget);
    if (Q_UNLIKELY(!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::RhiBasedRendering)
                   || !QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::OpenGL)))
        qWarning("QOpenGLWidget is not supported on this platform.");
    else
        d->setRenderToTexture();
}

/*!
  Destroys the QOpenGLWidget instance, freeing its resources.

  The QOpenGLWidget's context is made current in the destructor, allowing for
  safe destruction of any child object that may need to release OpenGL
  resources belonging to the context provided by this widget.

  \warning if you have objects wrapping OpenGL resources (such as
  QOpenGLBuffer, QOpenGLShaderProgram, etc.) as members of a OpenGLWidget
  subclass, you may need to add a call to makeCurrent() in that subclass'
  destructor as well. Due to the rules of C++ object destruction, those objects
  will be destroyed \e{before} calling this function (but after that the
  destructor of the subclass has run), therefore making the OpenGL context
  current in this function happens too late for their safe disposal.

  \sa makeCurrent
*/
QOpenGLWidget::~QOpenGLWidget()
{
    // NB! resetting graphics resources must be done from this destructor,
    // *not* from the private class' destructor. This is due to how destruction
    // works and due to the QWidget dtor (for toplevels) destroying the repaint
    // manager and rhi before the (QObject) private gets destroyed. Hence must
    // do it here early on.

    Q_D(QOpenGLWidget);
    d->reset();
}

/*!
  Sets this widget's update behavior to \a updateBehavior.
  \since 5.5
*/
void QOpenGLWidget::setUpdateBehavior(UpdateBehavior updateBehavior)
{
    Q_D(QOpenGLWidget);
    d->updateBehavior = updateBehavior;
}

/*!
  \return the update behavior of the widget.
  \since 5.5
*/
QOpenGLWidget::UpdateBehavior QOpenGLWidget::updateBehavior() const
{
    Q_D(const QOpenGLWidget);
    return d->updateBehavior;
}

/*!
  Sets the requested surface \a format.

  When the format is not explicitly set via this function, the format returned by
  QSurfaceFormat::defaultFormat() will be used. This means that when having multiple
  OpenGL widgets, individual calls to this function can be replaced by one single call to
  QSurfaceFormat::setDefaultFormat() before creating the first widget.

  \note Requesting an alpha buffer via this function will not lead to the
  desired results when the intention is to make other widgets beneath visible.
  Instead, use Qt::WA_AlwaysStackOnTop to enable semi-transparent QOpenGLWidget
  instances with other widgets visible underneath. Keep in mind however that
  this breaks the stacking order, so it will no longer be possible to have
  other widgets on top of the QOpenGLWidget.

  \sa format(), Qt::WA_AlwaysStackOnTop, QSurfaceFormat::setDefaultFormat()
 */
void QOpenGLWidget::setFormat(const QSurfaceFormat &format)
{
    Q_D(QOpenGLWidget);
    if (Q_UNLIKELY(d->initialized)) {
        qWarning("QOpenGLWidget: Already initialized, setting the format has no effect");
        return;
    }

    d->requestedFormat = format;
}

/*!
    Returns the context and surface format used by this widget and its toplevel
    window.

    After the widget and its toplevel have both been created, resized and shown,
    this function will return the actual format of the context. This may differ
    from the requested format if the request could not be fulfilled by the
    platform. It is also possible to get larger color buffer sizes than
    requested.

    When the widget's window and the related OpenGL resources are not yet
    initialized, the return value is the format that has been set via
    setFormat().

    \sa setFormat(), context()
 */
QSurfaceFormat QOpenGLWidget::format() const
{
    Q_D(const QOpenGLWidget);
    return d->initialized ? d->context->format() : d->requestedFormat;
}

/*!
    Sets a custom internal texture format of \a texFormat.

    When working with sRGB framebuffers, it will be necessary to specify a
    format like \c{GL_SRGB8_ALPHA8}. This can be achieved by calling this
    function.

    \note This function has no effect if called after the widget has already
    been shown and thus it performed initialization.

    \note This function will typically have to be used in combination with a
    QSurfaceFormat::setDefaultFormat() call that sets the color space to
    QSurfaceFormat::sRGBColorSpace.

    \since 5.10
 */
void QOpenGLWidget::setTextureFormat(GLenum texFormat)
{
    Q_D(QOpenGLWidget);
    if (Q_UNLIKELY(d->initialized)) {
        qWarning("QOpenGLWidget: Already initialized, setting the internal texture format has no effect");
        return;
    }

    d->textureFormat = texFormat;
}

/*!
    \return the active internal texture format if the widget has already
    initialized, the requested format if one was set but the widget has not yet
    been made visible, or \nullptr if setTextureFormat() was not called and the
    widget has not yet been made visible.

    \since 5.10
 */
GLenum QOpenGLWidget::textureFormat() const
{
    Q_D(const QOpenGLWidget);
    return d->textureFormat;
}

/*!
  \return \e true if the widget and OpenGL resources, like the context, have
  been successfully initialized. Note that the return value is always false
  until the widget is shown.
*/
bool QOpenGLWidget::isValid() const
{
    Q_D(const QOpenGLWidget);
    return d->initialized && d->context->isValid();
}

/*!
  Prepares for rendering OpenGL content for this widget by making the
  corresponding context current and binding the framebuffer object in that
  context.

  It is not necessary to call this function in most cases, because it
  is called automatically before invoking paintGL().

  \sa context(), paintGL(), doneCurrent()
 */
void QOpenGLWidget::makeCurrent()
{
    Q_D(QOpenGLWidget);
    if (!d->initialized)
        return;

    d->context->makeCurrent(d->surface);

    if (d->fbos[d->currentTargetBuffer]) // there may not be one if we are in reset()
        d->fbos[d->currentTargetBuffer]->bind();
}

/*!
  Prepares for rendering OpenGL content for this widget by making the
  context for the passed in buffer current and binding the framebuffer object in that
  context.

  \note This only makes sense to call when stereoscopic rendering is enabled.
  Nothing will happen if the right buffer is requested when it's disabled.

  It is not necessary to call this function in most cases, because it
  is called automatically before invoking paintGL().

  \since 6.5

  \sa context(), paintGL(), doneCurrent()
 */
void QOpenGLWidget::makeCurrent(TargetBuffer targetBuffer)
{
    Q_D(QOpenGLWidget);
    if (!d->initialized)
        return;

    // The FBO for the right buffer is only initialized when stereo is set
   if (targetBuffer == TargetBuffer::RightBuffer && !format().stereo())
       return;

    d->setCurrentTargetBuffer(targetBuffer); // calls makeCurrent
}

/*!
  Releases the context.

  It is not necessary to call this function in most cases, since the
  widget will make sure the context is bound and released properly
  when invoking paintGL().
 */
void QOpenGLWidget::doneCurrent()
{
    Q_D(QOpenGLWidget);
    if (!d->initialized)
        return;

    d->context->doneCurrent();
}

/*!
  \return The QOpenGLContext used by this widget or \c 0 if not yet initialized.

  \note The context and the framebuffer object used by the widget changes when
  reparenting the widget via setParent().

  \sa QOpenGLContext::setShareContext(), defaultFramebufferObject()
 */
QOpenGLContext *QOpenGLWidget::context() const
{
    Q_D(const QOpenGLWidget);
    return d->context;
}

/*!
  \return The framebuffer object handle or \c 0 if not yet initialized.

  \note The framebuffer object belongs to the context returned by context()
  and may not be accessible from other contexts.

  \note The context and the framebuffer object used by the widget changes when
  reparenting the widget via setParent(). In addition, the framebuffer object
  changes on each resize.

  \sa context()
 */
GLuint QOpenGLWidget::defaultFramebufferObject() const
{
    Q_D(const QOpenGLWidget);
    return d->fbos[TargetBuffer::LeftBuffer] ? d->fbos[TargetBuffer::LeftBuffer]->handle() : 0;
}

/*!
  \return The framebuffer object handle of the specified target buffer or
  \c 0 if not yet initialized.

  Calling this overload only makes sense if \l QSurfaceFormat::StereoBuffers is enabled
  and supported by the hardware. If not, this method will return the default buffer.

  \note The framebuffer object belongs to the context returned by context()
  and may not be accessible from other contexts. The context and the framebuffer
  object used by the widget changes when reparenting the widget via setParent().
  In addition, the framebuffer object changes on each resize.

  \since 6.5

  \sa context()
 */
GLuint QOpenGLWidget::defaultFramebufferObject(TargetBuffer targetBuffer) const
{
    Q_D(const QOpenGLWidget);
    return d->fbos[targetBuffer] ? d->fbos[targetBuffer]->handle() : 0;
}

/*!
  This virtual function is called once before the first call to
  paintGL() or resizeGL(). Reimplement it in a subclass.

  This function should set up any required OpenGL resources.

  There is no need to call makeCurrent() because this has already been
  done when this function is called. Note however that the framebuffer
  is not yet available at this stage, so avoid issuing draw calls from
  here. Defer such calls to paintGL() instead.

  \sa paintGL(), resizeGL()
*/
void QOpenGLWidget::initializeGL()
{
}

/*!
  This virtual function is called whenever the widget has been
  resized. Reimplement it in a subclass. The new size is passed in
  \a w and \a h.

  There is no need to call makeCurrent() because this has already been
  done when this function is called. Additionally, the framebuffer is
  also bound.

  \sa initializeGL(), paintGL()
*/
void QOpenGLWidget::resizeGL(int w, int h)
{
    Q_UNUSED(w);
    Q_UNUSED(h);
}

/*!
  This virtual function is called whenever the widget needs to be
  painted. Reimplement it in a subclass.

  There is no need to call makeCurrent() because this has already
  been done when this function is called.

  Before invoking this function, the context and the framebuffer are
  bound, and the viewport is set up by a call to glViewport(). No
  other state is set and no clearing or drawing is performed by the
  framework.

  \note To ensure portability, do not expect that state set in initializeGL()
  persists. Rather, set all necessary state, for example, by calling
  glEnable(), in paintGL(). This is because some platforms, such as WebAssembly
  with WebGL, may have limitations on OpenGL contexts in some situations, which
  can lead to using the context used with the QOpenGLWidget for other purposes
  as well.

  When \l QSurfaceFormat::StereoBuffers is enabled, this function
  will be called twice - once for each buffer. Query what buffer is
  currently bound by calling currentTargetBuffer().

  \note The framebuffer of each target will be drawn to even when
  stereoscopic rendering is not supported by the hardware.
  Only the left buffer will actually be visible in the window.

  \sa initializeGL(), resizeGL(), currentTargetBuffer()
*/
void QOpenGLWidget::paintGL()
{
}

/*!
  Handles resize events that are passed in the \a e event parameter.
  Calls the virtual function resizeGL().

  \note Avoid overriding this function in derived classes. If that is not
  feasible, make sure that QOpenGLWidget's implementation is invoked
  too. Otherwise the underlying framebuffer object and related resources will
  not get resized properly and will lead to incorrect rendering.
*/
void QOpenGLWidget::resizeEvent(QResizeEvent *e)
{
    Q_D(QOpenGLWidget);

    if (e->size().isEmpty()) {
        d->fakeHidden = true;
        return;
    }
    d->fakeHidden = false;

    d->initialize();
    if (!d->initialized)
        return;

    d->recreateFbos();
    // Make sure our own context is current before invoking user overrides. If
    // the fbo was recreated then there's a chance something else is current now.
    makeCurrent();
    resizeGL(width(), height());
    d->sendPaintEvent(QRect(QPoint(0, 0), size()));
}

/*!
  Handles paint events.

  Calling QWidget::update() will lead to sending a paint event \a e,
  and thus invoking this function. (NB this is asynchronous and will
  happen at some point after returning from update()). This function
  will then, after some preparation, call the virtual paintGL() to
  update the contents of the QOpenGLWidget's framebuffer. The widget's
  top-level window will then composite the framebuffer's texture with
  the rest of the window.
*/
void QOpenGLWidget::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e);
    Q_D(QOpenGLWidget);

    d->initialize();
    if (d->initialized) {
        d->ensureRhiDependentResources();
        if (updatesEnabled())
            d->render();
    }
}

/*!
  Renders and returns a 32-bit RGB image of the framebuffer.

  \note This is a potentially expensive operation because it relies on glReadPixels()
  to read back the pixels. This may be slow and can stall the GPU pipeline.
*/
QImage QOpenGLWidget::grabFramebuffer()
{
    Q_D(QOpenGLWidget);
    return d->grabFramebuffer();
}

/*!
  Renders and returns a 32-bit RGB image of the framebuffer of the specified target buffer.
  This overload only makes sense to call when \l QSurfaceFormat::StereoBuffers is enabled.
  Grabbing the framebuffer of the right target buffer will return the default image
  if stereoscopic rendering is disabled or if not supported by the hardware.

  \note This is a potentially expensive operation because it relies on glReadPixels()
  to read back the pixels. This may be slow and can stall the GPU pipeline.

  \since 6.5
*/
QImage QOpenGLWidget::grabFramebuffer(TargetBuffer targetBuffer)
{
    Q_D(QOpenGLWidget);
    return d->grabFramebuffer(targetBuffer);
}

/*!
  Returns the currently active target buffer. This will be the left buffer by default,
  the right buffer is only used when \l QSurfaceFormat::StereoBuffers is enabled.
  When stereoscopic rendering is enabled, this can be queried in paintGL() to know
  what buffer is currently in use. paintGL() will be called twice, once for each target.

  \since 6.5

  \sa paintGL()
*/
QOpenGLWidget::TargetBuffer QOpenGLWidget::currentTargetBuffer() const
{
    Q_D(const QOpenGLWidget);
    return d->currentTargetBuffer;
}

/*!
  \reimp
*/
int QOpenGLWidget::metric(QPaintDevice::PaintDeviceMetric metric) const
{
    Q_D(const QOpenGLWidget);
    if (d->inBackingStorePaint)
        return QWidget::metric(metric);

    auto window = d->windowHandle(QWidgetPrivate::WindowHandleMode::TopLevel);
    QScreen *screen = window ? window->screen() : QGuiApplication::primaryScreen();

    const float dpmx = qt_defaultDpiX() * 100. / 2.54;
    const float dpmy = qt_defaultDpiY() * 100. / 2.54;

    switch (metric) {
    case PdmWidth:
        return width();
    case PdmHeight:
        return height();
    case PdmDepth:
        return 32;
    case PdmWidthMM:
        if (screen)
            return width() * screen->physicalSize().width() / screen->geometry().width();
        else
            return width() * 1000 / dpmx;
    case PdmHeightMM:
        if (screen)
            return height() * screen->physicalSize().height() / screen->geometry().height();
        else
            return height() * 1000 / dpmy;
    case PdmNumColors:
        return 0;
    case PdmDpiX:
        if (screen)
            return qRound(screen->logicalDotsPerInchX());
        else
            return qRound(dpmx * 0.0254);
    case PdmDpiY:
        if (screen)
            return qRound(screen->logicalDotsPerInchY());
        else
            return qRound(dpmy * 0.0254);
    case PdmPhysicalDpiX:
        if (screen)
            return qRound(screen->physicalDotsPerInchX());
        else
            return qRound(dpmx * 0.0254);
    case PdmPhysicalDpiY:
        if (screen)
            return qRound(screen->physicalDotsPerInchY());
        else
            return qRound(dpmy * 0.0254);
    case PdmDevicePixelRatio:
        return QWidget::metric(metric);
    case PdmDevicePixelRatioScaled:
        return QWidget::metric(metric);
    default:
        qWarning("QOpenGLWidget::metric(): unknown metric %d", metric);
        return 0;
    }
}

/*!
  \reimp
*/
QPaintDevice *QOpenGLWidget::redirected(QPoint *p) const
{
    Q_D(const QOpenGLWidget);
    if (d->inBackingStorePaint)
        return QWidget::redirected(p);

    return d->paintDevice;
}

/*!
  \reimp
*/
QPaintEngine *QOpenGLWidget::paintEngine() const
{
    Q_D(const QOpenGLWidget);
    // QWidget needs to "punch a hole" into the backingstore. This needs the
    // normal paint engine and device, not the GL one. So in this mode, behave
    // like a normal widget.
    if (d->inBackingStorePaint)
        return QWidget::paintEngine();

    if (!d->initialized)
        return nullptr;

    return d->paintDevice->paintEngine();
}


bool QOpenGLWidgetPrivate::setCurrentTargetBuffer(QOpenGLWidget::TargetBuffer targetBuffer)
{
    Q_Q(QOpenGLWidget);

    if (targetBuffer == QOpenGLWidget::RightBuffer && !isStereoEnabled())
        return false;

    currentTargetBuffer = targetBuffer;
    q->makeCurrent();

    return true;
}

/*!
  \reimp
*/
bool QOpenGLWidget::event(QEvent *e)
{
    Q_D(QOpenGLWidget);
    switch (e->type()) {
    case QEvent::WindowAboutToChangeInternal:
        d->resetRhiDependentResources();
        break;
    case QEvent::WindowChangeInternal:
        if (QCoreApplication::testAttribute(Qt::AA_ShareOpenGLContexts))
            break;
        if (d->initialized)
            d->reset();
        if (isHidden())
            break;
        Q_FALLTHROUGH();
    case QEvent::Show: // reparenting may not lead to a resize so reinitialize on Show too
        if (d->initialized && !d->wrapperTextures[d->currentTargetBuffer] && window()->windowHandle()) {
            // Special case: did grabFramebuffer() for a hidden widget that then became visible.
            // Recreate all resources since the context now needs to share with the TLW's.
            if (!QCoreApplication::testAttribute(Qt::AA_ShareOpenGLContexts))
                d->reset();
        }
        if (QWidgetRepaintManager *repaintManager = QWidgetPrivate::get(window())->maybeRepaintManager()) {
            if (!d->initialized && !size().isEmpty() && repaintManager->rhi()) {
                d->initialize();
                if (d->initialized) {
                    d->recreateFbos();
                    // QTBUG-89812: generate a paint event, like resize would do,
                    // otherwise a QOpenGLWidget in a QDockWidget may not show the
                    // content upon (un)docking.
                    d->sendPaintEvent(QRect(QPoint(0, 0), size()));
                }
            }
        }
        break;
    case QEvent::DevicePixelRatioChange:
        if (d->initialized && d->paintDevice->devicePixelRatio() != devicePixelRatio())
            d->recreateFbos();
        break;
    default:
        break;
    }
    return QWidget::event(e);
}

QT_END_NAMESPACE

#include "moc_qopenglwidget.cpp"
