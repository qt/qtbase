/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qhtml5backingstore.h"
#include "qhtml5window.h"
#include "qhtml5compositor.h"

#include <QtGui/QOpenGLTexture>
#include <QtGui/QMatrix4x4>
#include <QtGui/qpainter.h>
#include <private/qguiapplication_p.h>
#include <qpa/qplatformscreen.h>

#include <QtGui/qbackingstore.h>

QT_BEGIN_NAMESPACE

QHTML5BackingStore::QHTML5BackingStore(QHtml5Compositor *compositor, QWindow *window)
    : QPlatformBackingStore(window)
    , mCompositor(compositor)
    , mTexture(new QOpenGLTexture(QOpenGLTexture::Target2D))
{
    window->setSurfaceType(QSurface::OpenGLSurface);

    if (window->handle())
        (static_cast<QHtml5Window *>(window->handle()))->setBackingStore(this);
    else
        (static_cast<QHTML5Screen *>(window->screen()->handle()))->addPendingBackingStore(this);
}

QHTML5BackingStore::~QHTML5BackingStore()
{
}

QPaintDevice *QHTML5BackingStore::paintDevice()
{
    return &mImage;
}

void QHTML5BackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(window);
    Q_UNUSED(region);
    Q_UNUSED(offset);

    mCompositor->requestRedraw();

    /*
    auto* screen = static_cast<QHTML5Screen *>(window->screen()->handle());

    mContext->makeCurrent(window);

    int dx = window->handle()->winId() == 1 ? 100 : 0;
    int dy = window->handle()->winId() == 1 ? 100 : 0;

    //glViewport(0, 0, screen->geometry().width(), screen->geometry().height());
    glViewport(offset.x() + dx, window->screen()->geometry().height() - offset.y() - window->height() - dy, window->width(), window->height());


    updateTexture();

    if (!mBlitter->isCreated())
        mBlitter->create();

    float x = (float)window->x() / (float)screen->geometry().width();
    float y = 1.0f - (float)window->y() / (float)screen->geometry().height();

    QMatrix4x4 m;
    //m.translate(offset.x(), offset.y());
    //m.translate(-0.5f + 1.0f * (float)(window->handle()->winId() - 1), 0.0f);
    //m.translate(x, y);
    //m.translate(0, y);
    //m.scale(0.5f, 0.5f);

    mBlitter->bind();
    mBlitter->setRedBlueSwizzle(true);
    mBlitter->blit(mTexture->textureId(), m, QOpenGLTextureBlitter::OriginTopLeft);
    mBlitter->release();

    mContext->swapBuffers(window);
    */
}

void QHTML5BackingStore::updateTexture()
{
    if (mDirty.isNull())
        return;

    if (!mTexture->isCreated()) {
        mTexture->setMinificationFilter(QOpenGLTexture::Nearest);
        mTexture->setMagnificationFilter(QOpenGLTexture::Nearest);
        mTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
        mTexture->setData(mImage, QOpenGLTexture::DontGenerateMipMaps);
        mTexture->create();
    }
    mTexture->bind();

    QRegion fixed;
    QRect imageRect = mImage.rect();

    for (const QRect &rect : mDirty) {
        // intersect with image rect to be sure
        QRect r = imageRect & rect;
        // if the rect is wide enough it is cheaper to just extend it instead of doing an image copy
        if (r.width() >= imageRect.width() / 2) {
            r.setX(0);
            r.setWidth(imageRect.width());
        }

        fixed |= r;
    }

    for (const QRect &rect : fixed) {
        // if the sub-rect is full-width we can pass the image data directly to
        // OpenGL instead of copying, since there is no gap between scanlines
        if (rect.width() == imageRect.width()) {
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, rect.y(), rect.width(), rect.height(), GL_RGBA, GL_UNSIGNED_BYTE,
                            mImage.constScanLine(rect.y()));
        } else {
            glTexSubImage2D(GL_TEXTURE_2D, 0, rect.x(), rect.y(), rect.width(), rect.height(), GL_RGBA, GL_UNSIGNED_BYTE,
                            mImage.copy(rect).constBits());
        }
    }
    /* End of code taken from QEGLPlatformBackingStore */

    mDirty = QRegion();
}

void QHTML5BackingStore::beginPaint(const QRegion &region)
{
    mDirty |= region;
    //mContext->makeCurrent(window());
    // Keep backing store device pixel ratio in sync with window
    if (mImage.devicePixelRatio() != window()->devicePixelRatio())
        resize(backingStore()->size(), backingStore()->staticContents());

    QPainter painter(&mImage);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    const QColor blank = Qt::transparent;
    for (const QRect &rect : region)
        painter.fillRect(rect, blank);
}

void QHTML5BackingStore::resize(const QSize &size, const QRegion &staticContents)
{
    Q_UNUSED(staticContents)

    mImage = QImage(size, QImage::Format_RGB32);

    //mContext->makeCurrent(window());

    if (mTexture->isCreated())
        mTexture->destroy();

    /*
    updateTexture();
    */
}

QImage QHTML5BackingStore::toImage() const
{
    // used by QPlatformBackingStore::composeAndFlush
    return mImage;
}

const QImage &QHTML5BackingStore::getImageRef() const
{
    return mImage;
}

const QOpenGLTexture* QHTML5BackingStore::getUpdatedTexture()
{
    updateTexture();
    return mTexture.data();
}

QT_END_NAMESPACE
