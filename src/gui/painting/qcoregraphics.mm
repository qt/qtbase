/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qcoregraphics_p.h"

#include <private/qcore_mac_p.h>
#include <qpa/qplatformpixmap.h>

#include <QtGui/private/qpaintengine_p.h>
#include <QtCore/qdebug.h>
#include <QtCore/qcoreapplication.h>

QT_BEGIN_NAMESPACE


// ---------------------- Color Management ----------------------

static CGColorSpaceRef m_genericColorSpace = 0;
static QHash<uint32_t, CGColorSpaceRef> m_displayColorSpaceHash;
static bool m_postRoutineRegistered = false;

static void qt_mac_cleanUpMacColorSpaces()
{
    if (m_genericColorSpace) {
        CFRelease(m_genericColorSpace);
        m_genericColorSpace = 0;
    }
    QHash<uint32_t, CGColorSpaceRef>::const_iterator it = m_displayColorSpaceHash.constBegin();
    while (it != m_displayColorSpaceHash.constEnd()) {
        if (it.value())
            CFRelease(it.value());
        ++it;
    }
    m_displayColorSpaceHash.clear();
}

static CGColorSpaceRef qt_mac_displayColorSpace(const QWindow *window)
{
    CGColorSpaceRef colorSpace = 0;
    uint32_t displayID = 0;

#ifdef Q_OS_MACOS
    if (window == 0) {
        displayID = CGMainDisplayID();
    } else {
        displayID = CGMainDisplayID();
        /*
        ### get correct display
        const QRect &qrect = window->geometry();
        CGRect rect = CGRectMake(qrect.x(), qrect.y(), qrect.width(), qrect.height());
        CGDisplayCount throwAway;
        CGDisplayErr dErr = CGGetDisplaysWithRect(rect, 1, &displayID, &throwAway);
        if (dErr != kCGErrorSuccess)
            return macDisplayColorSpace(0); // fall back on main display
        */
    }
    if ((colorSpace = m_displayColorSpaceHash.value(displayID)))
        return colorSpace;

    colorSpace = CGDisplayCopyColorSpace(displayID);
#else
    Q_UNUSED(window);
#endif

    if (colorSpace == 0)
        colorSpace = CGColorSpaceCreateDeviceRGB();

    m_displayColorSpaceHash.insert(displayID, colorSpace);
    if (!m_postRoutineRegistered) {
        m_postRoutineRegistered = true;
        qAddPostRoutine(qt_mac_cleanUpMacColorSpaces);
    }
    return colorSpace;
}

CGColorSpaceRef qt_mac_colorSpaceForDeviceType(const QPaintDevice *paintDevice)
{
    Q_UNUSED(paintDevice);

    // FIXME: Move logic into each paint device once Qt has support for color spaces
    return qt_mac_displayColorSpace(0);

    // The following code seems to take care of QWidget, but in reality doesn't, as
    // qt_mac_displayColorSpace ignores the argument and always uses the main display.
#if 0
    bool isWidget = (paintDevice->devType() == QInternal::Widget);
    return qt_mac_displayColorSpace(isWidget ? static_cast<const QWidget *>(paintDevice)->window() : 0);
#endif
}

CGColorSpaceRef qt_mac_genericColorSpace()
{
#if 0
    if (!m_genericColorSpace) {
        if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_4) {
            m_genericColorSpace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
        } else
        {
            m_genericColorSpace = CGColorSpaceCreateDeviceRGB();
        }
        if (!m_postRoutineRegistered) {
            m_postRoutineRegistered = true;
            qAddPostRoutine(QCoreGraphicsPaintEngine::cleanUpMacColorSpaces);
        }
    }
    return m_genericColorSpace;
#else
    // Just return the main display colorspace for the moment.
    return qt_mac_displayColorSpace(0);
#endif
}

// ---------------------- Geometry Helpers ----------------------

void qt_mac_clip_cg(CGContextRef hd, const QRegion &rgn, CGAffineTransform *orig_xform)
{
    CGAffineTransform old_xform = CGAffineTransformIdentity;
    if (orig_xform) { //setup xforms
        old_xform = CGContextGetCTM(hd);
        CGContextConcatCTM(hd, CGAffineTransformInvert(old_xform));
        CGContextConcatCTM(hd, *orig_xform);
    }

    //do the clipping
    CGContextBeginPath(hd);
    if (rgn.isEmpty()) {
        CGContextAddRect(hd, CGRectMake(0, 0, 0, 0));
    } else {
        for (const QRect &r : rgn) {
            CGRect mac_r = CGRectMake(r.x(), r.y(), r.width(), r.height());
            CGContextAddRect(hd, mac_r);
        }
    }
    CGContextClip(hd);

    if (orig_xform) {//reset xforms
        CGContextConcatCTM(hd, CGAffineTransformInvert(CGContextGetCTM(hd)));
        CGContextConcatCTM(hd, old_xform);
    }
}

