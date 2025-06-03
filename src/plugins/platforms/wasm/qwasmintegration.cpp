// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmintegration.h"
#include "qwasmeventdispatcher.h"
#include "qwasmcompositor.h"
#include "qwasmopenglcontext.h"
#include "qwasmtheme.h"
#include "qwasmclipboard.h"
#include "qwasmaccessibility.h"
#include "qwasmservices.h"
#include "qwasmoffscreensurface.h"
#include "qwasmstring.h"

#include "qwasmwindow.h"
#include "qwasmbackingstore.h"
#include "qwasmfontdatabase.h"
#if defined(Q_OS_UNIX)
#include <QtGui/private/qgenericunixeventdispatcher_p.h>
#endif
#include <qpa/qplatformwindow.h>
#include <QtGui/qscreen.h>
#include <qpa/qwindowsysteminterface.h>
#include <QtCore/qcoreapplication.h>
#include <qpa/qplatforminputcontextfactory_p.h>

#include <emscripten/bind.h>
#include <emscripten/val.h>

// this is where EGL headers are pulled in, make sure it is last
#include "qwasmscreen.h"
#include <private/qsimpledrag_p.h>

QT_BEGIN_NAMESPACE

using namespace emscripten;

using namespace Qt::StringLiterals;

static void addContainerElement(emscripten::val element)
{
    QWasmIntegration::get()->addScreen(element);
}

static void removeContainerElement(emscripten::val element)
{
    QWasmIntegration::get()->removeScreen(element);
}

static void resizeContainerElement(emscripten::val element)
{
    QWasmIntegration::get()->resizeScreen(element);
}

static void qtUpdateDpi()
{
    QWasmIntegration::get()->updateDpi();
}

static void resizeAllScreens(emscripten::val event)
{
    Q_UNUSED(event);
    QWasmIntegration::get()->resizeAllScreens();
}

EMSCRIPTEN_BINDINGS(qtQWasmIntegraton)
{
    function("qtAddContainerElement", &addContainerElement);
    function("qtRemoveContainerElement", &removeContainerElement);
    function("qtResizeContainerElement", &resizeContainerElement);
    function("qtUpdateDpi", &qtUpdateDpi);
    function("qtResizeAllScreens", &resizeAllScreens);
}

QWasmIntegration *QWasmIntegration::s_instance;

QWasmIntegration::QWasmIntegration()
    : m_fontDb(nullptr)
    , m_desktopServices(nullptr)
    , m_clipboard(new QWasmClipboard)
#if QT_CONFIG(accessibility)
    , m_accessibility(new QWasmAccessibility)
#endif
{
    s_instance = this;

    touchPoints = emscripten::val::global("navigator")["maxTouchPoints"].as<int>();

    // Create screens for container elements. Each container element will ultimately become a
    // div element. Qt historically supported supplying canvas for screen elements - these elements
    // will be transformed into divs and warnings about deprecation will be printed. See
    // QWasmScreen ctor.
    emscripten::val qtContainerElements = val::module_property("qtContainerElements");
    if (qtContainerElements.isArray()) {
        for (int i = 0; i < qtContainerElements["length"].as<int>(); ++i) {
            emscripten::val element = qtContainerElements[i].as<emscripten::val>();
            if (element.isNull() || element.isUndefined())
                qWarning() << "Skipping null or undefined element in qtContainerElements";
            else
                addScreen(element);
        }
    } else {
        // No screens, which may or may not be intended
        qWarning() << "The qtContainerElements module property was not set or is invalid. "
                      "Proceeding with no screens.";
    }

    // install browser window resize handler
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_TRUE,
                                   [](int, const EmscriptenUiEvent *, void *) -> int {
                                       // This resize event is called when the HTML window is
                                       // resized. Depending on the page layout the elements might
                                       // also have been resized, so we update the Qt screen sizes
                                       // (and canvas render sizes).
                                       if (QWasmIntegration *integration = QWasmIntegration::get())
                                           integration->resizeAllScreens();
                                       return 0;
                                   });

    // install visualViewport resize handler which picks up size and scale change on mobile.
    emscripten::val visualViewport = emscripten::val::global("window")["visualViewport"];
    if (!visualViewport.isUndefined()) {
        visualViewport.call<void>("addEventListener", val("resize"),
                                  val::module_property("qtResizeAllScreens"));
    }
    m_drag = std::make_unique<QSimpleDrag>();
}

QWasmIntegration::~QWasmIntegration()
{
    // Remove event listener
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_TRUE, nullptr);
    emscripten::val visualViewport = emscripten::val::global("window")["visualViewport"];
    if (!visualViewport.isUndefined()) {
        visualViewport.call<void>("removeEventListener", val("resize"),
                          val::module_property("qtResizeAllScreens"));
    }

    delete m_fontDb;
    delete m_desktopServices;
    if (m_platformInputContext)
        delete m_platformInputContext;
#if QT_CONFIG(accessibility)
    delete m_accessibility;
#endif

    for (const auto &elementAndScreen : m_screens)
        elementAndScreen.second->deleteScreen();

    m_screens.clear();

    s_instance = nullptr;
}

bool QWasmIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps: return true;
    case OpenGL: return true;
    case ThreadedOpenGL: return false;
    case RasterGLSurface: return false; // to enable this you need to fix qopenglwidget and quickwidget for wasm
    case MultipleWindows: return true;
    case WindowManagement: return true;
    case OpenGLOnRasterSurface: return true;
    default: return QPlatformIntegration::hasCapability(cap);
    }
}

