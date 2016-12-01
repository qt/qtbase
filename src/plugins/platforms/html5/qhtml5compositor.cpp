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
//    , m_context2D(0)
//    , m_imageData2D(0)
    , m_needComposit(false)
    , m_inFlush(false)
    , m_inResize(false)
    , m_targetDevicePixelRatio(1)
{
    qDebug() << Q_FUNC_INFO;
 //   m_callbackFactory.Initialize(this);

}

QHtml5Compositor::~QHtml5Compositor()
{
//    delete m_context2D;
//    delete m_imageData2D;
    delete m_frameBuffer;
}

void QHtml5Compositor::addWindow(QHtml5Window *window, QHtml5Window *parentWindow)
{
//    qCDebug(QT_PLATFORM_PEPPER_COMPOSITOR) << "addRasterWindow" << window << parentWindow;

    //qDebug() << Q_FUNC_INFO;

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
//    qCDebug(QT_PLATFORM_PEPPER_COMPOSITOR) << "removeWindow" << window;
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

//    qCDebug(QT_PLATFORM_PEPPER_COMPOSITOR) << "setVisible (effective)" << window << visible;

    compositedWindow.visible = visible;
    compositedWindow.flushPending = true;
    if (visible)
        compositedWindow.damage = compositedWindow.window->geometry();
    else
        globalDamage = compositedWindow.window->geometry(); // repaint previosly covered area.
    //maybeComposit();

    requestRedraw();
}

void QHtml5Compositor::raise(QHtml5Window *window)
{
//    qCDebug(QT_PLATFORM_PEPPER_COMPOSITOR) << "raise" << window;
    qDebug() << Q_FUNC_INFO;

    if (m_compositedWindows.size() <= 1)
        return;

    QHtml5CompositedWindow &compositedWindow = m_compositedWindows[window];
    compositedWindow.damage = compositedWindow.window->geometry();
    m_windowStack.removeAll(window);
    m_windowStack.append(window);
    //maybeComposit();

    notifyTopWindowChanged(window);
}

