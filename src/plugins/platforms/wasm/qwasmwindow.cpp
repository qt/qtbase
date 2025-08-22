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
#if QT_CONFIG(clipboard)
#include "qwasmclipboard.h"
#endif
#include "qwasmintegration.h"
#include "qwasmkeytranslator.h"
#include "qwasmwindow.h"
#include "qwasmscreen.h"
#include "qwasmcompositor.h"
#include "qwasmevent.h"
#include "qwasmeventdispatcher.h"
#include "qwasmaccessibility.h"
#if QT_CONFIG(draganddrop)
#include "qwasmdrag.h"
#endif
#include "qwasmopenglcontext.h"

#include <iostream>
#include <sstream>

#include <emscripten/val.h>

#include <QtCore/private/qstdweb_p.h>
#include <QKeySequence>

QT_BEGIN_NAMESPACE

Q_GUI_EXPORT int qt_defaultDpiX();

QWasmWindow::QWasmWindow(QWindow *w, QWasmDeadKeySupport *deadKeySupport,
                         QWasmCompositor *compositor, QWasmBackingStore *backingStore,
                         WId nativeHandle)
    : QPlatformWindow(w),
      m_compositor(compositor),
      m_backingStore(backingStore),
      m_deadKeySupport(deadKeySupport),
      m_document(dom::document()),
      m_decoratedWindow(m_document.call<emscripten::val>("createElement", emscripten::val("div"))),
      m_window(m_document.call<emscripten::val>("createElement", emscripten::val("div"))),
      m_a11yContainer(m_document.call<emscripten::val>("createElement", emscripten::val("div"))),
      m_canvas(m_document.call<emscripten::val>("createElement", emscripten::val("canvas"))),
      m_focusHelper(m_document.call<emscripten::val>("createElement", emscripten::val("div"))),
      m_inputElement(m_document.call<emscripten::val>("createElement", emscripten::val("input")))

