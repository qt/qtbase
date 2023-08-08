// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qpa/qwindowsysteminterface.h>
#include <private/qguiapplication_p.h>
#include <QtCore/qfile.h>
#include <QtGui/private/qwindow_p.h>
#include <QtGui/private/qhighdpiscaling_p.h>
#include <private/qpixmapcache_p.h>
#include <QtGui/qopenglfunctions.h>
#include <QBuffer>

#include "qwasmbase64iconstore.h"
#include "qwasmdom.h"
#include "qwasmclipboard.h"
#include "qwasmintegration.h"
#include "qwasmkeytranslator.h"
#include "qwasmwindow.h"
#include "qwasmwindowclientarea.h"
#include "qwasmscreen.h"
#include "qwasmcompositor.h"
#include "qwasmevent.h"
#include "qwasmeventdispatcher.h"
#include "qwasmaccessibility.h"
#include "qwasmclipboard.h"

#include <iostream>
#include <sstream>

#include <emscripten/val.h>

#include <QtCore/private/qstdweb_p.h>

QT_BEGIN_NAMESPACE

Q_GUI_EXPORT int qt_defaultDpiX();

QWasmWindow::QWasmWindow(QWindow *w, QWasmDeadKeySupport *deadKeySupport,
                         QWasmCompositor *compositor, QWasmBackingStore *backingStore)
    : QPlatformWindow(w),
      m_window(w),
      m_compositor(compositor),
      m_backingStore(backingStore),
      m_deadKeySupport(deadKeySupport),
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

    m_clientArea = std::make_unique<ClientArea>(this, compositor->screen(), m_windowContents);

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
    m_flags = window()->flags();

    const auto pointerCallback = std::function([this](emscripten::val event) {
        if (processPointer(*PointerEvent::fromWeb(event)))
            event.call<void>("preventDefault");
    });

    m_pointerEnterCallback =
            std::make_unique<qstdweb::EventCallback>(m_qtWindow, "pointerenter", pointerCallback);
    m_pointerLeaveCallback =
            std::make_unique<qstdweb::EventCallback>(m_qtWindow, "pointerleave", pointerCallback);

    m_dropCallback = std::make_unique<qstdweb::EventCallback>(
            m_qtWindow, "drop", [this](emscripten::val event) {
                if (processDrop(*DragEvent::fromWeb(event)))
                    event.call<void>("preventDefault");
            });

    m_wheelEventCallback = std::make_unique<qstdweb::EventCallback>(
            m_qtWindow, "wheel", [this](emscripten::val event) {
                if (processWheel(*WheelEvent::fromWeb(event)))
                    event.call<void>("preventDefault");
            });

    const auto keyCallback = std::function([this](emscripten::val event) {
        if (processKey(*KeyEvent::fromWebWithDeadKeyTranslation(event, m_deadKeySupport)))
            event.call<void>("preventDefault");
    });

    m_keyDownCallback =
            std::make_unique<qstdweb::EventCallback>(m_qtWindow, "keydown", keyCallback);
    m_keyUpCallback = std::make_unique<qstdweb::EventCallback>(m_qtWindow, "keyup", keyCallback);
}

QWasmWindow::~QWasmWindow()
{
    emscripten::val::module_property("specialHTMLTargets").delete_(canvasSelector());
    destroy();
    m_compositor->removeWindow(this);
    if (m_requestAnimationFrameId > -1)
        emscripten_cancel_animation_frame(m_requestAnimationFrameId);
#if QT_CONFIG(accessibility)
    QWasmAccessibility::removeAccessibilityEnableButton(window());
#endif
}

QSurfaceFormat QWasmWindow::format() const
{
    return window()->requestedFormat();
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
    QGuiApplicationPrivate::instance()->closeAllPopups();
}

