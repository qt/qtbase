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
#include <QKeySequence>

QT_BEGIN_NAMESPACE

namespace {
QWasmWindowStack::PositionPreference positionPreferenceFromWindowFlags(Qt::WindowFlags flags)
{
    if (flags.testFlag(Qt::WindowStaysOnTopHint))
        return QWasmWindowStack::PositionPreference::StayOnTop;
    if (flags.testFlag(Qt::WindowStaysOnBottomHint))
        return QWasmWindowStack::PositionPreference::StayOnBottom;
    return QWasmWindowStack::PositionPreference::Regular;
}
} // namespace

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

    m_windowContents.set("className", "qt-window-contents");
    m_qtWindow.call<void>("appendChild", m_windowContents);

    m_canvas["classList"].call<void>("add", emscripten::val("qt-window-content"));

    // Set contenteditable so that the canvas gets clipboard events,
    // then hide the resulting focus frame.
    m_canvas.set("contentEditable", std::string("true"));
    m_canvas["style"].set("outline", std::string("none"));

    QWasmClipboard::installEventHandlers(m_canvas);

    // set inputMode to none to stop mobile keyboard opening
    // when user clicks anywhere on the canvas.
    m_canvas.set("inputMode", std::string("none"));

    // Hide the canvas from screen readers.
    m_canvas.call<void>("setAttribute", std::string("aria-hidden"), std::string("true"));

    m_windowContents.call<void>("appendChild", m_canvasContainer);

    m_canvasContainer["classList"].call<void>("add", emscripten::val("qt-window-canvas-container"));
    m_canvasContainer.call<void>("appendChild", m_canvas);

    m_canvasContainer.call<void>("appendChild", m_a11yContainer);
    m_a11yContainer["classList"].call<void>("add", emscripten::val("qt-window-a11y-container"));

    const bool rendersTo2dContext = w->surfaceType() != QSurface::OpenGLSurface;
    if (rendersTo2dContext)
        m_context2d = m_canvas.call<emscripten::val>("getContext", emscripten::val("2d"));
    static int serialNo = 0;
    m_winId = ++serialNo;
    m_qtWindow.set("id", "qt-window-" + std::to_string(m_winId));
    emscripten::val::module_property("specialHTMLTargets").set(canvasSelector(), m_canvas);

    m_flags = window()->flags();

    m_pointerEnterCallback = std::make_unique<qstdweb::EventCallback>(m_qtWindow, "pointerenter",
        [this](emscripten::val event) { this->handlePointerEvent(event); });
    m_pointerLeaveCallback = std::make_unique<qstdweb::EventCallback>(m_qtWindow, "pointerleave",
        [this](emscripten::val event) { this->handlePointerEvent(event); });
    m_wheelEventCallback = std::make_unique<qstdweb::EventCallback>( m_qtWindow, "wheel",
        [this](emscripten::val event) { this->handleWheelEvent(event); });

    QWasmInputContext *wasmInput = QWasmIntegration::get()->wasmInputContext();
    if (wasmInput) {
        m_keyDownCallbackForInputContext =
            std::make_unique<qstdweb::EventCallback>(wasmInput->m_inputElement, "keydown",
            [this](emscripten::val event) { this->handleKeyForInputContextEvent(event); });
        m_keyUpCallbackForInputContext =
            std::make_unique<qstdweb::EventCallback>(wasmInput->m_inputElement, "keyup",
            [this](emscripten::val event) { this->handleKeyForInputContextEvent(event); });
    }

    m_keyDownCallback = std::make_unique<qstdweb::EventCallback>(m_qtWindow, "keydown",
        [this](emscripten::val event) { this->handleKeyEvent(event); });
    m_keyUpCallback =std::make_unique<qstdweb::EventCallback>(m_qtWindow, "keyup",
        [this](emscripten::val event) { this->handleKeyEvent(event); });

    setParent(parent());
}

