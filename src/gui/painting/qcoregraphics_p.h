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

#ifndef QCOREGRAPHICS_P_H
#define QCOREGRAPHICS_P_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include <QtGui/qregion.h>
#include <QtGui/qpalette.h>

#include <CoreGraphics/CoreGraphics.h>
#ifdef Q_OS_MACOS
#include <AppKit/AppKit.h>
#endif

QT_BEGIN_NAMESPACE

#ifdef Q_OS_MACOS
Q_GUI_EXPORT NSImage *qt_mac_create_nsimage(const QPixmap &pm);
Q_GUI_EXPORT NSImage *qt_mac_create_nsimage(const QIcon &icon, int defaultSize = 0);
Q_GUI_EXPORT QPixmap qt_mac_toQPixmap(const NSImage *image, const QSizeF &size);
#endif
Q_GUI_EXPORT CGImageRef qt_mac_toCGImage(const QImage &qImage);
Q_GUI_EXPORT CGImageRef qt_mac_toCGImageMask(const QImage &qImage);
Q_GUI_EXPORT QImage qt_mac_toQImage(CGImageRef image);

Q_GUI_EXPORT OSStatus qt_mac_drawCGImage(CGContextRef inContext, const CGRect *inBounds, CGImageRef inImage);

Q_GUI_EXPORT CGColorSpaceRef qt_mac_genericColorSpace();
Q_GUI_EXPORT CGColorSpaceRef qt_mac_colorSpaceForDeviceType(const QPaintDevice *paintDevice);

Q_GUI_EXPORT void qt_mac_clip_cg(CGContextRef hd, const QRegion &rgn, CGAffineTransform *orig_xform);

#ifdef Q_OS_MACOS
Q_GUI_EXPORT QColor qt_mac_toQColor(const NSColor *color);
Q_GUI_EXPORT QBrush qt_mac_toQBrush(const NSColor *color, QPalette::ColorGroup colorGroup = QPalette::Normal);
#endif
Q_GUI_EXPORT QColor qt_mac_toQColor(CGColorRef color);
Q_GUI_EXPORT QBrush qt_mac_toQBrush(CGColorRef color);

class Q_GUI_EXPORT QMacCGContext
{
public:
    inline QMacCGContext() { context = 0; }
    QMacCGContext(QPaintDevice *pdev);
    QMacCGContext(QPainter *p);
    inline QMacCGContext(CGContextRef cg, bool takeOwnership = false) {
        context = cg;
        if (!takeOwnership)
            CGContextRetain(context);
    }
    inline QMacCGContext(const QMacCGContext &copy) : context(0) { *this = copy; }
    inline ~QMacCGContext() {
        if (context)
            CGContextRelease(context);
    }
    inline bool isNull() const { return context; }
    inline operator CGContextRef() { return context; }
    inline QMacCGContext &operator=(const QMacCGContext &copy) {
        if (context)
            CGContextRelease(context);
        context = copy.context;
        CGContextRetain(context);
        return *this;
    }
    inline QMacCGContext &operator=(CGContextRef cg) {
        if (context)
            CGContextRelease(context);
        context = cg;
        CGContextRetain(context); //we do not take ownership
        return *this;
    }

private:
    CGContextRef context;
};

QT_END_NAMESPACE

#endif // QCOREGRAPHICS_P_H
