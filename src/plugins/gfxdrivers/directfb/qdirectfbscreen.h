/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QDIRECTFBSCREEN_H
#define QDIRECTFBSCREEN_H

#include <qglobal.h>
#ifndef QT_NO_QWS_DIRECTFB
#include <QtGui/qscreen_qws.h>
#include <directfb.h>
#include <directfb_version.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#if !defined QT_DIRECTFB_SUBSURFACE && !defined QT_NO_DIRECTFB_SUBSURFACE
#define QT_NO_DIRECTFB_SUBSURFACE
#endif
#if !defined QT_NO_DIRECTFB_LAYER && !defined QT_DIRECTFB_LAYER
#define QT_DIRECTFB_LAYER
#endif
#if !defined QT_NO_DIRECTFB_WM && !defined QT_DIRECTFB_WM
#define QT_DIRECTFB_WM
#endif
#if !defined QT_DIRECTFB_IMAGECACHE && !defined QT_NO_DIRECTFB_IMAGECACHE
#define QT_NO_DIRECTFB_IMAGECACHE
#endif
#if !defined QT_NO_DIRECTFB_IMAGEPROVIDER && !defined QT_DIRECTFB_IMAGEPROVIDER
#define QT_DIRECTFB_IMAGEPROVIDER
#endif
#if !defined QT_NO_DIRECTFB_STRETCHBLIT && !defined QT_DIRECTFB_STRETCHBLIT
#define QT_DIRECTFB_STRETCHBLIT
#endif
#if !defined QT_DIRECTFB_IMAGEPROVIDER_KEEPALIVE && !defined QT_NO_DIRECTFB_IMAGEPROVIDER_KEEPALIVE
#define QT_NO_DIRECTFB_IMAGEPROVIDER_KEEPALIVE
#endif
#if !defined QT_DIRECTFB_WINDOW_AS_CURSOR && !defined QT_NO_DIRECTFB_WINDOW_AS_CURSOR
#define QT_NO_DIRECTFB_WINDOW_AS_CURSOR
#endif
#if !defined QT_DIRECTFB_PALETTE && !defined QT_NO_DIRECTFB_PALETTE
#define QT_NO_DIRECTFB_PALETTE
#endif
#if !defined QT_NO_DIRECTFB_PREALLOCATED && !defined QT_DIRECTFB_PREALLOCATED
#define QT_DIRECTFB_PREALLOCATED
#endif
#if !defined QT_NO_DIRECTFB_MOUSE && !defined QT_DIRECTFB_MOUSE
#define QT_DIRECTFB_MOUSE
#endif
#if !defined QT_NO_DIRECTFB_KEYBOARD && !defined QT_DIRECTFB_KEYBOARD
#define QT_DIRECTFB_KEYBOARD
#endif
#if !defined QT_NO_DIRECTFB_OPAQUE_DETECTION && !defined QT_DIRECTFB_OPAQUE_DETECTION
#define QT_DIRECTFB_OPAQUE_DETECTION
#endif
#ifndef QT_NO_QWS_CURSOR
#if defined QT_DIRECTFB_WM && defined QT_DIRECTFB_WINDOW_AS_CURSOR
#define QT_DIRECTFB_CURSOR
#elif defined QT_DIRECTFB_LAYER
#define QT_DIRECTFB_CURSOR
#endif
#endif
#ifndef QT_DIRECTFB_CURSOR
#define QT_NO_DIRECTFB_CURSOR
#endif
#if defined QT_NO_DIRECTFB_LAYER && defined QT_DIRECTFB_WM
#error QT_NO_DIRECTFB_LAYER requires QT_NO_DIRECTFB_WM
#endif
#if defined QT_DIRECTFB_IMAGEPROVIDER_KEEPALIVE && defined QT_NO_DIRECTFB_IMAGEPROVIDER
#error QT_DIRECTFB_IMAGEPROVIDER_KEEPALIVE requires QT_DIRECTFB_IMAGEPROVIDER to be defined
#endif
#if defined QT_DIRECTFB_WINDOW_AS_CURSOR && defined QT_NO_DIRECTFB_WM
#error QT_DIRECTFB_WINDOW_AS_CURSOR requires QT_DIRECTFB_WM to be defined
#endif

