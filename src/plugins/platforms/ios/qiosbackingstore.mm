/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qiosbackingstore.h"
#include "qioswindow.h"

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLPaintDevice>

#include <QtDebug>

QIOSBackingStore::QIOSBackingStore(QWindow *window)
    : QPlatformBackingStore(window)
    , m_context(new QOpenGLContext)
    , m_device(0)
{
    m_context->setFormat(window->requestedFormat());
    m_context->setScreen(window->screen());
    m_context->create();
}

QIOSBackingStore::~QIOSBackingStore()
{
    delete m_context;
    delete m_device;
}

void QIOSBackingStore::beginPaint(const QRegion &)
{
    // Needed to prevent QOpenGLContext::makeCurrent() from failing
    window()->setSurfaceType(QSurface::OpenGLSurface);

    m_context->makeCurrent(window());
}

QPaintDevice *QIOSBackingStore::paintDevice()
{
    if (!m_device)
        m_device = new QOpenGLPaintDevice(window()->size());

    return m_device;
}

void QIOSBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(region);
    Q_UNUSED(offset);

    m_context->swapBuffers(window);
}

void QIOSBackingStore::endPaint()
{
    // Calling makeDone() on the context here would be an option,
    // but is not needed, and would actually add some overhead.
}

void QIOSBackingStore::resize(const QSize &size, const QRegion &staticContents)
{
    Q_UNUSED(size);
    Q_UNUSED(staticContents);

    // We don't need to resize the QOpenGLPaintDevice, as it's just a thin wrapper
    // around the OpenGL paint-engine, and the real geometry is handled by the
    // context and window.
}

QT_END_NAMESPACE
