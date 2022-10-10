// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmscreen.h"
#include "qwasmwindow.h"
#include "qwasmeventtranslator.h"
#include "qwasmcompositor.h"
#include "qwasmintegration.h"
#include "qwasmstring.h"

#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <QtGui/private/qeglconvenience_p.h>
#ifndef QT_NO_OPENGL
# include <QtGui/private/qeglplatformcontext_p.h>
#endif
#include <qpa/qwindowsysteminterface.h>
#include <QtCore/qcoreapplication.h>
#include <QtGui/qguiapplication.h>
#include <private/qhighdpiscaling_p.h>

#include <tuple>

QT_BEGIN_NAMESPACE

using namespace emscripten;

const char * QWasmScreen::m_canvasResizeObserverCallbackContextPropertyName = "data-qtCanvasResizeObserverCallbackContext";

QWasmScreen::QWasmScreen(const emscripten::val &containerOrCanvas)
    : m_container(containerOrCanvas)
    , m_canvas(emscripten::val::undefined())
    , m_compositor(new QWasmCompositor(this))
    , m_eventTranslator(new QWasmEventTranslator())
{
    // Each screen is backed by a html canvas element. Use either
    // a user-supplied canvas or create one as a child of the user-
    // supplied root element.
    std::string tagName = containerOrCanvas["tagName"].as<std::string>();
    if (tagName == "CANVAS" || tagName == "canvas") {
        m_canvas = containerOrCanvas;
    } else {
        // Create the canvas (for the correct document) as a child of the container
        m_canvas = containerOrCanvas["ownerDocument"].call<emscripten::val>("createElement", std::string("canvas"));
        containerOrCanvas.call<void>("appendChild", m_canvas);
        std::string screenId = std::string("qtcanvas_") + std::to_string(uint32_t(this));
        m_canvas.set("id", screenId);

        // Make the canvas occupy 100% of parent
        emscripten::val style = m_canvas["style"];
        style.set("width", std::string("100%"));
        style.set("height", std::string("100%"));
    }

    // Configure canvas
    emscripten::val style = m_canvas["style"];
    style.set("border", std::string("0px none"));
    style.set("background-color", std::string("white"));

    // Set contenteditable so that the canvas gets clipboard events,
    // then hide the resulting focus frame, and reset the cursor.
    m_canvas.set("contentEditable", std::string("true"));
    // set inputmode to none to stop mobile keyboard opening
    // when user clicks  anywhere on the canvas.
    m_canvas.set("inputmode", std::string("none"));
    style.set("outline", std::string("0px solid transparent"));
    style.set("caret-color", std::string("transparent"));
    style.set("cursor", std::string("default"));

    // Disable the default context menu; Qt applications typically
    // provide custom right-click behavior.
    m_onContextMenu = std::make_unique<qstdweb::EventCallback>(m_canvas, "contextmenu", [](emscripten::val event){
        event.call<void>("preventDefault");
    });

    // Create "specialHTMLTargets" mapping for the canvas. Normally, Emscripten
    // uses the html element id when targeting elements, for example when registering
    // event callbacks. However, this approach is limited to supporting single-document
    // apps/ages only, since Emscripten uses the main document to look up the element.
    // As a workaround for this, Emscripten supports registering custom mappings in the
    // "specialHTMLTargets" object. Add a mapping for the canvas for this screen.
    //
    // This functionality is gated on "specialHTMLTargets" being available as a module
    // property. One way to ensure this is the case is to add it to EXPORTED_RUNTIME_METHODS.
    // Qt does not currently do this by default since if added it _must_ be used in order
    // to avoid an undefined reference error at startup, and there are cases when Qt won't use
    // it, for example if QGuiApplication is not usded.
    if (hasSpecialHtmlTargets())
         emscripten::val::module_property("specialHTMLTargets").set(canvasSpecialHtmlTargetId(), m_canvas);

    // Install event handlers on the container/canvas. This must be
    // done after the canvas has been created above.
    m_compositor->initEventHandlers();

    updateQScreenAndCanvasRenderSize();
    m_canvas.call<void>("focus");
}

QWasmScreen::~QWasmScreen()
{
    // Delete the compositor before removing the screen from specialHTMLTargets,
    // since its destructor needs to look up the target when deregistering
    // event handlers.
    m_compositor = nullptr;

    if (hasSpecialHtmlTargets())
        emscripten::val::module_property("specialHTMLTargets")
            .set(canvasSpecialHtmlTargetId(), emscripten::val::undefined());

    m_canvas.set(m_canvasResizeObserverCallbackContextPropertyName, emscripten::val(intptr_t(0)));
}

void QWasmScreen::deleteScreen()
{
    m_compositor->destroy();
    QWindowSystemInterface::handleScreenRemoved(this);
}