#define Q_DIRECTFB_VERSION ((DIRECTFB_MAJOR_VERSION << 16) | (DIRECTFB_MINOR_VERSION << 8) | DIRECTFB_MICRO_VERSION)

#define DIRECTFB_DECLARE_OPERATORS_FOR_FLAGS(F)                         \
    static inline F operator~(F f) { return F(~int(f)); } \
    static inline F operator&(F left, F right) { return F(int(left) & int(right)); } \
    static inline F operator|(F left, F right) { return F(int(left) | int(right)); } \
    static inline F &operator|=(F &left, F right) { left = (left | right); return left; } \
    static inline F &operator&=(F &left, F right) { left = (left & right); return left; }

DIRECTFB_DECLARE_OPERATORS_FOR_FLAGS(DFBInputDeviceCapabilities);
DIRECTFB_DECLARE_OPERATORS_FOR_FLAGS(DFBWindowDescriptionFlags);
DIRECTFB_DECLARE_OPERATORS_FOR_FLAGS(DFBWindowCapabilities);
DIRECTFB_DECLARE_OPERATORS_FOR_FLAGS(DFBWindowOptions);
DIRECTFB_DECLARE_OPERATORS_FOR_FLAGS(DFBSurfaceDescriptionFlags);
DIRECTFB_DECLARE_OPERATORS_FOR_FLAGS(DFBSurfaceCapabilities);
DIRECTFB_DECLARE_OPERATORS_FOR_FLAGS(DFBSurfaceLockFlags);
DIRECTFB_DECLARE_OPERATORS_FOR_FLAGS(DFBSurfaceBlittingFlags);
DIRECTFB_DECLARE_OPERATORS_FOR_FLAGS(DFBSurfaceDrawingFlags);
DIRECTFB_DECLARE_OPERATORS_FOR_FLAGS(DFBSurfaceFlipFlags);

class QDirectFBScreenPrivate;
class Q_GUI_EXPORT QDirectFBScreen : public QScreen
{
public:
    QDirectFBScreen(int display_id);
    ~QDirectFBScreen();

    enum DirectFBFlag {
        NoFlags = 0x00,
        VideoOnly = 0x01,
        SystemOnly = 0x02,
        BoundingRectFlip = 0x04,
        NoPartialFlip = 0x08
    };

    Q_DECLARE_FLAGS(DirectFBFlags, DirectFBFlag);

    DirectFBFlags directFBFlags() const;

    bool connect(const QString &displaySpec);
    void disconnect();
    bool initDevice();
    void shutdownDevice();

    void exposeRegion(QRegion r, int changing);
    void solidFill(const QColor &color, const QRegion &region);
    static void solidFill(IDirectFBSurface *surface, const QColor &color, const QRegion &region);    

    void setMode(int width, int height, int depth);
    void blank(bool on);

    QWSWindowSurface *createSurface(QWidget *widget) const;
    QWSWindowSurface *createSurface(const QString &key) const;

    static QDirectFBScreen *instance();
    void waitIdle();
    IDirectFBSurface *surfaceForWidget(const QWidget *widget, QRect *rect) const;
#ifdef QT_DIRECTFB_SUBSURFACE
    IDirectFBSurface *subSurfaceForWidget(const QWidget *widget, const QRect &area = QRect()) const;
#endif
    IDirectFB *dfb();
#ifdef QT_DIRECTFB_WM
    IDirectFBWindow *windowForWidget(const QWidget *widget) const;
#else
    IDirectFBSurface *primarySurface();
#endif
#ifndef QT_NO_DIRECTFB_LAYER
    IDirectFBDisplayLayer *dfbDisplayLayer();
#endif