{
    m_decoratedWindow.set("className", "qt-decorated-window");
    m_decoratedWindow["style"].set("display", std::string("none"));

    m_nonClientArea = std::make_unique<NonClientArea>(this, m_decoratedWindow);
    m_nonClientArea->titleBar()->setTitle(window()->title());

    // If we are wrapping a foregin window, a.k.a. a native html element then that element becomes
    // the m_window element. In this case setting up event handlers and accessibility etc is not
    // needed since that is (presumably) handled by the native html element.
    //
    // The WId is an emscripten::val *, owned by QWindow user code. We dereference and make
    // a copy of the val here and don't strictly need it to be kept alive, but that's an
    // implementation detail. The pointer will be dereferenced again if the window is destroyed
    // and recreated.
    if (nativeHandle) {
        m_window = *(emscripten::val *)(nativeHandle);
        m_winId = nativeHandle;
        m_decoratedWindow.set("id", "qt-window-" + std::to_string(m_winId));
        m_decoratedWindow.call<void>("appendChild", m_window);
        return;
    }

    m_window.set("className", "qt-window");
    m_decoratedWindow.call<void>("appendChild", m_window);

    m_canvas["classList"].call<void>("add", emscripten::val("qt-window-canvas"));

#if QT_CONFIG(clipboard)
    if (QWasmClipboard::shouldInstallWindowEventHandlers()) {
        m_cutCallback = QWasmEventHandler(m_canvas, "cut", QWasmClipboard::cut);
        m_copyCallback = QWasmEventHandler(m_canvas, "copy", QWasmClipboard::copy);
        m_pasteCallback = QWasmEventHandler(m_canvas, "paste", QWasmClipboard::paste);
    }
#endif

    // Set up m_focusHelper, which is an invisible child element of the window which takes
    // focus on behalf of the window any time the window has focus in general, but none
    // of the special child elements such as the inputElment or a11y elements have focus.
    // Set inputMode=none set to prevent the virtual keyboard from popping up.
    m_focusHelper["classList"].call<void>("add", emscripten::val("qt-window-focus-helper"));
    m_focusHelper.set("inputMode", std::string("none"));
    m_focusHelper.call<void>("setAttribute", std::string("aria-hidden"), std::string("true"));
    m_focusHelper.call<void>("setAttribute", std::string("contenteditable"), std::string("true"));
    m_focusHelper["style"].set("position", "absolute");
    m_focusHelper["style"].set("left", 0);
    m_focusHelper["style"].set("top", 0);
    m_focusHelper["style"].set("width", "1px");
    m_focusHelper["style"].set("height", "1px");
    m_focusHelper["style"].set("z-index", -2);
    m_focusHelper["style"].set("opacity", 0);
    m_window.call<void>("appendChild", m_focusHelper);

    // Set up m_inputElement, which takes focus whenever a Qt text input UI element has
    // foucus.
    m_inputElement["classList"].call<void>("add", emscripten::val("qt-window-input-element"));
    m_inputElement.set("type", "text");
    m_inputElement.call<void>("setAttribute", std::string("aria-hidden"), std::string("true"));
    m_inputElement["style"].set("position", "absolute");
    m_inputElement["style"].set("left", 0);
    m_inputElement["style"].set("top", 0);
    m_inputElement["style"].set("width", "1px");
    m_inputElement["style"].set("height", "1px");
    m_inputElement["style"].set("z-index", -2);
    m_inputElement["style"].set("opacity", 0);
    m_inputElement["style"].set("display", "");
    m_window.call<void>("appendChild", m_inputElement);

    // Hide the canvas from screen readers.
    m_canvas.call<void>("setAttribute", std::string("aria-hidden"), std::string("true"));
    m_window.call<void>("appendChild", m_canvas);

    m_a11yContainer["classList"].call<void>("add", emscripten::val("qt-window-a11y-container"));
    m_window.call<void>("appendChild", m_a11yContainer);

    const bool rendersTo2dContext = w->surfaceType() != QSurface::OpenGLSurface;
    if (rendersTo2dContext)
        m_context2d = m_canvas.call<emscripten::val>("getContext", emscripten::val("2d"));

    m_winId = WId(&m_window);
    m_decoratedWindow.set("id", "qt-window-" + std::to_string(m_winId));
    emscripten::val::module_property("specialHTMLTargets").set(canvasSelector(), m_canvas);

    m_flags = window()->flags();

    registerEventHandlers();

    m_transientWindowChangedConnection =
        QObject::connect(
            window(), &QWindow::transientParentChanged,
            window(), [this](QWindow *tp) { onTransientParentChanged(tp); });

    m_modalityChangedConnection =
            QObject::connect(
                window(), &QWindow::modalityChanged,
                window(), [this](Qt::WindowModality) { onModalityChanged(); });

    setParent(parent());
}

