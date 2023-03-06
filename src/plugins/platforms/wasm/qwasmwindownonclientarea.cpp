// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmwindownonclientarea.h"

#include "qwasmbase64iconstore.h"
#include "qwasmdom.h"
#include "qwasmevent.h"
#include "qwasmintegration.h"

#include <qpa/qwindowsysteminterface.h>

#include <QtCore/qassert.h>

QT_BEGIN_NAMESPACE

WebImageButton::Callbacks::Callbacks() = default;
WebImageButton::Callbacks::Callbacks(std::function<void()> onInteraction,
                                     std::function<void()> onClick)
    : m_onInteraction(std::move(onInteraction)), m_onClick(std::move(onClick))
{
    Q_ASSERT_X(!!m_onInteraction == !!m_onClick, Q_FUNC_INFO,
               "Both callbacks need to be either null or non-null");
}
WebImageButton::Callbacks::~Callbacks() = default;

WebImageButton::Callbacks::Callbacks(Callbacks &&) = default;
WebImageButton::Callbacks &WebImageButton::Callbacks::operator=(Callbacks &&) = default;

void WebImageButton::Callbacks::onInteraction()
{
    return m_onInteraction();
}

void WebImageButton::Callbacks::onClick()
{
    return m_onClick();
}

WebImageButton::WebImageButton()
    : m_containerElement(
            dom::document().call<emscripten::val>("createElement", emscripten::val("div"))),
      m_imgElement(dom::document().call<emscripten::val>("createElement", emscripten::val("img")))
{
    m_imgElement.set("draggable", false);

    m_containerElement["classList"].call<void>("add", emscripten::val("image-button"));
    m_containerElement.call<void>("appendChild", m_imgElement);
}

WebImageButton::~WebImageButton() = default;

void WebImageButton::setCallbacks(Callbacks callbacks)
{
    if (callbacks) {
        if (!m_webClickEventCallback) {
            m_webMouseDownEventCallback = std::make_unique<qstdweb::EventCallback>(
                    m_containerElement, "pointerdown", [this](emscripten::val event) {
                        event.call<void>("preventDefault");
                        event.call<void>("stopPropagation");
                        m_callbacks.onInteraction();
                    });
            m_webClickEventCallback = std::make_unique<qstdweb::EventCallback>(
                    m_containerElement, "click", [this](emscripten::val event) {
                        m_callbacks.onClick();
                        event.call<void>("stopPropagation");
                    });
        }
    } else {
        m_webMouseDownEventCallback.reset();
        m_webClickEventCallback.reset();
    }
    dom::syncCSSClassWith(m_containerElement, "action-button", !!callbacks);
    m_callbacks = std::move(callbacks);
}

void WebImageButton::setImage(std::string_view imageData, std::string_view format)
{
    m_imgElement.set("src",
                     "data:image/" + std::string(format) + ";base64," + std::string(imageData));
}

void WebImageButton::setVisible(bool visible)
{
    m_containerElement["style"].set("display", visible ? "flex" : "none");
}

Resizer::ResizerElement::ResizerElement(emscripten::val parentElement, Qt::Edges edges,
                                        Resizer *resizer)
    : m_element(dom::document().call<emscripten::val>("createElement", emscripten::val("div"))),
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
    m_mouseMoveEvent = std::make_unique<qstdweb::EventCallback>(
            m_element, "pointermove", [this](emscripten::val event) {
                if (onPointerMove(*PointerEvent::fromWeb(event)))
                    event.call<void>("preventDefault");
            });
    m_mouseUpEvent = std::make_unique<qstdweb::EventCallback>(
            m_element, "pointerup", [this](emscripten::val event) {
                if (onPointerUp(*PointerEvent::fromWeb(event))) {
                    event.call<void>("preventDefault");
                    event.call<void>("stopPropagation");
                }
            });
}

Resizer::ResizerElement::~ResizerElement()
{
    m_element["parentElement"].call<emscripten::val>("removeChild", m_element);
}

Resizer::ResizerElement::ResizerElement(ResizerElement &&other) = default;

bool Resizer::ResizerElement::onPointerDown(const PointerEvent &event)
{
    m_element.call<void>("setPointerCapture", event.pointerId);
    m_capturedPointerId = event.pointerId;

    m_resizer->startResize(m_edges, event);
    return true;
}

bool Resizer::ResizerElement::onPointerMove(const PointerEvent &event)
{
    if (m_capturedPointerId != event.pointerId)
        return false;

    m_resizer->continueResize(event);
    return true;
}

bool Resizer::ResizerElement::onPointerUp(const PointerEvent &event)
{
    if (m_capturedPointerId != event.pointerId)
        return false;

    m_resizer->finishResize();
    m_element.call<void>("releasePointerCapture", event.pointerId);
    m_capturedPointerId = -1;
    return true;
}

Resizer::Resizer(QWasmWindow *window, emscripten::val parentElement)
    : m_window(window), m_windowElement(parentElement)
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
    std::transform(std::begin(ResizeEdges), std::end(ResizeEdges), std::back_inserter(m_elements),
                   [parentElement, this](int edges) {
                       return std::make_unique<ResizerElement>(parentElement,
                                                               Qt::Edges::fromInt(edges), this);
                   });
}

