/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QPIXMAPDATA_S60_P_H
#define QPIXMAPDATA_S60_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qpixmap_raster_p.h>

QT_BEGIN_NAMESPACE

class CFbsBitmap;
class CFbsBitmapDevice;
class CFbsBitGc;

class QSymbianBitmapDataAccess;

class QSymbianFbsHeapLock
{
public:

    enum LockAction {
        Unlock
    };

    explicit QSymbianFbsHeapLock(LockAction a);
    ~QSymbianFbsHeapLock();
    void relock();

private:

    LockAction action;
    bool wasLocked;
};

class QS60PlatformPixmap : public QRasterPlatformPixmap
{
public:
    QS60PlatformPixmap(PixelType type);
    ~QS60PlatformPixmap();

    QPlatformPixmap *createCompatiblePlatformPixmap() const;

    void resize(int width, int height);
    void fromImage(const QImage &image, Qt::ImageConversionFlags flags);
    void copy(const QPlatformPixmap *data, const QRect &rect);
    bool scroll(int dx, int dy, const QRect &rect);

    int metric(QPaintDevice::PaintDeviceMetric metric) const;
    void fill(const QColor &color);
    void setMask(const QBitmap &mask);
    void setAlphaChannel(const QPixmap &alphaChannel);
    QImage toImage() const;
    QPaintEngine* paintEngine() const;

    void beginDataAccess();
    void endDataAccess(bool readOnly=false) const;

    void* toNativeType(NativeType type);
    void fromNativeType(void* pixmap, NativeType type);

    void convertToDisplayMode(int mode);

private:
    void release();
    void fromSymbianBitmap(CFbsBitmap* bitmap, bool lockFormat=false);
    QImage toImage(const QRect &r) const;

    QSymbianBitmapDataAccess *symbianBitmapDataAccess;

    CFbsBitmap *cfbsBitmap;
    QPaintEngine *pengine;
    uchar* bytes;

    bool formatLocked;

    QS60PlatformPixmap *next;
    QS60PlatformPixmap *prev;

    static void qt_symbian_register_pixmap(QS60PlatformPixmap *pd);
    static void qt_symbian_unregister_pixmap(QS60PlatformPixmap *pd);
    static void qt_symbian_release_pixmaps();

    friend class QPixmap;
    friend class QS60WindowSurface;
    friend class QS60PaintEngine;
    friend class QS60Data;
};

QT_END_NAMESPACE

#endif // QPIXMAPDATA_S60_P_H

