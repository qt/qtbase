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

#include "qwasmcompositor.h"
#include "qwasmstylepixmaps_p.h"
#include "qwasmwindow.h"
#include "qwasmeventtranslator.h"
#include "qwasmeventdispatcher.h"
#include "qwasmclipboard.h"

#include <QtOpenGL/qopengltexture.h>

#include <QtGui/private/qwindow_p.h>
#include <QtGui/qopenglcontext.h>
#include <QtGui/qopenglfunctions.h>
#include <QtGui/qoffscreensurface.h>
#include <QtGui/qpainter.h>
#include <private/qpixmapcache_p.h>

#include <private/qguiapplication_p.h>

#include <qpa/qwindowsysteminterface.h>
#include <QtCore/qcoreapplication.h>
#include <QtGui/qguiapplication.h>

#include <emscripten/bind.h>

using namespace emscripten;

Q_GUI_EXPORT int qt_defaultDpiX();

QWasmCompositedWindow::QWasmCompositedWindow()
    : window(nullptr)
    , parentWindow(nullptr)
    , flushPending(false)
    , visible(false)
{
}

// macOS CTRL <-> META switching. We most likely want to enable
// the existing switching code in QtGui, but for now do it here.


bool g_useNaturalScrolling = true; // natural scrolling is default on linux/windows

static void mouseWheelEvent(emscripten::val event) {

    emscripten::val wheelInterted = event["webkitDirectionInvertedFromDevice"];

    if (wheelInterted.as<bool>()) {
        g_useNaturalScrolling = true;
    }
}

EMSCRIPTEN_BINDINGS(qtMouseModule) {
        function("qtMouseWheelEvent", &mouseWheelEvent);
}

QWasmCompositor::QWasmCompositor(QWasmScreen *screen)
    :QObject(screen)
    , m_blitter(new QOpenGLTextureBlitter)
    , m_needComposit(false)
    , m_inFlush(false)
    , m_inResize(false)
    , m_isEnabled(true)
    , m_targetDevicePixelRatio(1)
    , draggedWindow(nullptr)
    , lastWindow(nullptr)
    , pressedButtons(Qt::NoButton)
    , resizeMode(QWasmCompositor::ResizeNone)
    , eventTranslator(new QWasmEventTranslator())
    , mouseInCanvas(false)
{
    touchDevice = new QPointingDevice(
            "touchscreen", 1, QInputDevice::DeviceType::TouchScreen,
            QPointingDevice::PointerType::Finger,
            QPointingDevice::Capability::Position | QPointingDevice::Capability::Area | QPointingDevice::Capability::NormalizedPosition,
            10, 0);
    QWindowSystemInterface::registerInputDevice(touchDevice);

    initEventHandlers();
}

QWasmCompositor::~QWasmCompositor()
{
    windowUnderMouse.clear();

    if (m_requestAnimationFrameId != -1)
        emscripten_cancel_animation_frame(m_requestAnimationFrameId);

    deregisterEventHandlers();
    destroy();
}

void QWasmCompositor::deregisterEventHandlers()
{
    QByteArray canvasSelector = "#" + screen()->canvasId().toUtf8();
    emscripten_set_keydown_callback(canvasSelector.constData(), 0, 0, NULL);
    emscripten_set_keyup_callback(canvasSelector.constData(),  0, 0, NULL);

    emscripten_set_mousedown_callback(canvasSelector.constData(), 0, 0, NULL);
    emscripten_set_mouseup_callback(canvasSelector.constData(),  0, 0, NULL);
    emscripten_set_mousemove_callback(canvasSelector.constData(),  0, 0, NULL);
    emscripten_set_mouseenter_callback(canvasSelector.constData(),  0, 0, NULL);
    emscripten_set_mouseleave_callback(canvasSelector.constData(),  0, 0, NULL);

    emscripten_set_focus_callback(canvasSelector.constData(),  0, 0, NULL);

    emscripten_set_wheel_callback(canvasSelector.constData(),  0, 0, NULL);

    emscripten_set_touchstart_callback(canvasSelector.constData(),  0, 0, NULL);
    emscripten_set_touchend_callback(canvasSelector.constData(),  0, 0, NULL);
    emscripten_set_touchmove_callback(canvasSelector.constData(),  0, 0, NULL);
    emscripten_set_touchcancel_callback(canvasSelector.constData(),  0, 0, NULL);
}

void QWasmCompositor::destroy()
{
    // Destroy OpenGL resources. This is done here in a separate function
    // which can be called while screen() still returns a valid screen
    // (which it might not, during destruction). A valid QScreen is
    // a requirement for QOffscreenSurface on Wasm since the native
    // context is tied to a single canvas.
    if (m_context) {
        QOffscreenSurface offScreenSurface(screen()->screen());
        offScreenSurface.setFormat(m_context->format());
        offScreenSurface.create();
        m_context->makeCurrent(&offScreenSurface);
        for (QWasmWindow *window : m_windowStack)
            window->destroy();
        m_blitter.reset(nullptr);
        m_context.reset(nullptr);
    }

    m_isEnabled = false; // prevent frame() from creating a new m_context
}

void QWasmCompositor::initEventHandlers()
{
    QByteArray canvasSelector = "#" + screen()->canvasId().toUtf8();

    // The Platform Detect: expand coverage and move as needed
    enum Platform {
        GenericPlatform,
        MacOSPlatform
    };
    Platform platform = Platform(emscripten::val::global("navigator")["platform"]
                                         .call<bool>("includes", emscripten::val("Mac")));

    eventTranslator->setIsMac(platform == MacOSPlatform);

    if (platform == MacOSPlatform) {
        g_useNaturalScrolling = false; // make this !default on macOS

        if (!emscripten::val::global("window")["safari"].isUndefined()) {
            val canvas = screen()->canvas();
            canvas.call<void>("addEventListener",
                              val("wheel"),
                              val::module_property("qtMouseWheelEvent"));
        }
    }

    emscripten_set_keydown_callback(canvasSelector.constData(), (void *)this, 1, &keyboard_cb);
    emscripten_set_keyup_callback(canvasSelector.constData(), (void *)this, 1, &keyboard_cb);

    emscripten_set_mousedown_callback(canvasSelector.constData(), (void *)this, 1, &mouse_cb);
    emscripten_set_mouseup_callback(canvasSelector.constData(), (void *)this, 1, &mouse_cb);
    emscripten_set_mousemove_callback(canvasSelector.constData(), (void *)this, 1, &mouse_cb);
    emscripten_set_mouseenter_callback(canvasSelector.constData(), (void *)this, 1, &mouse_cb);
    emscripten_set_mouseleave_callback(canvasSelector.constData(), (void *)this, 1, &mouse_cb);

    emscripten_set_focus_callback(canvasSelector.constData(), (void *)this, 1, &focus_cb);

    emscripten_set_wheel_callback(canvasSelector.constData(), (void *)this, 1, &wheel_cb);

    emscripten_set_touchstart_callback(canvasSelector.constData(), (void *)this, 1, &touchCallback);
    emscripten_set_touchend_callback(canvasSelector.constData(), (void *)this, 1, &touchCallback);
    emscripten_set_touchmove_callback(canvasSelector.constData(), (void *)this, 1, &touchCallback);
    emscripten_set_touchcancel_callback(canvasSelector.constData(), (void *)this, 1, &touchCallback);
}

