// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmcompositor.h"
#include "qwasmwindow.h"
#include "qwasmeventtranslator.h"
#include "qwasmeventdispatcher.h"
#include "qwasmclipboard.h"
#include "qwasmevent.h"

#include <QtOpenGL/qopengltexture.h>

#include <QtGui/private/qwindow_p.h>
#include <QtGui/qopenglcontext.h>
#include <QtGui/qopenglfunctions.h>
#include <QtGui/qoffscreensurface.h>
#include <QtGui/qpainter.h>

#include <private/qguiapplication_p.h>

#include <qpa/qwindowsysteminterface.h>
#include <QtCore/qcoreapplication.h>
#include <QtGui/qguiapplication.h>

#include <emscripten/bind.h>

namespace {
QWasmWindow *asWasmWindow(QWindow *window)
{
    return static_cast<QWasmWindow*>(window->handle());
}
}  // namespace

using namespace emscripten;

Q_GUI_EXPORT int qt_defaultDpiX();

bool g_scrollingInvertedFromDevice = false;

static void mouseWheelEvent(emscripten::val event)
{
    emscripten::val wheelInverted = event["webkitDirectionInvertedFromDevice"];
    if (wheelInverted.as<bool>())
        g_scrollingInvertedFromDevice = true;
}

EMSCRIPTEN_BINDINGS(qtMouseModule) {
        function("qtMouseWheelEvent", &mouseWheelEvent);
}

QWasmCompositor::QWasmCompositor(QWasmScreen *screen)
    : QObject(screen),
      m_windowManipulation(screen),
      m_windowStack(std::bind(&QWasmCompositor::onTopWindowChanged, this)),
      m_blitter(new QOpenGLTextureBlitter),
      m_eventTranslator(std::make_unique<QWasmEventTranslator>())
{
    m_touchDevice = std::make_unique<QPointingDevice>(
            "touchscreen", 1, QInputDevice::DeviceType::TouchScreen,
            QPointingDevice::PointerType::Finger,
            QPointingDevice::Capability::Position | QPointingDevice::Capability::Area
                | QPointingDevice::Capability::NormalizedPosition,
            10, 0);
    QWindowSystemInterface::registerInputDevice(m_touchDevice.get());
}

QWasmCompositor::~QWasmCompositor()
{
    m_windowUnderMouse.clear();

    if (m_requestAnimationFrameId != -1)
        emscripten_cancel_animation_frame(m_requestAnimationFrameId);

    deregisterEventHandlers();
    destroy();
}

void QWasmCompositor::deregisterEventHandlers()
{
    QByteArray canvasSelector = screen()->canvasTargetId().toUtf8();
    emscripten_set_keydown_callback(canvasSelector.constData(), 0, 0, NULL);
    emscripten_set_keyup_callback(canvasSelector.constData(),  0, 0, NULL);

    emscripten_set_focus_callback(canvasSelector.constData(),  0, 0, NULL);

    emscripten_set_wheel_callback(canvasSelector.constData(),  0, 0, NULL);

    emscripten_set_touchstart_callback(canvasSelector.constData(),  0, 0, NULL);
    emscripten_set_touchend_callback(canvasSelector.constData(),  0, 0, NULL);
    emscripten_set_touchmove_callback(canvasSelector.constData(),  0, 0, NULL);
    emscripten_set_touchcancel_callback(canvasSelector.constData(),  0, 0, NULL);

    val canvas = screen()->canvas();
    canvas.call<void>("removeEventListener",
        std::string("drop"),
        val::module_property("qtDrop"), val(true));
}

void QWasmCompositor::destroy()
{
    // Destroy OpenGL resources. This is done here in a separate function
    // which can be called while screen() still returns a valid screen
    // (which it might not, during destruction). A valid QScreen is
    // a requirement for QOffscreenSurface on Wasm since the native
    // context is tied to a single canvas.
    if (m_context) {
        QOffscreenSurface offScreenSurface(screen()->screen());
        offScreenSurface.setFormat(m_context->format());
        offScreenSurface.create();
        m_context->makeCurrent(&offScreenSurface);
        for (QWasmWindow *window : m_windowStack)
            window->destroy();
        m_blitter.reset(nullptr);
        m_context.reset(nullptr);
    }

    m_isEnabled = false; // prevent frame() from creating a new m_context
}

