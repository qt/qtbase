/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qpepperbackingstore.h"
#include "qpeppercompositor.h"

#include <QtCore/qdebug.h>
#include <QtGui/QPainter>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(QT_PLATFORM_PEPPER_BACKINGSTORE, "qt.platform.pepper.backingstore")

QPepperBackingStore::QPepperBackingStore(QWindow *window)
    : QPlatformBackingStore(window)
{
    qCDebug(QT_PLATFORM_PEPPER_BACKINGSTORE) << "QPepperBackingStore for" << window;

    m_isInPaint = false;
    m_frameBuffer = 0;
    m_compositor = QPepperIntegration::getPepperIntegration()->pepperCompositor();
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
    m_compositor->waitForFlushed(window());
}

void QPepperBackingStore::endPaint()
{
    qCDebug(QT_PLATFORM_PEPPER_BACKINGSTORE) << "endPaint";

    m_isInPaint = false;
}

void QPepperBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    qCDebug(QT_PLATFORM_PEPPER_BACKINGSTORE) << "flush" << window << region.boundingRect() << offset;

    Q_UNUSED(window);
    Q_UNUSED(offset);

    m_compositor->flush(window, region);
}

void QPepperBackingStore::resize(const QSize &size, const QRegion &)
{
    qCDebug(QT_PLATFORM_PEPPER_BACKINGSTORE) << "resize" << size;

    qreal devicePixelRatio = QPepperInstance::get()->devicePixelRatio();

    if (!m_frameBuffer || size * devicePixelRatio != m_frameBuffer->size()) {
        createFrameBuffer(size, devicePixelRatio);
        m_size = size;
    }
}

void QPepperBackingStore::createFrameBuffer(QSize size, qreal devicePixelRatio)
{
    qCDebug(QT_PLATFORM_PEPPER_BACKINGSTORE) << "createFrameBuffer" << size << devicePixelRatio;

    if (m_ownsFrameBuffer)
        delete m_frameBuffer;
    m_frameBuffer = new QImage(size * devicePixelRatio, QImage::Format_ARGB32_Premultiplied);
    m_frameBuffer->setDevicePixelRatio(devicePixelRatio);
    m_ownsFrameBuffer = true;
    m_compositor->setFrameBuffer(window(),  m_frameBuffer);
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
