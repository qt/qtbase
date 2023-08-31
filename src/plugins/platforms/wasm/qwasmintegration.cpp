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
#include "qwasmplatform.h"
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

extern void qt_set_sequence_auto_mnemonic(bool);

using namespace emscripten;

using namespace Qt::StringLiterals;

static void setContainerElements(emscripten::val elementArray)
{
    QWasmIntegration::get()->setContainerElements(elementArray);
}

static void addContainerElement(emscripten::val element)
{
    QWasmIntegration::get()->addContainerElement(element);
}

static void removeContainerElement(emscripten::val element)
{
    QWasmIntegration::get()->removeContainerElement(element);
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
    function("qtSetContainerElements", &setContainerElements);
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

    if (platform() == Platform::MacOS)
        qt_set_sequence_auto_mnemonic(false);

    touchPoints = emscripten::val::global("navigator")["maxTouchPoints"].as<int>();

    // Create screens for container elements. Each container element will ultimately become a
    // div element. Qt historically supported supplying canvas for screen elements - these elements
    // will be transformed into divs and warnings about deprecation will be printed. See
    // QWasmScreen ctor.
    emscripten::val filtered = emscripten::val::array();
    emscripten::val qtContainerElements = val::module_property("qtContainerElements");
    if (qtContainerElements.isArray()) {
        for (int i = 0; i < qtContainerElements["length"].as<int>(); ++i) {
            emscripten::val element = qtContainerElements[i].as<emscripten::val>();
            if (element.isNull() || element.isUndefined())
                qWarning() << "Skipping null or undefined element in qtContainerElements";
            else
                filtered.call<void>("push", element);
        }
    } else {
        // No screens, which may or may not be intended
        qWarning() << "The qtContainerElements module property was not set or is invalid. "
                      "Proceeding with no screens.";
    }
    setContainerElements(filtered);

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
        elementAndScreen.wasmScreen->deleteScreen();

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
    switch (hint) {
    case ShowIsFullScreen:
        return true;
    case UnderlineShortcut:
        return platform() != Platform::MacOS;
    default:
        return QPlatformIntegration::styleHint(hint);
    }
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

void QWasmIntegration::setContainerElements(emscripten::val elementArray)
{
    const auto *primaryScreenBefore = m_screens.isEmpty() ? nullptr : m_screens[0].wasmScreen;
    QList<ScreenMapping> newScreens;

    QList<QWasmScreen *> screensToDelete;
    std::transform(m_screens.begin(), m_screens.end(), std::back_inserter(screensToDelete),
                   [](const ScreenMapping &mapping) { return mapping.wasmScreen; });

    for (int i = 0; i < elementArray["length"].as<int>(); ++i) {
        const auto element = elementArray[i];
        const auto it = std::find_if(
                m_screens.begin(), m_screens.end(),
                [&element](const ScreenMapping &screen) { return screen.emscriptenVal == element; });
        QWasmScreen *screen;
        if (it != m_screens.end()) {
            screen = it->wasmScreen;
            screensToDelete.erase(std::remove_if(screensToDelete.begin(), screensToDelete.end(),
                                                [screen](const QWasmScreen *removedScreen) {
                                                    return removedScreen == screen;
                                                }),
                                 screensToDelete.end());
        } else {
            screen = new QWasmScreen(element);
            QWindowSystemInterface::handleScreenAdded(screen);
        }
        newScreens.push_back({element, screen});
    }

    std::for_each(screensToDelete.begin(), screensToDelete.end(),
                  [](QWasmScreen *removed) { removed->deleteScreen(); });

    m_screens = newScreens;
    auto *primaryScreenAfter = m_screens.isEmpty() ? nullptr : m_screens[0].wasmScreen;
    if (primaryScreenAfter && primaryScreenAfter != primaryScreenBefore)
        QWindowSystemInterface::handlePrimaryScreenChanged(primaryScreenAfter);
}

void QWasmIntegration::addContainerElement(emscripten::val element)
{
    Q_ASSERT_X(m_screens.end()
                       == std::find_if(m_screens.begin(), m_screens.end(),
                                       [&element](const ScreenMapping &screen) {
                                           return screen.emscriptenVal == element;
                                       }),
               Q_FUNC_INFO, "Double-add of an element");

    QWasmScreen *screen = new QWasmScreen(element);
    QWindowSystemInterface::handleScreenAdded(screen);
    m_screens.push_back({element, screen});
}

void QWasmIntegration::removeContainerElement(emscripten::val element)
{
    const auto *primaryScreenBefore = m_screens.isEmpty() ? nullptr : m_screens[0].wasmScreen;

    const auto it =
            std::find_if(m_screens.begin(), m_screens.end(),
                         [&element](const ScreenMapping &screen) { return screen.emscriptenVal == element; });
    if (it == m_screens.end()) {
        qWarning() << "Attempt to remove a nonexistent screen.";
        return;
    }

    QWasmScreen *removedScreen = it->wasmScreen;
    removedScreen->deleteScreen();

    m_screens.erase(std::remove_if(m_screens.begin(), m_screens.end(),
                                   [removedScreen](const ScreenMapping &mapping) {
                                       return removedScreen == mapping.wasmScreen;
                                   }),
                    m_screens.end());
    auto *primaryScreenAfter = m_screens.isEmpty() ? nullptr : m_screens[0].wasmScreen;
    if (primaryScreenAfter && primaryScreenAfter != primaryScreenBefore)
        QWindowSystemInterface::handlePrimaryScreenChanged(primaryScreenAfter);
}

void QWasmIntegration::resizeScreen(const emscripten::val &element)
{
    auto it = std::find_if(m_screens.begin(), m_screens.end(),
        [&] (const ScreenMapping &candidate) { return candidate.emscriptenVal.equals(element); });
    if (it == m_screens.end()) {
        qWarning() << "Attempting to resize non-existing screen for element"
                   << QString::fromEcmaString(element["id"]);
        return;
    }
    it->wasmScreen->updateQScreenAndCanvasRenderSize();
}

void QWasmIntegration::updateDpi()
{
    emscripten::val dpi = emscripten::val::module_property("qtFontDpi");
    if (dpi.isUndefined())
        return;
    qreal dpiValue = dpi.as<qreal>();
    for (const auto &elementAndScreen : m_screens)
        QWindowSystemInterface::handleScreenLogicalDotsPerInchChange(elementAndScreen.wasmScreen->screen(), dpiValue, dpiValue);
}

void QWasmIntegration::resizeAllScreens()
{
    for (const auto &elementAndScreen : m_screens)
        elementAndScreen.wasmScreen->updateQScreenAndCanvasRenderSize();
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