void QWasmCompositor::initEventHandlers()
{
    QByteArray canvasSelector = screen()->canvasTargetId().toUtf8();

    if (platform() == Platform::MacOS) {
        if (!emscripten::val::global("window")["safari"].isUndefined()) {
            val canvas = screen()->canvas();
            canvas.call<void>("addEventListener",
                              val("wheel"),
                              val::module_property("qtMouseWheelEvent"));
        }
    }

    constexpr EM_BOOL UseCapture = 1;

    emscripten_set_keydown_callback(canvasSelector.constData(), (void *)this, UseCapture, &keyboard_cb);
    emscripten_set_keyup_callback(canvasSelector.constData(), (void *)this, UseCapture, &keyboard_cb);

    val canvas = screen()->canvas();
    const auto callback = std::function([this](emscripten::val event) {
        if (processPointer(*PointerEvent::fromWeb(event)))
            event.call<void>("preventDefault");
    });

    m_pointerDownCallback = std::make_unique<qstdweb::EventCallback>(canvas, "pointerdown", callback);
    m_pointerMoveCallback = std::make_unique<qstdweb::EventCallback>(canvas, "pointermove", callback);
    m_pointerUpCallback = std::make_unique<qstdweb::EventCallback>(canvas, "pointerup", callback);
    m_pointerEnterCallback = std::make_unique<qstdweb::EventCallback>(canvas, "pointerenter", callback);
    m_pointerLeaveCallback = std::make_unique<qstdweb::EventCallback>(canvas, "pointerleave", callback);

    emscripten_set_focus_callback(canvasSelector.constData(), (void *)this, UseCapture, &focus_cb);

    emscripten_set_wheel_callback(canvasSelector.constData(), (void *)this, UseCapture, &wheel_cb);

    emscripten_set_touchstart_callback(canvasSelector.constData(), (void *)this, UseCapture, &touchCallback);
    emscripten_set_touchend_callback(canvasSelector.constData(), (void *)this, UseCapture, &touchCallback);
    emscripten_set_touchmove_callback(canvasSelector.constData(), (void *)this, UseCapture, &touchCallback);
    emscripten_set_touchcancel_callback(canvasSelector.constData(), (void *)this, UseCapture, &touchCallback);

    canvas.call<void>("addEventListener",
        std::string("drop"),
        val::module_property("qtDrop"), val(true));
    canvas.set("data-qtdropcontext", // ? unique
                       emscripten::val(quintptr(reinterpret_cast<void *>(screen()))));
}

void QWasmCompositor::setEnabled(bool enabled)
{
    m_isEnabled = enabled;
}

void QWasmCompositor::startResize(Qt::Edges edges)
{
    m_windowManipulation.startResize(edges);
}

void QWasmCompositor::addWindow(QWasmWindow *window)
{
    m_windowVisibility.insert(window, false);
    m_windowStack.pushWindow(window);
    m_windowStack.topWindow()->requestActivateWindow();
}

void QWasmCompositor::removeWindow(QWasmWindow *window)
{
    m_windowVisibility.remove(window);
    m_requestUpdateWindows.remove(window);
    m_windowStack.removeWindow(window);
    if (m_windowStack.topWindow())
        m_windowStack.topWindow()->requestActivateWindow();
}

void QWasmCompositor::setVisible(QWasmWindow *window, bool visible)
{
    const bool wasVisible = m_windowVisibility[window];
    if (wasVisible == visible)
        return;

    m_windowVisibility[window] = visible;
    if (!visible)
        m_globalDamage = window->window()->geometry(); // repaint previously covered area.

    requestUpdateWindow(window, QWasmCompositor::ExposeEventDelivery);
}

void QWasmCompositor::raise(QWasmWindow *window)
{
    m_windowStack.raise(window);
}

void QWasmCompositor::lower(QWasmWindow *window)
{
    m_globalDamage = window->window()->geometry(); // repaint previously covered area.
    m_windowStack.lower(window);
}

int QWasmCompositor::windowCount() const
{
    return m_windowStack.size();
}

QWindow *QWasmCompositor::windowAt(QPoint targetPointInScreenCoords, int padding) const
{
    const auto found = std::find_if(
            m_windowStack.begin(), m_windowStack.end(),
            [this, padding, &targetPointInScreenCoords](const QWasmWindow *window) {
                const QRect geometry = window->windowFrameGeometry().adjusted(-padding, -padding,
                                                                              padding, padding);

                return m_windowVisibility[window] && geometry.contains(targetPointInScreenCoords);
            });
    return found != m_windowStack.end() ? (*found)->window() : nullptr;
}

QWindow *QWasmCompositor::keyWindow() const
{
    return m_windowStack.topWindow() ? m_windowStack.topWindow()->window() : nullptr;
}

void QWasmCompositor::blit(QOpenGLTextureBlitter *blitter, QWasmScreen *screen, const QOpenGLTexture *texture, QRect targetGeometry)
{
    QMatrix4x4 m;
    m.translate(-1.0f, -1.0f);

    m.scale(2.0f / (float)screen->geometry().width(),
            2.0f / (float)screen->geometry().height());

    m.translate((float)targetGeometry.width() / 2.0f,
                (float)-targetGeometry.height() / 2.0f);

    m.translate(targetGeometry.x(), screen->geometry().height() - targetGeometry.y());

    m.scale(0.5f * (float)targetGeometry.width(),
            0.5f * (float)targetGeometry.height());

    blitter->blit(texture->textureId(), m, QOpenGLTextureBlitter::OriginTopLeft);
}

void QWasmCompositor::drawWindowContent(QOpenGLTextureBlitter *blitter, QWasmScreen *screen,
                                        const QWasmWindow *window)
{
    QWasmBackingStore *backingStore = window->backingStore();
    if (!backingStore)
        return;

    QOpenGLTexture const *texture = backingStore->getUpdatedTexture();
    QRect windowCanvasGeometry = window->geometry().translated(-screen->geometry().topLeft());
    blit(blitter, screen, texture, windowCanvasGeometry);
}

void QWasmCompositor::requestUpdateAllWindows()
{
    m_requestUpdateAllWindows = true;
    requestUpdate();
}