void QWasmCompositor::setEnabled(bool enabled)
{
    m_isEnabled = enabled;
}

void QWasmCompositor::addWindow(QWasmWindow *window, QWasmWindow *parentWindow)
{
    QWasmCompositedWindow compositedWindow;
    compositedWindow.window = window;
    compositedWindow.parentWindow = parentWindow;
    m_compositedWindows.insert(window, compositedWindow);

    if (parentWindow == 0)
        m_windowStack.append(window);
    else
        m_compositedWindows[parentWindow].childWindows.append(window);

    notifyTopWindowChanged(window);
}

void QWasmCompositor::removeWindow(QWasmWindow *window)
{
    QWasmWindow *platformWindow = m_compositedWindows[window].parentWindow;

    if (platformWindow) {
        QWasmWindow *parentWindow = window;
        m_compositedWindows[parentWindow].childWindows.removeAll(window);
    }

    m_windowStack.removeAll(window);
    m_compositedWindows.remove(window);

    if (!m_windowStack.isEmpty() && !QGuiApplication::focusWindow()) {
        auto lastWindow = m_windowStack.last();
        lastWindow->requestActivateWindow();
        notifyTopWindowChanged(lastWindow);
    }
}

void QWasmCompositor::setVisible(QWasmWindow *window, bool visible)
{
    QWasmCompositedWindow &compositedWindow = m_compositedWindows[window];
    if (compositedWindow.visible == visible)
        return;

    compositedWindow.visible = visible;
    compositedWindow.flushPending = true;
    if (visible)
        compositedWindow.damage = compositedWindow.window->geometry();
    else
        m_globalDamage = compositedWindow.window->geometry(); // repaint previously covered area.

    requestUpdateWindow(window, QWasmCompositor::ExposeEventDelivery);
}

void QWasmCompositor::raise(QWasmWindow *window)
{
    if (m_compositedWindows.size() <= 1)
        return;

    QWasmCompositedWindow &compositedWindow = m_compositedWindows[window];
    compositedWindow.damage = compositedWindow.window->geometry();
    m_windowStack.removeAll(window);
    m_windowStack.append(window);

    notifyTopWindowChanged(window);
}

void QWasmCompositor::lower(QWasmWindow *window)
{
    if (m_compositedWindows.size() <= 1)
        return;

    m_windowStack.removeAll(window);
    m_windowStack.prepend(window);
    QWasmCompositedWindow &compositedWindow = m_compositedWindows[window];
    m_globalDamage = compositedWindow.window->geometry(); // repaint previously covered area.

    notifyTopWindowChanged(window);
}

void QWasmCompositor::setParent(QWasmWindow *window, QWasmWindow *parent)
{
    m_compositedWindows[window].parentWindow = parent;

    requestUpdate();
}

int QWasmCompositor::windowCount() const
{
    return m_windowStack.count();
}

QWindow *QWasmCompositor::windowAt(QPoint globalPoint, int padding) const
{
    int index = m_windowStack.count() - 1;
    // qDebug() << "window at" << "point" << p << "window count" << index;

    while (index >= 0) {
        const QWasmCompositedWindow &compositedWindow = m_compositedWindows[m_windowStack.at(index)];
        //qDebug() << "windwAt testing" << compositedWindow.window <<

        QRect geometry = compositedWindow.window->windowFrameGeometry()
                         .adjusted(-padding, -padding, padding, padding);

        if (compositedWindow.visible && geometry.contains(globalPoint))
            return m_windowStack.at(index)->window();
        --index;
    }

    return 0;
}

QWindow *QWasmCompositor::keyWindow() const
{
    return m_windowStack.at(m_windowStack.count() - 1)->window();
}

void QWasmCompositor::blit(QOpenGLTextureBlitter *blitter, QWasmScreen *screen, const QOpenGLTexture *texture, QRect targetGeometry)
{
    QMatrix4x4 m;
    m.translate(-1.0f, -1.0f);

    m.scale(2.0f / (float)screen->geometry().width(),
            2.0f / (float)screen->geometry().height());

    m.translate((float)targetGeometry.width() / 2.0f,
                (float)-targetGeometry.height() / 2.0f);

    m.translate(targetGeometry.x(), screen->geometry().height() - targetGeometry.y());

    m.scale(0.5f * (float)targetGeometry.width(),
            0.5f * (float)targetGeometry.height());

    blitter->blit(texture->textureId(), m, QOpenGLTextureBlitter::OriginTopLeft);
}

void QWasmCompositor::drawWindowContent(QOpenGLTextureBlitter *blitter, QWasmScreen *screen, QWasmWindow *window)
{
    QWasmBackingStore *backingStore = window->backingStore();
    if (!backingStore)
        return;

    QOpenGLTexture const *texture = backingStore->getUpdatedTexture();
    QPoint windowCanvasPosition = window->geometry().topLeft() - screen->geometry().topLeft();
    QRect windowCanvasGeometry = QRect(windowCanvasPosition, window->geometry().size());
    blit(blitter, screen, texture, windowCanvasGeometry);
}

QPalette QWasmCompositor::makeWindowPalette()
{
    QPalette palette;
    palette.setColor(QPalette::Active, QPalette::Highlight,
                     palette.color(QPalette::Active, QPalette::Highlight));
    palette.setColor(QPalette::Active, QPalette::Base,
                     palette.color(QPalette::Active, QPalette::Highlight));
    palette.setColor(QPalette::Inactive, QPalette::Highlight,
                     palette.color(QPalette::Inactive, QPalette::Dark));
    palette.setColor(QPalette::Inactive, QPalette::Base,
                     palette.color(QPalette::Inactive, QPalette::Dark));
    palette.setColor(QPalette::Inactive, QPalette::HighlightedText,
                     palette.color(QPalette::Inactive, QPalette::Window));

    return palette;
}

