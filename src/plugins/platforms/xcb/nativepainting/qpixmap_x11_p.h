/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
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
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QX11PLATFORMPIXMAP_H
#define QX11PLATFORMPIXMAP_H

#include <QBitmap>
#include <QPixmap>

#include <qpa/qplatformpixmap.h>
#include "qxcbnativepainting.h"

typedef unsigned long XID;
typedef XID Drawable;
typedef XID Picture;
typedef XID Pixmap;

QT_BEGIN_NAMESPACE

class QX11PaintEngine;
struct QXImageWrapper;

class QX11PlatformPixmap : public QPlatformPixmap
{
public:
    QX11PlatformPixmap(PixelType pixelType);
    ~QX11PlatformPixmap();

    QPlatformPixmap *createCompatiblePlatformPixmap() const Q_DECL_OVERRIDE;
    void resize(int width, int height) Q_DECL_OVERRIDE;
    void fromImage(const QImage &img, Qt::ImageConversionFlags flags) Q_DECL_OVERRIDE;
    void copy(const QPlatformPixmap *data, const QRect &rect) Q_DECL_OVERRIDE;
    bool scroll(int dx, int dy, const QRect &rect) Q_DECL_OVERRIDE;
    int metric(QPaintDevice::PaintDeviceMetric metric) const Q_DECL_OVERRIDE;
    void fill(const QColor &fillColor) Q_DECL_OVERRIDE;
    QBitmap mask() const Q_DECL_OVERRIDE;
    void setMask(const QBitmap &mask) Q_DECL_OVERRIDE;
    bool hasAlphaChannel() const Q_DECL_OVERRIDE;
    QPixmap transformed(const QTransform &matrix, Qt::TransformationMode mode) const Q_DECL_OVERRIDE;
    QImage toImage() const Q_DECL_OVERRIDE;
    QImage toImage(const QRect &rect) const Q_DECL_OVERRIDE;
    QPaintEngine *paintEngine() const Q_DECL_OVERRIDE;
    qreal devicePixelRatio() const Q_DECL_OVERRIDE;
    void setDevicePixelRatio(qreal scaleFactor) Q_DECL_OVERRIDE;

    inline Drawable handle() const { return hd; }
    inline Picture x11PictureHandle() const { return picture; }
    inline const QXcbX11Info *x11_info() const { return &xinfo; }

    Pixmap x11ConvertToDefaultDepth();
    static XID createBitmapFromImage(const QImage &image);

#if QT_CONFIG(xrender)
    void convertToARGB32(bool preserveContents = true);
#endif

private:
    friend class QX11PaintEngine;
    friend const QXcbX11Info &qt_x11Info(const QPixmap &pixmap);
    friend void qt_x11SetScreen(QPixmap &pixmap, int screen);

    void release();
    QImage toImage(const QXImageWrapper &xi, const QRect &rect) const;
    QBitmap mask_to_bitmap(int screen) const;
    static Pixmap bitmap_to_mask(const QBitmap &, int screen);
    void bitmapFromImage(const QImage &image);
    bool canTakeQImageFromXImage(const QXImageWrapper &xi) const;
    QImage takeQImageFromXImage(const QXImageWrapper &xi) const;

    Pixmap hd = 0;

    enum Flag {
        NoFlags = 0x0,
        Uninitialized = 0x1,
        Readonly = 0x2,
        InvertedWhenBoundToTexture = 0x4,
        GlSurfaceCreatedWithAlpha = 0x8
    };
    uint flags;

    QXcbX11Info xinfo;
    Pixmap x11_mask;
    Picture picture;
    Picture mask_picture;
    Pixmap hd2; // sorted in the default display depth
    //QPixmap::ShareMode share_mode;
    qreal dpr;

    QX11PaintEngine *pengine;
};

inline QX11PlatformPixmap *qt_x11Pixmap(const QPixmap &pixmap)
{
    return (pixmap.handle() && pixmap.handle()->classId() == QPlatformPixmap::X11Class)
            ? static_cast<QX11PlatformPixmap *>(pixmap.handle())
            : Q_NULLPTR;
}

inline Picture qt_x11PictureHandle(const QPixmap &pixmap)
{
    if (QX11PlatformPixmap *pm = qt_x11Pixmap(pixmap))
        return pm->x11PictureHandle();

    return 0;
}

inline Pixmap qt_x11PixmapHandle(const QPixmap &pixmap)
{
    if (QX11PlatformPixmap *pm = qt_x11Pixmap(pixmap))
        return pm->handle();

    return 0;
}

inline const QXcbX11Info &qt_x11Info(const QPixmap &pixmap)
{
    if (QX11PlatformPixmap *pm = qt_x11Pixmap(pixmap)) {
        return pm->xinfo;
    } else {
        static QXcbX11Info nullX11Info;
        return nullX11Info;
    }
}

int qt_x11SetDefaultScreen(int screen);
void qt_x11SetScreen(QPixmap &pixmap, int screen);

QT_END_NAMESPACE

#endif // QX11PLATFORMPIXMAP_H
