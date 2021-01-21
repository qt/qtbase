/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qminimaleglbackingstore.h"

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLPaintDevice>

QT_BEGIN_NAMESPACE

QMinimalEglBackingStore::QMinimalEglBackingStore(QWindow *window)
    : QPlatformBackingStore(window)
    , m_context(new QOpenGLContext)
    , m_device(0)
{
    m_context->setFormat(window->requestedFormat());
    m_context->setScreen(window->screen());
    m_context->create();
}

QMinimalEglBackingStore::~QMinimalEglBackingStore()
{
    delete m_context;
}

QPaintDevice *QMinimalEglBackingStore::paintDevice()
{
    return m_device;
}

void QMinimalEglBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(region);
    Q_UNUSED(offset);

#ifdef QEGL_EXTRA_DEBUG
    qWarning("QEglBackingStore::flush %p", window);
#endif

    m_context->swapBuffers(window);
}

void QMinimalEglBackingStore::beginPaint(const QRegion &)
{
    m_context->makeCurrent(window());
    m_device = new QOpenGLPaintDevice(window()->size());
}

void QMinimalEglBackingStore::endPaint()
{
    delete m_device;
}

void QMinimalEglBackingStore::resize(const QSize &size, const QRegion &staticContents)
{
    Q_UNUSED(size);
    Q_UNUSED(staticContents);
}

QT_END_NAMESPACE