void QWasmCompositor::requestUpdateWindow(QWasmWindow *window, UpdateRequestDeliveryType updateType)
{
    auto it = m_requestUpdateWindows.find(window);
    if (it == m_requestUpdateWindows.end()) {
        m_requestUpdateWindows.insert(window, updateType);
    } else {
        // Already registered, but upgrade ExposeEventDeliveryType to UpdateRequestDeliveryType.
        // if needed, to make sure QWindow::updateRequest's are matched.
        if (it.value() == ExposeEventDelivery && updateType == UpdateRequestDelivery)
            it.value() = UpdateRequestDelivery;
    }

    requestUpdate();
}

// Requests an update/new frame using RequestAnimationFrame
void QWasmCompositor::requestUpdate()
{
    if (m_requestAnimationFrameId != -1)
        return;

    static auto frame = [](double frameTime, void *context) -> int {
        Q_UNUSED(frameTime);
        QWasmCompositor *compositor = reinterpret_cast<QWasmCompositor *>(context);
        compositor->m_requestAnimationFrameId = -1;
        compositor->deliverUpdateRequests();
        return 0;
    };
    m_requestAnimationFrameId = emscripten_request_animation_frame(frame, this);
}

void QWasmCompositor::deliverUpdateRequests()
{
    // We may get new update requests during the window content update below:
    // prepare for recording the new update set by setting aside the current
    // update set.
    auto requestUpdateWindows = m_requestUpdateWindows;
    m_requestUpdateWindows.clear();
    bool requestUpdateAllWindows = m_requestUpdateAllWindows;
    m_requestUpdateAllWindows = false;

    // Update window content, either all windows or a spesific set of windows. Use the correct update
    // type: QWindow subclasses expect that requested and delivered updateRequests matches exactly.
    m_inDeliverUpdateRequest = true;
    if (requestUpdateAllWindows) {
        for (QWasmWindow *window : m_windowStack) {
            auto it = requestUpdateWindows.find(window);
            UpdateRequestDeliveryType updateType =
                (it == m_requestUpdateWindows.end() ? ExposeEventDelivery : it.value());
            deliverUpdateRequest(window, updateType);
        }
    } else {
        for (auto it = requestUpdateWindows.constBegin(); it != requestUpdateWindows.constEnd(); ++it) {
            auto *window = it.key();
            UpdateRequestDeliveryType updateType = it.value();
            deliverUpdateRequest(window, updateType);
        }
    }
    m_inDeliverUpdateRequest = false;

    // Compose window content
    frame();
}

void QWasmCompositor::deliverUpdateRequest(QWasmWindow *window, UpdateRequestDeliveryType updateType)
{
    // update by deliverUpdateRequest and expose event accordingly.
    if (updateType == UpdateRequestDelivery) {
        window->QPlatformWindow::deliverUpdateRequest();
    } else {
        QWindow *qwindow = window->window();
        QWindowSystemInterface::handleExposeEvent<QWindowSystemInterface::SynchronousDelivery>(
            qwindow, QRect(QPoint(0, 0), qwindow->geometry().size()));
    }
}

void QWasmCompositor::handleBackingStoreFlush()
{
    // Request update to flush the updated backing store content,
    // unless we are currently processing an update, in which case
    // the new content will flushed as a part of that update.
    if (!m_inDeliverUpdateRequest)
        requestUpdate();
}

int dpiScaled(qreal value)
{
    return value * (qreal(qt_defaultDpiX()) / 96.0);
}

void QWasmCompositor::drawWindowDecorations(QOpenGLTextureBlitter *blitter, QWasmScreen *screen,
                                            const QWasmWindow *window)
{
    int width = window->windowFrameGeometry().width();
    int height = window->windowFrameGeometry().height();
    qreal dpr = window->devicePixelRatio();

    QImage image(QSize(width * dpr, height * dpr), QImage::Format_ARGB32_Premultiplied);
    image.setDevicePixelRatio(dpr);
    QPainter painter(&image);
    painter.fillRect(QRect(0, 0, width, height), painter.background());

    window->drawTitleBar(&painter);

    QWasmFrameOptions frameOptions;
    frameOptions.rect = QRect(0, 0, width, height);
    frameOptions.lineWidth = dpiScaled(4.);

    drawFrameWindow(frameOptions, &painter);

    painter.end();

    QOpenGLTexture texture(QOpenGLTexture::Target2D);
    texture.setMinificationFilter(QOpenGLTexture::Nearest);
    texture.setMagnificationFilter(QOpenGLTexture::Nearest);
    texture.setWrapMode(QOpenGLTexture::ClampToEdge);
    texture.setFormat(QOpenGLTexture::RGBAFormat);
    texture.setSize(image.width(), image.height());
    texture.setMipLevels(1);
    texture.allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8);

    QOpenGLPixelTransferOptions uploadOptions;
    uploadOptions.setAlignment(1);

    texture.create();
    texture.bind();

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image.width(), image.height(), GL_RGBA,
                    GL_UNSIGNED_BYTE, image.constScanLine(0));

    QRect windowCanvasGeometry = window->windowFrameGeometry().translated(-screen->geometry().topLeft());
    blit(blitter, screen, &texture, windowCanvasGeometry);
}

