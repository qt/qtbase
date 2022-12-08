// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qpa/qwindowsysteminterface.h>
#include <private/qguiapplication_p.h>
#include <QtCore/qfile.h>
#include <QtGui/private/qwindow_p.h>
#include <private/qpixmapcache_p.h>
#include <QtGui/qopenglfunctions.h>
#include <QBuffer>

#include "qwasmbase64iconstore.h"
#include "qwasmdom.h"
#include "qwasmwindow.h"
#include "qwasmwindowclientarea.h"
#include "qwasmscreen.h"
#include "qwasmstylepixmaps_p.h"
#include "qwasmcompositor.h"
#include "qwasmevent.h"
#include "qwasmeventdispatcher.h"
#include "qwasmstring.h"
#include "qwasmaccessibility.h"
#include "qwasmclipboard.h"

#include <iostream>
#include <emscripten/val.h>

#include <GL/gl.h>

#include <QtCore/private/qstdweb_p.h>

QT_BEGIN_NAMESPACE

Q_GUI_EXPORT int qt_defaultDpiX();

QWasmWindow::QWasmWindow(QWindow *w, QWasmCompositor *compositor, QWasmBackingStore *backingStore)
    : QPlatformWindow(w),
      m_window(w),
      m_compositor(compositor),
      m_backingStore(backingStore),
      m_document(dom::document()),
      m_qtWindow(m_document.call<emscripten::val>("createElement", emscripten::val("div"))),
      m_windowContents(m_document.call<emscripten::val>("createElement", emscripten::val("div"))),
      m_canvasContainer(m_document.call<emscripten::val>("createElement", emscripten::val("div"))),
      m_a11yContainer(m_document.call<emscripten::val>("createElement", emscripten::val("div"))),
      m_canvas(m_document.call<emscripten::val>("createElement", emscripten::val("canvas")))
{
    m_qtWindow.set("className", "qt-window");
    m_qtWindow["style"].set("display", std::string("none"));

    m_nonClientArea = std::make_unique<NonClientArea>(this, m_qtWindow);
    m_nonClientArea->titleBar()->setTitle(window()->title());

    m_clientArea = std::make_unique<ClientArea>(this, compositor->screen(), m_canvas);

    m_qtWindow.call<void>("appendChild", m_windowContents);

    m_canvas["classList"].call<void>("add", emscripten::val("qt-window-content"));

    // Set contenteditable so that the canvas gets clipboard events,
    // then hide the resulting focus frame.
    m_canvas.set("contentEditable", std::string("true"));
    m_canvas["style"].set("outline", std::string("none"));

    QWasmClipboard::installEventHandlers(m_canvas);

    // set inputmode to none to stop mobile keyboard opening
    // when user clicks anywhere on the canvas.
    m_canvas.set("inputmode", std::string("none"));

    // Hide the canvas from screen readers.
    m_canvas.call<void>("setAttribute", std::string("aria-hidden"), std::string("true"));

    m_windowContents.call<void>("appendChild", m_canvasContainer);

    m_canvasContainer["classList"].call<void>("add", emscripten::val("qt-window-canvas-container"));
    m_canvasContainer.call<void>("appendChild", m_canvas);

    m_canvasContainer.call<void>("appendChild", m_a11yContainer);
    m_a11yContainer["classList"].call<void>("add", emscripten::val("qt-window-a11y-container"));

    compositor->screen()->element().call<void>("appendChild", m_qtWindow);

    const bool rendersTo2dContext = w->surfaceType() != QSurface::OpenGLSurface;
    if (rendersTo2dContext)
        m_context2d = m_canvas.call<emscripten::val>("getContext", emscripten::val("2d"));
    static int serialNo = 0;
    m_winId = ++serialNo;
    m_qtWindow.set("id", "qt-window-" + std::to_string(m_winId));
    emscripten::val::module_property("specialHTMLTargets").set(canvasSelector(), m_canvas);

    m_compositor->addWindow(this);

    const auto callback = std::function([this](emscripten::val event) {
        if (processPointer(*PointerEvent::fromWeb(event)))
            event.call<void>("preventDefault");
    });

    m_pointerEnterCallback =
            std::make_unique<qstdweb::EventCallback>(m_qtWindow, "pointerenter", callback);
    m_pointerLeaveCallback =
            std::make_unique<qstdweb::EventCallback>(m_qtWindow, "pointerleave", callback);
}

