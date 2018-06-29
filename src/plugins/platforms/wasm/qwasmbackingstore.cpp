/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "qwasmbackingstore.h"
#include "qwasmwindow.h"
#include "qwasmcompositor.h"

#include <QtGui/QOpenGLTexture>
#include <QtGui/QMatrix4x4>
#include <QtGui/qpainter.h>
#include <private/qguiapplication_p.h>
#include <qpa/qplatformscreen.h>

#include <QtGui/qbackingstore.h>

QT_BEGIN_NAMESPACE

QWasmBackingStore::QWasmBackingStore(QWasmCompositor *compositor, QWindow *window)
    : QPlatformBackingStore(window)
    , mCompositor(compositor)
    , mTexture(new QOpenGLTexture(QOpenGLTexture::Target2D))
{
    QWasmWindow *wasmWindow = static_cast<QWasmWindow *>(window->handle());
    if (wasmWindow)
        wasmWindow->setBackingStore(this);
}

QWasmBackingStore::~QWasmBackingStore()
{
}

QPaintDevice *QWasmBackingStore::paintDevice()
{
    return &mImage;
}

void QWasmBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(window);
    Q_UNUSED(region);
    Q_UNUSED(offset);

    mDirty |= region;
    mCompositor->requestRedraw();
}

void QWasmBackingStore::updateTexture()
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

        // Convert device-independent dirty region to device region
        qreal dpr = mImage.devicePixelRatio();
        QRect deviceRect = QRect(rect.topLeft() * dpr, rect.size() * dpr);

        // intersect with image rect to be sure
        QRect r = imageRect & deviceRect;
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

void QWasmBackingStore::beginPaint(const QRegion &region)
{
    mDirty |= region;
    // Keep backing store device pixel ratio in sync with window
    if (mImage.devicePixelRatio() != window()->devicePixelRatio())
        resize(backingStore()->size(), backingStore()->staticContents());

    QPainter painter(&mImage);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    const QColor blank = Qt::transparent;
    for (const QRect &rect : region)
        painter.fillRect(rect, blank);
}

void QWasmBackingStore::resize(const QSize &size, const QRegion &staticContents)
{
    Q_UNUSED(staticContents)

    mImage = QImage(size * window()->devicePixelRatio(), QImage::Format_RGB32);
    mImage.setDevicePixelRatio(window()->devicePixelRatio());

    if (mTexture->isCreated())
        mTexture->destroy();
}

QImage QWasmBackingStore::toImage() const
{
    // used by QPlatformBackingStore::composeAndFlush
    return mImage;
}

const QImage &QWasmBackingStore::getImageRef() const
{
    return mImage;
}

const QOpenGLTexture* QWasmBackingStore::getUpdatedTexture()
{
    updateTexture();
    return mTexture.data();
}

QT_END_NAMESPACE