void QWasmWindow::registerEventHandlers()
{
    m_pointerDownCallback = QWasmEventHandler(m_window, "pointerdown",
        [this](emscripten::val event){ processPointer(PointerEvent(EventType::PointerDown, event)); }
    );
    m_pointerMoveCallback = QWasmEventHandler(m_window, "pointermove",
        [this](emscripten::val event){ processPointer(PointerEvent(EventType::PointerMove, event)); }
    );
    m_pointerUpCallback = QWasmEventHandler(m_window, "pointerup",
        [this](emscripten::val event){ processPointer(PointerEvent(EventType::PointerUp, event)); }
    );
    m_pointerCancelCallback = QWasmEventHandler(m_window, "pointercancel",
        [this](emscripten::val event){ processPointer(PointerEvent(EventType::PointerCancel, event)); }
    );
    m_pointerEnterCallback = QWasmEventHandler(m_window, "pointerenter",
        [this](emscripten::val event) { this->handlePointerEnterLeaveEvent(PointerEvent(EventType::PointerEnter, event)); }
    );
    m_pointerLeaveCallback = QWasmEventHandler(m_window, "pointerleave",
        [this](emscripten::val event) { this->handlePointerEnterLeaveEvent(PointerEvent(EventType::PointerLeave, event)); }
    );

#if QT_CONFIG(draganddrop)
    m_window.call<void>("setAttribute", emscripten::val("draggable"), emscripten::val("true"));
    m_dragStartCallback = QWasmEventHandler(m_window, "dragstart",
        [this](emscripten::val event) {
            DragEvent dragEvent(EventType::DragStart, event, window());
            QWasmDrag::instance()->onNativeDragStarted(&dragEvent);
        }
    );
    m_dragOverCallback = QWasmEventHandler(m_window, "dragover",
        [this](emscripten::val event) {
            DragEvent dragEvent(EventType::DragOver, event, window());
            QWasmDrag::instance()->onNativeDragOver(&dragEvent);
        }
    );
    m_dropCallback = QWasmEventHandler(m_window, "drop",
        [this](emscripten::val event) {
            DragEvent dragEvent(EventType::Drop, event, window());
            QWasmDrag::instance()->onNativeDrop(&dragEvent);
        }
    );
    m_dragEndCallback = QWasmEventHandler(m_window, "dragend",
        [this](emscripten::val event) {
            DragEvent dragEvent(EventType::DragEnd, event, window());
            QWasmDrag::instance()->onNativeDragFinished(&dragEvent);
        }
    );
    m_dragLeaveCallback = QWasmEventHandler(m_window, "dragleave",
        [this](emscripten::val event) {
            DragEvent dragEvent(EventType::DragLeave, event, window());
            QWasmDrag::instance()->onNativeDragLeave(&dragEvent);
        }
    );
#endif // QT_CONFIG(draganddrop)

    m_wheelEventCallback = QWasmEventHandler(m_window, "wheel",
        [this](emscripten::val event) { this->handleWheelEvent(event); });

    m_keyDownCallback = QWasmEventHandler(m_window, "keydown",
        [this](emscripten::val event) { this->handleKeyEvent(KeyEvent(EventType::KeyDown, event, m_deadKeySupport)); });
    m_keyUpCallback =QWasmEventHandler(m_window, "keyup",
        [this](emscripten::val event) {this->handleKeyEvent(KeyEvent(EventType::KeyUp, event, m_deadKeySupport)); });

    m_inputCallback = QWasmEventHandler(m_window, "input",
        [this](emscripten::val event){ handleInputEvent(event); });
    m_compositionUpdateCallback = QWasmEventHandler(m_window, "compositionupdate",
        [this](emscripten::val event){ handleCompositionUpdateEvent(event); });
    m_compositionStartCallback = QWasmEventHandler(m_window, "compositionstart",
        [this](emscripten::val event){ handleCompositionStartEvent(event); });
    m_compositionEndCallback = QWasmEventHandler(m_window, "compositionend",
        [this](emscripten::val event){ handleCompositionEndEvent(event); });
    }

QWasmWindow::~QWasmWindow()
{
    QWasmOpenGLContext::destroyWebGLContext(this);

#if QT_CONFIG(accessibility)
    QWasmAccessibility::onRemoveWindow(window());
#endif
    QObject::disconnect(m_transientWindowChangedConnection);
    QObject::disconnect(m_modalityChangedConnection);

    shutdown();

    emscripten::val::module_property("specialHTMLTargets").delete_(canvasSelector());
    m_window.call<void>("removeChild", m_canvas);
    m_window.call<void>("removeChild", m_a11yContainer);
    m_context2d = emscripten::val::undefined();
    commitParent(nullptr);
    if (m_requestAnimationFrameId > -1)
        emscripten_cancel_animation_frame(m_requestAnimationFrameId);
}

void QWasmWindow::shutdown()
{
    if (!window() ||
        (QGuiApplication::focusWindow() && // Don't act if we have a focus window different from this
         QGuiApplication::focusWindow() != window()))
        return;

    // Make a list of all windows sorted on active index.
    // Skip windows with active index 0 as they have
    // never been active.
    std::map<uint64_t, QWasmWindow *> allWindows;
    for (const auto &w : platformScreen()->allWindows()) {
        if (w->getActiveIndex() > 0)
            allWindows.insert({w->getActiveIndex(), w});
    }

    // window is not in all windows
    if (getActiveIndex() > 0)
        allWindows.insert({getActiveIndex(), this});

    if (allWindows.size() >= 2) {
        const auto lastIt = std::prev(allWindows.end());
        const auto prevIt = std::prev(lastIt);
        const auto lastW = lastIt->second;
        const auto prevW = prevIt->second;

        if (lastW == this) // Only act if window is last to be active
            prevW->requestActivateWindow();
    }
}

QSurfaceFormat QWasmWindow::format() const
{
    return window()->requestedFormat();
}