QWasmWindow::~QWasmWindow()
{
    emscripten::val::module_property("specialHTMLTargets").delete_(canvasSelector());
    destroy();
    m_compositor->removeWindow(this);
    if (m_requestAnimationFrameId > -1)
        emscripten_cancel_animation_frame(m_requestAnimationFrameId);
    QWasmAccessibility::removeAccessibilityEnableButton(window());
}

void QWasmWindow::onRestoreClicked()
{
    window()->setWindowState(Qt::WindowNoState);
}

void QWasmWindow::onMaximizeClicked()
{
    window()->setWindowState(Qt::WindowMaximized);
}

void QWasmWindow::onToggleMaximized()
{
    window()->setWindowState(m_state.testFlag(Qt::WindowMaximized) ? Qt::WindowNoState
                                                                   : Qt::WindowMaximized);
}

void QWasmWindow::onCloseClicked()
{
    window()->close();
}

void QWasmWindow::onNonClientAreaInteraction()
{
    if (!isActive())
        requestActivateWindow();
}

bool QWasmWindow::onNonClientEvent(const PointerEvent &event)
{
    QPoint pointInScreen = platformScreen()->mapFromLocal(
            dom::mapPoint(event.target, platformScreen()->element(), event.localPoint));
    return QWindowSystemInterface::handleMouseEvent(
            window(), QWasmIntegration::getTimestamp(), window()->mapFromGlobal(pointInScreen),
            pointInScreen, event.mouseButtons, event.mouseButton, ([event]() {
                switch (event.type) {
                case EventType::PointerDown:
                    return QEvent::NonClientAreaMouseButtonPress;
                case EventType::PointerUp:
                    return QEvent::NonClientAreaMouseButtonRelease;
                case EventType::PointerMove:
                    return QEvent::NonClientAreaMouseMove;
                default:
                    Q_ASSERT(false); // notreached
                    return QEvent::None;
                }
            })(),
            event.modifiers);
}

void QWasmWindow::destroy()
{
    m_qtWindow["parentElement"].call<emscripten::val>("removeChild", m_qtWindow);

    m_canvasContainer.call<void>("removeChild", m_canvas);
    m_context2d = emscripten::val::undefined();
}

void QWasmWindow::initialize()
{
    QRect rect = windowGeometry();

    constexpr int minSizeBoundForDialogsAndRegularWindows = 100;
    const int windowType = window()->flags() & Qt::WindowType_Mask;
    const int systemMinSizeLowerBound = windowType == Qt::Window || windowType == Qt::Dialog
            ? minSizeBoundForDialogsAndRegularWindows
            : 0;

    const QSize minimumSize(std::max(windowMinimumSize().width(), systemMinSizeLowerBound),
                            std::max(windowMinimumSize().height(), systemMinSizeLowerBound));
    const QSize maximumSize = windowMaximumSize();
    const QSize targetSize = !rect.isEmpty() ? rect.size() : minimumSize;

    rect.setWidth(qBound(minimumSize.width(), targetSize.width(), maximumSize.width()));
    rect.setHeight(qBound(minimumSize.width(), targetSize.height(), maximumSize.height()));

    setWindowState(window()->windowStates());
    setWindowFlags(window()->flags());
    setWindowTitle(window()->title());
    if (window()->isTopLevel())
        setWindowIcon(window()->icon());
    m_normalGeometry = rect;
    QPlatformWindow::setGeometry(m_normalGeometry);

    // Add accessibility-enable button. The user can activate this
    // button to opt-in to accessibility.
     if (window()->isTopLevel())
        QWasmAccessibility::addAccessibilityEnableButton(window());
}

QWasmScreen *QWasmWindow::platformScreen() const
{
    return static_cast<QWasmScreen *>(window()->screen()->handle());
}

void QWasmWindow::paint()
{
    if (!m_backingStore || !isVisible() || m_context2d.isUndefined())
        return;

    auto image = m_backingStore->getUpdatedWebImage(this);
    if (image.isUndefined())
        return;
    m_context2d.call<void>("putImageData", image, emscripten::val(0), emscripten::val(0));
}

void QWasmWindow::setZOrder(int z)
{
    m_qtWindow["style"].set("zIndex", std::to_string(z));
}