QWasmWindow::~QWasmWindow()
{
    shutdown();

    emscripten::val::module_property("specialHTMLTargets").delete_(canvasSelector());
    m_canvasContainer.call<void>("removeChild", m_canvas);
    m_context2d = emscripten::val::undefined();
    commitParent(nullptr);
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

QWasmWindow *QWasmWindow::fromWindow(QWindow *window)
{
    return static_cast<QWasmWindow *>(window->handle());
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
    requestActivateWindow();
    QGuiApplicationPrivate::instance()->closeAllPopups();
}

bool QWasmWindow::onNonClientEvent(const PointerEvent &event)
{
    QPointF pointInScreen = platformScreen()->mapFromLocal(
        dom::mapPoint(event.target(), platformScreen()->element(), event.localPoint));
    return QWindowSystemInterface::handleMouseEvent(
            window(), QWasmIntegration::getTimestamp(), window()->mapFromGlobal(pointInScreen),
            pointInScreen, event.mouseButtons, event.mouseButton,
            MouseEvent::mouseEventTypeFromEventType(event.type, WindowArea::NonClient),
            event.modifiers);
}

void QWasmWindow::initialize()
{
    auto initialGeometry = QPlatformWindow::initialGeometry(window(),
            windowGeometry(), defaultWindowSize, defaultWindowSize);
    m_normalGeometry = initialGeometry;

    setWindowState(window()->windowStates());
    setWindowFlags(window()->flags());
    setWindowTitle(window()->title());
    setMask(QHighDpi::toNativeLocalRegion(window()->mask(), window()));

    if (window()->isTopLevel())
        setWindowIcon(window()->icon());
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

        auto offset = rect.topLeft() - (!parent() ? screen()->geometry().topLeft() : QPoint());

        // In viewport
        auto containerGeometryInViewport =
                QRectF::fromDOMRect(parentNode()->containerElement().call<emscripten::val>(
                                            "getBoundingClientRect"))
                        .toRect();

        auto rectInViewport = QRect(containerGeometryInViewport.topLeft() + offset, rect.size());

        QRect cappedGeometry(rectInViewport);
        if (!parent()) {
            // Clamp top level windows top position to the screen bounds
            cappedGeometry.moveTop(
                std::max(std::min(rectInViewport.y(), containerGeometryInViewport.bottom()),
                         containerGeometryInViewport.y() + margins.top()));
        }
        cappedGeometry.setSize(
                cappedGeometry.size().expandedTo(windowMinimumSize()).boundedTo(windowMaximumSize()));
        return QRect(QPoint(rect.x(), rect.y() + cappedGeometry.y() - rectInViewport.y()),
                     rect.size());
    })();
    m_nonClientArea->onClientAreaWidthChange(clientAreaRect.width());

    const auto frameRect =
            clientAreaRect
                    .adjusted(-margins.left(), -margins.top(), margins.right(), margins.bottom())
                    .translated(!parent() ? -screen()->geometry().topLeft() : QPoint());

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
    if (window() == QGuiApplication::focusWindow())
        focus();

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
    bringToTop();
    invalidate();
}

void QWasmWindow::lower()
{
    sendToBottom();
    invalidate();
}

WId QWasmWindow::winId() const
{
    return m_winId;
}

void QWasmWindow::propagateSizeHints()
{
    // setGeometry() will take care of minimum and maximum size constraints
    setGeometry(windowGeometry());
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
    if (flags.testFlag(Qt::WindowStaysOnTopHint) != m_flags.testFlag(Qt::WindowStaysOnTopHint)
        || flags.testFlag(Qt::WindowStaysOnBottomHint)
                != m_flags.testFlag(Qt::WindowStaysOnBottomHint)) {
        onPositionPreferenceChanged(positionPreferenceFromWindowFlags(flags));
    }
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

void QWasmWindow::commitParent(QWasmWindowTreeNode *parent)
{
    onParentChanged(m_commitedParent, parent, positionPreferenceFromWindowFlags(window()->flags()));
    m_commitedParent = parent;
}

void QWasmWindow::handleKeyEvent(const emscripten::val &event)
{
    qCDebug(qLcQpaWasmInputContext) << "processKey as KeyEvent";
    if (processKey(*KeyEvent::fromWebWithDeadKeyTranslation(event, m_deadKeySupport)))
        event.call<void>("preventDefault");
    event.call<void>("stopPropagation");
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
            event.modifiers, event.text, event.autoRepeat);
    return clipboardResult == ProcessKeyboardResult::NativeClipboardEventAndCopiedDataNeeded
            ? ProceedToNativeEvent
            : result;
}

