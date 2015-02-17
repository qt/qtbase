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

#include "qpepperbackingstore.h"
#include "qpeppercompositor.h"
#include "qpepperwindow.h"
#include "qpepperinstance.h"
#include "qpepperinstance_p.h"

#include <QtCore/qdebug.h>
#include <QtGui/QPainter>
#include <QtGui/QWindow>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(QT_PLATFORM_PEPPER_BACKINGSTORE, "qt.platform.pepper.backingstore")

QPepperBackingStore::QPepperBackingStore(QWindow *window)
    : QPlatformBackingStore(window)
    , m_callbackFactory(this)
{
    qCDebug(QT_PLATFORM_PEPPER_BACKINGSTORE) << "QPepperBackingStore for" << window;

    m_isInPaint = false;
    m_compositor = 0;
    m_context2D = 0;
    m_imageData2D = 0;
    m_frameBuffer = 0;
    m_ownsFrameBuffer = false;

#if 0
    // Compositor disabled
    m_compositor = QPepperIntegration::getPepperIntegration()->pepperCompositor();
    m_compositor->addRasterWindow(window);
    QPepperPlatformWindow *platformWindow = reinterpret_cast<QPepperPlatformWindow *>(window->handle());
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

void QPepperBackingStore::beginPaint(const QRegion &region)
{
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
        QPepperInstance *instance = QPepperInstancePrivate::getInstance();

        // Create a new 2D graphics context, an pp::ImageData with the new size
        // plus a QImage that shares the ImageData frame buffer.
        m_context2D = new pp::Graphics2D(instance, toPPSize(devicePixelSize), false);
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
        qFatal("QPepperBackingStore::setFrameBuffer called.");

    if (m_ownsFrameBuffer)
        delete m_frameBuffer;
    m_frameBuffer = frameBuffer;
    m_ownsFrameBuffer = false;

    window()->resize(frameBuffer->size());
}

QT_END_NAMESPACE