QRect QWasmCompositor::titlebarRect(QWasmTitleBarOptions tb, QWasmCompositor::SubControls subcontrol)
{
    QRect ret;
    const int controlMargin = 2;
    const int controlHeight = tb.rect.height() - controlMargin *2;
    const int delta = controlHeight + controlMargin;
    int offset = 0;

    bool isMinimized = tb.state & Qt::WindowMinimized;
    bool isMaximized = tb.state & Qt::WindowMaximized;

    ret = tb.rect;
    switch (subcontrol) {
    case SC_TitleBarLabel:
        if (tb.flags & Qt::WindowSystemMenuHint)
            ret.adjust(delta, 0, -delta, 0);
        break;
    case SC_TitleBarCloseButton:
        if (tb.flags & Qt::WindowSystemMenuHint) {
            ret.adjust(0, 0, -delta, 0);
            offset += delta;
        }
        break;
    case SC_TitleBarMaxButton:
        if (!isMaximized && tb.flags & Qt::WindowMaximizeButtonHint) {
            ret.adjust(0, 0, -delta*2, 0);
            offset += (delta +delta);
        }
        break;
    case SC_TitleBarNormalButton:
        if (isMinimized && (tb.flags & Qt::WindowMinimizeButtonHint)) {
            offset += delta;
        } else if (isMaximized && (tb.flags & Qt::WindowMaximizeButtonHint)) {
            ret.adjust(0, 0, -delta*2, 0);
            offset += (delta +delta);
        }
        break;
    case SC_TitleBarSysMenu:
        if (tb.flags & Qt::WindowSystemMenuHint) {
            ret.setRect(tb.rect.left() + controlMargin, tb.rect.top() + controlMargin,
                        controlHeight, controlHeight);
        }
        break;
    default:
        break;
    };

    if (subcontrol != SC_TitleBarLabel && subcontrol != SC_TitleBarSysMenu) {
        ret.setRect(tb.rect.right() - offset, tb.rect.top() + controlMargin,
                    controlHeight, controlHeight);
    }

    if (qApp->layoutDirection() == Qt::LeftToRight)
        return ret;

    QRect rect = ret;
    rect.translate(2 * (tb.rect.right() - ret.right()) +
                   ret.width() - tb.rect.width(), 0);

    return rect;
}

void QWasmCompositor::requestUpdateAllWindows()
{
    m_requestUpdateAllWindows = true;
    requestUpdate();
}

void QWasmCompositor::requestUpdateWindow(QWasmWindow *window, UpdateRequestDeliveryType updateType)
{
    auto it = m_requestUpdateWindows.find(window);
    if (it == m_requestUpdateWindows.end()) {
        m_requestUpdateWindows.insert(window, updateType);
    } else {
        // Already registered, but upgrade ExposeEventDeliveryType to UpdateRequestDeliveryType.
        // if needed, to make sure QWindow::updateRequest's are matched.
        if (it.value() == ExposeEventDelivery && updateType == UpdateRequestDelivery)
            it.value() = UpdateRequestDelivery;
    }

    requestUpdate();
}

// Requests an upate/new frame using RequestAnimationFrame
void QWasmCompositor::requestUpdate()
{
    if (m_requestAnimationFrameId != -1)
        return;

    static auto frame = [](double frameTime, void *context) -> int {
        Q_UNUSED(frameTime);
        QWasmCompositor *compositor = reinterpret_cast<QWasmCompositor *>(context);
        compositor->m_requestAnimationFrameId = -1;
        compositor->deliverUpdateRequests();
        return 0;
    };
    m_requestAnimationFrameId = emscripten_request_animation_frame(frame, this);
}

void QWasmCompositor::deliverUpdateRequests()
{
    // We may get new update requests during the window content update below:
    // prepare for recording the new update set by setting aside the current
    // update set.
    auto requestUpdateWindows = m_requestUpdateWindows;
    m_requestUpdateWindows.clear();
    bool requestUpdateAllWindows = m_requestUpdateAllWindows;
    m_requestUpdateAllWindows = false;

    // Update window content, either all windows or a spesific set of windows. Use the correct update
    // type: QWindow subclasses expect that requested and delivered updateRequests matches exactly.
    m_inDeliverUpdateRequest = true;
    if (requestUpdateAllWindows) {
        for (QWasmWindow *window : m_windowStack) {
            auto it = requestUpdateWindows.find(window);
            UpdateRequestDeliveryType updateType =
                (it == m_requestUpdateWindows.end() ? ExposeEventDelivery : it.value());
            deliverUpdateRequest(window, updateType);
        }
    } else {
        for (auto it = requestUpdateWindows.constBegin(); it != requestUpdateWindows.constEnd(); ++it) {
            auto *window = it.key();
            UpdateRequestDeliveryType updateType = it.value();
            deliverUpdateRequest(window, updateType);
        }
    }
    m_inDeliverUpdateRequest = false;

    // Compose window content
    frame();
}

void QWasmCompositor::deliverUpdateRequest(QWasmWindow *window, UpdateRequestDeliveryType updateType)
{
    // update by deliverUpdateRequest and expose event accordingly.
    if (updateType == UpdateRequestDelivery) {
        window->QPlatformWindow::deliverUpdateRequest();
    } else {
        QWindow *qwindow = window->window();
        QWindowSystemInterface::handleExposeEvent<QWindowSystemInterface::SynchronousDelivery>(
            qwindow, QRect(QPoint(0, 0), qwindow->geometry().size()));
    }
}

void QWasmCompositor::handleBackingStoreFlush()
{
    // Request update to flush the updated backing store content,
    // unless we are currently processing an update, in which case
    // the new content will flushed as a part of that update.
    if (!m_inDeliverUpdateRequest)
        requestUpdate();
}

int dpiScaled(qreal value)
{
    return value * (qreal(qt_defaultDpiX()) / 96.0);
}