// move to QRegion?
void qt_mac_scale_region(QRegion *region, qreal scaleFactor)
{
    if (!region || !region->rectCount())
        return;

    QVector<QRect> scaledRects;
    scaledRects.reserve(region->rectCount());

    for (const QRect &rect : *region)
        scaledRects.append(QRect(rect.topLeft() * scaleFactor, rect.size() * scaleFactor));

    region->setRects(&scaledRects[0], scaledRects.count());
}

// ---------------------- QMacCGContext ----------------------

QMacCGContext::QMacCGContext(QPaintDevice *paintDevice) : context(0)
{
    // In Qt 5, QWidget and QPixmap (and QImage) paint devices are all QImages under the hood.
    QImage *image = 0;
    if (paintDevice->devType() == QInternal::Image) {
        image = static_cast<QImage *>(paintDevice);
    } else if (paintDevice->devType() == QInternal::Pixmap) {

        const QPixmap *pm = static_cast<const QPixmap*>(paintDevice);
        QPlatformPixmap *data = const_cast<QPixmap *>(pm)->data_ptr().data();
        if (data && data->classId() == QPlatformPixmap::RasterClass) {
            image = data->buffer();
        } else {
            qDebug("QMacCGContext: Unsupported pixmap class");
        }
    } else if (paintDevice->devType() == QInternal::Widget) {
        // TODO test: image = static_cast<QImage *>(static_cast<const QWidget *>(paintDevice)->backingStore()->paintDevice());
        qDebug("QMacCGContext: not implemented: Widget class");
    }

    if (!image)
        return; // Context type not supported.

    CGColorSpaceRef colorspace = qt_mac_colorSpaceForDeviceType(paintDevice);
    uint flags = kCGImageAlphaPremultipliedFirst;
    flags |= kCGBitmapByteOrder32Host;

    context = CGBitmapContextCreate(image->bits(), image->width(), image->height(),
                                8, image->bytesPerLine(), colorspace, flags);
    CGContextTranslateCTM(context, 0, image->height());
    CGContextScaleCTM(context, 1, -1);
}

QMacCGContext::QMacCGContext(QPainter *painter) : context(0)
{
    QPaintEngine *paintEngine = painter->paintEngine();

    // Handle the case of QMacPrintEngine, which has an internal QCoreGraphicsPaintEngine
    while (QPaintEngine *aggregateEngine = QPaintEnginePrivate::get(paintEngine)->aggregateEngine())
        paintEngine = aggregateEngine;

    paintEngine->syncState();

    if (Qt::HANDLE handle = QPaintEnginePrivate::get(paintEngine)->nativeHandle()) {
        context = static_cast<CGContextRef>(handle);
        return;
    }

    int devType = painter->device()->devType();
    if (paintEngine->type() == QPaintEngine::Raster
            && (devType == QInternal::Widget ||
                devType == QInternal::Pixmap ||
                devType == QInternal::Image)) {

        CGColorSpaceRef colorspace = qt_mac_colorSpaceForDeviceType(paintEngine->paintDevice());
        uint flags = kCGImageAlphaPremultipliedFirst;
#ifdef kCGBitmapByteOrder32Host //only needed because CGImage.h added symbols in the minor version
        flags |= kCGBitmapByteOrder32Host;
#endif
        const QImage *image = static_cast<const QImage *>(paintEngine->paintDevice());

        context = CGBitmapContextCreate((void *)image->bits(), image->width(), image->height(),
                                        8, image->bytesPerLine(), colorspace, flags);

        // Invert y axis
        CGContextTranslateCTM(context, 0, image->height());
        CGContextScaleCTM(context, 1, -1);

        const qreal devicePixelRatio = image->devicePixelRatio();

        if (devType == QInternal::Widget) {
            // Set the clip rect which is an intersection of the system clip
            // and the painter clip. To make matters more interesting these
            // are in device pixels and device-independent pixels, respectively.
            QRegion clip = painter->paintEngine()->systemClip(); // get system clip in device pixels
            QTransform native = painter->deviceTransform();      // get device transform. dx/dy is in device pixels

            if (painter->hasClipping()) {
                QRegion r = painter->clipRegion();               // get painter clip, which is in device-independent pixels
                qt_mac_scale_region(&r, devicePixelRatio); // scale painter clip to device pixels
                r.translate(native.dx(), native.dy());
                if (clip.isEmpty())
                    clip = r;
                else
                    clip &= r;
            }
            qt_mac_clip_cg(context, clip, 0); // clip in device pixels

            // Scale the context so that painting happens in device-independent pixels
            CGContextScaleCTM(context, devicePixelRatio, devicePixelRatio);
            CGContextTranslateCTM(context, native.dx() / devicePixelRatio, native.dy() / devicePixelRatio);
        } else {
            // Scale to paint in device-independent pixels
            CGContextScaleCTM(context, devicePixelRatio, devicePixelRatio);
        }
    } else {
        qDebug() << "QMacCGContext:: Unsupported painter devtype type" << devType;
    }
}

QT_END_NAMESPACE
