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

#include "qpepperbackingstore.h"

#include "qpeppercompositor.h"
#include "qpepperhelpers.h"
#include "qpepperinstance_p.h"
#include "qpepperwindow.h"

#include <QtCore/QDebug>
#include <QtGui/QPainter>
#include <QtGui/QWindow>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(QT_PLATFORM_PEPPER_BACKINGSTORE, "qt.platform.pepper.backingstore")

QPepperBackingStore::QPepperBackingStore(QWindow *window)
    : QPlatformBackingStore(window)
    , m_isInPaint(false)
    , m_isInFlush(false)
    , m_compositor(0)
    , m_context2D(0)
    , m_imageData2D( 0)
    , m_frameBuffer(0)
    , m_ownsFrameBuffer(false)
    , m_callbackFactory(this)
{
    qCDebug(QT_PLATFORM_PEPPER_BACKINGSTORE) << "QPepperBackingStore for" << window;


#if 0
    // Compositor disabled
    m_compositor = QPepperIntegration::getPepperIntegration()->pepperCompositor();
    m_compositor->addRasterWindow(window);
    QPepperWindow *platformWindow = reinterpret_cast<QPepperWindow *>(window->handle());
    platformWindow->setCompositor(m_compositor);
#endif

    resize(window->size(), QRegion());
}

QPepperBackingStore::~QPepperBackingStore()
{
    if (m_ownsFrameBuffer)
        delete m_frameBuffer;
}

QPaintDevice *QPepperBackingStore::paintDevice()
{
    qCDebug(QT_PLATFORM_PEPPER_BACKINGSTORE) << "paintDevice framebuffer" << m_frameBuffer;

    return m_frameBuffer;
}

void QPepperBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    qCDebug(QT_PLATFORM_PEPPER_BACKINGSTORE) << "flush" << window << region.boundingRect()
                                             << offset;

    Q_UNUSED(window);
    Q_UNUSED(offset);

    if (m_compositor) {
        m_compositor->flush(window, region);
    } else {
        QRect flushRect = region.boundingRect();
        QRect deviceRect = QRect(flushRect.topLeft() * m_frameBuffer->devicePixelRatio(),
                                 flushRect.size() * m_frameBuffer->devicePixelRatio());

        m_context2D->PaintImageData(*m_imageData2D, pp::Point(0, 0), toPPRect(deviceRect));
        m_context2D->Flush(
            m_callbackFactory.NewCallback(&QPepperBackingStore::flushCompletedCallback));
        m_isInFlush = true;
    }
}

void QPepperBackingStore::flushCompletedCallback(int32_t) { m_isInFlush = false; }

void QPepperBackingStore::resize(const QSize &size, const QRegion &)
{
    qCDebug(QT_PLATFORM_PEPPER_BACKINGSTORE) << "resize" << size;

    qreal devicePixelRatio = QPepperInstancePrivate::get()->devicePixelRatio();

    if (!m_frameBuffer || size * devicePixelRatio != m_frameBuffer->size()) {
        createFrameBuffer(size, devicePixelRatio);
        m_size = size;
    }
}

void QPepperBackingStore::beginPaint(const QRegion &region)
{
    Q_UNUSED(region);
    qCDebug(QT_PLATFORM_PEPPER_BACKINGSTORE) << "beginPaint" << window();

    m_isInPaint = true;
    if (m_compositor) {
        m_compositor->waitForFlushed(window());
    } else {
        // noop
    }
}

void QPepperBackingStore::endPaint()
{
    qCDebug(QT_PLATFORM_PEPPER_BACKINGSTORE) << "endPaint";

    m_isInPaint = false;
}

void QPepperBackingStore::createFrameBuffer(QSize size, qreal devicePixelRatio)
{
    qCDebug(QT_PLATFORM_PEPPER_BACKINGSTORE) << "createFrameBuffer" << size << devicePixelRatio;

    if (m_compositor) {
        if (m_ownsFrameBuffer)
            delete m_frameBuffer;
        m_frameBuffer = new QImage(size * devicePixelRatio, QImage::Format_ARGB32_Premultiplied);
        m_frameBuffer->setDevicePixelRatio(devicePixelRatio);
        m_ownsFrameBuffer = true;
        m_compositor->setFrameBuffer(window(), m_frameBuffer);
    } else {
        QSize devicePixelSize = size * devicePixelRatio;
        pp::Instance *instance = QPepperInstancePrivate::getPPInstance();

        // Create a new 2D graphics context, an pp::ImageData with the new size
        // plus a QImage that shares the ImageData frame buffer.
        m_context2D = new pp::Graphics2D(instance, toPPSize(devicePixelSize), false);
        m_context2D->SetScale(1.0 / devicePixelRatio);
        if (!instance->BindGraphics(*m_context2D)) {
            qWarning("Couldn't bind the device context\n");
        }
        m_imageData2D = new pp::ImageData(instance, PP_IMAGEDATAFORMAT_BGRA_PREMUL,
                                          toPPSize(devicePixelSize), true);
        m_frameBuffer = new QImage(reinterpret_cast<uchar *>(m_imageData2D->data()),
                                   devicePixelSize.width(), devicePixelSize.height(),
                                   m_imageData2D->stride(), QImage::Format_ARGB32_Premultiplied);
        m_frameBuffer->setDevicePixelRatio(devicePixelRatio);
    }
}

void QPepperBackingStore::setFrameBuffer(QImage *frameBuffer)
{
    qCDebug(QT_PLATFORM_PEPPER_BACKINGSTORE) << "setFrameBuffer" << frameBuffer;

    if (m_isInPaint)
        qFatal("QPepperBackingStore::setFrameBuffer called while painting");

    if (m_ownsFrameBuffer)
        delete m_frameBuffer;
    m_frameBuffer = frameBuffer;
    m_ownsFrameBuffer = false;

    window()->resize(frameBuffer->size());
}

QT_END_NAMESPACE