QWasmCompositor::QWasmTitleBarOptions QWasmCompositor::makeTitleBarOptions(const QWasmWindow *window)
{
    int width = window->windowFrameGeometry().width();
    int border = window->borderWidth();

    QWasmTitleBarOptions titleBarOptions;

    titleBarOptions.rect = QRect(border, border, width - 2 * border, window->titleHeight());
    titleBarOptions.flags = window->window()->flags();
    titleBarOptions.state = window->window()->windowState();

    bool isMaximized = titleBarOptions.state & Qt::WindowMaximized; // this gets reset when maximized

    if (titleBarOptions.flags & (Qt::WindowTitleHint))
        titleBarOptions.subControls |= SC_TitleBarLabel;
    if (titleBarOptions.flags & Qt::WindowMaximizeButtonHint) {
        if (isMaximized)
            titleBarOptions.subControls |= SC_TitleBarNormalButton;
        else
            titleBarOptions.subControls |= SC_TitleBarMaxButton;
    }
    if (titleBarOptions.flags & Qt::WindowSystemMenuHint) {
        titleBarOptions.subControls |= SC_TitleBarCloseButton;
        titleBarOptions.subControls |= SC_TitleBarSysMenu;
    }


    titleBarOptions.palette = QWasmCompositor::makeWindowPalette();

    if (window->window()->isActive())
        titleBarOptions.palette.setCurrentColorGroup(QPalette::Active);
    else
        titleBarOptions.palette.setCurrentColorGroup(QPalette::Inactive);

    if (window->activeSubControl() != QWasmCompositor::SC_None)
        titleBarOptions.subControls = window->activeSubControl();

    if (!window->window()->title().isEmpty())
        titleBarOptions.titleBarOptionsString = window->window()->title();

    titleBarOptions.windowIcon = window->window()->icon();

    return titleBarOptions;
}

void QWasmCompositor::drawWindowDecorations(QOpenGLTextureBlitter *blitter, QWasmScreen *screen, QWasmWindow *window)
{
    int width = window->windowFrameGeometry().width();
    int height = window->windowFrameGeometry().height();
    qreal dpr = window->devicePixelRatio();

    QImage image(QSize(width * dpr, height * dpr), QImage::Format_RGB32);
    image.setDevicePixelRatio(dpr);
    QPainter painter(&image);
    painter.fillRect(QRect(0, 0, width, height), painter.background());

    QWasmTitleBarOptions titleBarOptions = makeTitleBarOptions(window);

    drawTitlebarWindow(titleBarOptions, &painter);

    QWasmFrameOptions frameOptions;
    frameOptions.rect = QRect(0, 0, width, height);
    frameOptions.lineWidth = dpiScaled(4.);

    drawFrameWindow(frameOptions, &painter);

    painter.end();

    QOpenGLTexture texture(QOpenGLTexture::Target2D);
    texture.setMinificationFilter(QOpenGLTexture::Nearest);
    texture.setMagnificationFilter(QOpenGLTexture::Nearest);
    texture.setWrapMode(QOpenGLTexture::ClampToEdge);
    texture.setData(image, QOpenGLTexture::DontGenerateMipMaps);
    texture.create();
    texture.bind();

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image.width(), image.height(), GL_RGBA, GL_UNSIGNED_BYTE,
                    image.constScanLine(0));

    blit(blitter, screen, &texture, QRect(window->windowFrameGeometry().topLeft(), QSize(width, height)));
}

void QWasmCompositor::drawFrameWindow(QWasmFrameOptions options, QPainter *painter)
{
    int x = options.rect.x();
    int y = options.rect.y();
    int w = options.rect.width();
    int h = options.rect.height();
    const QColor &c1 = options.palette.light().color();
    const QColor &c2 = options.palette.shadow().color();
    const QColor &c3 = options.palette.midlight().color();
    const QColor &c4 = options.palette.dark().color();
    const QBrush *fill = nullptr;

    const qreal devicePixelRatio = painter->device()->devicePixelRatio();
    if (!qFuzzyCompare(devicePixelRatio, qreal(1))) {
        const qreal inverseScale = qreal(1) / devicePixelRatio;
        painter->scale(inverseScale, inverseScale);
        x = qRound(devicePixelRatio * x);
        y = qRound(devicePixelRatio * y);
        w = qRound(devicePixelRatio * w);
        h = qRound(devicePixelRatio * h);
    }

    QPen oldPen = painter->pen();
    QPoint a[3] = { QPoint(x, y+h-2), QPoint(x, y), QPoint(x+w-2, y) };
    painter->setPen(c1);
    painter->drawPolyline(a, 3);
    QPoint b[3] = { QPoint(x, y+h-1), QPoint(x+w-1, y+h-1), QPoint(x+w-1, y) };
    painter->setPen(c2);
    painter->drawPolyline(b, 3);
    if (w > 4 && h > 4) {
        QPoint c[3] = { QPoint(x+1, y+h-3), QPoint(x+1, y+1), QPoint(x+w-3, y+1) };
        painter->setPen(c3);
        painter->drawPolyline(c, 3);
        QPoint d[3] = { QPoint(x+1, y+h-2), QPoint(x+w-2, y+h-2), QPoint(x+w-2, y+1) };
        painter->setPen(c4);
        painter->drawPolyline(d, 3);
        if (fill)
            painter->fillRect(QRect(x+2, y+2, w-4, h-4), *fill);
    }
    painter->setPen(oldPen);
}

//from commonstyle.cpp
static QPixmap cachedPixmapFromXPM(const char * const *xpm)
{
    QPixmap result;
    const QString tag = QString::asprintf("xpm:0x%p", static_cast<const void*>(xpm));
    if (!QPixmapCache::find(tag, &result)) {
        result = QPixmap(xpm);
        QPixmapCache::insert(tag, result);
    }
    return result;
}

void QWasmCompositor::drawItemPixmap(QPainter *painter, const QRect &rect, int alignment,
                                      const QPixmap &pixmap) const
{
    qreal scale = pixmap.devicePixelRatio();
    QSize size =  pixmap.size() / scale;
    int x = rect.x();
    int y = rect.y();
    int w = size.width();
    int h = size.height();
    if ((alignment & Qt::AlignVCenter) == Qt::AlignVCenter)
        y += rect.size().height()/2 - h/2;
    else if ((alignment & Qt::AlignBottom) == Qt::AlignBottom)
        y += rect.size().height() - h;
    if ((alignment & Qt::AlignRight) == Qt::AlignRight)
        x += rect.size().width() - w;
    else if ((alignment & Qt::AlignHCenter) == Qt::AlignHCenter)
        x += rect.size().width()/2 - w/2;

    QRect aligned = QRect(x, y, w, h);
    QRect inter = aligned.intersected(rect);

    painter->drawPixmap(inter.x(), inter.y(), pixmap, inter.x() - aligned.x(), inter.y() - aligned.y(), inter.width() * scale, inter.height() *scale);
}