void QWasmCompositor::drawFrameWindow(QWasmFrameOptions options, QPainter *painter)
{
    int x = options.rect.x();
    int y = options.rect.y();
    int w = options.rect.width();
    int h = options.rect.height();
    const QColor &c1 = options.palette.light().color();
    const QColor &c2 = options.palette.shadow().color();
    const QColor &c3 = options.palette.midlight().color();
    const QColor &c4 = options.palette.dark().color();
    const QBrush *fill = nullptr;

    const qreal devicePixelRatio = painter->device()->devicePixelRatio();
    if (!qFuzzyCompare(devicePixelRatio, qreal(1))) {
        const qreal inverseScale = qreal(1) / devicePixelRatio;
        painter->scale(inverseScale, inverseScale);
        x = qRound(devicePixelRatio * x);
        y = qRound(devicePixelRatio * y);
        w = qRound(devicePixelRatio * w);
        h = qRound(devicePixelRatio * h);
    }

    QPen oldPen = painter->pen();
    QPoint a[3] = { QPoint(x, y+h-2), QPoint(x, y), QPoint(x+w-2, y) };
    painter->setPen(c1);
    painter->drawPolyline(a, 3);
    QPoint b[3] = { QPoint(x, y+h-1), QPoint(x+w-1, y+h-1), QPoint(x+w-1, y) };
    painter->setPen(c2);
    painter->drawPolyline(b, 3);
    if (w > 4 && h > 4) {
        QPoint c[3] = { QPoint(x+1, y+h-3), QPoint(x+1, y+1), QPoint(x+w-3, y+1) };
        painter->setPen(c3);
        painter->drawPolyline(c, 3);
        QPoint d[3] = { QPoint(x+1, y+h-2), QPoint(x+w-2, y+h-2), QPoint(x+w-2, y+1) };
        painter->setPen(c4);
        painter->drawPolyline(d, 3);
        if (fill)
            painter->fillRect(QRect(x+2, y+2, w-4, h-4), *fill);
    }
    painter->setPen(oldPen);
}

void QWasmCompositor::drawWindow(QOpenGLTextureBlitter *blitter, QWasmScreen *screen,
                                 const QWasmWindow *window)
{
    if (window->window()->type() != Qt::Popup && !(window->m_windowState & Qt::WindowFullScreen))
        drawWindowDecorations(blitter, screen, window);
    drawWindowContent(blitter, screen, window);
}

void QWasmCompositor::frame()
{
    if (!m_isEnabled || m_windowStack.empty() || !screen())
        return;

    QWasmWindow *someWindow = nullptr;

    for (QWasmWindow *window : m_windowStack) {
        if (window->window()->surfaceClass() == QSurface::Window
            && qt_window_private(window->window())->receivedExpose) {
            someWindow = window;
            break;
        }
    }

    if (!someWindow)
        return;

    if (m_context.isNull()) {
        m_context.reset(new QOpenGLContext());
        m_context->setFormat(someWindow->window()->requestedFormat());
        m_context->setScreen(screen()->screen());
        m_context->create();
    }

    bool ok = m_context->makeCurrent(someWindow->window());
    if (!ok)
        return;

    if (!m_blitter->isCreated())
        m_blitter->create();

    qreal dpr = screen()->devicePixelRatio();
    glViewport(0, 0, screen()->geometry().width() * dpr, screen()->geometry().height() * dpr);

    m_context->functions()->glClearColor(0.2, 0.2, 0.2, 1.0);
    m_context->functions()->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    m_blitter->bind();
    m_blitter->setRedBlueSwizzle(true);

    std::for_each(m_windowStack.rbegin(), m_windowStack.rend(), [this](const QWasmWindow *window) {
        if (m_windowVisibility[window])
            drawWindow(m_blitter.data(), screen(), window);
    });

    m_blitter->release();

    if (someWindow && someWindow->window()->surfaceType() == QSurface::OpenGLSurface)
        m_context->swapBuffers(someWindow->window());
}

void QWasmCompositor::WindowManipulation::resizeWindow(const QPoint& amount)
{
    const auto& minShrink = std::get<ResizeState>(m_state->operationSpecific).m_minShrink;
    const auto& maxGrow = std::get<ResizeState>(m_state->operationSpecific).m_maxGrow;
    const auto &resizeEdges = std::get<ResizeState>(m_state->operationSpecific).m_resizeEdges;

    const QPoint cappedGrowVector(
            std::min(maxGrow.x(),
                     std::max(minShrink.x(),
                              (resizeEdges & Qt::Edge::LeftEdge)            ? -amount.x()
                                      : (resizeEdges & Qt::Edge::RightEdge) ? amount.x()
                                                                            : 0)),
            std::min(maxGrow.y(),
                     std::max(minShrink.y(),
                              (resizeEdges & Qt::Edge::TopEdge)              ? -amount.y()
                                      : (resizeEdges & Qt::Edge::BottomEdge) ? amount.y()
                                                                             : 0)));

    const auto& initialBounds =
        std::get<ResizeState>(m_state->operationSpecific).m_initialWindowBounds;
    m_state->window->setGeometry(initialBounds.adjusted(
            (resizeEdges & Qt::Edge::LeftEdge) ? -cappedGrowVector.x() : 0,
            (resizeEdges & Qt::Edge::TopEdge) ? -cappedGrowVector.y() : 0,
            (resizeEdges & Qt::Edge::RightEdge) ? cappedGrowVector.x() : 0,
            (resizeEdges & Qt::Edge::BottomEdge) ? cappedGrowVector.y() : 0));
}