QWasmWindow *QWasmWindow::fromWindow(const QWindow *window)
{
    if (!window ||!window->handle())
        return nullptr;
    return static_cast<QWasmWindow *>(window->handle());
}

QWasmWindow *QWasmWindow::transientParent() const
{
    if (!window())
        return nullptr;

    return fromWindow(window()->transientParent());
}

Qt::WindowFlags QWasmWindow::windowFlags() const
{
    return window()->flags();
}

bool QWasmWindow::isModal() const
{
    return window()->isModal();
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
    m_decoratedWindow["style"].set("zIndex", std::to_string(z));
}

void QWasmWindow::setWindowCursor(QByteArray cssCursorName)
{
    m_window["style"].set("cursor", emscripten::val(cssCursorName.constData()));
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

    m_decoratedWindow["style"].set("left", std::to_string(frameRect.left()) + "px");
    m_decoratedWindow["style"].set("top", std::to_string(frameRect.top()) + "px");
    m_canvas["style"].set("width", std::to_string(clientAreaRect.width()) + "px");
    m_canvas["style"].set("height", std::to_string(clientAreaRect.height()) + "px");
    m_a11yContainer["style"].set("width", std::to_string(clientAreaRect.width()) + "px");
    m_a11yContainer["style"].set("height", std::to_string(clientAreaRect.height()) + "px");

    // Important for the title flexbox to shrink correctly
    m_window["style"].set("width", std::to_string(clientAreaRect.width()) + "px");

    QSizeF canvasSize = clientAreaRect.size() * devicePixelRatio();

    m_canvas.set("width", canvasSize.width());
    m_canvas.set("height", canvasSize.height());

    bool shouldInvalidate = true;
    if (!m_state.testFlag(Qt::WindowFullScreen) && !m_state.testFlag(Qt::WindowMaximized)) {
        shouldInvalidate = m_normalGeometry.size() != clientAreaRect.size();
        m_normalGeometry = clientAreaRect;
    }

    QWasmInputContext *wasmInput = QWasmIntegration::get()->wasmInputContext();
    if (wasmInput && (QGuiApplication::focusWindow() == window()))
        wasmInput->updateGeometry();

    QWindowSystemInterface::handleGeometryChange(window(), clientAreaRect);
    if (shouldInvalidate)
        invalidate();
    else
        m_compositor->requestUpdateWindow(this, QRect(QPoint(0, 0), geometry().size()));
}

void QWasmWindow::setVisible(bool visible)
{
    // TODO(mikolajboc): isVisible()?
    const bool nowVisible = m_decoratedWindow["style"]["display"].as<std::string>() == "block";
    if (visible == nowVisible)
        return;

    m_compositor->requestUpdateWindow(this, QRect(QPoint(0, 0), geometry().size()), QWasmCompositor::ExposeEventDelivery);
    m_decoratedWindow["style"].set("display", visible ? "block" : "none");
    if (window() == QGuiApplication::focusWindow())
        focus();

    if (visible) {
        applyWindowState();
#if QT_CONFIG(accessibility)
        QWasmAccessibility::onShowWindow(window());
#endif
    }
}

bool QWasmWindow::isVisible() const
{
    return window()->isVisible();
}

QMargins QWasmWindow::frameMargins() const
{
    const auto frameRect =
            QRectF::fromDOMRect(m_decoratedWindow.call<emscripten::val>("getBoundingClientRect"));
    const auto canvasRect =
            QRectF::fromDOMRect(m_window.call<emscripten::val>("getBoundingClientRect"));
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
    m_decoratedWindow["style"].set("opacity", qBound(0.0, level, 1.0));
}

void QWasmWindow::invalidate()
{
    m_compositor->requestUpdateWindow(this, QRect(QPoint(0, 0), geometry().size()));
}

void QWasmWindow::onActivationChanged(bool active)
{
    dom::syncCSSClassWith(m_decoratedWindow, "inactive", !active);
}