void QWasmCompositor::drawTitlebarWindow(QWasmTitleBarOptions tb, QPainter *painter)
{
    QRect ir;
    if (tb.subControls.testFlag(SC_TitleBarLabel)) {
        QColor left = tb.palette.highlight().color();
        QColor right = tb.palette.base().color();

        QBrush fillBrush(left);
        if (left != right) {
            QPoint p1(tb.rect.x(), tb.rect.top() + tb.rect.height()/2);
            QPoint p2(tb.rect.right(), tb.rect.top() + tb.rect.height()/2);
            QLinearGradient lg(p1, p2);
            lg.setColorAt(0, left);
            lg.setColorAt(1, right);
            fillBrush = lg;
        }

        painter->fillRect(tb.rect, fillBrush);
        ir = titlebarRect(tb, SC_TitleBarLabel);
        painter->setPen(tb.palette.highlightedText().color());
        painter->drawText(ir.x() + 2, ir.y(), ir.width() - 2, ir.height(),
                          Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, tb.titleBarOptionsString);
    } // SC_TitleBarLabel

    QPixmap pixmap;

    if (tb.subControls.testFlag(SC_TitleBarCloseButton)
            && tb.flags & Qt::WindowSystemMenuHint) {
        ir = titlebarRect(tb, SC_TitleBarCloseButton);
        pixmap = cachedPixmapFromXPM(qt_close_xpm).scaled(QSize(10, 10));
        drawItemPixmap(painter, ir, Qt::AlignCenter, pixmap);
    } //SC_TitleBarCloseButton

    if (tb.subControls.testFlag(SC_TitleBarMaxButton)
            && tb.flags & Qt::WindowMaximizeButtonHint
            && !(tb.state & Qt::WindowMaximized)) {
        ir = titlebarRect(tb, SC_TitleBarMaxButton);
        pixmap = cachedPixmapFromXPM(qt_maximize_xpm).scaled(QSize(10, 10));
        drawItemPixmap(painter, ir, Qt::AlignCenter, pixmap);
    } //SC_TitleBarMaxButton

    bool drawNormalButton = (tb.subControls & SC_TitleBarNormalButton)
            && (((tb.flags & Qt::WindowMinimizeButtonHint)
                 && (tb.flags & Qt::WindowMinimized))
                || ((tb.flags & Qt::WindowMaximizeButtonHint)
                    && (tb.flags & Qt::WindowMaximized)));

    if (drawNormalButton) {
        ir = titlebarRect(tb, SC_TitleBarNormalButton);
        pixmap = cachedPixmapFromXPM(qt_normalizeup_xpm).scaled( QSize(10, 10));

        drawItemPixmap(painter, ir, Qt::AlignCenter, pixmap);
    } // SC_TitleBarNormalButton

    if (tb.subControls & SC_TitleBarSysMenu && tb.flags & Qt::WindowSystemMenuHint) {
        ir = titlebarRect(tb, SC_TitleBarSysMenu);
        if (!tb.windowIcon.isNull()) {
            tb.windowIcon.paint(painter, ir, Qt::AlignCenter);
        } else {
            pixmap = cachedPixmapFromXPM(qt_menu_xpm).scaled(QSize(10, 10));
            drawItemPixmap(painter, ir, Qt::AlignCenter, pixmap);
        }
    }
}

void QWasmCompositor::drawShadePanel(QWasmTitleBarOptions options, QPainter *painter)
{
    int lineWidth = 1;
    QPalette palette = options.palette;
    const QBrush *fill = &options.palette.brush(QPalette::Button);

    int x = options.rect.x();
    int y = options.rect.y();
    int w = options.rect.width();
    int h = options.rect.height();

    const qreal devicePixelRatio = painter->device()->devicePixelRatio();
    if (!qFuzzyCompare(devicePixelRatio, qreal(1))) {
        const qreal inverseScale = qreal(1) / devicePixelRatio;
        painter->scale(inverseScale, inverseScale);

        x = qRound(devicePixelRatio * x);
        y = qRound(devicePixelRatio * y);
        w = qRound(devicePixelRatio * w);
        h = qRound(devicePixelRatio * h);
        lineWidth = qRound(devicePixelRatio * lineWidth);
    }

    QColor shade = palette.dark().color();
    QColor light = palette.light().color();

    if (fill) {
        if (fill->color() == shade)
            shade = palette.shadow().color();
        if (fill->color() == light)
            light = palette.midlight().color();
    }
    QPen oldPen = painter->pen();
    QList<QLineF> lines;
    lines.reserve(2*lineWidth);

    painter->setPen(light);
    int x1, y1, x2, y2;
    int i;
    x1 = x;
    y1 = y2 = y;
    x2 = x + w - 2;
    for (i = 0; i < lineWidth; i++)                // top shadow
        lines << QLineF(x1, y1++, x2--, y2++);

    x2 = x1;
    y1 = y + h - 2;
    for (i = 0; i < lineWidth; i++)               // left shado
        lines << QLineF(x1++, y1, x2++, y2--);

    painter->drawLines(lines);
    lines.clear();
    painter->setPen(shade);
    x1 = x;
    y1 = y2 = y+h-1;
    x2 = x+w-1;
    for (i=0; i<lineWidth; i++) {                // bottom shadow
        lines << QLineF(x1++, y1--, x2, y2--);
    }
    x1 = x2;
    y1 = y;
    y2 = y + h - lineWidth - 1;
    for (i = 0; i < lineWidth; i++)                // right shadow
        lines << QLineF(x1--, y1++, x2--, y2);

    painter->drawLines(lines);
    if (fill)                                // fill with fill color
        painter->fillRect(x+lineWidth, y+lineWidth, w-lineWidth*2, h-lineWidth*2, *fill);
    painter->setPen(oldPen);                        // restore pen

}

void QWasmCompositor::drawWindow(QOpenGLTextureBlitter *blitter, QWasmScreen *screen, QWasmWindow *window)
{
    if (window->window()->type() != Qt::Popup && !(window->m_windowState & Qt::WindowFullScreen))
        drawWindowDecorations(blitter, screen, window);
    drawWindowContent(blitter, screen, window);
}

