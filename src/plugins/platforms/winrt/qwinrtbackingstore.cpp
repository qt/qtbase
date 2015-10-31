/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwinrtbackingstore.h"

#include "qwinrtscreen.h"
#include "qwinrtwindow.h"
#include "qwinrteglcontext.h"
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFramebufferObject>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

QT_BEGIN_NAMESPACE

class QWinRTBackingStorePrivate
{
public:
    bool initialized;
    QSize size;
    QScopedPointer<QOpenGLContext> context;
    QScopedPointer<QOpenGLFramebufferObject> fbo;
    QWinRTScreen *screen;
    QImage paintDevice;
};

QWinRTBackingStore::QWinRTBackingStore(QWindow *window)
    : QPlatformBackingStore(window), d_ptr(new QWinRTBackingStorePrivate)
{
    Q_D(QWinRTBackingStore);

    d->initialized = false;
    d->screen = static_cast<QWinRTScreen*>(window->screen()->handle());

    if (window->surfaceType() == QSurface::RasterSurface)
        window->setSurfaceType(QSurface::OpenGLSurface);
}

bool QWinRTBackingStore::initialize()
{
    Q_D(QWinRTBackingStore);

    if (d->initialized)
        return true;

    d->context.reset(new QOpenGLContext);
    QSurfaceFormat format = window()->requestedFormat();
    d->context->setFormat(format);
    d->context->setScreen(window()->screen());
    if (!d->context->create())
        return false;

    if (!d->context->makeCurrent(window()))
        return false;

    d->context->doneCurrent();
    d->initialized = true;
    return true;
}

QWinRTBackingStore::~QWinRTBackingStore()
{
}

QPaintDevice *QWinRTBackingStore::paintDevice()
{
    Q_D(QWinRTBackingStore);
    return &d->paintDevice;
}

void QWinRTBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    Q_D(QWinRTBackingStore);
    Q_UNUSED(offset)

    if (d->size.isEmpty())
        return;

    const bool ok = d->context->makeCurrent(window);
    if (!ok)
        qWarning("unable to flush");

    const QRect bounds = region.boundingRect();
    glBindTexture(GL_TEXTURE_2D, d->fbo->texture());
    // TODO: when ANGLE GLES3 support is finished, use the glPixelStorei functions to minimize upload
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, bounds.y(), d->size.width(), bounds.height(),
                    GL_RGBA, GL_UNSIGNED_BYTE, d->paintDevice.constScanLine(bounds.y()));
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_READ_FRAMEBUFFER_ANGLE, d->fbo->handle());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER_ANGLE, 0);
    const int y1 = bounds.y();
    const int y2 = y1 + bounds.height();
    const int x1 = bounds.x();
    const int x2 = x1 + bounds.width();
    glBlitFramebufferANGLE(x1, y1, x2, y2,
                      x1, d->size.height() - y1, x2, d->size.height() - y2,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);

    d->context->swapBuffers(window);
    d->context->doneCurrent();
}

void QWinRTBackingStore::resize(const QSize &size, const QRegion &staticContents)
{
    Q_D(QWinRTBackingStore);
    Q_UNUSED(staticContents)

    if (!initialize())
        return;

    if (d->size == size)
        return;

    d->size = size;
    if (d->size.isEmpty())
        return;

    d->paintDevice = QImage(d->size, QImage::Format_RGBA8888_Premultiplied);

    const bool ok = d->context->makeCurrent(window());
    if (!ok)
        qWarning("unable to resize");

    d->fbo.reset(new QOpenGLFramebufferObject(d->size));

    d->context->doneCurrent();
}

QImage QWinRTBackingStore::toImage() const
{
    Q_D(const QWinRTBackingStore);
    return d->paintDevice;
}

void QWinRTBackingStore::beginPaint(const QRegion &region)
{
    resize(window()->size(), region);
}

void QWinRTBackingStore::endPaint()
{
}

QT_END_NAMESPACE
