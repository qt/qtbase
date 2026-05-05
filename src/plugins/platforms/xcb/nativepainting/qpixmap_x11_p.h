// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

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

    QPlatformPixmap *createCompatiblePlatformPixmap() const override;
    void resize(int width, int height) override;
    void fromImage(const QImage &img, Qt::ImageConversionFlags flags) override;
    void copy(const QPlatformPixmap *data, const QRect &rect) override;
    bool scroll(int dx, int dy, const QRect &rect) override;
    int metric(QPaintDevice::PaintDeviceMetric metric) const override;
    void fill(const QColor &fillColor) override;
    QBitmap mask() const override;
    void setMask(const QBitmap &mask) override;
    bool hasAlphaChannel() const override;
    QPixmap transformed(const QTransform &matrix, Qt::TransformationMode mode) const override;
    QImage toImage() const override;
    QImage toImage(const QRect &rect) const override;
    QPaintEngine *paintEngine() const override;
    qreal devicePixelRatio() const override;
    void setDevicePixelRatio(qreal scaleFactor) override;

    inline Drawable handle() const { return hd; }
    inline Picture x11PictureHandle() const { return picture; }
    inline const QXcbX11Info *x11_info() const { return &xinfo; }

    Pixmap x11ConvertToDefaultDepth();
    static XID createBitmapFromImage(const QImage &image);

#if QT_CONFIG(xrender)
    void convertToARGB32(bool preserveContents = true);
#endif

    bool isBackingStore() const;
    void setIsBackingStore(bool on);
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
        GlSurfaceCreatedWithAlpha = 0x8,
        IsBackingStore = 0x10
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
            : nullptr;
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