void QWasmCompositor::onTopWindowChanged()
{
    requestUpdate();
}

QWasmScreen *QWasmCompositor::screen()
{
    return static_cast<QWasmScreen *>(parent());
}

QOpenGLContext *QWasmCompositor::context()
{
    return m_context.data();
}

int QWasmCompositor::keyboard_cb(int eventType, const EmscriptenKeyboardEvent *keyEvent, void *userData)
{
    QWasmCompositor *wasmCompositor = reinterpret_cast<QWasmCompositor *>(userData);
    return static_cast<int>(wasmCompositor->processKeyboard(eventType, keyEvent));
}

int QWasmCompositor::focus_cb(int eventType, const EmscriptenFocusEvent *focusEvent, void *userData)
{
    Q_UNUSED(eventType)
    Q_UNUSED(focusEvent)
    Q_UNUSED(userData)

    return 0;
}

int QWasmCompositor::wheel_cb(int eventType, const EmscriptenWheelEvent *wheelEvent, void *userData)
{
    QWasmCompositor *compositor = (QWasmCompositor *) userData;
    return static_cast<int>(compositor->processWheel(eventType, wheelEvent));
}

int QWasmCompositor::touchCallback(int eventType, const EmscriptenTouchEvent *touchEvent, void *userData)
{
    auto compositor = reinterpret_cast<QWasmCompositor*>(userData);
    return static_cast<int>(compositor->handleTouch(eventType, touchEvent));
}

bool QWasmCompositor::processPointer(const PointerEvent& event)
{
    if (event.pointerType != PointerType::Mouse)
        return false;

    QWindow *const targetWindow = ([this, &event]() -> QWindow * {
        auto *targetWindow = m_mouseCaptureWindow != nullptr ? m_mouseCaptureWindow.get()
                : m_windowManipulation.operation() == WindowManipulation::Operation::None
                ? screen()->compositor()->windowAt(event.point, 5)
                : nullptr;

        return targetWindow ? targetWindow : m_lastMouseTargetWindow.get();
    })();
    if (targetWindow)
        m_lastMouseTargetWindow = targetWindow;

    const QPoint pointInTargetWindowCoords = targetWindow->mapFromGlobal(event.point);
    const bool pointerIsWithinTargetWindowBounds = targetWindow->geometry().contains(event.point);
    const bool isTargetWindowBlocked = QGuiApplicationPrivate::instance()->isWindowBlocked(targetWindow);

    if (m_mouseInCanvas && m_windowUnderMouse != targetWindow && pointerIsWithinTargetWindowBounds) {
        // delayed mouse enter
        enterWindow(targetWindow, pointInTargetWindowCoords, event.point);
        m_windowUnderMouse = targetWindow;
    }

    QWasmWindow *wasmTargetWindow = asWasmWindow(targetWindow);
    Qt::WindowStates windowState = targetWindow->windowState();
    const bool isTargetWindowResizable = !windowState.testFlag(Qt::WindowMaximized) && !windowState.testFlag(Qt::WindowFullScreen);

    switch (event.type) {
    case EventType::PointerDown:
    {
        screen()->canvas().call<void>("setPointerCapture", event.pointerId);

        if (targetWindow)
            targetWindow->requestActivate();

        m_pressedWindow = targetWindow;

        m_windowManipulation.onPointerDown(event, targetWindow);

        wasmTargetWindow->injectMousePressed(pointInTargetWindowCoords, event.point,
                                             event.mouseButton, event.modifiers);
        break;
    }
    case EventType::PointerUp:
    {
        screen()->canvas().call<void>("releasePointerCapture", event.pointerId);

        m_windowManipulation.onPointerUp(event);

        if (m_pressedWindow) {
            // Always deliver the released event to the same window that was pressed
            asWasmWindow(m_pressedWindow)
                    ->injectMouseReleased(pointInTargetWindowCoords, event.point, event.mouseButton,
                                          event.modifiers);
            if (event.mouseButton == Qt::MouseButton::LeftButton)
                m_pressedWindow = nullptr;
        } else {
            wasmTargetWindow->injectMouseReleased(pointInTargetWindowCoords, event.point,
                                                  event.mouseButton, event.modifiers);
        }
        break;
    }
    case EventType::PointerMove:
    {
        if (wasmTargetWindow && event.mouseButtons.testFlag(Qt::NoButton)) {
            const bool isOnResizeRegion = wasmTargetWindow->isPointOnResizeRegion(event.point);

            if (isTargetWindowResizable && isOnResizeRegion && !isTargetWindowBlocked) {
                const QCursor resizingCursor = QWasmEventTranslator::cursorForEdges(
                        wasmTargetWindow->resizeEdgesAtPoint(event.point));

                if (resizingCursor != targetWindow->cursor()) {
                    m_isResizeCursorDisplayed = true;
                    QWasmCursor::setOverrideWasmCursor(resizingCursor, targetWindow->screen());
                }
            } else if (m_isResizeCursorDisplayed) {  // off resizing area
                m_isResizeCursorDisplayed = false;
                QWasmCursor::clearOverrideWasmCursor(targetWindow->screen());
            }
        }

        m_windowManipulation.onPointerMove(event);
        if (m_windowManipulation.operation() != WindowManipulation::Operation::None)
            requestUpdate();
        break;
    }
    case EventType::PointerEnter:
        processMouseEnter(nullptr);
        break;
    case EventType::PointerLeave:
        processMouseLeave();
        break;
    default:
        break;
    };

    if (!pointerIsWithinTargetWindowBounds && event.mouseButtons.testFlag(Qt::NoButton)) {
        leaveWindow(m_lastMouseTargetWindow);
    }

    const bool eventAccepted = deliverEventToTarget(event, targetWindow);
    if (!eventAccepted && event.type == EventType::PointerDown)
        QGuiApplicationPrivate::instance()->closeAllPopups();
    return eventAccepted;
}