Resizer::~Resizer() = default;

ResizeConstraints Resizer::getResizeConstraints() {
    const auto *window = m_window->window();
    const auto minShrink = QPoint(window->minimumWidth() - window->geometry().width(),
                              window->minimumHeight() - window->geometry().height());
    const auto maxGrow = QPoint(window->maximumWidth() - window->geometry().width(),
                          window->maximumHeight() - window->geometry().height());

    const auto frameRect =
            QRectF::fromDOMRect(m_windowElement.call<emscripten::val>("getBoundingClientRect"));
    const auto screenRect = QRectF::fromDOMRect(
            m_window->platformScreen()->element().call<emscripten::val>("getBoundingClientRect"));
    const int maxGrowTop = frameRect.top() - screenRect.top();

    return ResizeConstraints{minShrink, maxGrow, maxGrowTop};
}

void Resizer::onInteraction()
{
    m_window->onNonClientAreaInteraction();
}

void Resizer::startResize(Qt::Edges resizeEdges, const PointerEvent &event)
{
    Q_ASSERT_X(!m_currentResizeData, Q_FUNC_INFO, "Another resize in progress");

    m_currentResizeData.reset(new ResizeData{
            .edges = resizeEdges,
            .originInScreenCoords = dom::mapPoint(
                    event.target, m_window->platformScreen()->element(), event.localPoint),
    });

    const auto resizeConstraints = getResizeConstraints();
    m_currentResizeData->minShrink = resizeConstraints.minShrink;
    m_currentResizeData->maxGrow =
            QPoint(resizeConstraints.maxGrow.x(),
                   std::min(resizeEdges & Qt::Edge::TopEdge ? resizeConstraints.maxGrowTop : INT_MAX,
                            resizeConstraints.maxGrow.y()));

    m_currentResizeData->initialBounds = m_window->window()->geometry();
}

void Resizer::continueResize(const PointerEvent &event)
{
    const auto pointInScreen =
            dom::mapPoint(event.target, m_window->platformScreen()->element(), event.localPoint);
    const auto amount = (pointInScreen - m_currentResizeData->originInScreenCoords).toPoint();
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

    auto bounds = m_currentResizeData->initialBounds.adjusted(
            (m_currentResizeData->edges & Qt::Edge::LeftEdge) ? -cappedGrowVector.x() : 0,
            (m_currentResizeData->edges & Qt::Edge::TopEdge) ? -cappedGrowVector.y() : 0,
            (m_currentResizeData->edges & Qt::Edge::RightEdge) ? cappedGrowVector.x() : 0,
            (m_currentResizeData->edges & Qt::Edge::BottomEdge) ? cappedGrowVector.y() : 0);

    m_window->window()->setGeometry(bounds);
}

void Resizer::finishResize()
{
    Q_ASSERT_X(m_currentResizeData, Q_FUNC_INFO, "No resize in progress");
    m_currentResizeData.reset();
}

TitleBar::TitleBar(QWasmWindow *window, emscripten::val parentElement)
    : m_window(window),
      m_element(dom::document().call<emscripten::val>("createElement", emscripten::val("div"))),
      m_label(dom::document().call<emscripten::val>("createElement", emscripten::val("div")))
{
    m_icon = std::make_unique<WebImageButton>();
    m_icon->setImage(Base64IconStore::get()->getIcon(Base64IconStore::IconType::QtLogo), "svg+xml");
    m_element.call<void>("appendChild", m_icon->htmlElement());
    m_element.set("className", "title-bar");

    auto spacer = dom::document().call<emscripten::val>("createElement", emscripten::val("div"));
    spacer["style"].set("width", "4px");
    m_element.call<void>("appendChild", spacer);

    m_label.set("className", "window-name");

    m_element.call<void>("appendChild", m_label);

    spacer = dom::document().call<emscripten::val>("createElement", emscripten::val("div"));
    spacer.set("className", "spacer");
    m_element.call<void>("appendChild", spacer);

    m_restore = std::make_unique<WebImageButton>();
    m_restore->setImage(Base64IconStore::get()->getIcon(Base64IconStore::IconType::Restore),
                        "svg+xml");
    m_restore->setCallbacks(
            WebImageButton::Callbacks([this]() { m_window->onNonClientAreaInteraction(); },
                                      [this]() { m_window->onRestoreClicked(); }));

    m_element.call<void>("appendChild", m_restore->htmlElement());

    m_maximize = std::make_unique<WebImageButton>();
    m_maximize->setImage(Base64IconStore::get()->getIcon(Base64IconStore::IconType::Maximize),
                         "svg+xml");
    m_maximize->setCallbacks(
            WebImageButton::Callbacks([this]() { m_window->onNonClientAreaInteraction(); },
                                      [this]() { m_window->onMaximizeClicked(); }));

    m_element.call<void>("appendChild", m_maximize->htmlElement());

    m_close = std::make_unique<WebImageButton>();
    m_close->setImage(Base64IconStore::get()->getIcon(Base64IconStore::IconType::X), "svg+xml");
    m_close->setCallbacks(
            WebImageButton::Callbacks([this]() { m_window->onNonClientAreaInteraction(); },
                                      [this]() { m_window->onCloseClicked(); }));

    m_element.call<void>("appendChild", m_close->htmlElement());

    parentElement.call<void>("appendChild", m_element);

    m_mouseDownEvent = std::make_unique<qstdweb::EventCallback>(
            m_element, "pointerdown", [this](emscripten::val event) {
                if (!onPointerDown(*PointerEvent::fromWeb(event)))
                    return;
                m_window->onNonClientAreaInteraction();
                event.call<void>("preventDefault");
                event.call<void>("stopPropagation");
            });
    m_mouseMoveEvent = std::make_unique<qstdweb::EventCallback>(
            m_element, "pointermove", [this](emscripten::val event) {
                if (onPointerMove(*PointerEvent::fromWeb(event))) {
                    event.call<void>("preventDefault");
                }
            });
    m_mouseUpEvent = std::make_unique<qstdweb::EventCallback>(
            m_element, "pointerup", [this](emscripten::val event) {
                if (onPointerUp(*PointerEvent::fromWeb(event))) {
                    event.call<void>("preventDefault");
                    event.call<void>("stopPropagation");
                }
            });
    m_doubleClickEvent = std::make_unique<qstdweb::EventCallback>(
            m_element, "dblclick", [this](emscripten::val event) {
                if (onDoubleClick()) {
                    event.call<void>("preventDefault");
                    event.call<void>("stopPropagation");
                }
            });
}

