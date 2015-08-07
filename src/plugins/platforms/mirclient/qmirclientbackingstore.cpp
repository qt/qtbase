/****************************************************************************
**
** Copyright (C) 2014 Canonical, Ltd.
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


#include "qmirclientbackingstore.h"
#include "qmirclientlogging.h"
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLTexture>
#include <QtGui/QMatrix4x4>
#include <QtGui/private/qopengltextureblitter_p.h>
#include <QtGui/qopenglfunctions.h>

QMirClientBackingStore::QMirClientBackingStore(QWindow* window)
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

QMirClientBackingStore::~QMirClientBackingStore()
{
}

void QMirClientBackingStore::flush(QWindow* window, const QRegion& region, const QPoint& offset)
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

void QMirClientBackingStore::updateTexture()
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


void QMirClientBackingStore::beginPaint(const QRegion& region)
{
    mDirty |= region;
}

void QMirClientBackingStore::resize(const QSize& size, const QRegion& /*staticContents*/)
{
    mImage = QImage(size, QImage::Format_RGB32);

    if (mTexture->isCreated())
        mTexture->destroy();
}

QPaintDevice* QMirClientBackingStore::paintDevice()
{
    return &mImage;
}