bool QWasmCompositor::deliverEventToTarget(const PointerEvent &event, QWindow *eventTarget)
{
    Q_ASSERT(!m_mouseCaptureWindow || m_mouseCaptureWindow.get() == eventTarget);

    const QPoint targetPointClippedToScreen(
            std::max(screen()->geometry().left(),
                     std::min(screen()->geometry().right(), event.point.x())),
            std::max(screen()->geometry().top(),
                     std::min(screen()->geometry().bottom(), event.point.y())));

    bool deliveringToPreviouslyClickedWindow = false;

    if (!eventTarget) {
        if (event.type != EventType::PointerUp || !m_lastMouseTargetWindow)
            return false;

        eventTarget = m_lastMouseTargetWindow;
        m_lastMouseTargetWindow = nullptr;
        deliveringToPreviouslyClickedWindow = true;
    }

    WindowArea windowArea = WindowArea::Client;
    if (!deliveringToPreviouslyClickedWindow && !m_mouseCaptureWindow
        && !eventTarget->geometry().contains(targetPointClippedToScreen)) {
        if (!eventTarget->frameGeometry().contains(targetPointClippedToScreen))
            return false;
        windowArea = WindowArea::NonClient;
    }

    const QEvent::Type eventType =
        MouseEvent::mouseEventTypeFromEventType(event.type, windowArea);

    return eventType != QEvent::None &&
           QWindowSystemInterface::handleMouseEvent<QWindowSystemInterface::SynchronousDelivery>(
               eventTarget, QWasmIntegration::getTimestamp(),
               eventTarget->mapFromGlobal(targetPointClippedToScreen),
               targetPointClippedToScreen, event.mouseButtons, event.mouseButton,
               eventType, event.modifiers);
}

QWasmCompositor::WindowManipulation::WindowManipulation(QWasmScreen *screen)
    : m_screen(screen)
{
    Q_ASSERT(!!screen);
}

QWasmCompositor::WindowManipulation::Operation QWasmCompositor::WindowManipulation::operation() const
{
    if (!m_state)
        return Operation::None;

    return std::holds_alternative<MoveState>(m_state->operationSpecific)
        ? Operation::Move : Operation::Resize;
}

void QWasmCompositor::WindowManipulation::onPointerDown(
    const PointerEvent& event, QWindow* windowAtPoint)
{
    // Only one operation at a time.
    if (operation() != Operation::None)
        return;

    if (event.mouseButton != Qt::MouseButton::LeftButton)
        return;

    const bool isTargetWindowResizable =
        !windowAtPoint->windowStates().testFlag(Qt::WindowMaximized) &&
        !windowAtPoint->windowStates().testFlag(Qt::WindowFullScreen);
    if (!isTargetWindowResizable)
        return;

    const bool isTargetWindowBlocked =
        QGuiApplicationPrivate::instance()->isWindowBlocked(windowAtPoint);
    if (isTargetWindowBlocked)
        return;

    std::unique_ptr<std::variant<ResizeState, MoveState>> operationSpecific;
    if (asWasmWindow(windowAtPoint)->isPointOnTitle(event.point)) {
        operationSpecific = std::make_unique<std::variant<ResizeState, MoveState>>(
                MoveState{ .m_lastPointInScreenCoords = event.point });
    } else if (asWasmWindow(windowAtPoint)->isPointOnResizeRegion(event.point)) {
        operationSpecific = std::make_unique<std::variant<ResizeState, MoveState>>(ResizeState{
                .m_resizeEdges = asWasmWindow(windowAtPoint)->resizeEdgesAtPoint(event.point),
                .m_originInScreenCoords = event.point,
                .m_initialWindowBounds = windowAtPoint->geometry(),
                .m_minShrink =
                        QPoint(windowAtPoint->minimumWidth() - windowAtPoint->geometry().width(),
                               windowAtPoint->minimumHeight() - windowAtPoint->geometry().height()),
                .m_maxGrow =
                        QPoint(windowAtPoint->maximumWidth() - windowAtPoint->geometry().width(),
                               windowAtPoint->maximumHeight() - windowAtPoint->geometry().height()),
        });
    } else {
        return;
    }

    m_state.reset(new OperationState{
        .pointerId = event.pointerId,
        .window = windowAtPoint,
        .operationSpecific = std::move(*operationSpecific),
    });
}