void QWasmWindow::setWindowFlags(Qt::WindowFlags flags)
{
    flags = fixTopLevelWindowFlags(flags);

    if ((flags.testFlag(Qt::WindowStaysOnTopHint) != m_flags.testFlag(Qt::WindowStaysOnTopHint))
        || (flags.testFlag(Qt::WindowStaysOnBottomHint)
            != m_flags.testFlag(Qt::WindowStaysOnBottomHint))
        || shouldBeAboveTransientParentFlags(flags) != shouldBeAboveTransientParentFlags(m_flags)) {
        onPositionPreferenceChanged(positionPreferenceFromWindowFlags(flags));
    }
    m_flags = flags;
    dom::syncCSSClassWith(m_decoratedWindow, "frameless", !hasFrame() || !window()->isTopLevel());
    dom::syncCSSClassWith(m_decoratedWindow, "has-border", hasBorder());
    dom::syncCSSClassWith(m_decoratedWindow, "has-shadow", hasShadow());
    dom::syncCSSClassWith(m_decoratedWindow, "has-title", hasTitleBar());
    dom::syncCSSClassWith(m_decoratedWindow, "transparent-for-input",
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

    dom::syncCSSClassWith(m_decoratedWindow, "has-border", hasBorder());
    dom::syncCSSClassWith(m_decoratedWindow, "maximized", isMaximized);

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

void QWasmWindow::handleKeyEvent(const KeyEvent &event)
{
    qCDebug(qLcQpaWasmInputContext) << "handleKeyEvent";

    if (QWasmInputContext *inputContext = QWasmIntegration::get()->wasmInputContext(); inputContext->isActive()) {
        handleKeyForInputContextEvent(event);
    } else {
        if (processKey(event)) {
            event.webEvent.call<void>("preventDefault");
            event.webEvent.call<void>("stopPropagation");
        }
    }
}

bool QWasmWindow::processKey(const KeyEvent &event)
{
    constexpr bool ProceedToNativeEvent = false;
    Q_ASSERT(event.type == EventType::KeyDown || event.type == EventType::KeyUp);

#if QT_CONFIG(clipboard)
    const auto clipboardResult =
            QWasmIntegration::get()->getWasmClipboard()->processKeyboard(event);

    using ProcessKeyboardResult = QWasmClipboard::ProcessKeyboardResult;
    if (clipboardResult == ProcessKeyboardResult::NativeClipboardEventNeeded)
        return ProceedToNativeEvent;
#endif

    const auto result = QWindowSystemInterface::handleKeyEvent(
            0, event.type == EventType::KeyDown ? QEvent::KeyPress : QEvent::KeyRelease, event.key,
            event.modifiers, event.text, event.autoRepeat);

#if QT_CONFIG(clipboard)
    return clipboardResult == ProcessKeyboardResult::NativeClipboardEventAndCopiedDataNeeded
            ? ProceedToNativeEvent
            : result;
#else
    return result;
#endif
}

void QWasmWindow::handleKeyForInputContextEvent(const KeyEvent &keyEvent)
{
    //
    // Things to consider:
    //
    // (Alt + '̃~') + a      -> compose('~', 'a')
    // (Compose) + '\'' + e -> compose('\'', 'e')
    // complex (i.e Chinese et al) input handling
    // Multiline text edit backspace at start of line
    //
    emscripten::val event = keyEvent.webEvent;
    bool useInputContext = [event]() -> bool {
        const QWasmInputContext *wasmInput = QWasmIntegration::get()->wasmInputContext();
        if (!wasmInput)
            return false;

        const auto keyString = QString::fromStdString(event["key"].as<std::string>());
        qCDebug(qLcQpaWasmInputContext) << "Key callback" << keyString << keyString.size();

        // Events with isComposing set are handled by the input context
        bool composing = event["isComposing"].as<bool>();

        // Android makes a bunch of KeyEvents as "Unidentified",
        // make inputContext handle those.
        bool androidUnidentified = (keyString == "Unidentified");

        // Not all platforms use 'isComposing' for '~' + 'a', in this
        // case send the key with state ('ctrl', 'alt', or 'meta') to
        // processKeyForInputContext
        bool hasModifiers = event["ctrlKey"].as<bool>()
                                 || event["altKey"].as<bool>()
                                 || event["metaKey"].as<bool>();

        // This is like; 'Shift','ArrowRight','AltGraph', ...
        // send all of these to processKeyForInputContext
        bool hasNoncharacterKeyString = keyString.size() != 1;

        bool overrideCompose = !hasModifiers && !hasNoncharacterKeyString && wasmInput->inputMethodAccepted();
        return composing || androidUnidentified || overrideCompose;
    }();

    if (!useInputContext) {
        qCDebug(qLcQpaWasmInputContext) << "processKey as KeyEvent";
        if (processKeyForInputContext(keyEvent))
            event.call<void>("preventDefault");
        event.call<void>("stopImmediatePropagation");
    }
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

#if QT_CONFIG(clipboard)
    // Copy/Cut callback required to copy qtClipboard to system clipboard
    if (keySeq == QKeySequence::Copy || keySeq == QKeySequence::Cut)
        return false;
#endif

    return result;
}

void QWasmWindow::handleInputEvent(emscripten::val event)
{
    if (QWasmInputContext *inputContext = QWasmIntegration::get()->wasmInputContext(); inputContext->isActive())
        inputContext->inputCallback(event);
    else
        m_focusHelper.set("innerHTML", std::string());
}

void QWasmWindow::handleCompositionStartEvent(emscripten::val event)
{
    if (QWasmInputContext *inputContext = QWasmIntegration::get()->wasmInputContext(); inputContext->isActive())
        inputContext->compositionStartCallback(event);
    else
        m_focusHelper.set("innerHTML", std::string());
}

void QWasmWindow::handleCompositionUpdateEvent(emscripten::val event)
{
    if (QWasmInputContext *inputContext = QWasmIntegration::get()->wasmInputContext(); inputContext->isActive())
        inputContext->compositionUpdateCallback(event);
    else
        m_focusHelper.set("innerHTML", std::string());
}

void QWasmWindow::handleCompositionEndEvent(emscripten::val event)
{
    if (QWasmInputContext *inputContext = QWasmIntegration::get()->wasmInputContext(); inputContext->isActive())
        inputContext->compositionEndCallback(event);
    else
        m_focusHelper.set("innerHTML", std::string());
}

void QWasmWindow::handlePointerEnterLeaveEvent(const PointerEvent &event)
{
    if (processPointerEnterLeave(event))
        event.webEvent.call<void>("preventDefault");
}

bool QWasmWindow::processPointerEnterLeave(const PointerEvent &event)
{
    if (event.pointerType != PointerType::Mouse && event.pointerType != PointerType::Pen)
        return false;

    switch (event.type) {
    case EventType::PointerEnter: {
        const auto pointInScreen = platformScreen()->mapFromLocal(
            dom::mapPoint(event.target(), platformScreen()->element(), event.localPoint));
        QWindowSystemInterface::handleEnterEvent(
                window(), mapFromGlobal(pointInScreen.toPoint()), pointInScreen);
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

void QWasmWindow::processPointer(const PointerEvent &event)
{
    switch (event.type) {
    case EventType::PointerDown:
        if (event.isTargetedForQtElement())
            m_window.call<void>("setPointerCapture", event.pointerId);

        if ((window()->flags() & Qt::WindowDoesNotAcceptFocus)
                    != Qt::WindowDoesNotAcceptFocus
            && window()->isTopLevel())
                window()->requestActivate();
        break;
    case EventType::PointerUp:
        if (event.isTargetedForQtElement())
            m_window.call<void>("releasePointerCapture", event.pointerId);
        break;
    default:
        break;
    };

    const bool eventAccepted = deliverPointerEvent(event);
    if (!eventAccepted && event.type == EventType::PointerDown)
        QGuiApplicationPrivate::instance()->closeAllPopups();

    if (eventAccepted) {
        event.webEvent.call<void>("preventDefault");
        event.webEvent.call<void>("stopPropagation");
    }
}

bool QWasmWindow::deliverPointerEvent(const PointerEvent &event)
{
    const auto pointInScreen = platformScreen()->mapFromLocal(
        dom::mapPoint(event.target(), platformScreen()->element(), event.localPoint));

    const auto geometryF = platformScreen()->geometry().toRectF();
    const QPointF targetPointClippedToScreen(
            qBound(geometryF.left(), pointInScreen.x(), geometryF.right()),
            qBound(geometryF.top(), pointInScreen.y(), geometryF.bottom()));

    if (event.pointerType == PointerType::Mouse) {
        const QEvent::Type eventType =
                MouseEvent::mouseEventTypeFromEventType(event.type, WindowArea::Client);

        return eventType != QEvent::None
                && QWindowSystemInterface::handleMouseEvent(
                        window(), QWasmIntegration::getTimestamp(),
                        window()->mapFromGlobal(targetPointClippedToScreen),
                        targetPointClippedToScreen, event.mouseButtons, event.mouseButton,
                        eventType, event.modifiers);
    }

    if (event.pointerType == PointerType::Pen) {
        qreal pressure;
        switch (event.type) {
            case EventType::PointerDown :
            case EventType::PointerMove :
                pressure = event.pressure;
                break;
            case EventType::PointerUp :
                pressure = 0.0;
                break;
            default:
                return false;
        }
        // Tilt in the browser is in the range +-90, but QTabletEvent only goes to +-60.
        qreal xTilt = qBound(-60.0, event.tiltX, 60.0);
        qreal yTilt = qBound(-60.0, event.tiltY, 60.0);
        // Barrel rotation is reported as 0 to 359, but QTabletEvent wants a signed value.
        qreal rotation = event.twist > 180.0 ? 360.0 - event.twist : event.twist;
        return QWindowSystemInterface::handleTabletEvent(
            window(), QWasmIntegration::getTimestamp(), platformScreen()->tabletDevice(),
            window()->mapFromGlobal(targetPointClippedToScreen),
            targetPointClippedToScreen, event.mouseButtons, pressure, xTilt, yTilt,
            event.tangentialPressure, rotation, event.modifiers);
    }

    QWindowSystemInterface::TouchPoint *touchPoint;

    QPointF pointInTargetWindowCoords =
            QPointF(window()->mapFromGlobal(targetPointClippedToScreen));
    QPointF normalPosition(pointInTargetWindowCoords.x() / window()->width(),
                           pointInTargetWindowCoords.y() / window()->height());

    const auto tp = m_pointerIdToTouchPoints.find(event.pointerId);
    if (event.pointerType != PointerType::Pen && tp != m_pointerIdToTouchPoints.end()) {
        touchPoint = &tp.value();
    } else {
        touchPoint = &m_pointerIdToTouchPoints
                              .insert(event.pointerId, QWindowSystemInterface::TouchPoint())
                              .value();

        // Assign touch point id. TouchPoint::id is int, but QGuiApplicationPrivate::processTouchEvent()
        // will not synthesize mouse events for touch points with negative id; use the absolute value for
        // the touch point id.
        touchPoint->id = qAbs(event.pointerId);

        touchPoint->state = QEventPoint::State::Pressed;
    }

    const bool stationaryTouchPoint = (normalPosition == touchPoint->normalPosition);
    touchPoint->normalPosition = normalPosition;
    touchPoint->area = QRectF(targetPointClippedToScreen, QSizeF(event.width, event.height))
                                   .translated(-event.width / 2, -event.height / 2);
    touchPoint->pressure = event.pressure;

    switch (event.type) {
    case EventType::PointerUp:
        touchPoint->state = QEventPoint::State::Released;
        break;
    case EventType::PointerMove:
        touchPoint->state = (stationaryTouchPoint ? QEventPoint::State::Stationary
                                                  : QEventPoint::State::Updated);
        break;
    default:
        break;
    }

    QList<QWindowSystemInterface::TouchPoint> touchPointList;
    touchPointList.reserve(m_pointerIdToTouchPoints.size());
    std::transform(m_pointerIdToTouchPoints.begin(), m_pointerIdToTouchPoints.end(),
                   std::back_inserter(touchPointList),
                   [](const QWindowSystemInterface::TouchPoint &val) { return val; });

    if (event.type == EventType::PointerUp)
        m_pointerIdToTouchPoints.remove(event.pointerId);

    return event.type == EventType::PointerCancel
            ? QWindowSystemInterface::handleTouchCancelEvent(
                    window(), QWasmIntegration::getTimestamp(), platformScreen()->touchDevice(),
                    event.modifiers)
            : QWindowSystemInterface::handleTouchEvent(
                    window(), QWasmIntegration::getTimestamp(), platformScreen()->touchDevice(),
                    touchPointList, event.modifiers);
}

void QWasmWindow::handleWheelEvent(const emscripten::val &event)
{
    if (processWheel(WheelEvent(EventType::Wheel, event)))
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

// Fix top level window flags in case only the type flags are passed.
Qt::WindowFlags QWasmWindow::fixTopLevelWindowFlags(Qt::WindowFlags flags) const
{
    if (!(flags.testFlag(Qt::CustomizeWindowHint))) {
        if (flags.testFlag(Qt::Window)) {
            flags |= Qt::WindowTitleHint | Qt::WindowSystemMenuHint
                  |Qt::WindowMaximizeButtonHint|Qt::WindowCloseButtonHint;
        }
        if (flags.testFlag(Qt::Dialog) || flags.testFlag(Qt::Tool))
            flags |= Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint;

        if ((flags & Qt::WindowType_Mask) == Qt::SplashScreen)
            flags |= Qt::FramelessWindowHint;
    }
    return flags;
}

bool QWasmWindow::shouldBeAboveTransientParentFlags(Qt::WindowFlags flags) const
{
    if (!transientParent())
        return false;

    if (isModal())
        return true;

    if (flags.testFlag(Qt::Tool) ||
        flags.testFlag(Qt::SplashScreen) ||
        flags.testFlag(Qt::ToolTip) ||
        flags.testFlag(Qt::Popup))
    {
        return true;
    }

    return false;
}

QWasmWindowStack<>::PositionPreference QWasmWindow::positionPreferenceFromWindowFlags(Qt::WindowFlags flags) const
{
    flags = fixTopLevelWindowFlags(flags);

    if (flags.testFlag(Qt::WindowStaysOnTopHint))
        return QWasmWindowStack<>::PositionPreference::StayOnTop;
    if (flags.testFlag(Qt::WindowStaysOnBottomHint))
        return QWasmWindowStack<>::PositionPreference::StayOnBottom;
    if (shouldBeAboveTransientParentFlags(flags))
        return QWasmWindowStack<>::PositionPreference::StayAboveTransientParent;
    return QWasmWindowStack<>::PositionPreference::Regular;
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
    m_compositor->requestUpdateWindow(this, QRect(QPoint(0, 0), geometry().size()), QWasmCompositor::UpdateRequestDelivery);
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
    m_focusHelper.call<void>("focus");
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
        m_decoratedWindow["classList"].call<void>("add", emscripten::val("blocked"));
        return false; // Propagate further
    case QEvent::WindowUnblocked:;
        m_decoratedWindow["classList"].call<void>("remove", emscripten::val("blocked"));
        return false; // Propagate further
    default:
        return QPlatformWindow::windowEvent(event);
    }
}

void QWasmWindow::setMask(const QRegion &region)
{
    if (region.isEmpty()) {
        m_decoratedWindow["style"].set("clipPath", emscripten::val(""));
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
    m_decoratedWindow["style"].set("clipPath", emscripten::val(cssClipPath.str()));
}

void QWasmWindow::onTransientParentChanged(QWindow *newTransientParent)
{
    Q_UNUSED(newTransientParent);

    const auto positionPreference = positionPreferenceFromWindowFlags(window()->flags());
    QWasmWindowTreeNode::onParentChanged(parentNode(), nullptr, positionPreference);
    QWasmWindowTreeNode::onParentChanged(nullptr, parentNode(), positionPreference);
}

void QWasmWindow::onModalityChanged()
{
    const auto positionPreference = positionPreferenceFromWindowFlags(window()->flags());
    QWasmWindowTreeNode::onParentChanged(parentNode(), nullptr, positionPreference);
    QWasmWindowTreeNode::onParentChanged(nullptr, parentNode(), positionPreference);
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
    return m_window;
}

QWasmWindowTreeNode<> *QWasmWindow::parentNode()
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
                                  QWasmWindowStack<>::PositionPreference positionPreference)
{
    if (previous)
        previous->containerElement().call<void>("removeChild", m_decoratedWindow);
    if (current)
        current->containerElement().call<void>("appendChild", m_decoratedWindow);
    QWasmWindowTreeNode::onParentChanged(previous, current, positionPreference);
}

QT_END_NAMESPACE
