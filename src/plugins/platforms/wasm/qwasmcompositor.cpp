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
#include "qwasmwindow.h"
#include "qwasmstylepixmaps_p.h"

#include <QtGui/qopengltexture.h>

#include <QtGui/private/qwindow_p.h>
#include <QtGui/qopenglcontext.h>
#include <QtGui/qopenglfunctions.h>
#include <QtGui/qopengltextureblitter.h>
#include <QtGui/qoffscreensurface.h>
#include <QtGui/qpainter.h>
#include <private/qpixmapcache_p.h>

#include <private/qguiapplication_p.h>

#include <qpa/qwindowsysteminterface.h>
#include <QtCore/qcoreapplication.h>
#include <QtGui/qguiapplication.h>

Q_GUI_EXPORT int qt_defaultDpiX();

QWasmCompositedWindow::QWasmCompositedWindow()
    : window(nullptr)
    , parentWindow(nullptr)
    , flushPending(false)
    , visible(false)
{
}

QWasmCompositor::QWasmCompositor(QWasmScreen *screen)
    :QObject(screen)
    , m_blitter(new QOpenGLTextureBlitter)
    , m_needComposit(false)
    , m_inFlush(false)
    , m_inResize(false)
    , m_isEnabled(true)
    , m_targetDevicePixelRatio(1)
{
}

QWasmCompositor::~QWasmCompositor()
{
    destroy();
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

    notifyTopWindowChanged(window);
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
        m_globalDamage = compositedWindow.window->geometry(); // repaint previosly covered area.

    requestRedraw();
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
    m_globalDamage = compositedWindow.window->geometry(); // repaint previosly covered area.

    notifyTopWindowChanged(window);
}

void QWasmCompositor::setParent(QWasmWindow *window, QWasmWindow *parent)
{
    m_compositedWindows[window].parentWindow = parent;

    requestRedraw();
}

void QWasmCompositor::flush(QWasmWindow *window, const QRegion &region)
{
    QWasmCompositedWindow &compositedWindow = m_compositedWindows[window];
    compositedWindow.flushPending = true;
    compositedWindow.damage = region;

    requestRedraw();
}

int QWasmCompositor::windowCount() const
{
    return m_windowStack.count();
}


void QWasmCompositor::redrawWindowContent()
{
    // Redraw window content by sending expose events. This redraw
    // will cause a backing store flush, which will call requestRedraw()
    // to composit.
    for (QWasmWindow *platformWindow : m_windowStack) {
        QWindow *window = platformWindow->window();
        QWindowSystemInterface::handleExposeEvent<QWindowSystemInterface::SynchronousDelivery>(
            window, QRect(QPoint(0, 0), window->geometry().size()));
    }
}

void QWasmCompositor::requestRedraw()
{
    if (m_needComposit)
        return;

    m_needComposit = true;
    QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
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

bool QWasmCompositor::event(QEvent *ev)
{
    if (ev->type() == QEvent::UpdateRequest) {
        if (m_isEnabled)
            frame();
        return true;
    }

    return QObject::event(ev);
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
    const QBrush *fill = 0;

    const qreal devicePixelRatio = painter->device()->devicePixelRatioF();
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

    bool down = false;
    QPixmap pixmap;

    if (tb.subControls.testFlag(SC_TitleBarCloseButton)
            && tb.flags & Qt::WindowSystemMenuHint) {
        ir = titlebarRect(tb, SC_TitleBarCloseButton);
        down = tb.subControls & SC_TitleBarCloseButton && (tb.state & State_Sunken);
        pixmap = cachedPixmapFromXPM(qt_close_xpm).scaled(QSize(10, 10));
        drawItemPixmap(painter, ir, Qt::AlignCenter, pixmap);
    } //SC_TitleBarCloseButton

    if (tb.subControls.testFlag(SC_TitleBarMaxButton)
            && tb.flags & Qt::WindowMaximizeButtonHint
            && !(tb.state & Qt::WindowMaximized)) {
        ir = titlebarRect(tb, SC_TitleBarMaxButton);
        down = tb.subControls & SC_TitleBarMaxButton && (tb.state & State_Sunken);
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
        down = tb.subControls & SC_TitleBarNormalButton && (tb.state & State_Sunken);
        pixmap = cachedPixmapFromXPM(qt_normalizeup_xpm).scaled( QSize(10, 10));

        drawItemPixmap(painter, ir, Qt::AlignCenter, pixmap);
    } // SC_TitleBarNormalButton

    if (tb.subControls & SC_TitleBarSysMenu && tb.flags & Qt::WindowSystemMenuHint) {
        ir = titlebarRect(tb, SC_TitleBarSysMenu);
        pixmap = cachedPixmapFromXPM(qt_menu_xpm).scaled(QSize(10, 10));
        drawItemPixmap(painter, ir, Qt::AlignCenter, pixmap);
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

    const qreal devicePixelRatio = painter->device()->devicePixelRatioF();
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
    QVector<QLineF> lines;
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
    if (!m_needComposit)
        return;

    m_needComposit = false;

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

void QWasmCompositor::notifyTopWindowChanged(QWasmWindow *window)
{
    QWindow *modalWindow;
    bool blocked = QGuiApplicationPrivate::instance()->isWindowBlocked(window->window(), &modalWindow);

    if (blocked) {
        raise(static_cast<QWasmWindow*>(modalWindow->handle()));
        return;
    }

    requestRedraw();
    QWindowSystemInterface::handleWindowActivated(window->window());
}

QWasmScreen *QWasmCompositor::screen()
{
    return static_cast<QWasmScreen *>(parent());
}

QOpenGLContext *QWasmCompositor::context()
{
    return m_context.data();
}