void QWasmCompositor::WindowManipulation::onPointerMove(
    const PointerEvent& event)
{
    m_systemDragInitData = {
        .lastMouseMovePoint = m_screen->clipPoint(event.point),
        .lastMousePointerId = event.pointerId,
    };

    if (operation() == Operation::None || event.pointerId != m_state->pointerId)
        return;

    switch (operation()) {
        case Operation::Move: {
            const QPoint targetPointClippedToScreen = m_screen->clipPoint(event.point);
            const QPoint difference = targetPointClippedToScreen -
                std::get<MoveState>(m_state->operationSpecific).m_lastPointInScreenCoords;

            std::get<MoveState>(m_state->operationSpecific).m_lastPointInScreenCoords = targetPointClippedToScreen;

            m_state->window->setPosition(m_state->window->position() + difference);
            break;
        }
        case Operation::Resize: {
            const auto pointInScreenCoords = m_screen->geometry().topLeft() + event.point;
            resizeWindow(pointInScreenCoords -
                std::get<ResizeState>(m_state->operationSpecific).m_originInScreenCoords);
            break;
        }
        case Operation::None:
            Q_ASSERT(0);
            break;
    }
}

void QWasmCompositor::WindowManipulation::onPointerUp(const PointerEvent& event)
{
    if (operation() == Operation::None || event.mouseButtons != 0 || event.pointerId != m_state->pointerId)
        return;

    m_state.reset();
}

void QWasmCompositor::WindowManipulation::startResize(Qt::Edges edges)
{
    Q_ASSERT_X(operation() == Operation::None, Q_FUNC_INFO,
               "Resize must not start anew when one is in progress");

    auto *window = m_screen->compositor()->windowAt(m_systemDragInitData.lastMouseMovePoint);
    if (Q_UNLIKELY(!window))
        return;

    m_state.reset(new OperationState{
        .pointerId = m_systemDragInitData.lastMousePointerId,
        .window = window,
        .operationSpecific =
            ResizeState{
                .m_resizeEdges = edges,
                .m_originInScreenCoords = m_systemDragInitData.lastMouseMovePoint,
                .m_initialWindowBounds = window->geometry(),
                .m_minShrink =
                    QPoint(window->minimumWidth() - window->geometry().width(),
                        window->minimumHeight() - window->geometry().height()),
                .m_maxGrow =
                    QPoint(window->maximumWidth() - window->geometry().width(),
                        window->maximumHeight() - window->geometry().height()),
            },
    });
    m_screen->canvas().call<void>("setPointerCapture", m_systemDragInitData.lastMousePointerId);
}

bool QWasmCompositor::processKeyboard(int eventType, const EmscriptenKeyboardEvent *emKeyEvent)
{
    constexpr bool ProceedToNativeEvent = false;
    Q_ASSERT(eventType == EMSCRIPTEN_EVENT_KEYDOWN || eventType == EMSCRIPTEN_EVENT_KEYUP);

    auto translatedEvent = m_eventTranslator->translateKeyEvent(eventType, emKeyEvent);

    const QFlags<Qt::KeyboardModifier> modifiers = KeyboardModifier::getForEvent(*emKeyEvent);

    const auto clipboardResult = QWasmIntegration::get()->getWasmClipboard()->processKeyboard(
            translatedEvent, modifiers);

    using ProcessKeyboardResult = QWasmClipboard::ProcessKeyboardResult;
    if (clipboardResult == ProcessKeyboardResult::NativeClipboardEventNeeded)
        return ProceedToNativeEvent;

    if (translatedEvent.text.isEmpty())
        translatedEvent.text = QString(emKeyEvent->key);
    if (translatedEvent.text.size() > 1)
        translatedEvent.text.clear();
    const auto result =
            QWindowSystemInterface::handleKeyEvent<QWindowSystemInterface::SynchronousDelivery>(
                    0, translatedEvent.type, translatedEvent.key, modifiers, translatedEvent.text);
    return clipboardResult == ProcessKeyboardResult::NativeClipboardEventAndCopiedDataNeeded
            ? ProceedToNativeEvent
            : result;
}

bool QWasmCompositor::processWheel(int eventType, const EmscriptenWheelEvent *wheelEvent)
{
    Q_UNUSED(eventType);

    const EmscriptenMouseEvent* mouseEvent = &wheelEvent->mouse;

    int scrollFactor = 0;
    switch (wheelEvent->deltaMode) {
        case DOM_DELTA_PIXEL:
            scrollFactor = 1;
            break;
        case DOM_DELTA_LINE:
            scrollFactor = 12;
            break;
        case DOM_DELTA_PAGE:
            scrollFactor = 20;
            break;
    };

    scrollFactor = -scrollFactor; // Web scroll deltas are inverted from Qt deltas.

    Qt::KeyboardModifiers modifiers = KeyboardModifier::getForEvent(*mouseEvent);
    QPoint targetPointInCanvasCoords(mouseEvent->targetX, mouseEvent->targetY);
    QPoint targetPointInScreenCoords = screen()->geometry().topLeft() + targetPointInCanvasCoords;

    QWindow *targetWindow = screen()->compositor()->windowAt(targetPointInScreenCoords, 5);
    if (!targetWindow)
        return 0;
    QPoint pointInTargetWindowCoords = targetWindow->mapFromGlobal(targetPointInScreenCoords);

    QPoint pixelDelta;

    if (wheelEvent->deltaY != 0) pixelDelta.setY(wheelEvent->deltaY * scrollFactor);
    if (wheelEvent->deltaX != 0) pixelDelta.setX(wheelEvent->deltaX * scrollFactor);

    QPoint angleDelta = pixelDelta; // FIXME: convert from pixels?

    bool accepted = QWindowSystemInterface::handleWheelEvent(
            targetWindow, QWasmIntegration::getTimestamp(), pointInTargetWindowCoords,
            targetPointInScreenCoords, pixelDelta, angleDelta, modifiers,
            Qt::NoScrollPhase, Qt::MouseEventNotSynthesized,
            g_scrollingInvertedFromDevice);
    return accepted;
}

