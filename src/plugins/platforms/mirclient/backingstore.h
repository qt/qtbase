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

#ifndef UBUNTU_BACKING_STORE_H
#define UBUNTU_BACKING_STORE_H

#include <qpa/qplatformbackingstore.h>

class QOpenGLContext;
class QOpenGLTexture;
class QOpenGLTextureBlitter;

class UbuntuBackingStore : public QPlatformBackingStore
{
public:
    UbuntuBackingStore(QWindow* window);
    virtual ~UbuntuBackingStore();

    // QPlatformBackingStore methods.
    void beginPaint(const QRegion&) override;
    void flush(QWindow* window, const QRegion& region, const QPoint& offset) override;
    void resize(const QSize& size, const QRegion& staticContents) override;
    QPaintDevice* paintDevice() override;

protected:
    void updateTexture();

private:
    QScopedPointer<QOpenGLContext> mContext;
    QScopedPointer<QOpenGLTexture> mTexture;
    QScopedPointer<QOpenGLTextureBlitter> mBlitter;
    QImage mImage;
    QRegion mDirty;
};

#endif // UBUNTU_BACKING_STORE_H
