/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwasmintegration.h"
#include "qwasmeventtranslator.h"
#include "qwasmeventdispatcher.h"
#include "qwasmcompositor.h"
#include "qwasmopenglcontext.h"
#include "qwasmtheme.h"
#include "qwasmclipboard.h"
#include "qwasmservices.h"
#include "qwasmoffscreensurface.h"
#include "qwasmstring.h"

#include "qwasmwindow.h"
#ifndef QT_NO_OPENGL
# include "qwasmbackingstore.h"
#endif
#include "qwasmfontdatabase.h"
#if defined(Q_OS_UNIX)
#include <QtEventDispatcherSupport/private/qgenericunixeventdispatcher_p.h>
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

using namespace emscripten;
QT_BEGIN_NAMESPACE

static void browserBeforeUnload(emscripten::val)
{
    QWasmIntegration::QWasmBrowserExit();
}

static void addCanvasElement(emscripten::val canvas)
{
    QString canvasId = QWasmString::toQString(canvas["id"]);
    QWasmIntegration::get()->addScreen(canvasId);
}

static void removeCanvasElement(emscripten::val canvas)
{
    QString canvasId = QWasmString::toQString(canvas["id"]);
    QWasmIntegration::get()->removeScreen(canvasId);
}

static void resizeCanvasElement(emscripten::val canvas)
{
    QString canvasId = QWasmString::toQString(canvas["id"]);
    QWasmIntegration::get()->resizeScreen(canvasId);
}

static void qtUpdateDpi()
{
    QWasmIntegration::get()->updateDpi();
}

EMSCRIPTEN_BINDINGS(qtQWasmIntegraton)
{
    function("qtBrowserBeforeUnload", &browserBeforeUnload);
    function("qtAddCanvasElement", &addCanvasElement);
    function("qtRemoveCanvasElement", &removeCanvasElement);
    function("qtResizeCanvasElement", &resizeCanvasElement);
    function("qtUpdateDpi", &qtUpdateDpi);
}

QWasmIntegration *QWasmIntegration::s_instance;

QWasmIntegration::QWasmIntegration()
    : m_fontDb(nullptr),
      m_desktopServices(nullptr),
      m_clipboard(new QWasmClipboard)
{
    s_instance = this;

    // We expect that qtloader.js has populated Module.qtCanvasElements with one or more canvases.
    // Also check Module.canvas, which may be set if the emscripen or a custom loader is used.
    emscripten::val qtCanvaseElements = val::module_property("qtCanvasElements");
    emscripten::val canvas = val::module_property("canvas");

    if (!qtCanvaseElements.isUndefined()) {
        int screenCount = qtCanvaseElements["length"].as<int>();
        for (int i = 0; i < screenCount; ++i) {
            emscripten::val canvas = qtCanvaseElements[i].as<emscripten::val>();
            QString canvasId = QWasmString::toQString(canvas["id"]);
            addScreen(canvasId);
        }
    } else if (!canvas.isUndefined()){
        QString canvasId = QWasmString::toQString(canvas["id"]);
        addScreen(canvasId);
    }

    emscripten::val::global("window").set("onbeforeunload", val::module_property("qtBrowserBeforeUnload"));

    // install browser window resize handler
    auto onWindowResize = [](int eventType, const EmscriptenUiEvent *e, void *userData) -> int {
        Q_UNUSED(eventType);
        Q_UNUSED(e);
        Q_UNUSED(userData);

        // This resize event is called when the HTML window is resized. Depending
        // on the page layout the canvas(es) might also have been resized, so we
        // update the Qt screen sizes (and canvas render sizes).
        if (QWasmIntegration *integration = QWasmIntegration::get())
            integration->resizeAllScreens();
        return 0;
    };
    emscripten_set_resize_callback(nullptr, nullptr, 1, onWindowResize);
}

QWasmIntegration::~QWasmIntegration()
{
    delete m_fontDb;
    delete m_desktopServices;

    for (auto it = m_screens.constBegin(); it != m_screens.constEnd(); ++it)
        QWindowSystemInterface::handleScreenRemoved(*it);
    m_screens.clear();

    s_instance = nullptr;
}

void QWasmIntegration::QWasmBrowserExit()
{
    QCoreApplication *app = QCoreApplication::instance();
    app->quit();
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
    QWasmCompositor *compositor = QWasmScreen::get(window->screen())->compositor();
    return new QWasmWindow(window, compositor, m_backingStores.value(window));
}

QPlatformBackingStore *QWasmIntegration::createPlatformBackingStore(QWindow *window) const
{
#ifndef QT_NO_OPENGL
    QWasmCompositor *compositor = QWasmScreen::get(window->screen())->compositor();
    QWasmBackingStore *backingStore = new QWasmBackingStore(compositor, window);
    m_backingStores.insert(window, backingStore);
    return backingStore;
#else
    return nullptr;
#endif
}

void QWasmIntegration::removeBackingStore(QWindow* window)
{
    m_backingStores.remove(window);
}

#ifndef QT_NO_OPENGL
QPlatformOpenGLContext *QWasmIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    return new QWasmOpenGLContext(context->format());
}
#endif

void QWasmIntegration::initialize()
{
    QString icStr = QPlatformInputContextFactory::requested();
    if (!icStr.isNull())
        m_inputContext.reset(QPlatformInputContextFactory::create(icStr));
}

QPlatformInputContext *QWasmIntegration::inputContext() const
{
    return m_inputContext.data();
}

QPlatformOffscreenSurface *QWasmIntegration::createPlatformOffscreenSurface(QOffscreenSurface *surface) const
{
    return new QWasmOffscrenSurface(surface);
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
    // Don't maximize dialogs
    if (flags & Qt::Dialog & ~Qt::Window)
        return Qt::WindowNoState;

    return QPlatformIntegration::defaultWindowState(flags);
}

QStringList QWasmIntegration::themeNames() const
{
    return QStringList() << QLatin1String("webassembly");
}

QPlatformTheme *QWasmIntegration::createPlatformTheme(const QString &name) const
{
    if (name == QLatin1String("webassembly"))
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

void QWasmIntegration::addScreen(const QString &canvasId)
{
    QWasmScreen *screen = new QWasmScreen(canvasId);
    m_clipboard->installEventHandlers(canvasId);
    m_screens.insert(canvasId, screen);
    QWindowSystemInterface::handleScreenAdded(screen);
}

void QWasmIntegration::removeScreen(const QString &canvasId)
{
    QWasmScreen *exScreen = m_screens.take(canvasId);
    exScreen->destroy(); // clean up before deleting the screen
    QWindowSystemInterface::handleScreenRemoved(exScreen);
}

void QWasmIntegration::resizeScreen(const QString &canvasId)
{
    m_screens.value(canvasId)->updateQScreenAndCanvasRenderSize();
}

void QWasmIntegration::updateDpi()
{
    emscripten::val dpi = emscripten::val::module_property("qtFontDpi");
    if (dpi.isUndefined())
        return;
    qreal dpiValue = dpi.as<qreal>();
    for (QWasmScreen *screen : m_screens)
        QWindowSystemInterface::handleScreenLogicalDotsPerInchChange(screen->screen(), dpiValue, dpiValue);
}

void QWasmIntegration::resizeAllScreens()
{
    qDebug() << "resizeAllScreens";
    for (QWasmScreen *screen : m_screens)
        screen->updateQScreenAndCanvasRenderSize();
}

QT_END_NAMESPACE