int QWasmCompositor::handleTouch(int eventType, const EmscriptenTouchEvent *touchEvent)
{
    QList<QWindowSystemInterface::TouchPoint> touchPointList;
    touchPointList.reserve(touchEvent->numTouches);
    QWindow *targetWindow = nullptr;

    for (int i = 0; i < touchEvent->numTouches; i++) {

        const EmscriptenTouchPoint *touches = &touchEvent->touches[i];

        QPoint targetPointInCanvasCoords(touches->targetX, touches->targetY);
        QPoint targetPointInScreenCoords = screen()->geometry().topLeft() + targetPointInCanvasCoords;

        targetWindow = screen()->compositor()->windowAt(targetPointInScreenCoords, 5);
        if (targetWindow == nullptr)
            continue;

        QWindowSystemInterface::TouchPoint touchPoint;

        touchPoint.area = QRect(0, 0, 8, 8);
        touchPoint.id = touches->identifier;
        touchPoint.pressure = 1.0;

        touchPoint.area.moveCenter(targetPointInScreenCoords);

        const auto tp = m_pressedTouchIds.constFind(touchPoint.id);
        if (tp != m_pressedTouchIds.constEnd())
            touchPoint.normalPosition = tp.value();

        QPointF pointInTargetWindowCoords = QPointF(targetWindow->mapFromGlobal(targetPointInScreenCoords));
        QPointF normalPosition(pointInTargetWindowCoords.x() / targetWindow->width(),
                               pointInTargetWindowCoords.y() / targetWindow->height());

        const bool stationaryTouchPoint = (normalPosition == touchPoint.normalPosition);
        touchPoint.normalPosition = normalPosition;

        switch (eventType) {
            case EMSCRIPTEN_EVENT_TOUCHSTART:
                if (tp != m_pressedTouchIds.constEnd()) {
                    touchPoint.state = (stationaryTouchPoint
                                        ? QEventPoint::State::Stationary
                                        : QEventPoint::State::Updated);
                } else {
                    touchPoint.state = QEventPoint::State::Pressed;
                }
                m_pressedTouchIds.insert(touchPoint.id, touchPoint.normalPosition);

                break;
            case EMSCRIPTEN_EVENT_TOUCHEND:
                touchPoint.state = QEventPoint::State::Released;
                m_pressedTouchIds.remove(touchPoint.id);
                break;
            case EMSCRIPTEN_EVENT_TOUCHMOVE:
                touchPoint.state = (stationaryTouchPoint
                                    ? QEventPoint::State::Stationary
                                    : QEventPoint::State::Updated);

                m_pressedTouchIds.insert(touchPoint.id, touchPoint.normalPosition);
                break;
            default:
                break;
        }

        touchPointList.append(touchPoint);
    }

    QFlags<Qt::KeyboardModifier> keyModifier = KeyboardModifier::getForEvent(*touchEvent);

    bool accepted = false;

    if (eventType == EMSCRIPTEN_EVENT_TOUCHCANCEL)
        accepted = QWindowSystemInterface::handleTouchCancelEvent(targetWindow, QWasmIntegration::getTimestamp(), m_touchDevice.get(), keyModifier);
    else
        accepted = QWindowSystemInterface::handleTouchEvent<QWindowSystemInterface::SynchronousDelivery>(
                targetWindow, QWasmIntegration::getTimestamp(), m_touchDevice.get(), touchPointList, keyModifier);

    return static_cast<int>(accepted);
}

void QWasmCompositor::setCapture(QWasmWindow *window)
{
    Q_ASSERT(std::find(m_windowStack.begin(), m_windowStack.end(), window) != m_windowStack.end());
    m_mouseCaptureWindow = window->window();
}

void QWasmCompositor::releaseCapture()
{
    m_mouseCaptureWindow = nullptr;
}

void QWasmCompositor::leaveWindow(QWindow *window)
{
    m_windowUnderMouse = nullptr;
    QWindowSystemInterface::handleLeaveEvent<QWindowSystemInterface::SynchronousDelivery>(window);
}

void QWasmCompositor::enterWindow(QWindow *window, const QPoint &pointInTargetWindowCoords, const QPoint &targetPointInScreenCoords)
{
    QWindowSystemInterface::handleEnterEvent<QWindowSystemInterface::SynchronousDelivery>(window, pointInTargetWindowCoords, targetPointInScreenCoords);
}

bool QWasmCompositor::processMouseEnter(const EmscriptenMouseEvent *mouseEvent)
{
    Q_UNUSED(mouseEvent)
    // mouse has entered the canvas area
    m_mouseInCanvas = true;
    return true;
}

bool QWasmCompositor::processMouseLeave()
{
    m_mouseInCanvas = false;
    return true;
}