void QWasmWindow::handleKeyForInputContextEvent(const emscripten::val &event)
{
    //
    // Things to consider:
    //
    // (Alt + '̃~') + a      -> compose('~', 'a')
    // (Compose) + '\'' + e -> compose('\'', 'e')
    // complex (i.e Chinese et al) input handling
    // Multiline text edit backspace at start of line
    //
    const QWasmInputContext *wasmInput = QWasmIntegration::get()->wasmInputContext();
    if (wasmInput) {
        const auto keyString = QString::fromStdString(event["key"].as<std::string>());
        qCDebug(qLcQpaWasmInputContext) << "Key callback" << keyString << keyString.size();
        if (keyString == "Unidentified") {
            // Android makes a bunch of KeyEvents as "Unidentified"
            // They will be processed just in InputContext.
            return;
        } else if (event["isComposing"].as<bool>()) {
            // Handled by the input context
            return;
        } else if (event["ctrlKey"].as<bool>()
                || event["altKey"].as<bool>()
                || event["metaKey"].as<bool>()) {
            // Not all platforms use 'isComposing' for '~' + 'a', in this
            // case send the key with state ('ctrl', 'alt', or 'meta') to
            // processKeyForInputContext

            ; // fallthrough
        } else if (keyString.size() != 1) {
            // This is like; 'Shift','ArrowRight','AltGraph', ...
            // send all of these to processKeyForInputContext

            ; // fallthrough
        } else if (wasmInput->inputMethodAccepted()) {
            // processed in inputContext with skipping processKey
            return;
        }
    }

    qCDebug(qLcQpaWasmInputContext) << "processKey as KeyEvent";
    if (processKeyForInputContext(*KeyEvent::fromWebWithDeadKeyTranslation(event, m_deadKeySupport)))
        event.call<void>("preventDefault");
    event.call<void>("stopImmediatePropagation");
}

bool QWasmWindow::processKeyForInputContext(const KeyEvent &event)
{
    qCDebug(qLcQpaWasmInputContext) << Q_FUNC_INFO;
    Q_ASSERT(event.type == EventType::KeyDown || event.type == EventType::KeyUp);

    QKeySequence keySeq(event.modifiers | event.key);

    if (keySeq == QKeySequence::Paste) {
        // Process it in pasteCallback and inputCallback
        return false;
    }

    const auto result = QWindowSystemInterface::handleKeyEvent(
            0, event.type == EventType::KeyDown ? QEvent::KeyPress : QEvent::KeyRelease, event.key,
            event.modifiers, event.text);

    // Copy/Cut callback required to copy qtClipboard to system clipboard
    if (keySeq == QKeySequence::Copy || keySeq == QKeySequence::Cut)
        return false;

    return result;
}

void QWasmWindow::handlePointerEvent(const emscripten::val &event)
{
    if (processPointer(*PointerEvent::fromWeb(event)))
        event.call<void>("preventDefault");
}

bool QWasmWindow::processPointer(const PointerEvent &event)
{
    if (event.pointerType != PointerType::Mouse && event.pointerType != PointerType::Pen)
        return false;

    switch (event.type) {
    case EventType::PointerEnter: {
        const auto pointInScreen = platformScreen()->mapFromLocal(
            dom::mapPoint(event.target(), platformScreen()->element(), event.localPoint));
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

void QWasmWindow::handleWheelEvent(const emscripten::val &event)
{
    if (processWheel(*WheelEvent::fromWeb(event)))
        event.call<void>("preventDefault");
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
        dom::mapPoint(event.target(), platformScreen()->element(), event.localPoint));

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

    raise();
    setAsActiveNode();

    if (!QWasmIntegration::get()->inputContext())
        focus();
    QPlatformWindow::requestActivateWindow();
}

void QWasmWindow::focus()
{
    m_canvas.call<void>("focus");
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

void QWasmWindow::setParent(const QPlatformWindow *)
{
    // The window flags depend on whether we are a
    // child window or not, so update them here.
    setWindowFlags(window()->flags());

    commitParent(parentNode());
}

std::string QWasmWindow::canvasSelector() const
{
    return "!qtwindow" + std::to_string(m_winId);
}

emscripten::val QWasmWindow::containerElement()
{
    return m_windowContents;
}

QWasmWindowTreeNode *QWasmWindow::parentNode()
{
    if (parent())
        return static_cast<QWasmWindow *>(parent());
    return platformScreen();
}

QWasmWindow *QWasmWindow::asWasmWindow()
{
    return this;
}

void QWasmWindow::onParentChanged(QWasmWindowTreeNode *previous, QWasmWindowTreeNode *current,
                                  QWasmWindowStack::PositionPreference positionPreference)
{
    if (previous)
        previous->containerElement().call<void>("removeChild", m_qtWindow);
    if (current)
        current->containerElement().call<void>("appendChild", m_qtWindow);
    QWasmWindowTreeNode::onParentChanged(previous, current, positionPreference);
}

QT_END_NAMESPACE
