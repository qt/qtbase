// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qpa/qwindowsysteminterface.h>
#include <private/qguiapplication_p.h>
#include <QtCore/qfile.h>
#include <QtGui/private/qwindow_p.h>
#include <private/qpixmapcache_p.h>
#include <QtGui/qopenglfunctions.h>
#include <QBuffer>

#include "qwasmwindow.h"
#include "qwasmscreen.h"
#include "qwasmstylepixmaps_p.h"
#include "qwasmcompositor.h"
#include "qwasmevent.h"
#include "qwasmeventdispatcher.h"
#include "qwasmstring.h"
#include "qwasmaccessibility.h"

#include <iostream>
#include <emscripten/val.h>

#include <GL/gl.h>

QT_BEGIN_NAMESPACE

Q_GUI_EXPORT int qt_defaultDpiX();

namespace {
enum class IconType {
    Maximize,
    First = Maximize,
    QtLogo,
    Restore,
    X,
    Size,
};

void syncCSSClassWith(emscripten::val element, std::string cssClassName, bool flag)
{
    if (flag) {
        element["classList"].call<void>("add", emscripten::val(std::move(cssClassName)));
        return;
    }

    element["classList"].call<void>("remove", emscripten::val(std::move(cssClassName)));
}

} // namespace

class QWasmWindow::Resizer
{
public:
    class ResizerElement
    {
    public:
        static constexpr const char *cssClassNameForEdges(Qt::Edges edges)
        {
            switch (edges) {
            case Qt::TopEdge | Qt::LeftEdge:;
                return "nw";
            case Qt::TopEdge:
                return "n";
            case Qt::TopEdge | Qt::RightEdge:
                return "ne";
            case Qt::LeftEdge:
                return "w";
            case Qt::RightEdge:
                return "e";
            case Qt::BottomEdge | Qt::LeftEdge:
                return "sw";
            case Qt::BottomEdge:
                return "s";
            case Qt::BottomEdge | Qt::RightEdge:
                return "se";
            default:
                Q_ASSERT(false); // notreached
                return "";
            }
        }

        ResizerElement(emscripten::val parentElement, Qt::Edges edges, Resizer *resizer)
            : m_element(emscripten::val::global("document")
                                .call<emscripten::val>("createElement", emscripten::val("div"))),
              m_edges(edges),
              m_resizer(resizer)
        {
            Q_ASSERT_X(m_resizer, Q_FUNC_INFO, "Resizer cannot be null");

            m_element["classList"].call<void>("add", emscripten::val("resize-outline"));
            m_element["classList"].call<void>("add", emscripten::val(cssClassNameForEdges(edges)));

            parentElement.call<void>("appendChild", m_element);

            m_mouseDownEvent = std::make_unique<qstdweb::EventCallback>(
                    m_element, "pointerdown", [this](emscripten::val event) {
                        if (!onPointerDown(*PointerEvent::fromWeb(event)))
                            return;
                        m_resizer->onInteraction();
                        event.call<void>("preventDefault");
                        event.call<void>("stopPropagation");
                    });
            m_mouseDragEvent = std::make_unique<qstdweb::EventCallback>(
                    m_element, "pointermove", [this](emscripten::val event) {
                        if (onPointerMove(*PointerEvent::fromWeb(event))) {
                            event.call<void>("preventDefault");
                            event.call<void>("stopPropagation");
                        }
                    });
            m_mouseUpEvent = std::make_unique<qstdweb::EventCallback>(
                    m_element, "pointerup", [this](emscripten::val event) {
                        if (onPointerUp(*PointerEvent::fromWeb(event))) {
                            event.call<void>("preventDefault");
                            event.call<void>("stopPropagation");
                        }
                    });
        }

        ~ResizerElement()
        {
            m_element["parentElement"].call<emscripten::val>("removeChild", m_element);
        }
        ResizerElement(const ResizerElement &other) = delete;
        ResizerElement(ResizerElement &&other) = default;
        ResizerElement &operator=(const ResizerElement &other) = delete;
        ResizerElement &operator=(ResizerElement &&other) = delete;