void QWasmCompositor::frame()
{
    if (!m_isEnabled || m_windowStack.empty() || !screen())
        return;

    QWasmWindow *someWindow = nullptr;

    for (QWasmWindow *window : qAsConst(m_windowStack)) {
        if (window->window()->surfaceClass() == QSurface::Window
                && qt_window_private(static_cast<QWindow *>(window->window()))->receivedExpose) {
            someWindow = window;
            break;
        }
    }

    if (!someWindow)
        return;

    if (m_context.isNull()) {
        m_context.reset(new QOpenGLContext());
        m_context->setFormat(someWindow->window()->requestedFormat());
        m_context->setScreen(screen()->screen());
        m_context->create();
    }

    bool ok = m_context->makeCurrent(someWindow->window());
    if (!ok)
        return;

    if (!m_blitter->isCreated())
        m_blitter->create();

    qreal dpr = screen()->devicePixelRatio();
    glViewport(0, 0, screen()->geometry().width() * dpr, screen()->geometry().height() * dpr);

    m_context->functions()->glClearColor(0.2, 0.2, 0.2, 1.0);
    m_context->functions()->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    m_blitter->bind();
    m_blitter->setRedBlueSwizzle(true);

    for (QWasmWindow *window : qAsConst(m_windowStack)) {
        QWasmCompositedWindow &compositedWindow = m_compositedWindows[window];

        if (!compositedWindow.visible)
            continue;

        drawWindow(m_blitter.data(), screen(), window);
    }

    m_blitter->release();

    if (someWindow && someWindow->window()->surfaceType() == QSurface::OpenGLSurface)
        m_context->swapBuffers(someWindow->window());
}

void QWasmCompositor::resizeWindow(QWindow *window, QWasmCompositor::ResizeMode mode,
                  QRect startRect, QPoint amount)
{
    if (mode == QWasmCompositor::ResizeNone)
        return;

    bool top = mode == QWasmCompositor::ResizeTopLeft ||
               mode == QWasmCompositor::ResizeTop ||
               mode == QWasmCompositor::ResizeTopRight;

    bool bottom = mode == QWasmCompositor::ResizeBottomLeft ||
                  mode == QWasmCompositor::ResizeBottom ||
                  mode == QWasmCompositor::ResizeBottomRight;

    bool left = mode == QWasmCompositor::ResizeLeft ||
                mode == QWasmCompositor::ResizeTopLeft ||
                mode == QWasmCompositor::ResizeBottomLeft;

    bool right = mode == QWasmCompositor::ResizeRight ||
                 mode == QWasmCompositor::ResizeTopRight ||
                 mode == QWasmCompositor::ResizeBottomRight;

    int x1 = startRect.left();
    int y1 = startRect.top();
    int x2 = startRect.right();
    int y2 = startRect.bottom();

    if (left)
        x1 += amount.x();
    if (top)
        y1 += amount.y();
    if (right)
        x2 += amount.x();
    if (bottom)
        y2 += amount.y();

    int w = x2-x1;
    int h = y2-y1;

    if (w < window->minimumWidth()) {
        if (left)
            x1 -= window->minimumWidth() - w;

        w = window->minimumWidth();
    }

    if (h < window->minimumHeight()) {
        if (top)
            y1 -= window->minimumHeight() - h;

        h = window->minimumHeight();
    }

    window->setGeometry(x1, y1, w, h);
}

void QWasmCompositor::notifyTopWindowChanged(QWasmWindow *window)
{
    QWindow *modalWindow;
    bool blocked = QGuiApplicationPrivate::instance()->isWindowBlocked(window->window(), &modalWindow);

    if (blocked) {
        modalWindow->requestActivate();
        raise(static_cast<QWasmWindow*>(modalWindow->handle()));
        return;
    }

    requestUpdate();
}

QWasmScreen *QWasmCompositor::screen()
{
    return static_cast<QWasmScreen *>(parent());
}

QOpenGLContext *QWasmCompositor::context()
{
    return m_context.data();
}

int QWasmCompositor::keyboard_cb(int eventType, const EmscriptenKeyboardEvent *keyEvent, void *userData)
{
    QWasmCompositor *wasmCompositor = reinterpret_cast<QWasmCompositor *>(userData);
    bool accepted = wasmCompositor->processKeyboard(eventType, keyEvent);

    return accepted ? 1 : 0;
}

int QWasmCompositor::mouse_cb(int eventType, const EmscriptenMouseEvent *mouseEvent, void *userData)
{
    QWasmCompositor *compositor = (QWasmCompositor*)userData;
    bool accepted = compositor->processMouse(eventType, mouseEvent);
    return accepted;
}

int QWasmCompositor::focus_cb(int eventType, const EmscriptenFocusEvent *focusEvent, void *userData)
{
    Q_UNUSED(eventType)
    Q_UNUSED(focusEvent)
    Q_UNUSED(userData)

    return 0;
}

int QWasmCompositor::wheel_cb(int eventType, const EmscriptenWheelEvent *wheelEvent, void *userData)
{
    QWasmCompositor *compositor = (QWasmCompositor *) userData;
    bool accepted = compositor->processWheel(eventType, wheelEvent);
    return accepted ? 1 : 0;
}

int QWasmCompositor::touchCallback(int eventType, const EmscriptenTouchEvent *touchEvent, void *userData)
{
    auto compositor = reinterpret_cast<QWasmCompositor*>(userData);
    return compositor->handleTouch(eventType, touchEvent);
}