QWasmScreen *QWasmScreen::get(QPlatformScreen *screen)
{
    return static_cast<QWasmScreen *>(screen);
}

QWasmScreen *QWasmScreen::get(QScreen *screen)
{
    return get(screen->handle());
}

QWasmCompositor *QWasmScreen::compositor()
{
    return m_compositor.get();
}

QWasmEventTranslator *QWasmScreen::eventTranslator()
{
    return m_eventTranslator.get();
}

emscripten::val QWasmScreen::container() const
{
    return m_container;
}

emscripten::val QWasmScreen::canvas() const
{
    return m_canvas;
}

// Returns the html element id for the screen's canvas.
QString QWasmScreen::canvasId() const
{
    return QWasmString::toQString(m_canvas["id"]);
}

// Returns the canvas _target_ id, for use with Emscripten's event registration
// functions. This either based on the id registered in specialHtmlTargets, or
// on the canvas id.
QString QWasmScreen::canvasTargetId() const
{
    if (hasSpecialHtmlTargets())
        return QString::fromStdString(canvasSpecialHtmlTargetId());
    else
        return QStringLiteral("#") + canvasId();
}

std::string QWasmScreen::canvasSpecialHtmlTargetId() const
{
    // Return a globally unique id for the canvas. We can choose any string,
    // as long as it starts with a "!".
    return std::string("!qtcanvas_") + std::to_string(uint32_t(this));
}

namespace {

// Compare Emscripten versions, returns > 0 if a is greater than b.

int compareVersionComponents(int a, int b)
{
    return a >= 0 && b >= 0 ? a - b : 0;
}

int compareEmscriptenVersions(std::tuple<int, int, int> a, std::tuple<int, int, int> b)
{
    if (std::get<0>(a) == std::get<0>(b)) {
        if (std::get<1>(a) == std::get<1>(b)) {
            return compareVersionComponents(std::get<2>(a), std::get<2>(b));
        }
        return compareVersionComponents(std::get<1>(a), std::get<1>(b));
    }
    return compareVersionComponents(std::get<0>(a), std::get<0>(b));
}

bool isEmsdkVersionGreaterThan(std::tuple<int, int, int> test)
{
    return compareEmscriptenVersions(
        std::make_tuple(__EMSCRIPTEN_major__, __EMSCRIPTEN_minor__, __EMSCRIPTEN_tiny__), test) > 0;
}

} // namespace

bool QWasmScreen::hasSpecialHtmlTargets() const
{
    static bool gotIt = []{
        // Enable use of specialHTMLTargets, if available

        // On Emscripten > 3.1.14 (exact version not known), emscripten::val::module_property()
        // aborts instead of returning undefined when attempting to resolve the specialHTMLTargets
        // property, in the case where it is not defined. Disable the availability test in this case.
        // FIXME: Add alternative way to enable.
        if (isEmsdkVersionGreaterThan(std::make_tuple(3, 1, 14)))
            return false;

        emscripten::val htmlTargets = emscripten::val::module_property("specialHTMLTargets");
        if (htmlTargets.isUndefined())
            return false;

        // Check that the object has the expected type - it can also be
        // defined as an abort() function which prints an error on usage.
        return htmlTargets["constructor"]["name"].as<std::string>() == std::string("Array");
    }();
    return gotIt;
}

QRect QWasmScreen::geometry() const
{
    return m_geometry;
}

int QWasmScreen::depth() const
{
    return m_depth;
}

QImage::Format QWasmScreen::format() const
{
    return m_format;
}

QDpi QWasmScreen::logicalDpi() const
{
    emscripten::val dpi = emscripten::val::module_property("qtFontDpi");
    if (!dpi.isUndefined()) {
        qreal dpiValue = dpi.as<qreal>();
        return QDpi(dpiValue, dpiValue);
    }
    const qreal defaultDpi = 96;
    return QDpi(defaultDpi, defaultDpi);
}

qreal QWasmScreen::devicePixelRatio() const
{
    // window.devicePixelRatio gives us the scale factor between CSS and device pixels.
    // This property reflects hardware configuration, and also browser zoom on desktop.
    //
    // window.visualViewport.scale gives us the zoom factor on mobile. If the html page is
    // configured with "<meta name="viewport" content="width=device-width">" then this scale
    // factor will be 1. Omitting the viewport configuration typically results on a zoomed-out
    // viewport, with a scale factor <1. User pinch-zoom will change the scale factor; an event
    // handler is installed in the QWasmIntegration constructor. Changing zoom level on desktop
    // does not appear to change visualViewport.scale.
    //
    // The effective devicePixelRatio is the product of these two scale factors, upper-bounded
    // by window.devicePixelRatio in order to avoid e.g. allocating a 10x widget backing store.
    double dpr = emscripten::val::global("window")["devicePixelRatio"].as<double>();
    emscripten::val visualViewport = emscripten::val::global("window")["visualViewport"];
    double scale = visualViewport.isUndefined() ? 1.0 : visualViewport["scale"].as<double>();
    double effectiveDevicePixelRatio = std::min(dpr * scale, dpr);
    return qreal(effectiveDevicePixelRatio);
}