        bool onPointerDown(const PointerEvent &event)
        {
            if (event.pointerType != PointerType::Mouse)
                return false;

            m_element.call<void>("setPointerCapture", event.pointerId);
            m_capturedPointerId = event.pointerId;

            m_resizer->startResize(m_edges, event.pointInPage);
            return true;
        }

        bool onPointerMove(const PointerEvent &event)
        {
            if (m_capturedPointerId != event.pointerId)
                return false;

            m_resizer->continueResize(event.pointInPage);
            return true;
        }

        bool onPointerUp(const PointerEvent &event)
        {
            if (m_capturedPointerId != event.pointerId)
                return false;

            m_resizer->finishResize();
            m_element.call<void>("releasePointerCapture", event.pointerId);
            m_capturedPointerId = -1;
            return true;
        }

    private:
        emscripten::val m_element;

        int m_capturedPointerId = -1;

        const Qt::Edges m_edges;

        Resizer *m_resizer;

        std::unique_ptr<qstdweb::EventCallback> m_mouseDownEvent;
        std::unique_ptr<qstdweb::EventCallback> m_mouseDragEvent;
        std::unique_ptr<qstdweb::EventCallback> m_mouseUpEvent;
    };

    using ClickCallback = std::function<void()>;

    Resizer(QWasmWindow *window, emscripten::val parentElement) : m_window(window)
    {
        Q_ASSERT_X(m_window, Q_FUNC_INFO, "Window must not be null");

        constexpr std::array<int, 8> ResizeEdges = { Qt::TopEdge | Qt::LeftEdge,
                                                     Qt::TopEdge,
                                                     Qt::TopEdge | Qt::RightEdge,
                                                     Qt::LeftEdge,
                                                     Qt::RightEdge,
                                                     Qt::BottomEdge | Qt::LeftEdge,
                                                     Qt::BottomEdge,
                                                     Qt::BottomEdge | Qt::RightEdge };
        std::transform(std::begin(ResizeEdges), std::end(ResizeEdges),
                       std::back_inserter(m_elements), [parentElement, this](int edges) {
                           return std::make_unique<ResizerElement>(parentElement,
                                                                   Qt::Edges::fromInt(edges), this);
                       });
    }

    ~Resizer() = default;

private:
    void onInteraction() { m_window->onInteraction(); }

    void startResize(Qt::Edges resizeEdges, const QPoint &origin)
    {
        Q_ASSERT_X(!m_currentResizeData, Q_FUNC_INFO, "Another resize in progress");

        const QWindow *window = m_window->window();
        // TODO(mikolajboc): Implement system resize
        // .m_originInScreenCoords = m_systemDragInitData.lastMouseMovePoint,
        m_currentResizeData.reset(new ResizeData{
                .edges = resizeEdges,
                .originInScreenCoords = origin,
                .initialWindowBounds = window->geometry(),
                .minShrink = QPoint(window->minimumWidth() - window->geometry().width(),
                                    window->minimumHeight() - window->geometry().height()),
                .maxGrow = QPoint(window->maximumWidth() - window->geometry().width(),
                                  window->maximumHeight() - window->geometry().height()) });
    }

