// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qminimaleglbackingstore.h"

#include <QtGui/QOpenGLContext>
#include <QtOpenGL/QOpenGLPaintDevice>

QT_BEGIN_NAMESPACE

QMinimalEglBackingStore::QMinimalEglBackingStore(QWindow *window)
    : QPlatformBackingStore(window)
    , m_context(new QOpenGLContext)
    , m_device(nullptr)
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
