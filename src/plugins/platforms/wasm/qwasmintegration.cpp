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

#include <emscripten/bind.h>

// this is where EGL headers are pulled in, make sure it is last
#include "qwasmscreen.h"

using namespace emscripten;
QT_BEGIN_NAMESPACE

void browserBeforeUnload()
{
    QWasmIntegration::QWasmBrowserExit();
}

EMSCRIPTEN_BINDINGS(my_module)
{
    function("browserBeforeUnload", &browserBeforeUnload);
}

static QWasmIntegration *globalHtml5Integration;
QWasmIntegration *QWasmIntegration::get() { return globalHtml5Integration; }

QWasmIntegration::QWasmIntegration()
    : m_fontDb(nullptr),
      m_compositor(new QWasmCompositor),
      m_screen(new QWasmScreen(m_compositor)),
      m_eventDispatcher(nullptr)
{

    globalHtml5Integration = this;

    updateQScreenAndCanvasRenderSize();
    QWindowSystemInterface::handleScreenAdded(m_screen);
    emscripten_set_resize_callback(0, (void *)this, 1, uiEvent_cb);

    m_eventTranslator = new QWasmEventTranslator;

    EM_ASM(// exit app if browser closes
           window.onbeforeunload = function () {
           Module.browserBeforeUnload();
           };
     );
}

QWasmIntegration::~QWasmIntegration()
{
    delete m_compositor;
    QWindowSystemInterface::handleScreenRemoved(m_screen);
    delete m_fontDb;
    delete m_eventTranslator;
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
    case ThreadedOpenGL: return true;
    case RasterGLSurface: return false; // to enable this you need to fix qopenglwidget and quickwidget for wasm
    case MultipleWindows: return true;
    case WindowManagement: return true;
    case OpenGLOnRasterSurface: return true;
    default: return QPlatformIntegration::hasCapability(cap);
    }
}

QPlatformWindow *QWasmIntegration::createPlatformWindow(QWindow *window) const
{
    return new QWasmWindow(window, m_compositor, m_backingStores.value(window));
}

QPlatformBackingStore *QWasmIntegration::createPlatformBackingStore(QWindow *window) const
{
#ifndef QT_NO_OPENGL
    QWasmBackingStore *backingStore = new QWasmBackingStore(m_compositor, window);
    m_backingStores.insert(window, backingStore);
    return backingStore;
#else
    return nullptr;
#endif
}

#ifndef QT_NO_OPENGL
QPlatformOpenGLContext *QWasmIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    return new QWasmOpenGLContext(context->format());
}
#endif

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
    return QPlatformIntegration::styleHint(hint);
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

int QWasmIntegration::uiEvent_cb(int eventType, const EmscriptenUiEvent *e, void *userData)
{
    Q_UNUSED(e)
    Q_UNUSED(userData)

    if (eventType == EMSCRIPTEN_EVENT_RESIZE) {
        // This resize event is called when the HTML window is resized. Depending
        // on the page layout the the canvas might also have been resized, so we
        // update the Qt screen size (and canvas render size).
        updateQScreenAndCanvasRenderSize();
    }

    return 0;
}

static void set_canvas_size(double width, double height)
{
    EM_ASM_({
        var canvas = Module.canvas;
        canvas.width = $0;
        canvas.height = $1;
    }, width, height);
}

void QWasmIntegration::updateQScreenAndCanvasRenderSize()
{
    // The HTML canvas has two sizes: the CSS size and the canvas render size.
    // The CSS size is determined according to standard CSS rules, while the
    // render size is set using the "width" and "height" attributes. The render
    // size must be set manually and is not auto-updated on CSS size change.
    // Setting the render size to a value larger than the CSS size enables high-dpi
    // rendering.

    double css_width;
    double css_height;
    emscripten_get_element_css_size(0, &css_width, &css_height);
    QSizeF cssSize(css_width, css_height);

    QWasmScreen *screen = QWasmIntegration::get()->m_screen;
    QSizeF canvasSize = cssSize * screen->devicePixelRatio();

    set_canvas_size(canvasSize.width(), canvasSize.height());
    screen->setGeometry(QRect(QPoint(0, 0), cssSize.toSize()));
    QWasmIntegration::get()->m_compositor->redrawWindowContent();
}

QT_END_NAMESPACE