    void continueResize(const QPoint &point)
    {
        const auto amount = point - m_currentResizeData->originInScreenCoords;
        const QPoint cappedGrowVector(
                std::min(m_currentResizeData->maxGrow.x(),
                         std::max(m_currentResizeData->minShrink.x(),
                                  (m_currentResizeData->edges & Qt::Edge::LeftEdge) ? -amount.x()
                                          : (m_currentResizeData->edges & Qt::Edge::RightEdge)
                                          ? amount.x()
                                          : 0)),
                std::min(m_currentResizeData->maxGrow.y(),
                         std::max(m_currentResizeData->minShrink.y(),
                                  (m_currentResizeData->edges & Qt::Edge::TopEdge) ? -amount.y()
                                          : (m_currentResizeData->edges & Qt::Edge::BottomEdge)
                                          ? amount.y()
                                          : 0)));

        auto bounds = m_currentResizeData->initialWindowBounds.adjusted(
                (m_currentResizeData->edges & Qt::Edge::LeftEdge) ? -cappedGrowVector.x() : 0,
                (m_currentResizeData->edges & Qt::Edge::TopEdge) ? -cappedGrowVector.y() : 0,
                (m_currentResizeData->edges & Qt::Edge::RightEdge) ? cappedGrowVector.x() : 0,
                (m_currentResizeData->edges & Qt::Edge::BottomEdge) ? cappedGrowVector.y() : 0);
        bounds.setY(std::max(m_window->screen()->geometry().y() + m_window->frameMargins().top(),
                             bounds.y()));
        m_window->setGeometry(std::move(bounds));
    }

    void finishResize()
    {
        Q_ASSERT_X(m_currentResizeData, Q_FUNC_INFO, "No resize in progress");
        m_currentResizeData.reset();
    }

    struct ResizeData
    {
        Qt::Edges edges = Qt::Edges::fromInt(0);
        QPoint originInScreenCoords;
        QRect initialWindowBounds;
        QPoint minShrink;
        QPoint maxGrow;
    };
    std::unique_ptr<ResizeData> m_currentResizeData;

    QWasmWindow *m_window;
    std::vector<std::unique_ptr<ResizerElement>> m_elements;
};

class QWasmWindow::WebImageButton
{
public:
    class Callbacks
    {
    public:
        Callbacks() = default;
        Callbacks(std::function<void()> onInteraction, std::function<void()> onClick)
            : m_onInteraction(std::move(onInteraction)), m_onClick(std::move(onClick))
        {
            Q_ASSERT_X(!!m_onInteraction == !!m_onClick, Q_FUNC_INFO,
                       "Both callbacks need to be either null or non-null");
        }
        ~Callbacks() = default;

        Callbacks(const Callbacks &) = delete;
        Callbacks(Callbacks &&) = default;
        Callbacks &operator=(const Callbacks &) = delete;
        Callbacks &operator=(Callbacks &&) = default;

        operator bool() const { return !!m_onInteraction; }

        void onInteraction() { return m_onInteraction(); }
        void onClick() { return m_onClick(); }

    private:
        std::function<void()> m_onInteraction;
        std::function<void()> m_onClick;
    };

    WebImageButton()
        : m_containerElement(
                emscripten::val::global("document")
                        .call<emscripten::val>("createElement", emscripten::val("div"))),
          m_imageHolderElement(
                  emscripten::val::global("document")
                          .call<emscripten::val>("createElement", emscripten::val("span")))
    {
        m_imageHolderElement.set("draggable", false);

        m_containerElement["classList"].call<void>("add", emscripten::val("image-button"));
        m_containerElement.call<void>("appendChild", m_imageHolderElement);
    }

    ~WebImageButton() = default;

    void setCallbacks(Callbacks callbacks)
    {
        if (callbacks) {
            if (!m_webClickEventCallback) {
                m_webMouseDownEventCallback = std::make_unique<qstdweb::EventCallback>(
                        m_containerElement, "mousedown", [this](emscripten::val event) {
                            event.call<void>("preventDefault");
                            event.call<void>("stopPropagation");
                            m_callbacks.onInteraction();
                        });
                m_webClickEventCallback = std::make_unique<qstdweb::EventCallback>(
                        m_containerElement, "click",
                        [this](emscripten::val) { m_callbacks.onClick(); });
            }
        } else {
            m_webMouseDownEventCallback.reset();
            m_webClickEventCallback.reset();
        }
        syncCSSClassWith(m_containerElement, "action-button", !!callbacks);
        m_callbacks = std::move(callbacks);
    }

    void setImage(std::string_view imageData, std::string_view format)
    {
        m_imageHolderElement.call<void>("removeAttribute",
                                        emscripten::val("qt-builtin-image-type"));
        m_imageHolderElement["style"].set("backgroundImage",
                                          "url('data:image/" + std::string(format) + ";base64,"
                                                  + std::string(imageData) + "')");
    }