QPlatformWindow *QWasmIntegration::createPlatformWindow(QWindow *window) const
{
    auto *wasmScreen = QWasmScreen::get(window->screen());
    QWasmCompositor *compositor = wasmScreen->compositor();
    return new QWasmWindow(window, wasmScreen->deadKeySupport(), compositor,
                           m_backingStores.value(window));
}

QPlatformBackingStore *QWasmIntegration::createPlatformBackingStore(QWindow *window) const
{
    QWasmCompositor *compositor = QWasmScreen::get(window->screen())->compositor();
    QWasmBackingStore *backingStore = new QWasmBackingStore(compositor, window);
    m_backingStores.insert(window, backingStore);
    return backingStore;
}

void QWasmIntegration::removeBackingStore(QWindow* window)
{
    m_backingStores.remove(window);
}

#ifndef QT_NO_OPENGL
QPlatformOpenGLContext *QWasmIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    return new QWasmOpenGLContext(context);
}
#endif

void QWasmIntegration::initialize()
{
    if (qgetenv("QT_IM_MODULE").isEmpty() && touchPoints < 1)
        return;

    QString icStr = QPlatformInputContextFactory::requested();
    if (!icStr.isNull())
        m_inputContext.reset(QPlatformInputContextFactory::create(icStr));
    else
        m_inputContext.reset(new QWasmInputContext());
}

QPlatformInputContext *QWasmIntegration::inputContext() const
{
    return m_inputContext.data();
}

QPlatformOffscreenSurface *QWasmIntegration::createPlatformOffscreenSurface(QOffscreenSurface *surface) const
{
    return new QWasmOffscreenSurface(surface);
}

QPlatformFontDatabase *QWasmIntegration::fontDatabase() const
{
    if (m_fontDb == nullptr)
        m_fontDb = new QWasmFontDatabase;

    return m_fontDb;
}

QAbstractEventDispatcher *QWasmIntegration::createEventDispatcher() const
{
    return new QWasmEventDispatcher;
}

QVariant QWasmIntegration::styleHint(QPlatformIntegration::StyleHint hint) const
{
    if (hint == ShowIsFullScreen)
        return true;

    return QPlatformIntegration::styleHint(hint);
}

Qt::WindowState QWasmIntegration::defaultWindowState(Qt::WindowFlags flags) const
{
    // Don't maximize dialogs or popups
    if (flags.testFlag(Qt::Dialog) || flags.testFlag(Qt::Popup))
        return Qt::WindowNoState;

    return QPlatformIntegration::defaultWindowState(flags);
}

QStringList QWasmIntegration::themeNames() const
{
    return QStringList() << "webassembly"_L1;
}

QPlatformTheme *QWasmIntegration::createPlatformTheme(const QString &name) const
{
    if (name == "webassembly"_L1)
        return new QWasmTheme;
    return QPlatformIntegration::createPlatformTheme(name);
}

QPlatformServices *QWasmIntegration::services() const
{
    if (m_desktopServices == nullptr)
        m_desktopServices = new QWasmServices();
    return m_desktopServices;
}

QPlatformClipboard* QWasmIntegration::clipboard() const
{
    return m_clipboard;
}

#ifndef QT_NO_ACCESSIBILITY
QPlatformAccessibility *QWasmIntegration::accessibility() const
{
    return m_accessibility;
}
#endif


void QWasmIntegration::addScreen(const emscripten::val &element)
{
    QWasmScreen *screen = new QWasmScreen(element);
    m_screens.append(qMakePair(element, screen));
    QWindowSystemInterface::handleScreenAdded(screen);
}

void QWasmIntegration::removeScreen(const emscripten::val &element)
{
    auto it = std::find_if(m_screens.begin(), m_screens.end(),
        [&] (const QPair<emscripten::val, QWasmScreen *> &candidate) { return candidate.first.equals(element); });
    if (it == m_screens.end()) {
        qWarning() << "Attempting to remove non-existing screen for element" << QWasmString::toQString(element["id"]);;
        return;
    }
    it->second->deleteScreen();
    m_screens.erase(it);
}

void QWasmIntegration::resizeScreen(const emscripten::val &element)
{
    auto it = std::find_if(m_screens.begin(), m_screens.end(),
        [&] (const QPair<emscripten::val, QWasmScreen *> &candidate) { return candidate.first.equals(element); });
    if (it == m_screens.end()) {
        qWarning() << "Attempting to resize non-existing screen for element" << QWasmString::toQString(element["id"]);;
        return;
    }
    it->second->updateQScreenAndCanvasRenderSize();
}

void QWasmIntegration::updateDpi()
{
    emscripten::val dpi = emscripten::val::module_property("qtFontDpi");
    if (dpi.isUndefined())
        return;
    qreal dpiValue = dpi.as<qreal>();
    for (const auto &elementAndScreen : m_screens)
        QWindowSystemInterface::handleScreenLogicalDotsPerInchChange(elementAndScreen.second->screen(), dpiValue, dpiValue);
}

void QWasmIntegration::resizeAllScreens()
{
    for (const auto &elementAndScreen : m_screens)
        elementAndScreen.second->updateQScreenAndCanvasRenderSize();
}

quint64 QWasmIntegration::getTimestamp()
{
    return emscripten_performance_now();
}

#if QT_CONFIG(draganddrop)
QPlatformDrag *QWasmIntegration::drag() const
{
    return m_drag.get();
}
#endif // QT_CONFIG(draganddrop)

QT_END_NAMESPACE