TitleBar::~TitleBar()
{
    m_element["parentElement"].call<emscripten::val>("removeChild", m_element);
}

void TitleBar::setTitle(const QString &title)
{
    m_label.set("innerText", emscripten::val(title.toStdString()));
}

void TitleBar::setRestoreVisible(bool visible)
{
    m_restore->setVisible(visible);
}

void TitleBar::setMaximizeVisible(bool visible)
{
    m_maximize->setVisible(visible);
}

void TitleBar::setCloseVisible(bool visible)
{
    m_close->setVisible(visible);
}

void TitleBar::setIcon(std::string_view imageData, std::string_view format)
{
    m_icon->setImage(imageData, format);
}

void TitleBar::setWidth(int width)
{
    m_element["style"].set("width", std::to_string(width) + "px");
}

QRectF TitleBar::geometry() const
{
    return QRectF::fromDOMRect(m_element.call<emscripten::val>("getBoundingClientRect"));
}

bool TitleBar::onPointerDown(const PointerEvent &event)
{
    m_element.call<void>("setPointerCapture", event.pointerId);
    m_capturedPointerId = event.pointerId;

    m_moveStartWindowPosition = m_window->window()->position();
    m_moveStartPoint = clipPointWithScreen(event.localPoint);
    m_window->onNonClientEvent(event);
    return true;
}

bool TitleBar::onPointerMove(const PointerEvent &event)
{
    if (m_capturedPointerId != event.pointerId)
        return false;

    const QPoint delta = (clipPointWithScreen(event.localPoint) - m_moveStartPoint).toPoint();

    m_window->window()->setPosition(m_moveStartWindowPosition + delta);
    m_window->onNonClientEvent(event);
    return true;
}

bool TitleBar::onPointerUp(const PointerEvent &event)
{
    if (m_capturedPointerId != event.pointerId)
        return false;

    m_element.call<void>("releasePointerCapture", event.pointerId);
    m_capturedPointerId = -1;
    m_window->onNonClientEvent(event);
    return true;
}

bool TitleBar::onDoubleClick()
{
    m_window->onToggleMaximized();
    return true;
}

QPointF TitleBar::clipPointWithScreen(const QPointF &pointInTitleBarCoords) const
{
    auto *screen = m_window->platformScreen();
    return screen->clipPoint(screen->mapFromLocal(
            dom::mapPoint(m_element, screen->element(), pointInTitleBarCoords)));
}

NonClientArea::NonClientArea(QWasmWindow *window, emscripten::val qtWindowElement)
    : m_qtWindowElement(qtWindowElement),
      m_resizer(std::make_unique<Resizer>(window, m_qtWindowElement)),
      m_titleBar(std::make_unique<TitleBar>(window, m_qtWindowElement))
{
    updateResizability();
}

NonClientArea::~NonClientArea() = default;

void NonClientArea::onClientAreaWidthChange(int width)
{
    m_titleBar->setWidth(width);
}

void NonClientArea::propagateSizeHints()
{
    updateResizability();
}

void NonClientArea::updateResizability()
{
    const auto resizeConstraints = m_resizer->getResizeConstraints();
    const bool nonResizable = resizeConstraints.minShrink.isNull()
            && resizeConstraints.maxGrow.isNull() && resizeConstraints.maxGrowTop == 0;
    dom::syncCSSClassWith(m_qtWindowElement, "no-resize", nonResizable);
}

QT_END_NAMESPACE
