/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Native Client platform plugin.
**
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
**
****************************************************************************/

#include "qpeppercompositor.h"

#include "qpepperhelpers.h"
#include "qpepperinstance.h"
#include "qpepperinstance_p.h"

#include <QtGui>
#include <qpa/qwindowsysteminterface.h>

#include <ppapi/cpp/graphics_2d.h>
#include <ppapi/cpp/image_data.h>
#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/var.h>

using namespace pp;

QPepperCompositedWindow::QPepperCompositedWindow()
    : window(0)
    , frameBuffer(0)
    , parentWindow(0)
    , flushPending(false)
    , visible(false)
{
}

QPepperCompositor::QPepperCompositor()
    : m_frameBuffer(0)
    , m_context2D(0)
    , m_imageData2D(0)
    , m_needComposit(false)
    , m_inFlush(false)
    , m_inResize(false)
    , m_targetDevicePixelRatio(1)
{
    m_callbackFactory.Initialize(this);
}

QPepperCompositor::~QPepperCompositor()
{
    delete m_context2D;
    delete m_imageData2D;
    delete m_frameBuffer;
}

Q_LOGGING_CATEGORY(QT_PLATFORM_PEPPER_COMPOSITOR, "qt.platform.pepper.compositor")

void QPepperCompositor::addRasterWindow(QWindow *window, QWindow *parentWindow)
{
    qCDebug(QT_PLATFORM_PEPPER_COMPOSITOR) << "addRasterWindow" << window << parentWindow;

    QPepperCompositedWindow compositedWindow;
    compositedWindow.window = window;
    compositedWindow.parentWindow = parentWindow;
    m_compositedWindows.insert(window, compositedWindow);

    if (parentWindow == 0) {
        m_windowStack.append(window);
    } else {
        m_compositedWindows[parentWindow].childWindows.append(window);
    }
}

void QPepperCompositor::removeWindow(QWindow *window)
{
    qCDebug(QT_PLATFORM_PEPPER_COMPOSITOR) << "removeWindow" << window;

    QWindow *platformWindow = m_compositedWindows[window].parentWindow;
    if (platformWindow) {
        QWindow *parentWindow = window;
        m_compositedWindows[parentWindow].childWindows.removeAll(window);
    }
    m_windowStack.removeAll(window);
    m_compositedWindows.remove(window);
}

void QPepperCompositor::setVisible(QWindow *window, bool visible)
{
    QPepperCompositedWindow &compositedWindow = m_compositedWindows[window];
    if (compositedWindow.visible == visible)
        return;

    qCDebug(QT_PLATFORM_PEPPER_COMPOSITOR) << "setVisible (effective)" << window << visible;

    compositedWindow.visible = visible;
    compositedWindow.flushPending = true;
    if (visible)
        compositedWindow.damage = compositedWindow.window->geometry();
    else
        globalDamage = compositedWindow.window->geometry(); // repaint previosly covered area.
    maybeComposit();
}

void QPepperCompositor::raise(QWindow *window)
{
    qCDebug(QT_PLATFORM_PEPPER_COMPOSITOR) << "raise" << window;

    QPepperCompositedWindow &compositedWindow = m_compositedWindows[window];
    compositedWindow.damage = compositedWindow.window->geometry();
    m_windowStack.removeAll(window);
    m_windowStack.append(window);
    maybeComposit();
}

void QPepperCompositor::lower(QWindow *window)
{
    qCDebug(QT_PLATFORM_PEPPER_COMPOSITOR) << "lower" << window;

    m_windowStack.removeAll(window);
    m_windowStack.prepend(window);
    QPepperCompositedWindow &compositedWindow = m_compositedWindows[window];
    globalDamage = compositedWindow.window->geometry(); // repaint previosly covered area.
}

void QPepperCompositor::setParent(QWindow *window, QWindow *parent)
{
    m_compositedWindows[window].parentWindow = parent;
}

void QPepperCompositor::setFrameBuffer(QWindow *window, QImage *frameBuffer)
{
    qCDebug(QT_PLATFORM_PEPPER_COMPOSITOR) << "setFrameBuffer" << window << frameBuffer;

    // QPepperBackingStore instances may be created before the corresponding QPepperWindow
    // instance, in which case this call comes to early. This will be rectified when the
    // QPepperWindow is created, at which point the backing store resizes the frame buffer.
    if (!m_compositedWindows.contains(window))
        return;

    m_compositedWindows[window].frameBuffer = frameBuffer;
    m_compositedWindows[window].frameBuffer->fill(Qt::blue);
}

void QPepperCompositor::flush(QWindow *window, const QRegion &region)
{
    qCDebug(QT_PLATFORM_PEPPER_COMPOSITOR) << "flush" << window << region.boundingRect();

    QPepperCompositedWindow &compositedWindow = m_compositedWindows[window];
    compositedWindow.flushPending = true;
    compositedWindow.damage = region;
    maybeComposit();
}

void QPepperCompositor::waitForFlushed(QWindow *surface)
{
    if (!m_compositedWindows[surface].flushPending)
        return;
}

