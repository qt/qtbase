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

#include <QOpenGLTexture>
#include <QtWidgets/QStyle>
#include <QtWidgets/QStyleOptionTitleBar>
#include <QtGui/private/qwindow_p.h>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/qopengltextureblitter.h>
#include <QtGui/QPainter>

#include <private/qguiapplication_p.h>

#include <qpa/qwindowsysteminterface.h>
#include <QtWidgets/QApplication>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QDebug>

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
    qDebug() << Q_FUNC_INFO;

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
    qDebug() << "window: " << window->window()->flags();

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
    qDebug() << Q_FUNC_INFO;

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
    qDebug() << Q_FUNC_INFO;
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
    qDebug() << Q_FUNC_INFO;

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
    qDebug() << Q_FUNC_INFO;

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
    qDebug() << Q_FUNC_INFO;
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
        //compositedWindow.window->geometry();


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

void blit(QOpenGLTextureBlitter *blitter, QHTML5Screen *screen, const QOpenGLTexture *texture, QRect targetGeometry)
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

void drawWindowContent(QOpenGLTextureBlitter *blitter, QHTML5Screen *screen, QHtml5Window *window)
{
    QHTML5BackingStore* backingStore = window->backingStore();

    QOpenGLTexture const* texture = backingStore->getUpdatedTexture();

    blit(blitter, screen, texture, window->geometry());
}

QPalette makeWindowPalette()
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

QStyleOptionTitleBar makeTitleBarOptions(const QHtml5Window *window)
{
    int width = window->windowFrameGeometry().width();

    QApplication *app = static_cast<QApplication*>(QApplication::instance());
    QStyle *style = app->style();

    int border = style->pixelMetric(QStyle::PM_MDIFrameWidth);

    QStyleOptionTitleBar titleBarOptions;
    int titleHeight = style->pixelMetric(QStyle::PM_TitleBarHeight, &titleBarOptions, nullptr);
    titleBarOptions.rect = QRect(border, border, width - 2*border, titleHeight);
    titleBarOptions.titleBarFlags = window->window()->flags();
    titleBarOptions.titleBarState = window->window()->windowState();

    // Disable minimize button
    titleBarOptions.titleBarFlags.setFlag(Qt::WindowMinimizeButtonHint, false);

    titleBarOptions.palette = makeWindowPalette();

    if (window->window()->isActive()) {
        titleBarOptions.state |= QStyle::State_Active;
        titleBarOptions.titleBarState |= QStyle::State_Active;
        titleBarOptions.palette.setCurrentColorGroup(QPalette::Active);
    } else {
        titleBarOptions.state &= ~QStyle::State_Active;
        titleBarOptions.palette.setCurrentColorGroup(QPalette::Inactive);
    }

    if (window->activeSubControl() != QStyle::SC_None) {
        titleBarOptions.activeSubControls = window->activeSubControl();
        titleBarOptions.state |= QStyle::State_Sunken;
    }

    if (!window->window()->title().isEmpty()) {
        int titleWidth = style->subControlRect(QStyle::CC_TitleBar, &titleBarOptions,
                                               QStyle::SC_TitleBarLabel, nullptr).width();
        titleBarOptions.text = titleBarOptions.fontMetrics
                               .elidedText(window->window()->title(), Qt::ElideRight, titleWidth);
    }

    return titleBarOptions;
}

void drawWindowDecorations(QOpenGLTextureBlitter *blitter, QHTML5Screen *screen, QHtml5Window *window)
{
    QApplication *app = static_cast<QApplication*>(QApplication::instance());
    QStyle *style = app->style();

    int width = window->windowFrameGeometry().width();
    int height = window->windowFrameGeometry().height();

    QImage image(QSize(width, height), QImage::Format_RGB32);
    QPainter painter(&image);
    painter.fillRect(QRect(0, 0, width, height), painter.background());

    QStyleOptionTitleBar titleBarOptions = makeTitleBarOptions(window);

    style->drawComplexControl(QStyle::CC_TitleBar, &titleBarOptions, &painter);

    QStyleOptionFrame frameOptions;
    frameOptions.rect = QRect(0, 0, width, height);
    frameOptions.lineWidth = style->pixelMetric(QStyle::PM_MdiSubWindowFrameWidth, 0, nullptr);
    frameOptions.state.setFlag(QStyle::State_Active, window->window()->isActive());

    style->drawPrimitive(QStyle::PE_FrameWindow, &frameOptions, &painter, nullptr);

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

void drawWindow(QOpenGLTextureBlitter *blitter, QHTML5Screen *screen, QHtml5Window *window)
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