    void setImage(IconType type)
    {
        m_imageHolderElement["style"].set("backgroundImage", emscripten::val::undefined());
        const auto imageType = ([type]() {
            switch (type) {
            case IconType::QtLogo:
                return "qt-logo";
            case IconType::X:
                return "x";
            case IconType::Restore:
                return "restore";
            case IconType::Maximize:
                return "maximize";
            default:
                return "err";
            }
        })();
        m_imageHolderElement.call<void>("setAttribute", emscripten::val("qt-builtin-image-type"),
                                        emscripten::val(imageType));
    }

    void setVisible(bool visible)
    {
        m_containerElement["style"].set("display", visible ? "flex" : "none");
    }

    emscripten::val htmlElement() const { return m_containerElement; }
    emscripten::val imageElement() const { return m_imageHolderElement; }

private:
    emscripten::val m_containerElement;
    emscripten::val m_imageHolderElement;
    std::unique_ptr<qstdweb::EventCallback> m_webMouseMoveEventCallback;
    std::unique_ptr<qstdweb::EventCallback> m_webMouseDownEventCallback;
    std::unique_ptr<qstdweb::EventCallback> m_webClickEventCallback;

    Callbacks m_callbacks;
};

QWasmWindow::QWasmWindow(QWindow *w, QWasmCompositor *compositor, QWasmBackingStore *backingStore)
    : QPlatformWindow(w),
      m_window(w),
      m_compositor(compositor),
      m_backingStore(backingStore),
      m_document(emscripten::val::global("document")),
      m_qtWindow(m_document.call<emscripten::val>("createElement", emscripten::val("div"))),
      m_windowContents(m_document.call<emscripten::val>("createElement", emscripten::val("div"))),
      m_titleBar(m_document.call<emscripten::val>("createElement", emscripten::val("div"))),
      m_label(m_document.call<emscripten::val>("createElement", emscripten::val("div"))),
      m_canvasContainer(m_document.call<emscripten::val>("createElement", emscripten::val("div"))),
      m_a11yContainer(m_document.call<emscripten::val>("createElement", emscripten::val("div"))),
      m_canvas(m_document.call<emscripten::val>("createElement", emscripten::val("canvas")))
{
    m_qtWindow.set("className", "qt-window");
    m_qtWindow["style"].set("display", std::string("none"));

    m_qtWindow.call<void>("appendChild", m_windowContents);

    m_resizer = std::make_unique<Resizer>(this, m_qtWindow);

    m_icon = std::make_unique<WebImageButton>();
    m_icon->setImage(IconType::QtLogo);

    m_titleBar.call<void>("appendChild", m_icon->htmlElement());
    m_titleBar.set("className", "title-bar");

    auto spacer = m_document.call<emscripten::val>("createElement", emscripten::val("div"));
    spacer["style"].set("width", "4px");
    m_titleBar.call<void>("appendChild", spacer);

    m_label.set("innerText", emscripten::val(window()->title().toStdString()));
    m_label.set("className", "window-name");

    m_titleBar.call<void>("appendChild", m_label);

    spacer = m_document.call<emscripten::val>("createElement", emscripten::val("div"));
    spacer.set("className", "spacer");
    m_titleBar.call<void>("appendChild", spacer);

    m_restore = std::make_unique<WebImageButton>();
    m_restore->setImage(IconType::Restore);
    m_restore->setCallbacks(WebImageButton::Callbacks([this]() { onInteraction(); },
                                                      [this]() { onRestoreClicked(); }));

    m_titleBar.call<void>("appendChild", m_restore->htmlElement());

    m_maximize = std::make_unique<WebImageButton>();
    m_maximize->setImage(IconType::Maximize);
    m_maximize->setCallbacks(WebImageButton::Callbacks([this]() { onInteraction(); },
                                                       [this]() { onMaximizeClicked(); }));

    m_titleBar.call<void>("appendChild", m_maximize->htmlElement());

    m_close = std::make_unique<WebImageButton>();
    m_close->setImage(IconType::X);
    m_close->setCallbacks(WebImageButton::Callbacks([this]() { onInteraction(); },
                                                    [this]() { onCloseClicked(); }));

    m_titleBar.call<void>("appendChild", m_close->htmlElement());

    m_windowContents.call<void>("appendChild", m_titleBar);

    m_canvas["classList"].call<void>("add", emscripten::val("qt-window-content"));

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
    const auto border = borderMargins();
    const auto titleBarBounds =
            QRectF::fromDOMRect(m_titleBar.call<emscripten::val>("getBoundingClientRect"));

    return QMarginsF(border.left(), border.top() + titleBarBounds.height(), border.right(),
                     border.bottom())
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

bool QWasmWindow::startSystemResize(Qt::Edges)
{
    // TODO(mikolajboc): This can only be implemented if per-window events are up and running
    return false;
}

void QWasmWindow::onRestoreClicked()
{
    window()->setWindowState(Qt::WindowNoState);
}

void QWasmWindow::onMaximizeClicked()
{
    window()->setWindowState(Qt::WindowMaximized);
}

void QWasmWindow::onCloseClicked()
{
    window()->close();
}

void QWasmWindow::onInteraction()
{
    if (!isActive())
        requestActivateWindow();
}

QMarginsF QWasmWindow::borderMargins() const
{
    const auto frameRect =
            QRectF::fromDOMRect(m_qtWindow.call<emscripten::val>("getBoundingClientRect"));
    const auto canvasRect =
            QRectF::fromDOMRect(m_windowContents.call<emscripten::val>("getBoundingClientRect"));
    return QMarginsF(canvasRect.left() - frameRect.left(), canvasRect.top() - frameRect.top(),
                     frameRect.right() - canvasRect.right(),
                     frameRect.bottom() - canvasRect.bottom());
}

bool QWasmWindow::isPointOnTitle(QPoint globalPoint) const
{
    return QRectF::fromDOMRect(m_titleBar.call<emscripten::val>("getBoundingClientRect"))
            .contains(globalPoint);
}

void QWasmWindow::invalidate()
{
    m_compositor->requestUpdateWindow(this);
}

void QWasmWindow::onActivationChanged(bool active)
{
    syncCSSClassWith(m_qtWindow, "inactive", !active);
}

void QWasmWindow::setWindowFlags(Qt::WindowFlags flags)
{
    m_flags = flags;
    syncCSSClassWith(m_qtWindow, "has-title-bar", hasTitleBar());
}

void QWasmWindow::setWindowState(Qt::WindowStates newState)
{
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
    m_label.set("innerText", emscripten::val(title.toStdString()));
}

void QWasmWindow::setWindowIcon(const QIcon &icon)
{
    const auto dpi = screen()->devicePixelRatio();
    auto pixmap = icon.pixmap(10 * dpi, 10 * dpi);
    if (pixmap.isNull()) {
        m_icon->setImage(IconType::QtLogo);
        return;
    }

    QByteArray bytes;
    QBuffer buffer(&bytes);
    pixmap.save(&buffer, "png");
    m_icon->setImage(bytes.toBase64().toStdString(), "png");
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

    syncCSSClassWith(m_qtWindow, "has-title-bar", hasTitleBar());
    syncCSSClassWith(m_qtWindow, "maximized", isMaximized);

    m_restore->setVisible(isMaximized);
    m_maximize->setVisible(!isMaximized);

    if (isVisible())
        QWindowSystemInterface::handleWindowStateChanged(window(), m_state, m_previousWindowState);
    setGeometry(newGeom);
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
    QPlatformWindow::requestActivateWindow();
}

bool QWasmWindow::setMouseGrabEnabled(bool grab)
{
    if (grab)
        m_compositor->setCapture(this);
    else
        m_compositor->releaseCapture();
    return true;
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
