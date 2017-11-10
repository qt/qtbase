/****************************************************************************
**
** Copyright (C) 2015 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt for Native Client platform plugin.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qhtml5compositor.h"
#include "qhtml5window.h"
#include "qhtml5stylepixmaps_p.h"

#include <QOpenGLTexture>

#include <QtGui/private/qwindow_p.h>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/qopengltextureblitter.h>
#include <QtGui/QPainter>
#include <private/qpixmapcache_p.h>
#include <QFontMetrics>

#include <private/qguiapplication_p.h>

#include <qpa/qwindowsysteminterface.h>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QDebug>

Q_GUI_EXPORT int qt_defaultDpiX();

QHtml5CompositedWindow::QHtml5CompositedWindow()
    : window(0)
    , parentWindow(0)
    , flushPending(false)
    , visible(false)
{
}

QHtml5Compositor::QHtml5Compositor()
    : m_frameBuffer(0)
    , mBlitter(new QOpenGLTextureBlitter)
    , m_needComposit(false)
    , m_inFlush(false)
    , m_inResize(false)
    , m_isEnabled(true)
    , m_targetDevicePixelRatio(1)
{
}

QHtml5Compositor::~QHtml5Compositor()
{
    delete m_frameBuffer;
}

void QHtml5Compositor::setEnabled(bool enabled)
{
    m_isEnabled = enabled;
}

void QHtml5Compositor::addWindow(QHtml5Window *window, QHtml5Window *parentWindow)
{
    QHtml5CompositedWindow compositedWindow;
    compositedWindow.window = window;
    compositedWindow.parentWindow = parentWindow;
    m_compositedWindows.insert(window, compositedWindow);

    if (parentWindow == 0) {
        m_windowStack.append(window);
    } else {
        m_compositedWindows[parentWindow].childWindows.append(window);
    }

    notifyTopWindowChanged(window);
}

void QHtml5Compositor::removeWindow(QHtml5Window *window)
{
    QHtml5Window *platformWindow = m_compositedWindows[window].parentWindow;

    if (platformWindow) {
        QHtml5Window *parentWindow = window;
        m_compositedWindows[parentWindow].childWindows.removeAll(window);
    }

    m_windowStack.removeAll(window);
    m_compositedWindows.remove(window);

    notifyTopWindowChanged(window);
}

void QHtml5Compositor::setScreen(QHTML5Screen *screen)
{
    mScreen = screen;
}

void QHtml5Compositor::setVisible(QHtml5Window *window, bool visible)
{
    QHtml5CompositedWindow &compositedWindow = m_compositedWindows[window];
    if (compositedWindow.visible == visible)
        return;

    compositedWindow.visible = visible;
    compositedWindow.flushPending = true;
    if (visible)
        compositedWindow.damage = compositedWindow.window->geometry();
    else
        globalDamage = compositedWindow.window->geometry(); // repaint previosly covered area.

    requestRedraw();
}

void QHtml5Compositor::raise(QHtml5Window *window)
{
    if (m_compositedWindows.size() <= 1)
        return;

    QHtml5CompositedWindow &compositedWindow = m_compositedWindows[window];
    compositedWindow.damage = compositedWindow.window->geometry();
    m_windowStack.removeAll(window);
    m_windowStack.append(window);

    notifyTopWindowChanged(window);
}

void QHtml5Compositor::lower(QHtml5Window *window)
{
    if (m_compositedWindows.size() <= 1)
        return;

    m_windowStack.removeAll(window);
    m_windowStack.prepend(window);
    QHtml5CompositedWindow &compositedWindow = m_compositedWindows[window];
    globalDamage = compositedWindow.window->geometry(); // repaint previosly covered area.

    notifyTopWindowChanged(window);
}

void QHtml5Compositor::setParent(QHtml5Window *window, QHtml5Window *parent)
{
    m_compositedWindows[window].parentWindow = parent;

    requestRedraw();
}

void QHtml5Compositor::flush(QHtml5Window *window, const QRegion &region)
{
    QHtml5CompositedWindow &compositedWindow = m_compositedWindows[window];
    compositedWindow.flushPending = true;
    compositedWindow.damage = region;

    requestRedraw();
}

int QHtml5Compositor::windowCount() const
{
    return m_windowStack.count();
}

void QHtml5Compositor::requestRedraw()
{
    if (m_needComposit)
        return;

    m_needComposit = true;
    QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
}

QWindow *QHtml5Compositor::windowAt(QPoint p, int padding) const
{
    int index = m_windowStack.count() - 1;
    // qDebug() << "window at" << "point" << p << "window count" << index;

    while (index >= 0) {
        const QHtml5CompositedWindow &compositedWindow = m_compositedWindows[m_windowStack.at(index)];
        //qDebug() << "windwAt testing" << compositedWindow.window <<

        QRect geometry = compositedWindow.window->windowFrameGeometry()
                         .adjusted(-padding, -padding, padding, padding);

        if (compositedWindow.visible && geometry.contains(p))
            return m_windowStack.at(index)->window();
        --index;
    }

    return 0;
}

QWindow *QHtml5Compositor::keyWindow() const
{
    return m_windowStack.at(m_windowStack.count() - 1)->window();
}

bool QHtml5Compositor::event(QEvent *ev)
{
    if (ev->type() == QEvent::UpdateRequest) {
        if (m_isEnabled)
            frame();
        return true;
    }

    return QObject::event(ev);
}

void QHtml5Compositor::blit(QOpenGLTextureBlitter *blitter, QHTML5Screen *screen, const QOpenGLTexture *texture, QRect targetGeometry)
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

void QHtml5Compositor::drawWindowContent(QOpenGLTextureBlitter *blitter, QHTML5Screen *screen, QHtml5Window *window)
{
    QHTML5BackingStore* backingStore = window->backingStore();

    QOpenGLTexture const* texture = backingStore->getUpdatedTexture();

    blit(blitter, screen, texture, window->geometry());
}

QPalette QHtml5Compositor::makeWindowPalette()
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

QRect QHtml5Compositor::titlebarRect(QHtml5TitleBarOptions tb, QHtml5Compositor::SubControls subcontrol)
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
    case  SC_TitleBarCloseButton:
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
        if (isMinimized && (tb.flags & Qt::WindowMinimizeButtonHint))
            offset += delta;
        else if (isMaximized && (tb.flags & Qt::WindowMaximizeButtonHint))
            ret.adjust(0, 0, -delta*2, 0);
            offset += (delta +delta);
//            offset += delta;
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

    if (subcontrol != SC_TitleBarLabel && subcontrol != SC_TitleBarSysMenu)  {
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

QHtml5Compositor::QHtml5TitleBarOptions QHtml5Compositor::makeTitleBarOptions(const QHtml5Window *window)
{
    int width = window->windowFrameGeometry().width();
    int border = window->borderWidth();

    QHtml5TitleBarOptions titleBarOptions;

    titleBarOptions.rect = QRect(border, border, width - 2 * border, window->titleHeight());
    titleBarOptions.flags = window->window()->flags();
    titleBarOptions.state = window->window()->windowState();

   // bool isMinimized = titleBarOptions.flags & Qt::WindowMinimized;
    bool isMaximized = titleBarOptions.state & Qt::WindowMaximized; // this gets reset when maximized

    if (titleBarOptions.flags & (Qt::WindowTitleHint))
        titleBarOptions.subControls |= SC_TitleBarLabel;
    if (titleBarOptions.flags & Qt::WindowMaximizeButtonHint) {
        if (isMaximized) {
            titleBarOptions.subControls |= SC_TitleBarNormalButton;
        } else {
            titleBarOptions.subControls |= SC_TitleBarMaxButton;
        }
    }
    if (titleBarOptions.flags & Qt::WindowSystemMenuHint) {
        titleBarOptions.subControls |= SC_TitleBarCloseButton;
        titleBarOptions.subControls |= SC_TitleBarSysMenu;
    }

    // Disable minimize button
    //   titleBarOptions.flags.setFlag(Qt::WindowMinimizeButtonHint, false);

    titleBarOptions.palette = QHtml5Compositor::makeWindowPalette();

    if (window->window()->isActive()) {
        //  titleBarOptions.state |= QHtml5Compositor::State_Active;
        //  titleBarOptions.state |= QHtml5Compositor::State_Active;
        titleBarOptions.palette.setCurrentColorGroup(QPalette::Active);
    } else {
        //        titleBarOptions.state &= ~QHtml5Compositor::State_Active;
        titleBarOptions.palette.setCurrentColorGroup(QPalette::Inactive);
    }

        if (window->activeSubControl() != QHtml5Compositor::SC_None) {
            titleBarOptions.subControls = window->activeSubControl();
       //     titleBarOptions.state |= QHtml5Compositor::State_Sunken;
        }

    if (!window->window()->title().isEmpty()) {

        titleBarOptions.titleBarOptionsString = window->window()->title();

#warning FIXME QFontMetrics
//        QFont font = QApplication::font("QMdiSubWindowTitleBar");
//        QFontMetrics fontMetrics = QFontMetrics(font);
//        fontMetrics.elidedText(window->window()->title(), Qt::ElideRight, titleWidth);
    }

    return titleBarOptions;
}

void QHtml5Compositor::drawWindowDecorations(QOpenGLTextureBlitter *blitter, QHTML5Screen *screen, QHtml5Window *window)
{
    int width = window->windowFrameGeometry().width();
    int height = window->windowFrameGeometry().height();

    QImage image(QSize(width, height), QImage::Format_RGB32);
    QPainter painter(&image);
    painter.fillRect(QRect(0, 0, width, height), painter.background());

    QHtml5TitleBarOptions titleBarOptions = makeTitleBarOptions(window);

    drawTitlebarWindow(titleBarOptions, &painter);

    QHtml5FrameOptions frameOptions;
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

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE,
                    image.constScanLine(0));

    blit(blitter, screen, &texture, QRect(window->windowFrameGeometry().topLeft(), QSize(width, height)));
}

void QHtml5Compositor::drawFrameWindow(QHtml5FrameOptions options, QPainter *painter)
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
        painter->save();
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

void QHtml5Compositor::drawItemPixmap(QPainter *painter, const QRect &rect, int alignment,
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


void QHtml5Compositor::drawTitlebarWindow(QHtml5TitleBarOptions tb, QPainter *painter)
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

//    QHtml5TitleBarOptions tool = tb;
//    tool.rect = ir;
//    tool.state = down ? State_Sunken : State_Raised;

    // FIXME

    //        painter->save();
    //        if (down)
    //            painter->translate(proxy()->pixelMetric(PM_ButtonShiftHorizontal, tb, widget),
    //                         proxy()->pixelMetric(PM_ButtonShiftVertical, tb, widget));

    //        painter->restore();
    //    }

    //    if (tb->subControls & SC_TitleBarMinButton
    //            && tb->titleBarFlags & Qt::WindowMinimizeButtonHint
    //            && !(tb->titleBarState & Qt::WindowMinimized)) {
    //        ir = proxy()->subControlRect(CC_TitleBar, tb, SC_TitleBarMinButton, widget);
    //        down = tb->activeSubControls & SC_TitleBarMinButton && (opt->state & State_Sunken);
    //        pm = proxy()->standardIcon(SP_TitleBarMinButton, &tool, widget).pixmap(qt_getWindow(widget), QSize(10, 10));
    //        tool.rect = ir;
    //        tool.state = down ? State_Sunken : State_Raised;
    //        proxy()->drawPrimitive(PE_PanelButtonTool, &tool, p, widget);

    //        painter->save();
    //        if (down)
    //            painter->translate(proxy()->pixelMetric(PM_ButtonShiftHorizontal, tb, widget),
    //                         proxy()->pixelMetric(PM_ButtonShiftVertical, tb, widget));
    //        proxy()->drawItemPixmap(p, ir, Qt::AlignCenter, pm);
    //        painter->restore();
    //    }

        bool drawNormalButton = (tb.subControls & SC_TitleBarNormalButton)
                                && (((tb.flags & Qt::WindowMinimizeButtonHint)
                                && (tb.flags & Qt::WindowMinimized))
                                || ((tb.flags & Qt::WindowMaximizeButtonHint)
                                && (tb.flags & Qt::WindowMaximized)));

        if (drawNormalButton) {
            ir = titlebarRect(tb, SC_TitleBarNormalButton);
            down = tb.subControls & SC_TitleBarNormalButton && (tb.state & State_Sunken);
            pixmap = cachedPixmapFromXPM(qt_normalizeup_xpm).scaled( QSize(10, 10));

//            tool.rect = ir;
//            tool.state = down ? State_Sunken : State_Raised;
            drawItemPixmap(painter, ir, Qt::AlignCenter, pixmap);

            //        painter->save();
            //        if (down)
            //            painter->translate(proxy()->pixelMetric(PM_ButtonShiftHorizontal, tb, widget),
            //                         proxy()->pixelMetric(PM_ButtonShiftVertical, tb, widget));

            //      painter->restore();
        } // SC_TitleBarNormalButton

    //    if (tb->subControls & SC_TitleBarShadeButton
    //            && tb->titleBarFlags & Qt::WindowShadeButtonHint
    //            && !(tb->titleBarState & Qt::WindowMinimized)) {
    //        ir = proxy()->subControlRect(CC_TitleBar, tb, SC_TitleBarShadeButton, widget);
    //        down = (tb->activeSubControls & SC_TitleBarShadeButton && (opt->state & State_Sunken));
    //        pm = proxy()->standardIcon(SP_TitleBarShadeButton, &tool, widget).pixmap(qt_getWindow(widget), QSize(10, 10));
    //        tool.rect = ir;
    //        tool.state = down ? State_Sunken : State_Raised;
    //        proxy()->drawPrimitive(PE_PanelButtonTool, &tool, p, widget);
    //        painter->save();
    //        if (down)
    //            painter->translate(proxy()->pixelMetric(PM_ButtonShiftHorizontal, tb, widget),
    //                         proxy()->pixelMetric(PM_ButtonShiftVertical, tb, widget));
    //        proxy()->drawItemPixmap(p, ir, Qt::AlignCenter, pm);
    //        painter->restore();
    //    }

    //    if (tb->subControls & SC_TitleBarUnshadeButton
    //            && tb->titleBarFlags & Qt::WindowShadeButtonHint
    //            && tb->titleBarState & Qt::WindowMinimized) {
    //        ir = proxy()->subControlRect(CC_TitleBar, tb, SC_TitleBarUnshadeButton, widget);

    //        down = tb->activeSubControls & SC_TitleBarUnshadeButton  && (opt->state & State_Sunken);
    //        pm = proxy()->standardIcon(SP_TitleBarUnshadeButton, &tool, widget).pixmap(qt_getWindow(widget), QSize(10, 10));
    //        tool.rect = ir;
    //        tool.state = down ? State_Sunken : State_Raised;
    //        proxy()->drawPrimitive(PE_PanelButtonTool, &tool, p, widget);
    //        painter->save();
    //        if (down)
    //            painter->translate(proxy()->pixelMetric(PM_ButtonShiftHorizontal, tb, widget),
    //                         proxy()->pixelMetric(PM_ButtonShiftVertical, tb, widget));
    //        proxy()->drawItemPixmap(p, ir, Qt::AlignCenter, pm);
    //        painter->restore();
    //    }


    //    if (tb->subControls & SC_TitleBarContextHelpButton
    //            && tb->titleBarFlags & Qt::WindowContextHelpButtonHint) {
    //        ir = proxy()->subControlRect(CC_TitleBar, tb, SC_TitleBarContextHelpButton, widget);

    //        down = tb->activeSubControls & SC_TitleBarContextHelpButton  && (opt->state & State_Sunken);
    //        pm = proxy()->standardIcon(SP_TitleBarContextHelpButton, &tool, widget).pixmap(qt_getWindow(widget), QSize(10, 10));
    //        tool.rect = ir;
    //        tool.state = down ? State_Sunken : State_Raised;
    //        proxy()->drawPrimitive(PE_PanelButtonTool, &tool, p, widget);
    //        painter->save();
    //        if (down)
    //            painter->translate(proxy()->pixelMetric(PM_ButtonShiftHorizontal, tb, widget),
    //                         proxy()->pixelMetric(PM_ButtonShiftVertical, tb, widget));
    //        proxy()->drawItemPixmap(p, ir, Qt::AlignCenter, pm);
    //        painter->restore();
    //    }

        if (tb.subControls & SC_TitleBarSysMenu && tb.flags & Qt::WindowSystemMenuHint) {
            ir = titlebarRect(tb, SC_TitleBarSysMenu);
//            if (!tb.icon.isNull()) {
//                tb.icon.paint(p, ir);
//            } else {
          //      int iconSize = proxy()->pixelMetric(PM_SmallIconSize, tb, widget);
                pixmap = cachedPixmapFromXPM(qt_menu_xpm).scaled(QSize(10, 10));
        //        tool.rect = ir;
            //    painter->save();
                drawItemPixmap(painter, ir, Qt::AlignCenter, pixmap);
              //  painter->restore();
//            }
        }
    //}
}

void QHtml5Compositor::drawShadePanel(QHtml5TitleBarOptions options, QPainter *painter)
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
        //  painter->save();
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
    QPen oldPen = painter->pen();                        // save pen
    QVector<QLineF> lines;
    lines.reserve(2*lineWidth);

    //    if (sunken)
    //        painter->setPen(shade);
    //    else
    painter->setPen(light);
    int x1, y1, x2, y2;
    int i;
    x1 = x;
    y1 = y2 = y;
    x2 = x+w-2;
    for (i=0; i<lineWidth; i++) {                // top shadow
        lines << QLineF(x1, y1++, x2--, y2++);
    }
    x2 = x1;
    y1 = y+h-2;
    for (i=0; i<lineWidth; i++) {                // left shado
        lines << QLineF(x1++, y1, x2++, y2--);
    }
    painter->drawLines(lines);
    lines.clear();
    //    if (sunken)
    //        painter->setPen(light);
    //    else
    painter->setPen(shade);
    x1 = x;
    y1 = y2 = y+h-1;
    x2 = x+w-1;
    for (i=0; i<lineWidth; i++) {                // bottom shadow
        lines << QLineF(x1++, y1--, x2, y2--);
    }
    x1 = x2;
    y1 = y;
    y2 = y+h-lineWidth-1;
    for (i=0; i<lineWidth; i++) {                // right shadow
        lines << QLineF(x1--, y1++, x2--, y2);
    }
    painter->drawLines(lines);
    if (fill)                                // fill with fill color
        painter->fillRect(x+lineWidth, y+lineWidth, w-lineWidth*2, h-lineWidth*2, *fill);
    painter->setPen(oldPen);                        // restore pen

}

void QHtml5Compositor::drawWindow(QOpenGLTextureBlitter *blitter, QHTML5Screen *screen, QHtml5Window *window)
{
    drawWindowDecorations(blitter, screen, window);
    drawWindowContent(blitter, screen, window);
}

void QHtml5Compositor::frame()
{
    if (!m_needComposit)
        return;

    m_needComposit = false;

    if (m_windowStack.empty() || !mScreen)
        return;

    QHtml5Window *someWindow = nullptr;

    foreach (QHtml5Window *window, m_windowStack) {
        if (window->window()->surfaceClass() == QSurface::Window
                && qt_window_private(static_cast<QWindow *>(window->window()))->receivedExpose) {
            someWindow = window;
            break;
        }
    }

    if (!someWindow)
        return;

    if (mContext.isNull()) {
        mContext.reset(new QOpenGLContext());
        //mContext->setFormat(mScreen->format());
        mContext->setScreen(mScreen->screen());
        mContext->create();
    }

    mContext->makeCurrent(someWindow->window());

    if (!mBlitter->isCreated())
        mBlitter->create();

    glViewport(0, 0, mScreen->geometry().width(), mScreen->geometry().height());

    mContext->functions()->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    mBlitter->bind();
    mBlitter->setRedBlueSwizzle(true);

    foreach (QHtml5Window *window, m_windowStack) {
        QHtml5CompositedWindow &compositedWindow = m_compositedWindows[window];

        if (!compositedWindow.visible)
            continue;

        drawWindow(mBlitter.data(), mScreen, window);
    }

    mBlitter->release();

    if (someWindow)
        mContext->swapBuffers(someWindow->window());
}

void QHtml5Compositor::notifyTopWindowChanged(QHtml5Window* window)
{
    QWindow *modalWindow;
    bool blocked = QGuiApplicationPrivate::instance()->isWindowBlocked(window->window(), &modalWindow);

    if (blocked) {
        raise(static_cast<QHtml5Window*>(modalWindow->handle()));
        return;
    }

    //if (keyWindow()->handle() == window)
    //return;

    requestRedraw();
    QWindowSystemInterface::handleWindowActivated(window->window());
}