void QHtml5Compositor::lower(QHtml5Window *window)
{
//    qCDebug(QT_PLATFORM_PEPPER_COMPOSITOR) << "lower" << window;
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

/*
void QHtml5Compositor::setFrameBuffer(QHtml5Window *window, QImage *frameBuffer)
{
    qDebug() << Q_FUNC_INFO;
//    qCDebug(QT_PLATFORM_PEPPER_COMPOSITOR) << "setFrameBuffer" << window << frameBuffer;

    // QPepperBackingStore instances may be created before the corresponding QPepperWindow
    // instance, in which case this call comes to early. This will be rectified when the
    // QPepperWindow is created, at which point the backing store resizes the frame buffer.
    if (!m_compositedWindows.contains(window))
        return;

    m_compositedWindows[window].frameBuffer = frameBuffer;
    m_compositedWindows[window].frameBuffer->fill(Qt::darkMagenta);
}
*/

void QHtml5Compositor::flush(QHtml5Window *window, const QRegion &region)
{
//    qCDebug(QT_PLATFORM_PEPPER_COMPOSITOR) << "flush" << window << region.boundingRect();

    QHtml5CompositedWindow &compositedWindow = m_compositedWindows[window];
    compositedWindow.flushPending = true;
    compositedWindow.damage = region;
    //maybeComposit();

    requestRedraw();
}

/*
void QHtml5Compositor::waitForFlushed(QHtml5Window *surface)
{
    if (!m_compositedWindows[surface].flushPending)
        return;
}
*/

/*
void QHtml5Compositor::beginResize(QSize newSize, qreal newDevicePixelRatio)
{
//    qCDebug(QT_PLATFORM_PEPPER_COMPOSITOR) << "beginResize" << newSize;

    m_inResize = true;
    m_targetSize = newSize;
    m_targetDevicePixelRatio = newDevicePixelRatio;

    // Delete the current frame buffer to trigger creation of a new one later on.
    delete m_frameBuffer;
    m_frameBuffer = 0;

    requestRedraw();
}

void QHtml5Compositor::endResize()
{
//    qCDebug(QT_PLATFORM_PEPPER_COMPOSITOR) << "endResize";
    m_inResize = false;
    globalDamage = QRect(QPoint(), m_targetSize);
    composit();

    requestRedraw();
}
*/

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

#if 0
void QHtml5Compositor::maybeComposit()
{
    if (m_inResize)
        return; // endResize will composit everything.

    if (!m_inFlush)
        composit();
    else
        m_needComposit = true;
}

void QHtml5Compositor::composit()
{
//    qCDebug(QT_PLATFORM_PEPPER_COMPOSITOR) << "composit" << m_targetSize;

    if (!m_targetSize.isValid())
        return;

    if (!m_frameBuffer) {
        createFrameBuffer();
    }

    QPainter p(m_frameBuffer);
    QRegion painted;

    // Composit all windows in stacking order, paint and flush damaged area only.
    foreach (QHtml5Window *window, m_windowStack) {
        QHtml5CompositedWindow &compositedWindow = m_compositedWindows[window];
//        qCDebug(QT_PLATFORM_PEPPER_COMPOSITOR) << "composit window" << window
//                                               << compositedWindow.frameBuffer;

        if (compositedWindow.visible) {
            const QRect windowGeometry = compositedWindow.window->geometry();
            const QRegion globalDamageForWindow = globalDamage.intersected(QRegion(windowGeometry));
            const QRegion localDamageForWindow = compositedWindow.damage;
            const QRegion totalDamageForWindow = localDamageForWindow + globalDamageForWindow;
            const QRect sourceRect = totalDamageForWindow.boundingRect();
            const QRect destinationRect
                = QRect(windowGeometry.topLeft() + sourceRect.topLeft(), sourceRect.size());
            if (compositedWindow.frameBuffer) {
                p.drawImage(destinationRect, *compositedWindow.frameBuffer, sourceRect);
                painted += destinationRect;
            }
        }

        compositedWindow.flushPending = false;
        compositedWindow.damage = QRect();
    }

//    qCDebug(QT_PLATFORM_PEPPER_COMPOSITOR) << "composit painted" << painted;

    globalDamage = QRect();

    // m_inFlush = true;
    // emit flush(painted);
    if (!painted.isEmpty())
        flush2(painted);
}
#endif // 0

bool QHtml5Compositor::event(QEvent *ev)
{
    if (ev->type() == QEvent::UpdateRequest) {
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
            && qt_window_private(static_cast<QWindow *>(window->window()))->receivedExpose)
        {
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

/*
void QHtml5Compositor::createFrameBuffer()
{
//    qCDebug(QT_PLATFORM_PEPPER_COMPOSITOR) << "createFrameBuffer" << m_targetSize
//                                           << m_targetDevicePixelRatio;

//    delete m_imageData2D;
    delete m_frameBuffer;

    if (!m_targetSize.isValid())
        return;

//    pp::Size devicePixelSize(m_targetSize.width() * m_targetDevicePixelRatio,
//                             m_targetSize.height() * m_targetDevicePixelRatio);

//    pp::Instance *instance = QPepperInstancePrivate::getPPInstance();

    // Create new graphics context and frame buffer.
  //  m_context2D = new pp::Graphics2D(instance, devicePixelSize, false);
//    if (!instance->BindGraphics(*m_context2D)) {
//        qWarning("Couldn't bind the device context\n");
//    }

//    m_imageData2D
//        = new pp::ImageData(instance, PP_IMAGEDATAFORMAT_BGRA_PREMUL, devicePixelSize, true);

//    m_frameBuffer = new QImage(reinterpret_cast<uchar *>(m_imageData2D->data()),
//                               m_targetSize.width() * m_targetDevicePixelRatio,
//                               m_targetSize.height() * m_targetDevicePixelRatio,
//                               m_imageData2D->stride(), QImage::Format_ARGB32_Premultiplied);

    m_frameBuffer->setDevicePixelRatio(m_targetDevicePixelRatio);
}
*/

#if 0
void QHtml5Compositor::flush2(const QRegion &region)
{
//    qCDebug(QT_PLATFORM_PEPPER_COMPOSITOR) << "flush" << region << m_targetDevicePixelRatio;

//    if (!m_context2D) {
//        m_inFlush = false;
//        return;
//    }

    QRect flushRect = region.boundingRect();
    QRect deviceRect = QRect(flushRect.topLeft() * m_targetDevicePixelRatio,
                             flushRect.size() * m_targetDevicePixelRatio);

//    qCDebug(QT_PLATFORM_PEPPER_COMPOSITOR) << "flushing" << flushRect << deviceRect;
  //  m_context2D->PaintImageData(*m_imageData2D, pp::Point(0, 0), toPPRect(deviceRect));
 //   m_context2D->Flush(m_callbackFactory.NewCallback(&QHtml5Compositor::flushCompletedCallback));
    m_inFlush = true;
}

void QHtml5Compositor::flushCompletedCallback(int32_t)
{
    m_inFlush = false;
    if (m_needComposit) {
        composit();
        m_needComposit = false;
    }
}
j
#endif

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