void QWasmWindow::setGeometry(const QRect &rect)
{
    const auto margins = frameMargins();

    const QRect clientAreaRect = ([this, &rect, &margins]() {
        if (m_state.testFlag(Qt::WindowFullScreen))
            return platformScreen()->geometry();
        if (m_state.testFlag(Qt::WindowMaximized))
            return platformScreen()->availableGeometry().marginsRemoved(frameMargins());

        const auto screenGeometry = screen()->geometry();

        QRect result(rect);
        result.moveTop(std::max(std::min(rect.y(), screenGeometry.bottom()),
                                screenGeometry.y() + margins.top()));
        return result;
    })();
    m_nonClientArea->onClientAreaWidthChange(clientAreaRect.width());

    const auto frameRect =
            clientAreaRect
                    .adjusted(-margins.left(), -margins.top(), margins.right(), margins.bottom())
                    .translated(-screen()->geometry().topLeft());

    m_qtWindow["style"].set("left", std::to_string(frameRect.left()) + "px");
    m_qtWindow["style"].set("top", std::to_string(frameRect.top()) + "px");
    m_canvasContainer["style"].set("width", std::to_string(clientAreaRect.width()) + "px");
    m_canvasContainer["style"].set("height", std::to_string(clientAreaRect.height()) + "px");
    m_a11yContainer["style"].set("width", std::to_string(clientAreaRect.width()) + "px");
    m_a11yContainer["style"].set("height", std::to_string(clientAreaRect.height()) + "px");

    // Important for the title flexbox to shrink correctly
    m_windowContents["style"].set("width", std::to_string(clientAreaRect.width()) + "px");

    QSizeF canvasSize = clientAreaRect.size() * devicePixelRatio();

    m_canvas.set("width", canvasSize.width());
    m_canvas.set("height", canvasSize.height());

    bool shouldInvalidate = true;
    if (!m_state.testFlag(Qt::WindowFullScreen) && !m_state.testFlag(Qt::WindowMaximized)) {
        shouldInvalidate = m_normalGeometry.size() != clientAreaRect.size();
        m_normalGeometry = clientAreaRect;
    }
    QWindowSystemInterface::handleGeometryChange(window(), clientAreaRect);
    if (shouldInvalidate)
        invalidate();
    else
        m_compositor->requestUpdateWindow(this);
}

void QWasmWindow::setVisible(bool visible)
{
    // TODO(mikolajboc): isVisible()?
    const bool nowVisible = m_qtWindow["style"]["display"].as<std::string>() == "block";
    if (visible == nowVisible)
        return;

    m_compositor->requestUpdateWindow(this, QWasmCompositor::ExposeEventDelivery);
    m_qtWindow["style"].set("display", visible ? "block" : "none");
    if (visible)
        applyWindowState();
}

bool QWasmWindow::isVisible() const
{
    return window()->isVisible();
}

QMargins QWasmWindow::frameMargins() const
{
    const auto frameRect =
            QRectF::fromDOMRect(m_qtWindow.call<emscripten::val>("getBoundingClientRect"));
    const auto canvasRect =
            QRectF::fromDOMRect(m_windowContents.call<emscripten::val>("getBoundingClientRect"));
    return QMarginsF(canvasRect.left() - frameRect.left(), canvasRect.top() - frameRect.top(),
                     frameRect.right() - canvasRect.right(),
                     frameRect.bottom() - canvasRect.bottom())
            .toMargins();
}

void QWasmWindow::raise()
{
    m_compositor->raise(this);
    invalidate();
}

void QWasmWindow::lower()
{
    m_compositor->lower(this);
    invalidate();
}

WId QWasmWindow::winId() const
{
    return m_winId;
}

void QWasmWindow::propagateSizeHints()
{
    QRect rect = windowGeometry();
    if (rect.size().width() < windowMinimumSize().width()
        && rect.size().height() < windowMinimumSize().height()) {
        rect.setSize(windowMinimumSize());
        setGeometry(rect);
    }
}

void QWasmWindow::invalidate()
{
    m_compositor->requestUpdateWindow(this);
}

void QWasmWindow::onActivationChanged(bool active)
{
    dom::syncCSSClassWith(m_qtWindow, "inactive", !active);
}

void QWasmWindow::setWindowFlags(Qt::WindowFlags flags)
{
    m_flags = flags;
    dom::syncCSSClassWith(m_qtWindow, "has-title-bar", hasTitleBar());
}

void QWasmWindow::setWindowState(Qt::WindowStates newState)
{
    // Child windows can not have window states other than Qt::WindowActive
    if (parent())
        newState &= Qt::WindowActive;

    const Qt::WindowStates oldState = m_state;

    if (newState.testFlag(Qt::WindowMinimized)) {
        newState.setFlag(Qt::WindowMinimized, false);
        qWarning("Qt::WindowMinimized is not implemented in wasm");
        window()->setWindowStates(newState);
        return;
    }

    if (newState == oldState)
        return;

    m_state = newState;
    m_previousWindowState = oldState;

    applyWindowState();
}