bool QWasmWindow::onNonClientEvent(const PointerEvent &event)
{
    QPointF pointInScreen = platformScreen()->mapFromLocal(
            dom::mapPoint(event.target, platformScreen()->element(), event.localPoint));
    return QWindowSystemInterface::handleMouseEvent(
            window(), QWasmIntegration::getTimestamp(), window()->mapFromGlobal(pointInScreen),
            pointInScreen, event.mouseButtons, event.mouseButton,
            MouseEvent::mouseEventTypeFromEventType(event.type, WindowArea::NonClient),
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

    const auto windowFlags = window()->flags();
    const bool shouldRestrictMinSize =
            !windowFlags.testFlag(Qt::FramelessWindowHint) && !windowIsPopupType(windowFlags);
    const bool isMinSizeUninitialized = window()->minimumSize() == QSize(0, 0);

    if (shouldRestrictMinSize && isMinSizeUninitialized)
        window()->setMinimumSize(QSize(minSizeForRegularWindows, minSizeForRegularWindows));


    const QSize minimumSize = windowMinimumSize();
    const QSize maximumSize = windowMaximumSize();
    const QSize targetSize = !rect.isEmpty() ? rect.size() : minimumSize;

    rect.setWidth(qBound(minimumSize.width(), targetSize.width(), maximumSize.width()));
    rect.setHeight(qBound(minimumSize.height(), targetSize.height(), maximumSize.height()));

    setWindowState(window()->windowStates());
    setWindowFlags(window()->flags());
    setWindowTitle(window()->title());
    setMask(QHighDpi::toNativeLocalRegion(window()->mask(), window()));

    if (window()->isTopLevel())
        setWindowIcon(window()->icon());
    m_normalGeometry = rect;
    QPlatformWindow::setGeometry(m_normalGeometry);

#if QT_CONFIG(accessibility)
    // Add accessibility-enable button. The user can activate this
    // button to opt-in to accessibility.
     if (window()->isTopLevel())
        QWasmAccessibility::addAccessibilityEnableButton(window());
#endif
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

void QWasmWindow::setWindowCursor(QByteArray cssCursorName)
{
    m_windowContents["style"].set("cursor", emscripten::val(cssCursorName.constData()));
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
        if (!parent()) {
            // Clamp top level windows top position to the screen bounds
            result.moveTop(std::max(std::min(rect.y(), screenGeometry.bottom()),
                                screenGeometry.y() + margins.top()));
        }
        result.setSize(
                result.size().expandedTo(windowMinimumSize()).boundedTo(windowMaximumSize()));
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
    m_nonClientArea->propagateSizeHints();
}

void QWasmWindow::setOpacity(qreal level)
{
    m_qtWindow["style"].set("opacity", qBound(0.0, level, 1.0));
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
    if (flags.testFlag(Qt::WindowStaysOnTopHint) != m_flags.testFlag(Qt::WindowStaysOnTopHint))
        m_compositor->windowPositionPreferenceChanged(this, flags);
    m_flags = flags;
    dom::syncCSSClassWith(m_qtWindow, "frameless", !hasFrame());
    dom::syncCSSClassWith(m_qtWindow, "has-border", hasBorder());
    dom::syncCSSClassWith(m_qtWindow, "has-shadow", hasShadow());
    dom::syncCSSClassWith(m_qtWindow, "has-title", hasTitleBar());
    dom::syncCSSClassWith(m_qtWindow, "transparent-for-input",
                          flags.testFlag(Qt::WindowTransparentForInput));

    m_nonClientArea->titleBar()->setMaximizeVisible(hasMaximizeButton());
    m_nonClientArea->titleBar()->setCloseVisible(m_flags.testFlag(Qt::WindowCloseButtonHint));
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

    dom::syncCSSClassWith(m_qtWindow, "has-border", hasBorder());
    dom::syncCSSClassWith(m_qtWindow, "maximized", isMaximized);

    m_nonClientArea->titleBar()->setRestoreVisible(isMaximized);
    m_nonClientArea->titleBar()->setMaximizeVisible(hasMaximizeButton());

    if (isVisible())
        QWindowSystemInterface::handleWindowStateChanged(window(), m_state, m_previousWindowState);
    setGeometry(newGeom);
}

bool QWasmWindow::processKey(const KeyEvent &event)
{
    constexpr bool ProceedToNativeEvent = false;
    Q_ASSERT(event.type == EventType::KeyDown || event.type == EventType::KeyUp);

    const auto clipboardResult =
            QWasmIntegration::get()->getWasmClipboard()->processKeyboard(event);

    using ProcessKeyboardResult = QWasmClipboard::ProcessKeyboardResult;
    if (clipboardResult == ProcessKeyboardResult::NativeClipboardEventNeeded)
        return ProceedToNativeEvent;

    const auto result = QWindowSystemInterface::handleKeyEvent(
            0, event.type == EventType::KeyDown ? QEvent::KeyPress : QEvent::KeyRelease, event.key,
            event.modifiers, event.text);
    return clipboardResult == ProcessKeyboardResult::NativeClipboardEventAndCopiedDataNeeded
            ? ProceedToNativeEvent
            : result;
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

bool QWasmWindow::processDrop(const DragEvent &event)
{
    m_dropDataReadCancellationFlag = qstdweb::readDataTransfer(
            event.dataTransfer,
            [](QByteArray fileContent) {
                QImage image;
                image.loadFromData(fileContent, nullptr);
                return image;
            },
            [this, event](std::unique_ptr<QMimeData> data) {
                QWindowSystemInterface::handleDrag(window(), data.get(),
                                                   event.pointInPage.toPoint(), event.dropAction,
                                                   event.mouseButton, event.modifiers);

                QWindowSystemInterface::handleDrop(window(), data.get(),
                                                   event.pointInPage.toPoint(), event.dropAction,
                                                   event.mouseButton, event.modifiers);

                QWindowSystemInterface::handleDrag(window(), nullptr, QPoint(), Qt::IgnoreAction,
                                                   {}, {});
            });
    return true;
}

bool QWasmWindow::processWheel(const WheelEvent &event)
{
    // Web scroll deltas are inverted from Qt deltas - negate.
    const int scrollFactor = -([&event]() {
        switch (event.deltaMode) {
        case DeltaMode::Pixel:
            return 1;
        case DeltaMode::Line:
            return 12;
        case DeltaMode::Page:
            return 20;
        };
    })();

    const auto pointInScreen = platformScreen()->mapFromLocal(
            dom::mapPoint(event.target, platformScreen()->element(), event.localPoint));

    return QWindowSystemInterface::handleWheelEvent(
            window(), QWasmIntegration::getTimestamp(), window()->mapFromGlobal(pointInScreen),
            pointInScreen, (event.delta * scrollFactor).toPoint(),
            (event.delta * scrollFactor).toPoint(), event.modifiers, Qt::NoScrollPhase,
            Qt::MouseEventNotSynthesized, event.webkitDirectionInvertedFromDevice);
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

bool QWasmWindow::hasFrame() const
{
    return !m_flags.testFlag(Qt::FramelessWindowHint);
}

bool QWasmWindow::hasBorder() const
{
    return hasFrame() && !m_state.testFlag(Qt::WindowFullScreen) && !m_flags.testFlag(Qt::SubWindow)
            && !windowIsPopupType(m_flags) && !parent();
}

bool QWasmWindow::hasTitleBar() const
{
    return hasBorder() && m_flags.testFlag(Qt::WindowTitleHint);
}

bool QWasmWindow::hasShadow() const
{
    return hasBorder() && !m_flags.testFlag(Qt::NoDropShadowWindowHint);
}

bool QWasmWindow::hasMaximizeButton() const
{
    return !m_state.testFlag(Qt::WindowMaximized) && m_flags.testFlag(Qt::WindowMaximizeButtonHint);
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

    if (window()->isTopLevel()) {
        raise();
        m_compositor->setActive(this);
    }

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

void QWasmWindow::setMask(const QRegion &region)
{
    if (region.isEmpty()) {
        m_qtWindow["style"].set("clipPath", emscripten::val(""));
        return;
    }

    std::ostringstream cssClipPath;
    cssClipPath << "path('";
    for (const auto &rect : region) {
        const auto cssRect = rect.adjusted(0, 0, 1, 1);
        cssClipPath << "M " << cssRect.left() << " " << cssRect.top() << " ";
        cssClipPath << "L " << cssRect.right() << " " << cssRect.top() << " ";
        cssClipPath << "L " << cssRect.right() << " " << cssRect.bottom() << " ";
        cssClipPath << "L " << cssRect.left() << " " << cssRect.bottom() << " z ";
    }
    cssClipPath << "')";
    m_qtWindow["style"].set("clipPath", emscripten::val(cssClipPath.str()));
}

std::string QWasmWindow::canvasSelector() const
{
    return "!qtwindow" + std::to_string(m_winId);
}

QT_END_NAMESPACE
