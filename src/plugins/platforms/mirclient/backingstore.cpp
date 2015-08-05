/*
 * Copyright (C) 2014 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,
 * SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "backingstore.h"
#include "logging.h"
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLTexture>
#include <QtGui/QMatrix4x4>
#include <QtGui/private/qopengltextureblitter_p.h>
#include <QtGui/qopenglfunctions.h>

UbuntuBackingStore::UbuntuBackingStore(QWindow* window)
    : QPlatformBackingStore(window)
    , mContext(new QOpenGLContext)
    , mTexture(new QOpenGLTexture(QOpenGLTexture::Target2D))
    , mBlitter(new QOpenGLTextureBlitter)
{
    mContext->setFormat(window->requestedFormat());
    mContext->setScreen(window->screen());
    mContext->create();

    window->setSurfaceType(QSurface::OpenGLSurface);
}

UbuntuBackingStore::~UbuntuBackingStore()
{
}

void UbuntuBackingStore::flush(QWindow* window, const QRegion& region, const QPoint& offset)
{
    Q_UNUSED(region);
    Q_UNUSED(offset);
    mContext->makeCurrent(window);
    glViewport(0, 0, window->width(), window->height());

    updateTexture();

    if (!mBlitter->isCreated())
        mBlitter->create();

    mBlitter->bind();
    mBlitter->setSwizzleRB(true);
    mBlitter->blit(mTexture->textureId(), QMatrix4x4(), QOpenGLTextureBlitter::OriginTopLeft);
    mBlitter->release();

    mContext->swapBuffers(window);
}

void UbuntuBackingStore::updateTexture()
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

    /* Following code taken from QEGLPlatformBackingStore under the terms of the Lesser GPL v2.1 licence
     * Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies). */
    Q_FOREACH (const QRect &rect, mDirty.rects()) {
        // intersect with image rect to be sure
        QRect r = imageRect & rect;

        // if the rect is wide enough it is cheaper to just extend it instead of doing an image copy
        if (r.width() >= imageRect.width() / 2) {
            r.setX(0);
            r.setWidth(imageRect.width());
        }

        fixed |= r;
    }

    Q_FOREACH (const QRect &rect, fixed.rects()) {
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


void UbuntuBackingStore::beginPaint(const QRegion& region)
{
    mDirty |= region;
}

void UbuntuBackingStore::resize(const QSize& size, const QRegion& /*staticContents*/)
{
    mImage = QImage(size, QImage::Format_RGB32);

    if (mTexture->isCreated())
        mTexture->destroy();
}

QPaintDevice* UbuntuBackingStore::paintDevice()
{
    return &mImage;
}