bool QWasmCompositor::processMouse(int eventType, const EmscriptenMouseEvent *mouseEvent)
{
    QPoint targetPoint(mouseEvent->targetX, mouseEvent->targetY);
    QPoint globalPoint = screen()->geometry().topLeft() + targetPoint;

    QEvent::Type buttonEventType = QEvent::None;
    Qt::MouseButton button = Qt::NoButton;
    Qt::KeyboardModifiers modifiers = eventTranslator->translateMouseEventModifier(mouseEvent);

    QWindow *window2 = nullptr;
    if (resizeMode == QWasmCompositor::ResizeNone)
        window2 = screen()->compositor()->windowAt(globalPoint, 5);

    if (window2 == nullptr) {
        window2 = lastWindow;
    } else {
        lastWindow = window2;
    }

    QPoint localPoint = window2->mapFromGlobal(globalPoint);
    bool interior = window2->geometry().contains(globalPoint);
    bool blocked = QGuiApplicationPrivate::instance()->isWindowBlocked(window2);

    if (mouseInCanvas) {
        if (windowUnderMouse != window2 && interior) {
        // delayed mouse enter
            enterWindow(window2, localPoint, globalPoint);
            windowUnderMouse = window2;
        }
    }

    QWasmWindow *htmlWindow = static_cast<QWasmWindow*>(window2->handle());
    Qt::WindowStates windowState = htmlWindow->window()->windowState();
    bool isResizable = !(windowState.testFlag(Qt::WindowMaximized) ||
                         windowState.testFlag(Qt::WindowFullScreen));

    switch (eventType) {
    case EMSCRIPTEN_EVENT_MOUSEDOWN:
    {
        button = QWasmEventTranslator::translateMouseButton(mouseEvent->button);

        if (window2)
            window2->requestActivate();

        pressedButtons.setFlag(button);

        pressedWindow = window2;
        buttonEventType = QEvent::MouseButtonPress;

        // button overview:
        // 0 = primary mouse button, usually left click
        // 1 = middle mouse button, usually mouse wheel
        // 2 = right mouse button, usually right click
        // from: https://w3c.github.io/uievents/#dom-mouseevent-button
        if (mouseEvent->button == 0) {
            if (!blocked && !(htmlWindow->m_windowState & Qt::WindowFullScreen)
                    && !(htmlWindow->m_windowState & Qt::WindowMaximized)) {
                if (htmlWindow && window2->flags().testFlag(Qt::WindowTitleHint)
                        && htmlWindow->isPointOnTitle(globalPoint))
                    draggedWindow = window2;
                else if (htmlWindow && htmlWindow->isPointOnResizeRegion(globalPoint)) {
                    draggedWindow = window2;
                    resizeMode = htmlWindow->resizeModeAtPoint(globalPoint);
                    resizePoint = globalPoint;
                    resizeStartRect = window2->geometry();
                }
            }
        }

        htmlWindow->injectMousePressed(localPoint, globalPoint, button, modifiers);
        break;
    }
    case EMSCRIPTEN_EVENT_MOUSEUP:
    {
        button = QWasmEventTranslator::translateMouseButton(mouseEvent->button);
        pressedButtons.setFlag(button, false);
        buttonEventType = QEvent::MouseButtonRelease;
        QWasmWindow *oldWindow = nullptr;

        if (mouseEvent->button == 0 && pressedWindow) {
            oldWindow = static_cast<QWasmWindow*>(pressedWindow->handle());
            pressedWindow = nullptr;
        }

        if (draggedWindow && pressedButtons.testFlag(Qt::NoButton)) {
            draggedWindow = nullptr;
            resizeMode = QWasmCompositor::ResizeNone;
        }

        if (oldWindow)
            oldWindow->injectMouseReleased(localPoint, globalPoint, button, modifiers);
        else
            htmlWindow->injectMouseReleased(localPoint, globalPoint, button, modifiers);
        break;
    }
    case EMSCRIPTEN_EVENT_MOUSEMOVE: // drag event
    {
        buttonEventType = QEvent::MouseMove;

        if (htmlWindow && pressedButtons.testFlag(Qt::NoButton)) {

            bool isOnResizeRegion = htmlWindow->isPointOnResizeRegion(globalPoint);

            if (isResizable && isOnResizeRegion && !blocked) {
                QCursor resizingCursor = eventTranslator->cursorForMode(htmlWindow->resizeModeAtPoint(globalPoint));

                if (resizingCursor != window2->cursor()) {
                    isCursorOverridden = true;
                    QWasmCursor::setOverrideWasmCursor(&resizingCursor, window2->screen());
                }
            } else { // off resizing area
                if (isCursorOverridden) {
                    isCursorOverridden = false;
                    QWasmCursor::clearOverrideWasmCursor(window2->screen());
                }
            }
        }

        if (!(htmlWindow->m_windowState & Qt::WindowFullScreen) && !(htmlWindow->m_windowState & Qt::WindowMaximized)) {
            if (resizeMode == QWasmCompositor::ResizeNone && draggedWindow) {
                draggedWindow->setX(draggedWindow->x() + mouseEvent->movementX);
                draggedWindow->setY(draggedWindow->y() + mouseEvent->movementY);
            }

            if (resizeMode != QWasmCompositor::ResizeNone && !(htmlWindow->m_windowState & Qt::WindowFullScreen)) {
                QPoint delta = QPoint(mouseEvent->targetX, mouseEvent->targetY) - resizePoint;
                resizeWindow(draggedWindow, resizeMode, resizeStartRect, delta);
            }
        }
        break;
    }
        case EMSCRIPTEN_EVENT_MOUSEENTER:
            processMouseEnter(mouseEvent);
        break;
        case EMSCRIPTEN_EVENT_MOUSELEAVE:
            processMouseLeave();
        break;
    default:        break;
    };

    if (!interior && pressedButtons.testFlag(Qt::NoButton)) {
        leaveWindow(lastWindow);
    }

    if (!window2 && buttonEventType == QEvent::MouseButtonRelease) {
        window2 = lastWindow;
        lastWindow = nullptr;
        interior = true;
    }
    bool accepted = false;
    if (window2 && interior) {
        accepted = QWindowSystemInterface::handleMouseEvent<QWindowSystemInterface::SynchronousDelivery>(
                window2, QWasmIntegration::getTimestamp(), localPoint, globalPoint, pressedButtons, button, buttonEventType, modifiers);
    }

    if (eventType == EMSCRIPTEN_EVENT_MOUSEDOWN && !accepted)
        QGuiApplicationPrivate::instance()->closeAllPopups();

    return accepted;
}

bool QWasmCompositor::processKeyboard(int eventType, const EmscriptenKeyboardEvent *keyEvent)
{
    Qt::Key qtKey;
    QString keyText;
    QEvent::Type keyType = QEvent::None;
    switch (eventType) {
        case EMSCRIPTEN_EVENT_KEYPRESS:
        case EMSCRIPTEN_EVENT_KEYDOWN: // down
            keyType = QEvent::KeyPress;
            qtKey = this->eventTranslator->getKey(keyEvent);
            keyText = this->eventTranslator->getKeyText(keyEvent, qtKey);
            break;
        case EMSCRIPTEN_EVENT_KEYUP: // up
            keyType = QEvent::KeyRelease;
            this->eventTranslator->setStickyDeadKey(keyEvent);
            break;
        default:
            break;
    };

    if (keyType == QEvent::None)
        return 0;

    QFlags<Qt::KeyboardModifier> modifiers = eventTranslator->translateKeyboardEventModifier(keyEvent);

    // Clipboard fallback path: cut/copy/paste are handled by clipboard event
    // handlers if direct clipboard access is not available.
    if (!QWasmIntegration::get()->getWasmClipboard()->hasClipboardApi && modifiers & Qt::ControlModifier &&
        (qtKey == Qt::Key_X || qtKey == Qt::Key_C || qtKey == Qt::Key_V)) {
        if (qtKey == Qt::Key_V) {
            QWasmIntegration::get()->getWasmClipboard()->isPaste = true;
        }
        return false;
    }

    bool accepted = false;

    if (keyType == QEvent::KeyPress &&
        modifiers.testFlag(Qt::ControlModifier)
        && qtKey == Qt::Key_V) {
        QWasmIntegration::get()->getWasmClipboard()->isPaste = true;
        accepted = false; // continue on to event
    } else {
        if (keyText.isEmpty())
            keyText = QString(keyEvent->key);
        if (keyText.size() > 1)
            keyText.clear();
        accepted = QWindowSystemInterface::handleKeyEvent<QWindowSystemInterface::SynchronousDelivery>(
                0, keyType, qtKey, modifiers, keyText);
    }
    if (keyType == QEvent::KeyPress &&
        modifiers.testFlag(Qt::ControlModifier)
        && qtKey == Qt::Key_C) {
        QWasmIntegration::get()->getWasmClipboard()->isPaste = false;
        accepted = false; // continue on to event
    }

    return accepted;
}

