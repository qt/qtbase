/****************************************************************************
**
** Copyright (C) 2016 Canonical, Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include "qmirclientbackingstore.h"
#include "qmirclientlogging.h"
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLTexture>
#include <QtGui/QMatrix4x4>
#include <QtGui/qopengltextureblitter.h>
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
    mContext->makeCurrent(window()); // needed as QOpenGLTexture destructor assumes current context
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


void QMirClientBackingStore::beginPaint(const QRegion& region)
{
    mDirty |= region;
}

void QMirClientBackingStore::resize(const QSize& size, const QRegion& /*staticContents*/)
{
    mImage = QImage(size, QImage::Format_RGBA8888);

    mContext->makeCurrent(window());

    if (mTexture->isCreated())
        mTexture->destroy();
}

QPaintDevice* QMirClientBackingStore::paintDevice()
{
    return &mImage;
}

QImage QMirClientBackingStore::toImage() const
{
    // used by QPlatformBackingStore::composeAndFlush
    return mImage;
}