void QPepperCompositor::beginResize(QSize newSize, qreal newDevicePixelRatio)
{
    qCDebug(QT_PLATFORM_PEPPER_COMPOSITOR) << "beginResize" << newSize;

    m_inResize = true;
    m_targetSize = newSize;
    m_targetDevicePixelRatio = newDevicePixelRatio;

    // Delete the current frame buffer to trigger creation of a new one later on.
    delete m_frameBuffer;
    m_frameBuffer = 0;
}

void QPepperCompositor::endResize()
{
    qCDebug(QT_PLATFORM_PEPPER_COMPOSITOR) << "endResize";
    m_inResize = false;
    globalDamage = QRect(QPoint(), m_targetSize);
    composit();
}

QWindow *QPepperCompositor::windowAt(QPoint p)
{
    int index = m_windowStack.count() - 1;
    // qDebug() << "window at" << "point" << p << "window count" << index;

    while (index >= 0) {
        QPepperCompositedWindow &compositedWindow = m_compositedWindows[m_windowStack.at(index)];
        // qDebug() << "windwAt testing" << compositedWindow.window <<
        // compositedWindow.window->geometry();
        if (compositedWindow.visible && compositedWindow.window->geometry().contains(p))
            return m_windowStack.at(index);
        --index;
    }
    return 0;
}

QWindow *QPepperCompositor::keyWindow() { return m_windowStack.at(m_windowStack.count() - 1); }

void QPepperCompositor::maybeComposit()
{
    if (m_inResize)
        return; // endResize will composit everything.

    if (!m_inFlush)
        composit();
    else
        m_needComposit = true;
}

void QPepperCompositor::composit()
{
    qCDebug(QT_PLATFORM_PEPPER_COMPOSITOR) << "composit" << m_targetSize;

    if (!m_targetSize.isValid())
        return;

    if (!m_frameBuffer) {
        createFrameBuffer();
    }

    QPainter p(m_frameBuffer);
    QRegion painted;

    // Composit all windows in stacking order, paint and flush damaged area only.
    foreach (QWindow *window, m_windowStack) {
        QPepperCompositedWindow &compositedWindow = m_compositedWindows[window];
        qCDebug(QT_PLATFORM_PEPPER_COMPOSITOR) << "composit window" << window
                                               << compositedWindow.frameBuffer;

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

    qCDebug(QT_PLATFORM_PEPPER_COMPOSITOR) << "composit painted" << painted;

    globalDamage = QRect();

    // m_inFlush = true;
    // emit flush(painted);
    if (!painted.isEmpty())
        flush2(painted);
}

void QPepperCompositor::createFrameBuffer()
{
    qCDebug(QT_PLATFORM_PEPPER_COMPOSITOR) << "createFrameBuffer" << m_targetSize
                                           << m_targetDevicePixelRatio;

    delete m_imageData2D;
    delete m_frameBuffer;

    if (!m_targetSize.isValid())
        return;

    Size devicePixelSize(m_targetSize.width() * m_targetDevicePixelRatio,
                         m_targetSize.height() * m_targetDevicePixelRatio);

    QPepperInstance *instance = QPepperInstancePrivate::getInstance();

    // Create new graphics context and frame buffer.
    m_context2D = new Graphics2D(instance, devicePixelSize, false);
    if (!instance->BindGraphics(*m_context2D)) {
        qWarning("Couldn't bind the device context\n");
    }

    m_imageData2D = new ImageData(instance, PP_IMAGEDATAFORMAT_BGRA_PREMUL, devicePixelSize, true);

    m_frameBuffer = new QImage(reinterpret_cast<uchar *>(m_imageData2D->data()),
                               m_targetSize.width() * m_targetDevicePixelRatio,
                               m_targetSize.height() * m_targetDevicePixelRatio,
                               m_imageData2D->stride(), QImage::Format_ARGB32_Premultiplied);

    m_frameBuffer->setDevicePixelRatio(m_targetDevicePixelRatio);
}

void QPepperCompositor::flush2(const QRegion &region)
{
    qCDebug(QT_PLATFORM_PEPPER_COMPOSITOR) << "flush" << region << m_targetDevicePixelRatio;

    if (!m_context2D) {
        m_inFlush = false;
        return;
    }

    QRect flushRect = region.boundingRect();
    QRect deviceRect = QRect(flushRect.topLeft() * m_targetDevicePixelRatio,
                             flushRect.size() * m_targetDevicePixelRatio);

    qCDebug(QT_PLATFORM_PEPPER_COMPOSITOR) << "flushing" << flushRect << deviceRect;
    m_context2D->PaintImageData(*m_imageData2D, pp::Point(0, 0), toPPRect(deviceRect));
    m_context2D->Flush(m_callbackFactory.NewCallback(&QPepperCompositor::flushCompletedCallback));
    m_inFlush = true;
}

void QPepperCompositor::flushCompletedCallback(int32_t)
{
    m_inFlush = false;
    if (m_needComposit) {
        composit();
        m_needComposit = false;
    }
}