bool QWasmCompositor::processWheel(int eventType, const EmscriptenWheelEvent *wheelEvent)
{
    Q_UNUSED(eventType);

    EmscriptenMouseEvent mouseEvent = wheelEvent->mouse;

    int scrollFactor = 0;
    switch (wheelEvent->deltaMode) {
        case DOM_DELTA_PIXEL://chrome safari
            scrollFactor = 1;
            break;
        case DOM_DELTA_LINE: //firefox
            scrollFactor = 12;
            break;
        case DOM_DELTA_PAGE:
            scrollFactor = 20;
            break;
    };

    if (g_useNaturalScrolling) //macOS platform has document oriented scrolling
        scrollFactor = -scrollFactor;

    Qt::KeyboardModifiers modifiers = eventTranslator->translateMouseEventModifier(&mouseEvent);
    QPoint targetPoint(mouseEvent.targetX, mouseEvent.targetY);
    QPoint globalPoint = screen()->geometry().topLeft() + targetPoint;

    QWindow *window2 = screen()->compositor()->windowAt(globalPoint, 5);
    if (!window2)
        return 0;
    QPoint localPoint = window2->mapFromGlobal(globalPoint);

    QPoint pixelDelta;

    if (wheelEvent->deltaY != 0) pixelDelta.setY(wheelEvent->deltaY * scrollFactor);
    if (wheelEvent->deltaX != 0) pixelDelta.setX(wheelEvent->deltaX * scrollFactor);

    bool accepted = QWindowSystemInterface::handleWheelEvent(
            window2, QWasmIntegration::getTimestamp(), localPoint,
            globalPoint, QPoint(), pixelDelta, modifiers);
    return accepted;
}

int QWasmCompositor::handleTouch(int eventType, const EmscriptenTouchEvent *touchEvent)
{
    QList<QWindowSystemInterface::TouchPoint> touchPointList;
    touchPointList.reserve(touchEvent->numTouches);
    QWindow *window2;

    for (int i = 0; i < touchEvent->numTouches; i++) {

        const EmscriptenTouchPoint *touches = &touchEvent->touches[i];

        QPoint targetPoint(touches->targetX, touches->targetY);
        QPoint globalPoint = screen()->geometry().topLeft() + targetPoint;

        window2 = this->screen()->compositor()->windowAt(globalPoint, 5);
        if (window2 == nullptr)
            continue;

        QWindowSystemInterface::TouchPoint touchPoint;

        touchPoint.area = QRect(0, 0, 8, 8);
        touchPoint.id = touches->identifier;
        touchPoint.pressure = 1.0;

        touchPoint.area.moveCenter(globalPoint);

        const auto tp = pressedTouchIds.constFind(touchPoint.id);
        if (tp != pressedTouchIds.constEnd())
            touchPoint.normalPosition = tp.value();

        QPointF localPoint = QPointF(window2->mapFromGlobal(globalPoint));
        QPointF normalPosition(localPoint.x() / window2->width(),
                               localPoint.y() / window2->height());

        const bool stationaryTouchPoint = (normalPosition == touchPoint.normalPosition);
        touchPoint.normalPosition = normalPosition;

        switch (eventType) {
            case EMSCRIPTEN_EVENT_TOUCHSTART:
                if (tp != pressedTouchIds.constEnd()) {
                    touchPoint.state = (stationaryTouchPoint
                                        ? QEventPoint::State::Stationary
                                        : QEventPoint::State::Updated);
                } else {
                    touchPoint.state = QEventPoint::State::Pressed;
                }
                pressedTouchIds.insert(touchPoint.id, touchPoint.normalPosition);

                break;
            case EMSCRIPTEN_EVENT_TOUCHEND:
                touchPoint.state = QEventPoint::State::Released;
                pressedTouchIds.remove(touchPoint.id);
                break;
            case EMSCRIPTEN_EVENT_TOUCHMOVE:
                touchPoint.state = (stationaryTouchPoint
                                    ? QEventPoint::State::Stationary
                                    : QEventPoint::State::Updated);

                pressedTouchIds.insert(touchPoint.id, touchPoint.normalPosition);
                break;
            default:
                break;
        }

        touchPointList.append(touchPoint);
    }

    QFlags<Qt::KeyboardModifier> keyModifier = eventTranslator->translateTouchEventModifier(touchEvent);

    bool accepted = QWindowSystemInterface::handleTouchEvent<QWindowSystemInterface::SynchronousDelivery>(
            window2, QWasmIntegration::getTimestamp(), touchDevice, touchPointList, keyModifier);

    if (eventType == EMSCRIPTEN_EVENT_TOUCHCANCEL)
        accepted = QWindowSystemInterface::handleTouchCancelEvent(window2, QWasmIntegration::getTimestamp(), touchDevice, keyModifier);

    return static_cast<int>(accepted);
}

void QWasmCompositor::leaveWindow(QWindow *window)
{
    windowUnderMouse = nullptr;
    QWindowSystemInterface::handleLeaveEvent<QWindowSystemInterface::SynchronousDelivery>(window);
}
void QWasmCompositor::enterWindow(QWindow *window, const QPoint &localPoint, const QPoint &globalPoint)
{
    QWindowSystemInterface::handleEnterEvent<QWindowSystemInterface::SynchronousDelivery>(window, localPoint, globalPoint);
}
bool QWasmCompositor::processMouseEnter(const EmscriptenMouseEvent *mouseEvent)
{
    // mouse has entered the canvas area
    mouseInCanvas = true;
    return true;
}
bool QWasmCompositor::processMouseLeave()
{
    mouseInCanvas = false;
    return true;
}