    // Track surface creation/release so we can release all on exit
    enum SurfaceCreationOption {
        DontTrackSurface = 0x1,
        TrackSurface = 0x2,
        NoPreallocated = 0x4
    };
    Q_DECLARE_FLAGS(SurfaceCreationOptions, SurfaceCreationOption);
    IDirectFBSurface *createDFBSurface(const QImage &image,
                                       QImage::Format format,
                                       SurfaceCreationOptions options,
                                       DFBResult *result = 0);
    IDirectFBSurface *createDFBSurface(const QSize &size,
                                       QImage::Format format,
                                       SurfaceCreationOptions options,
                                       DFBResult *result = 0);
    IDirectFBSurface *copyDFBSurface(IDirectFBSurface *src,
                                     QImage::Format format,
                                     SurfaceCreationOptions options,
                                     DFBResult *result = 0);
    IDirectFBSurface *createDFBSurface(DFBSurfaceDescription desc,
                                       SurfaceCreationOptions options,
                                       DFBResult *result);
#ifdef QT_DIRECTFB_SUBSURFACE
    IDirectFBSurface *getSubSurface(IDirectFBSurface *surface,
                                    const QRect &rect,
                                    SurfaceCreationOptions options,
                                    DFBResult *result);
#endif

    void flipSurface(IDirectFBSurface *surface, DFBSurfaceFlipFlags flipFlags,
                     const QRegion &region, const QPoint &offset);
    void releaseDFBSurface(IDirectFBSurface *surface);

    using QScreen::depth;
    static int depth(DFBSurfacePixelFormat format);
    static int depth(QImage::Format format);

    static DFBSurfacePixelFormat getSurfacePixelFormat(QImage::Format format);
    static DFBSurfaceDescription getSurfaceDescription(const uint *buffer,
                                                       int length);
    static QImage::Format getImageFormat(IDirectFBSurface *surface);
    static bool initSurfaceDescriptionPixelFormat(DFBSurfaceDescription *description, QImage::Format format);
    static inline bool isPremultiplied(QImage::Format format);
    static inline bool hasAlphaChannel(DFBSurfacePixelFormat format);
    static inline bool hasAlphaChannel(IDirectFBSurface *surface);
    QImage::Format alphaPixmapFormat() const;

#ifndef QT_NO_DIRECTFB_PALETTE
    static void setSurfaceColorTable(IDirectFBSurface *surface,
                                     const QImage &image);
#endif

    static uchar *lockSurface(IDirectFBSurface *surface, DFBSurfaceLockFlags flags, int *bpl = 0);
#if defined QT_DIRECTFB_IMAGEPROVIDER && defined QT_DIRECTFB_IMAGEPROVIDER_KEEPALIVE
    void setDirectFBImageProvider(IDirectFBImageProvider *provider);
#endif
private:
    QDirectFBScreenPrivate *d_ptr;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QDirectFBScreen::SurfaceCreationOptions);
Q_DECLARE_OPERATORS_FOR_FLAGS(QDirectFBScreen::DirectFBFlags);

inline bool QDirectFBScreen::isPremultiplied(QImage::Format format)
{
    switch (format) {
    case QImage::Format_ARGB32_Premultiplied:
    case QImage::Format_ARGB8565_Premultiplied:
    case QImage::Format_ARGB6666_Premultiplied:
    case QImage::Format_ARGB8555_Premultiplied:
    case QImage::Format_ARGB4444_Premultiplied:
        return true;
    default:
        break;
    }
    return false;
}

inline bool QDirectFBScreen::hasAlphaChannel(DFBSurfacePixelFormat format)
{
    switch (format) {
    case DSPF_ARGB1555:
    case DSPF_ARGB:
    case DSPF_LUT8:
    case DSPF_AiRGB:
    case DSPF_A1:
    case DSPF_ARGB2554:
    case DSPF_ARGB4444:
#if (Q_DIRECTFB_VERSION >= 0x000923)
    case DSPF_AYUV:
#endif
#if (Q_DIRECTFB_VERSION >= 0x010000)
    case DSPF_A4:
    case DSPF_ARGB1666:
    case DSPF_ARGB6666:
    case DSPF_LUT2:
#endif
        return true;
    default:
        return false;
    }
}

inline bool QDirectFBScreen::hasAlphaChannel(IDirectFBSurface *surface)
{
    Q_ASSERT(surface);
    DFBSurfacePixelFormat format;
    surface->GetPixelFormat(surface, &format);
    return QDirectFBScreen::hasAlphaChannel(format);
}

QT_END_NAMESPACE

QT_END_HEADER

#endif // QT_NO_QWS_DIRECTFB
#endif // QDIRECTFBSCREEN_H