void QWasmWindow::setWindowTitle(const QString &title)
{
    m_nonClientArea->titleBar()->setTitle(title);
}

void QWasmWindow::setWindowIcon(const QIcon &icon)
{
    const auto dpi = screen()->devicePixelRatio();
    auto pixmap = icon.pixmap(10 * dpi, 10 * dpi);
    if (pixmap.isNull()) {
        m_nonClientArea->titleBar()->setIcon(
                Base64IconStore::get()->getIcon(Base64IconStore::IconType::QtLogo), "svg+xml");
        return;
    }

    QByteArray bytes;
    QBuffer buffer(&bytes);
    pixmap.save(&buffer, "png");
    m_nonClientArea->titleBar()->setIcon(bytes.toBase64().toStdString(), "png");
}

void QWasmWindow::applyWindowState()
{
    QRect newGeom;

    const bool isFullscreen = m_state.testFlag(Qt::WindowFullScreen);
    const bool isMaximized = m_state.testFlag(Qt::WindowMaximized);
    if (isFullscreen)
        newGeom = platformScreen()->geometry();
    else if (isMaximized)
        newGeom = platformScreen()->availableGeometry().marginsRemoved(frameMargins());
    else
        newGeom = normalGeometry();

    dom::syncCSSClassWith(m_qtWindow, "has-title-bar", hasTitleBar());
    dom::syncCSSClassWith(m_qtWindow, "maximized", isMaximized);

    m_nonClientArea->titleBar()->setRestoreVisible(isMaximized);
    m_nonClientArea->titleBar()->setMaximizeVisible(!isMaximized);

    if (isVisible())
        QWindowSystemInterface::handleWindowStateChanged(window(), m_state, m_previousWindowState);
    setGeometry(newGeom);
}

bool QWasmWindow::processPointer(const PointerEvent &event)
{
    if (event.pointerType != PointerType::Mouse)
        return false;

    switch (event.type) {
    case EventType::PointerEnter: {
        const auto pointInScreen = platformScreen()->mapFromLocal(
                dom::mapPoint(event.target, platformScreen()->element(), event.localPoint));
        QWindowSystemInterface::handleEnterEvent(
                window(), m_window->mapFromGlobal(pointInScreen), pointInScreen);
        break;
    }
    case EventType::PointerLeave:
        QWindowSystemInterface::handleLeaveEvent(window());
        break;
    default:
        break;
    }

    return false;
}

QRect QWasmWindow::normalGeometry() const
{
    return m_normalGeometry;
}

qreal QWasmWindow::devicePixelRatio() const
{
    return screen()->devicePixelRatio();
}

void QWasmWindow::requestUpdate()
{
    m_compositor->requestUpdateWindow(this, QWasmCompositor::UpdateRequestDelivery);
}

bool QWasmWindow::hasTitleBar() const
{
    return !m_state.testFlag(Qt::WindowFullScreen) && m_flags.testFlag(Qt::WindowTitleHint)
            && !windowIsPopupType(m_flags);
}

bool QWasmWindow::windowIsPopupType(Qt::WindowFlags flags) const
{
    if (flags.testFlag(Qt::Tool))
        return false; // Qt::Tool has the Popup bit set but isn't an actual Popup window

    return (flags.testFlag(Qt::Popup));
}

void QWasmWindow::requestActivateWindow()
{
    QWindow *modalWindow;
    if (QGuiApplicationPrivate::instance()->isWindowBlocked(window(), &modalWindow)) {
        static_cast<QWasmWindow *>(modalWindow->handle())->requestActivateWindow();
        return;
    }

    if (window()->isTopLevel())
        raise();

    if (!QWasmIntegration::get()->inputContext())
        m_canvas.call<void>("focus");

    QPlatformWindow::requestActivateWindow();
}

bool QWasmWindow::setMouseGrabEnabled(bool grab)
{
    Q_UNUSED(grab);
    return false;
}

bool QWasmWindow::windowEvent(QEvent *event)
{
    switch (event->type()) {
    case QEvent::WindowBlocked:
        m_qtWindow["classList"].call<void>("add", emscripten::val("blocked"));
        return false; // Propagate further
    case QEvent::WindowUnblocked:;
        m_qtWindow["classList"].call<void>("remove", emscripten::val("blocked"));
        return false; // Propagate further
    default:
        return QPlatformWindow::windowEvent(event);
    }
}

std::string QWasmWindow::canvasSelector() const
{
    return "!qtwindow" + std::to_string(m_winId);
}

QT_END_NAMESPACE