QString QWasmScreen::name() const
{
    return canvasId();
}

QPlatformCursor *QWasmScreen::cursor() const
{
    return const_cast<QWasmCursor *>(&m_cursor);
}

void QWasmScreen::resizeMaximizedWindows()
{
    if (!screen())
        return;
    QPlatformScreen::resizeMaximizedWindows();
}

QWindow *QWasmScreen::topWindow() const
{
    return m_compositor->keyWindow();
}

QWindow *QWasmScreen::topLevelAt(const QPoint &p) const
{
    return m_compositor->windowAt(p);
}

QPoint QWasmScreen::translateAndClipGlobalPoint(const QPoint &p) const
{
    return QPoint(
            std::max(screen()->geometry().left(),
                     std::min(screen()->geometry().right(), screen()->geometry().left() + p.x())),
            std::max(screen()->geometry().top(),
                     std::min(screen()->geometry().bottom(), screen()->geometry().top() + p.y())));
}

void QWasmScreen::invalidateSize()
{
    m_geometry = QRect();
}

void QWasmScreen::setGeometry(const QRect &rect)
{
    m_geometry = rect;
    QWindowSystemInterface::handleScreenGeometryChange(QPlatformScreen::screen(), geometry(), availableGeometry());
    resizeMaximizedWindows();
}

void QWasmScreen::updateQScreenAndCanvasRenderSize()
{
    // The HTML canvas has two sizes: the CSS size and the canvas render size.
    // The CSS size is determined according to standard CSS rules, while the
    // render size is set using the "width" and "height" attributes. The render
    // size must be set manually and is not auto-updated on CSS size change.
    // Setting the render size to a value larger than the CSS size enables high-dpi
    // rendering.

    QByteArray canvasSelector = canvasTargetId().toUtf8();
    double css_width;
    double css_height;
    emscripten_get_element_css_size(canvasSelector.constData(), &css_width, &css_height);
    QSizeF cssSize(css_width, css_height);

    QSizeF canvasSize = cssSize * devicePixelRatio();

    m_canvas.set("width", canvasSize.width());
    m_canvas.set("height", canvasSize.height());

    // Returns the html elements document/body position
    auto getElementBodyPosition = [](const emscripten::val &element) -> QPoint {
        emscripten::val bodyRect = element["ownerDocument"]["body"].call<emscripten::val>("getBoundingClientRect");
        emscripten::val canvasRect = element.call<emscripten::val>("getBoundingClientRect");
        return QPoint (canvasRect["left"].as<int>() - bodyRect["left"].as<int>(),
                       canvasRect["top"].as<int>() - bodyRect["top"].as<int>());
    };

    setGeometry(QRect(getElementBodyPosition(m_canvas), cssSize.toSize()));
    m_compositor->requestUpdateAllWindows();
}

void QWasmScreen::canvasResizeObserverCallback(emscripten::val entries, emscripten::val)
{
    int count = entries["length"].as<int>();
    if (count == 0)
        return;
    emscripten::val entry = entries[0];
    QWasmScreen *screen =
        reinterpret_cast<QWasmScreen *>(entry["target"][m_canvasResizeObserverCallbackContextPropertyName].as<intptr_t>());
    if (!screen) {
        qWarning() << "QWasmScreen::canvasResizeObserverCallback: missing screen pointer";
        return;
    }

    // We could access contentBoxSize|contentRect|devicePixelContentBoxSize on the entry here, but
    // these are not universally supported across all browsers. Get the sizes from the canvas instead.
    screen->updateQScreenAndCanvasRenderSize();
}

EMSCRIPTEN_BINDINGS(qtCanvasResizeObserverCallback) {
    emscripten::function("qtCanvasResizeObserverCallback", &QWasmScreen::canvasResizeObserverCallback);
}

void QWasmScreen::installCanvasResizeObserver()
{
    emscripten::val ResizeObserver = emscripten::val::global("ResizeObserver");
    if (ResizeObserver == emscripten::val::undefined())
        return; // ResizeObserver API is not available
    emscripten::val resizeObserver = ResizeObserver.new_(emscripten::val::module_property("qtCanvasResizeObserverCallback"));
    if (resizeObserver == emscripten::val::undefined())
        return; // Something went horribly wrong

    // We need to get back to this instance from the (static) resize callback;
    // set a "data-" property on the canvas element.
    m_canvas.set(m_canvasResizeObserverCallbackContextPropertyName, emscripten::val(intptr_t(this)));

    resizeObserver.call<void>("observe", m_canvas);
}

QT_END_NAMESPACE
