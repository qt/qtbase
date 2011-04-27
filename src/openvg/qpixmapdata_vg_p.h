/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtOpenVG module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QPIXMAPDATA_VG_P_H
#define QPIXMAPDATA_VG_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qpixmap_raster_p.h>
#include <QtGui/private/qvolatileimage_p.h>
#include "qvg_p.h"

#if defined(Q_OS_SYMBIAN)
class RSGImage;
#endif

QT_BEGIN_NAMESPACE

class QEglContext;
class QVGImagePool;
class QImageReader;

#if !defined(QT_NO_EGL)
class QVGPixmapData;
class QVGSharedContext;

void qt_vg_register_pixmap(QVGPixmapData *pd);
void qt_vg_unregister_pixmap(QVGPixmapData *pd);
void qt_vg_hibernate_pixmaps(QVGSharedContext *context);
#endif

class QNativeImageHandleProvider;

class Q_OPENVG_EXPORT QVGPixmapData : public QPixmapData
{
public:
    QVGPixmapData(PixelType type);
    ~QVGPixmapData();

    QPixmapData *createCompatiblePixmapData() const;

    // Is this pixmap valid (i.e. non-zero in size)?
    bool isValid() const;

    void resize(int width, int height);
    void fromImage(const QImage &image, Qt::ImageConversionFlags flags);
    void fromImageReader(QImageReader *imageReader,
                          Qt::ImageConversionFlags flags);
    bool fromFile(const QString &filename, const char *format,
                          Qt::ImageConversionFlags flags);
    bool fromData(const uchar *buffer, uint len, const char *format,
                          Qt::ImageConversionFlags flags);

    void fill(const QColor &color);
    bool hasAlphaChannel() const;
    void setAlphaChannel(const QPixmap &alphaChannel);
    QImage toImage() const;
    void copy(const QPixmapData *data, const QRect &rect);
    QImage *buffer();
    QPaintEngine* paintEngine() const;

    // Return the VGImage form of this pixmap, creating it if necessary.
    // This assumes that there is a VG context current.
    virtual VGImage toVGImage();

    // Return the VGImage form for a specific opacity setting.
    virtual VGImage toVGImage(qreal opacity);

    // Detach this image from the image pool.
    virtual void detachImageFromPool();

    // Release the VG resources associated with this pixmap and copy
    // the pixmap's contents out of the GPU back into main memory.
    // The VG resource will be automatically recreated the next time
    // toVGImage() is called.  Does nothing if the pixmap cannot be
    // hibernated for some reason (e.g. VGImage is shared with another
    // process via a SgImage).
    virtual void hibernate();

    // Called when the QVGImagePool wants to reclaim this pixmap's
    // VGImage objects to reuse storage.
    virtual void reclaimImages();

    // If vgImage is valid but source is null, copies pixel data from GPU back
    // into main memory and destroys vgImage. For a normal pixmap this function
    // does nothing, however if the pixmap was created directly from a VGImage
    // (e.g. via SgImage on Symbian) then by doing the readback this ensures
    // that QImage-based functions can operate too.
    virtual void ensureReadback(bool readOnly) const;

    QSize size() const { return QSize(w, h); }

#if defined(Q_OS_SYMBIAN)
    void* toNativeType(NativeType type);
    void fromNativeType(void* pixmap, NativeType type);
    bool initFromNativeImageHandle(void *handle, const QString &type);
    void createFromNativeImageHandleProvider();
    void releaseNativeImageHandle();
#endif

protected:
    int metric(QPaintDevice::PaintDeviceMetric metric) const;
    void createPixmapForImage(QImage &image, Qt::ImageConversionFlags flags, bool inPlace);

#if defined(Q_OS_SYMBIAN)
    void cleanup();
#endif

private:
    QVGPixmapData *nextLRU;
    QVGPixmapData *prevLRU;
    bool inLRU;
    bool failedToAlloc;
    friend class QVGImagePool;
    friend class QVGPaintEngine;

#if !defined(QT_NO_EGL)
    QVGPixmapData *next;
    QVGPixmapData *prev;

    friend void qt_vg_register_pixmap(QVGPixmapData *pd);
    friend void qt_vg_unregister_pixmap(QVGPixmapData *pd);
    friend void qt_vg_hibernate_pixmaps(QVGSharedContext *context);
#endif

protected:
    QSize prevSize;
    VGImage vgImage;
    VGImage vgImageOpacity;
    qreal cachedOpacity;
    mutable QVolatileImage source;
    mutable bool recreate;
    bool inImagePool;
#if !defined(QT_NO_EGL)
    mutable QEglContext *context;
#endif

#if defined(Q_OS_SYMBIAN)
    mutable QNativeImageHandleProvider *nativeImageHandleProvider;
    void *nativeImageHandle;
    QString nativeImageType;
#endif

    void forceToImage(bool allowReadback = true);
    QImage::Format sourceFormat() const;
    QImage::Format idealFormat(QImage *image, Qt::ImageConversionFlags flags) const;
    void updateSerial();

    void destroyImageAndContext();
    void destroyImages();
};

QT_END_NAMESPACE

#endif
