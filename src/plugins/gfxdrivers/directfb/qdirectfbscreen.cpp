/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qdirectfbscreen.h"
#include "qdirectfbwindowsurface.h"
#include "qdirectfbpixmap.h"
#include "qdirectfbmouse.h"
#include "qdirectfbkeyboard.h"
#include <QtGui/qwsdisplay_qws.h>
#include <QtGui/qcolor.h>
#include <QtGui/qapplication.h>
#include <QtGui/qwindowsystem_qws.h>
#include <QtGui/private/qgraphicssystem_qws_p.h>
#include <QtGui/private/qwssignalhandler_p.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qvector.h>
#include <QtCore/qrect.h>

#ifndef QT_NO_QWS_DIRECTFB

QT_BEGIN_NAMESPACE

class QDirectFBScreenPrivate : public QObject, public QWSGraphicsSystem
{
    Q_OBJECT
public:
    QDirectFBScreenPrivate(QDirectFBScreen *qptr);
    ~QDirectFBScreenPrivate();

    void setFlipFlags(const QStringList &args);
    QPixmapData *createPixmapData(QPixmapData::PixelType type) const;
public slots:
#ifdef QT_DIRECTFB_WM
    void onWindowEvent(QWSWindow *window, QWSServer::WindowEvent event);
#endif
public:
    IDirectFB *dfb;
    DFBSurfaceFlipFlags flipFlags;
    QDirectFBScreen::DirectFBFlags directFBFlags;
    QImage::Format alphaPixmapFormat;
    IDirectFBScreen *dfbScreen;
#ifdef QT_NO_DIRECTFB_WM
    IDirectFBSurface *primarySurface;
    QColor backgroundColor;
#endif
#ifndef QT_NO_DIRECTFB_LAYER
    IDirectFBDisplayLayer *dfbLayer;
#endif
    QSet<IDirectFBSurface*> allocatedSurfaces;

#ifndef QT_NO_DIRECTFB_MOUSE
    QDirectFBMouseHandler *mouse;
#endif
#ifndef QT_NO_DIRECTFB_KEYBOARD
    QDirectFBKeyboardHandler *keyboard;
#endif
#if defined QT_DIRECTFB_IMAGEPROVIDER && defined QT_DIRECTFB_IMAGEPROVIDER_KEEPALIVE
    IDirectFBImageProvider *imageProvider;
#endif
    IDirectFBSurface *cursorSurface;
    qint64 cursorImageKey;

    QDirectFBScreen *q;
    static QDirectFBScreen *instance;
};

QDirectFBScreen *QDirectFBScreenPrivate::instance = 0;

QDirectFBScreenPrivate::QDirectFBScreenPrivate(QDirectFBScreen *qptr)
    : QWSGraphicsSystem(qptr), dfb(0), flipFlags(DSFLIP_NONE),
      directFBFlags(QDirectFBScreen::NoFlags), alphaPixmapFormat(QImage::Format_Invalid),
      dfbScreen(0)
#ifdef QT_NO_DIRECTFB_WM
    , primarySurface(0)
#endif
#ifndef QT_NO_DIRECTFB_LAYER
    , dfbLayer(0)
#endif
#ifndef QT_NO_DIRECTFB_MOUSE
    , mouse(0)
#endif
#ifndef QT_NO_DIRECTFB_KEYBOARD
    , keyboard(0)
#endif
#if defined QT_DIRECTFB_IMAGEPROVIDER && defined QT_DIRECTFB_IMAGEPROVIDER_KEEPALIVE
    , imageProvider(0)
#endif
    , cursorSurface(0)
    , cursorImageKey(0)
    , q(qptr)
{
#ifndef QT_NO_QWS_SIGNALHANDLER
    QWSSignalHandler::instance()->addObject(this);
#endif
#ifdef QT_DIRECTFB_WM
    connect(QWSServer::instance(), SIGNAL(windowEvent(QWSWindow*,QWSServer::WindowEvent)),
            this, SLOT(onWindowEvent(QWSWindow*,QWSServer::WindowEvent)));
#endif
}

QDirectFBScreenPrivate::~QDirectFBScreenPrivate()
{
#ifndef QT_NO_DIRECTFB_MOUSE
    delete mouse;
#endif
#ifndef QT_NO_DIRECTFB_KEYBOARD
    delete keyboard;
#endif
#if defined QT_DIRECTFB_IMAGEPROVIDER_KEEPALIVE
    if (imageProvider)
        imageProvider->Release(imageProvider);
#endif

    for (QSet<IDirectFBSurface*>::const_iterator it = allocatedSurfaces.begin(); it != allocatedSurfaces.end(); ++it) {
        (*it)->Release(*it);
    }

#ifdef QT_NO_DIRECTFB_WM
    if (primarySurface)
        primarySurface->Release(primarySurface);
#endif

#ifndef QT_NO_DIRECTFB_LAYER
    if (dfbLayer)
        dfbLayer->Release(dfbLayer);
#endif

    if (dfbScreen)
        dfbScreen->Release(dfbScreen);

    if (dfb)
        dfb->Release(dfb);
}

IDirectFBSurface *QDirectFBScreen::createDFBSurface(const QImage &image, QImage::Format format, SurfaceCreationOptions options, DFBResult *resultPtr)
{
    if (image.isNull()) // assert?
        return 0;

    if (QDirectFBScreen::getSurfacePixelFormat(format) == DSPF_UNKNOWN) {
        format = QDirectFBPixmapData::hasAlphaChannel(image) ? d_ptr->alphaPixmapFormat : pixelFormat();
    }
    if (image.format() != format) {
        return createDFBSurface(image.convertToFormat(format), format, options | NoPreallocated, resultPtr);
    }

    DFBSurfaceDescription description;
    memset(&description, 0, sizeof(DFBSurfaceDescription));
    description.width = image.width();
    description.height = image.height();
    description.flags = DSDESC_WIDTH|DSDESC_HEIGHT|DSDESC_PIXELFORMAT;
    initSurfaceDescriptionPixelFormat(&description, format);
    bool doMemCopy = true;
#ifdef QT_DIRECTFB_PREALLOCATED
    if (!(options & NoPreallocated)) {
        doMemCopy = false;
        description.flags |= DSDESC_PREALLOCATED;
        description.preallocated[0].data = const_cast<uchar*>(image.bits());
        description.preallocated[0].pitch = image.bytesPerLine();
        description.preallocated[1].data = 0;
        description.preallocated[1].pitch = 0;
    }
#endif
    DFBResult result;
    IDirectFBSurface *surface = createDFBSurface(description, options, &result);
    if (resultPtr)
        *resultPtr = result;
    if (!surface) {
        DirectFBError("Couldn't create surface createDFBSurface(QImage, QImage::Format, SurfaceCreationOptions)", result);
        return 0;
    }
    if (doMemCopy) {
        int bplDFB;
        uchar *mem = QDirectFBScreen::lockSurface(surface, DSLF_WRITE, &bplDFB);
        if (mem) {
            const int height = image.height();
            const int bplQt = image.bytesPerLine();
            if (bplQt == bplDFB && bplQt == (image.width() * image.depth() / 8)) {
                memcpy(mem, image.bits(), image.byteCount());
            } else {
                for (int i=0; i<height; ++i) {
                    memcpy(mem, image.scanLine(i), bplQt);
                    mem += bplDFB;
                }
            }
            surface->Unlock(surface);
        }
    }
#ifdef QT_DIRECTFB_PALETTE
    if (image.colorCount() != 0 && surface)
        QDirectFBScreen::setSurfaceColorTable(surface, image);
#endif
    return surface;
}

IDirectFBSurface *QDirectFBScreen::copyDFBSurface(IDirectFBSurface *src,
                                                  QImage::Format format,
                                                  SurfaceCreationOptions options,
                                                  DFBResult *result)
{
    Q_ASSERT(src);
    QSize size;
    src->GetSize(src, &size.rwidth(), &size.rheight());
    IDirectFBSurface *surface = createDFBSurface(size, format, options, result);
    DFBSurfaceBlittingFlags flags = QDirectFBScreen::hasAlphaChannel(surface)
                                    ? DSBLIT_BLEND_ALPHACHANNEL
                                    : DSBLIT_NOFX;
    if (flags & DSBLIT_BLEND_ALPHACHANNEL)
        surface->Clear(surface, 0, 0, 0, 0);

    surface->SetBlittingFlags(surface, flags);
    surface->Blit(surface, src, 0, 0, 0);
#if (Q_DIRECTFB_VERSION >= 0x010000)
    surface->ReleaseSource(surface);
#endif
    return surface;
}

IDirectFBSurface *QDirectFBScreen::createDFBSurface(const QSize &size,
                                                    QImage::Format format,
                                                    SurfaceCreationOptions options,
                                                    DFBResult *result)
{
    DFBSurfaceDescription desc;
    memset(&desc, 0, sizeof(DFBSurfaceDescription));
    desc.flags |= DSDESC_WIDTH|DSDESC_HEIGHT;
    if (!QDirectFBScreen::initSurfaceDescriptionPixelFormat(&desc, format))
        return 0;
    desc.width = size.width();
    desc.height = size.height();
    return createDFBSurface(desc, options, result);
}

IDirectFBSurface *QDirectFBScreen::createDFBSurface(DFBSurfaceDescription desc, SurfaceCreationOptions options, DFBResult *resultPtr)
{
    DFBResult tmp;
    DFBResult &result = (resultPtr ? *resultPtr : tmp);
    result = DFB_OK;
    IDirectFBSurface *newSurface = 0;

    if (!d_ptr->dfb) {
        qWarning("QDirectFBScreen::createDFBSurface() - not connected");
        return 0;
    }

    if (d_ptr->directFBFlags & VideoOnly
        && !(desc.flags & DSDESC_PREALLOCATED)
        && (!(desc.flags & DSDESC_CAPS) || !(desc.caps & DSCAPS_SYSTEMONLY))) {
        // Add the video only capability. This means the surface will be created in video ram
        if (!(desc.flags & DSDESC_CAPS)) {
            desc.caps = DSCAPS_VIDEOONLY;
            desc.flags |= DSDESC_CAPS;
        } else {
            desc.caps |= DSCAPS_VIDEOONLY;
        }
        result = d_ptr->dfb->CreateSurface(d_ptr->dfb, &desc, &newSurface);
        if (result != DFB_OK
#ifdef QT_NO_DEBUG
            && (desc.flags & DSDESC_CAPS) && (desc.caps & DSCAPS_PRIMARY)
#endif
            ) {
            qWarning("QDirectFBScreen::createDFBSurface() Failed to create surface in video memory!\n"
                     "   Flags %0x Caps %0x width %d height %d pixelformat %0x %d preallocated %p %d\n%s",
                     desc.flags, desc.caps, desc.width, desc.height,
                     desc.pixelformat, DFB_PIXELFORMAT_INDEX(desc.pixelformat),
                     desc.preallocated[0].data, desc.preallocated[0].pitch,
                     DirectFBErrorString(result));
        }
        desc.caps &= ~DSCAPS_VIDEOONLY;
    }

    if (d_ptr->directFBFlags & SystemOnly)
        desc.caps |= DSCAPS_SYSTEMONLY;

    if (!newSurface)
        result = d_ptr->dfb->CreateSurface(d_ptr->dfb, &desc, &newSurface);

    if (result != DFB_OK) {
        qWarning("QDirectFBScreen::createDFBSurface() Failed!\n"
                 "   Flags %0x Caps %0x width %d height %d pixelformat %0x %d preallocated %p %d\n%s",
                 desc.flags, desc.caps, desc.width, desc.height,
                 desc.pixelformat, DFB_PIXELFORMAT_INDEX(desc.pixelformat),
                 desc.preallocated[0].data, desc.preallocated[0].pitch,
                 DirectFBErrorString(result));
        return 0;
    }

    Q_ASSERT(newSurface);

    if (options & TrackSurface) {
        d_ptr->allocatedSurfaces.insert(newSurface);
    }

    return newSurface;
}

#ifdef QT_DIRECTFB_SUBSURFACE
IDirectFBSurface *QDirectFBScreen::getSubSurface(IDirectFBSurface *surface,
                                                 const QRect &rect,
                                                 SurfaceCreationOptions options,
                                                 DFBResult *resultPtr)
{
    Q_ASSERT(!(options & NoPreallocated));
    Q_ASSERT(surface);
    DFBResult res;
    DFBResult &result = (resultPtr ? *resultPtr : res);
    IDirectFBSurface *subSurface = 0;
    if (rect.isNull()) {
        result = surface->GetSubSurface(surface, 0, &subSurface);
    } else {
        const DFBRectangle subRect = { rect.x(), rect.y(), rect.width(), rect.height() };
        result = surface->GetSubSurface(surface, &subRect, &subSurface);
    }
    if (result != DFB_OK) {
        DirectFBError("Can't get sub surface", result);
    } else if (options & TrackSurface) {
        d_ptr->allocatedSurfaces.insert(subSurface);
    }
    return subSurface;
}
#endif


void QDirectFBScreen::releaseDFBSurface(IDirectFBSurface *surface)
{
    Q_ASSERT(QDirectFBScreen::instance());
    Q_ASSERT(surface);
    surface->Release(surface);
    if (!d_ptr->allocatedSurfaces.remove(surface))
        qWarning("QDirectFBScreen::releaseDFBSurface() - %p not in list", surface);

    //qDebug("Released surface at %p. New count = %d", surface, d_ptr->allocatedSurfaces.count());
}

QDirectFBScreen::DirectFBFlags QDirectFBScreen::directFBFlags() const
{
    return d_ptr->directFBFlags;
}

IDirectFB *QDirectFBScreen::dfb()
{
    return d_ptr->dfb;
}

#ifdef QT_NO_DIRECTFB_WM
IDirectFBSurface *QDirectFBScreen::primarySurface()
{
    return d_ptr->primarySurface;
}
#endif

#ifndef QT_NO_DIRECTFB_LAYER
IDirectFBDisplayLayer *QDirectFBScreen::dfbDisplayLayer()
{
    return d_ptr->dfbLayer;
}
#endif

DFBSurfacePixelFormat QDirectFBScreen::getSurfacePixelFormat(QImage::Format format)
{
    switch (format) {
#ifndef QT_NO_DIRECTFB_PALETTE
    case QImage::Format_Indexed8:
        return DSPF_LUT8;
#endif
    case QImage::Format_RGB888:
        return DSPF_RGB24;
    case QImage::Format_ARGB4444_Premultiplied:
        return DSPF_ARGB4444;
#if (Q_DIRECTFB_VERSION >= 0x010100)
    case QImage::Format_RGB444:
        return DSPF_RGB444;
    case QImage::Format_RGB555:
        return DSPF_RGB555;
#endif
    case QImage::Format_RGB16:
        return DSPF_RGB16;
#if (Q_DIRECTFB_VERSION >= 0x010000)
    case QImage::Format_ARGB6666_Premultiplied:
        return DSPF_ARGB6666;
    case QImage::Format_RGB666:
        return DSPF_RGB18;
#endif
    case QImage::Format_RGB32:
        return DSPF_RGB32;
    case QImage::Format_ARGB32_Premultiplied:
    case QImage::Format_ARGB32:
        return DSPF_ARGB;
    default:
        return DSPF_UNKNOWN;
    };
}

QImage::Format QDirectFBScreen::getImageFormat(IDirectFBSurface *surface)
{
    DFBSurfacePixelFormat format;
    surface->GetPixelFormat(surface, &format);

    switch (format) {
    case DSPF_LUT8:
        return QImage::Format_Indexed8;
    case DSPF_RGB24:
        return QImage::Format_RGB888;
    case DSPF_ARGB4444:
        return QImage::Format_ARGB4444_Premultiplied;
#if (Q_DIRECTFB_VERSION >= 0x010100)
    case DSPF_RGB444:
        return QImage::Format_RGB444;
    case DSPF_RGB555:
#endif
    case DSPF_ARGB1555:
        return QImage::Format_RGB555;
    case DSPF_RGB16:
        return QImage::Format_RGB16;
#if (Q_DIRECTFB_VERSION >= 0x010000)
    case DSPF_ARGB6666:
        return QImage::Format_ARGB6666_Premultiplied;
    case DSPF_RGB18:
        return QImage::Format_RGB666;
#endif
    case DSPF_RGB32:
        return QImage::Format_RGB32;
    case DSPF_ARGB: {
        DFBSurfaceCapabilities caps;
        const DFBResult result = surface->GetCapabilities(surface, &caps);
        Q_ASSERT(result == DFB_OK);
        Q_UNUSED(result);
        return (caps & DSCAPS_PREMULTIPLIED
                ? QImage::Format_ARGB32_Premultiplied
                : QImage::Format_ARGB32); }
    default:
        break;
    }
    return QImage::Format_Invalid;
}

DFBSurfaceDescription QDirectFBScreen::getSurfaceDescription(const uint *buffer,
                                                             int length)
{
    DFBSurfaceDescription description;
    memset(&description, 0, sizeof(DFBSurfaceDescription));

    description.flags = DSDESC_CAPS|DSDESC_WIDTH|DSDESC_HEIGHT|DSDESC_PIXELFORMAT|DSDESC_PREALLOCATED;
    description.caps = DSCAPS_PREMULTIPLIED;
    description.width = length;
    description.height = 1;
    description.pixelformat = DSPF_ARGB;
    description.preallocated[0].data = (void*)buffer;
    description.preallocated[0].pitch = length * sizeof(uint);
    description.preallocated[1].data = 0;
    description.preallocated[1].pitch = 0;
    return description;
}

#ifndef QT_NO_DIRECTFB_PALETTE
void QDirectFBScreen::setSurfaceColorTable(IDirectFBSurface *surface,
                                           const QImage &image)
{
    if (!surface)
        return;

    const int numColors = image.colorCount();
    if (numColors == 0)
        return;

    QVarLengthArray<DFBColor, 256> colors(numColors);
    for (int i = 0; i < numColors; ++i) {
        QRgb c = image.color(i);
        colors[i].a = qAlpha(c);
        colors[i].r = qRed(c);
        colors[i].g = qGreen(c);
        colors[i].b = qBlue(c);
    }

    IDirectFBPalette *palette;
    DFBResult result;
    result = surface->GetPalette(surface, &palette);
    if (result != DFB_OK) {
        DirectFBError("QDirectFBScreen::setSurfaceColorTable GetPalette",
                      result);
        return;
    }
    result = palette->SetEntries(palette, colors.data(), numColors, 0);
    if (result != DFB_OK) {
        DirectFBError("QDirectFBScreen::setSurfaceColorTable SetEntries",
                      result);
    }
    palette->Release(palette);
}

#endif // QT_NO_DIRECTFB_PALETTE

#if defined QT_DIRECTFB_CURSOR
class Q_GUI_EXPORT QDirectFBScreenCursor : public QScreenCursor
{
public:
    QDirectFBScreenCursor();
    virtual void set(const QImage &image, int hotx, int hoty);
    virtual void move(int x, int y);
    virtual void show();
    virtual void hide();
private:
#ifdef QT_DIRECTFB_WINDOW_AS_CURSOR
    ~QDirectFBScreenCursor();
    bool createWindow();
    IDirectFBWindow *window;
#endif
    IDirectFBDisplayLayer *layer;
};

QDirectFBScreenCursor::QDirectFBScreenCursor()
{
    IDirectFB *fb = QDirectFBScreen::instance()->dfb();
    if (!fb)
        qFatal("QDirectFBScreenCursor: DirectFB not initialized");

    layer = QDirectFBScreen::instance()->dfbDisplayLayer();
    Q_ASSERT(layer);

    enable = false;
    hwaccel = true;
    supportsAlpha = true;
#ifdef QT_DIRECTFB_WINDOW_AS_CURSOR
    window = 0;
    DFBResult result = layer->SetCooperativeLevel(layer, DLSCL_ADMINISTRATIVE);
    if (result != DFB_OK) {
        DirectFBError("QDirectFBScreenCursor::hide: "
                      "Unable to set cooperative level", result);
    }
    result = layer->SetCursorOpacity(layer, 0);
    if (result != DFB_OK) {
        DirectFBError("QDirectFBScreenCursor::hide: "
                      "Unable to set cursor opacity", result);
    }

    result = layer->SetCooperativeLevel(layer, DLSCL_SHARED);
    if (result != DFB_OK) {
        DirectFBError("QDirectFBScreenCursor::hide: "
                      "Unable to set cooperative level", result);
    }
#endif
}

#ifdef QT_DIRECTFB_WINDOW_AS_CURSOR
QDirectFBScreenCursor::~QDirectFBScreenCursor()
{
    if (window) {
        window->Release(window);
        window = 0;
    }
}

bool QDirectFBScreenCursor::createWindow()
{
    Q_ASSERT(!window);
    Q_ASSERT(!cursor.isNull());
    DFBWindowDescription description;
    memset(&description, 0, sizeof(DFBWindowDescription));
    description.flags = DWDESC_POSX|DWDESC_POSY|DWDESC_WIDTH|DWDESC_HEIGHT|DWDESC_CAPS|DWDESC_PIXELFORMAT|DWDESC_SURFACE_CAPS;
    description.width = cursor.width();
    description.height = cursor.height();
    description.posx = pos.x() - hotspot.x();
    description.posy = pos.y() - hotspot.y();
#if (Q_DIRECTFB_VERSION >= 0x010100)
    description.flags |= DWDESC_OPTIONS;
    description.options = DWOP_GHOST|DWOP_ALPHACHANNEL;
#endif
    description.caps = DWCAPS_NODECORATION|DWCAPS_DOUBLEBUFFER;
    const QImage::Format format = QDirectFBScreen::instance()->alphaPixmapFormat();
    description.pixelformat = QDirectFBScreen::getSurfacePixelFormat(format);
    if (QDirectFBScreen::isPremultiplied(format))
        description.surface_caps = DSCAPS_PREMULTIPLIED;

    DFBResult result = layer->CreateWindow(layer, &description, &window);
    if (result != DFB_OK) {
        DirectFBError("QDirectFBScreenCursor::createWindow: Unable to create window", result);
        return false;
    }
    result = window->SetOpacity(window, 255);
    if (result != DFB_OK) {
        DirectFBError("QDirectFBScreenCursor::createWindow: Unable to set opacity ", result);
        return false;
    }

    result = window->SetStackingClass(window, DWSC_UPPER);
    if (result != DFB_OK) {
        DirectFBError("QDirectFBScreenCursor::createWindow: Unable to set stacking class ", result);
        return false;
    }

    result = window->RaiseToTop(window);
    if (result != DFB_OK) {
        DirectFBError("QDirectFBScreenCursor::createWindow: Unable to raise window ", result);
        return false;
    }

    return true;
}
#endif

void QDirectFBScreenCursor::move(int x, int y)
{
    pos = QPoint(x, y);
#ifdef QT_DIRECTFB_WINDOW_AS_CURSOR
    if (window) {
        const QPoint p = pos - hotspot;
        DFBResult result = window->MoveTo(window, p.x(), p.y());
        if (result != DFB_OK) {
            DirectFBError("QDirectFBScreenCursor::move: Unable to move window", result);
        }
    }
#else
    layer->WarpCursor(layer, x, y);
#endif
}

void QDirectFBScreenCursor::hide()
{
    if (enable) {
        enable = false;
        DFBResult result;
#ifndef QT_DIRECTFB_WINDOW_AS_CURSOR
        result = layer->SetCooperativeLevel(layer, DLSCL_ADMINISTRATIVE);
        if (result != DFB_OK) {
            DirectFBError("QDirectFBScreenCursor::hide: "
                          "Unable to set cooperative level", result);
        }
        result = layer->SetCursorOpacity(layer, 0);
        if (result != DFB_OK) {
            DirectFBError("QDirectFBScreenCursor::hide: "
                          "Unable to set cursor opacity", result);
        }
        result = layer->SetCooperativeLevel(layer, DLSCL_SHARED);
        if (result != DFB_OK) {
            DirectFBError("QDirectFBScreenCursor::hide: "
                          "Unable to set cooperative level", result);
        }
#else
        if (window) {
            result = window->SetOpacity(window, 0);
            if (result != DFB_OK) {
                DirectFBError("QDirectFBScreenCursor::hide: "
                              "Unable to set window opacity", result);
            }
        }
#endif
    }
}

void QDirectFBScreenCursor::show()
{
    if (!enable) {
        enable = true;
        DFBResult result;
        result = layer->SetCooperativeLevel(layer, DLSCL_ADMINISTRATIVE);
        if (result != DFB_OK) {
            DirectFBError("QDirectFBScreenCursor::show: "
                          "Unable to set cooperative level", result);
        }
        result = layer->SetCursorOpacity(layer,
#ifdef QT_DIRECTFB_WINDOW_AS_CURSOR
                                         0
#else
                                         255
#endif
            );
        if (result != DFB_OK) {
            DirectFBError("QDirectFBScreenCursor::show: "
                          "Unable to set cursor shape", result);
        }
        result = layer->SetCooperativeLevel(layer, DLSCL_SHARED);
        if (result != DFB_OK) {
            DirectFBError("QDirectFBScreenCursor::show: "
                          "Unable to set cooperative level", result);
        }
#ifdef QT_DIRECTFB_WINDOW_AS_CURSOR
        if (window) {
            DFBResult result = window->SetOpacity(window, 255);
            if (result != DFB_OK) {
                DirectFBError("QDirectFBScreenCursor::show: "
                              "Unable to set window opacity", result);
            }
        }
#endif
    }
}

void QDirectFBScreenCursor::set(const QImage &image, int hotx, int hoty)
{
    QDirectFBScreen *screen = QDirectFBScreen::instance();
    if (!screen)
        return;

    if (image.isNull()) {
        cursor = QImage();
        hide();
    } else {
        cursor = image.convertToFormat(screen->alphaPixmapFormat());
        size = cursor.size();
        hotspot = QPoint(hotx, hoty);
        DFBResult result = DFB_OK;
        IDirectFBSurface *surface = screen->createDFBSurface(cursor, screen->alphaPixmapFormat(),
                                                             QDirectFBScreen::DontTrackSurface, &result);
        if (!surface) {
            DirectFBError("QDirectFBScreenCursor::set: Unable to create surface", result);
            return;
        }
#ifndef QT_DIRECTFB_WINDOW_AS_CURSOR
        result = layer->SetCooperativeLevel(layer, DLSCL_ADMINISTRATIVE);
        if (result != DFB_OK) {
            DirectFBError("QDirectFBScreenCursor::show: "
                          "Unable to set cooperative level", result);
        }
        result = layer->SetCursorShape(layer, surface, hotx, hoty);
        if (result != DFB_OK) {
            DirectFBError("QDirectFBScreenCursor::show: "
                          "Unable to set cursor shape", result);
        }
        result = layer->SetCooperativeLevel(layer, DLSCL_SHARED);
        if (result != DFB_OK) {
            DirectFBError("QDirectFBScreenCursor::show: "
                          "Unable to set cooperative level", result);
        }
#else
        if (window || createWindow()) {
            QSize windowSize;
            result = window->GetSize(window, &windowSize.rwidth(), &windowSize.rheight());
            if (result != DFB_OK) {
                DirectFBError("QDirectFBScreenCursor::set: "
                              "Unable to get window size", result);
            }
            result = window->Resize(window, size.width(), size.height());
            if (result != DFB_OK) {
                DirectFBError("QDirectFBScreenCursor::set: Unable to resize window", result);
            }

            IDirectFBSurface *windowSurface;
            result = window->GetSurface(window, &windowSurface);
            if (result != DFB_OK) {
                DirectFBError("QDirectFBScreenCursor::set: Unable to get window surface", result);
            } else {
                result = windowSurface->Clear(windowSurface, 0, 0, 0, 0);
                if (result != DFB_OK) {
                    DirectFBError("QDirectFBScreenCursor::set: Unable to clear surface", result);
                }

                result = windowSurface->Blit(windowSurface, surface, 0, 0, 0);
                if (result != DFB_OK) {
                    DirectFBError("QDirectFBScreenCursor::set: Unable to blit to surface", result);
                }
            }
            result = windowSurface->Flip(windowSurface, 0, DSFLIP_NONE);
            if (result != DFB_OK) {
                DirectFBError("QDirectFBScreenCursor::set: Unable to flip window", result);
            }

            windowSurface->Release(windowSurface);
        }
#endif
        surface->Release(surface);
        show();
    }

}
#endif // QT_DIRECTFB_CURSOR

QDirectFBScreen::QDirectFBScreen(int display_id)
    : QScreen(display_id, DirectFBClass), d_ptr(new QDirectFBScreenPrivate(this))
{
    QDirectFBScreenPrivate::instance = this;
}

QDirectFBScreen::~QDirectFBScreen()
{
    if (QDirectFBScreenPrivate::instance == this)
        QDirectFBScreenPrivate::instance = 0;
    delete d_ptr;
}

QDirectFBScreen *QDirectFBScreen::instance()
{
    return QDirectFBScreenPrivate::instance;
}

int QDirectFBScreen::depth(DFBSurfacePixelFormat format)
{
    switch (format) {
    case DSPF_A1:
        return 1;
    case DSPF_A8:
    case DSPF_RGB332:
    case DSPF_LUT8:
    case DSPF_ALUT44:
        return 8;
    case DSPF_I420:
    case DSPF_YV12:
    case DSPF_NV12:
    case DSPF_NV21:
#if (Q_DIRECTFB_VERSION >= 0x010100)
    case DSPF_RGB444:
#endif
        return 12;
#if (Q_DIRECTFB_VERSION >= 0x010100)
    case DSPF_RGB555:
        return 15;
#endif
    case DSPF_ARGB1555:
    case DSPF_RGB16:
    case DSPF_YUY2:
    case DSPF_UYVY:
    case DSPF_NV16:
    case DSPF_ARGB2554:
    case DSPF_ARGB4444:
        return 16;
    case DSPF_RGB24:
        return 24;
    case DSPF_RGB32:
    case DSPF_ARGB:
    case DSPF_AiRGB:
        return 32;
    case DSPF_UNKNOWN:
    default:
        return 0;
    };
    return 0;
}

int QDirectFBScreen::depth(QImage::Format format)
{
    int depth = 0;
    switch(format) {
    case QImage::Format_Invalid:
    case QImage::NImageFormats:
        Q_ASSERT(false);
    case QImage::Format_Mono:
    case QImage::Format_MonoLSB:
        depth = 1;
        break;
    case QImage::Format_Indexed8:
        depth = 8;
        break;
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
        depth = 32;
        break;
    case QImage::Format_RGB555:
    case QImage::Format_RGB16:
    case QImage::Format_RGB444:
    case QImage::Format_ARGB4444_Premultiplied:
        depth = 16;
        break;
    case QImage::Format_RGB666:
    case QImage::Format_ARGB6666_Premultiplied:
    case QImage::Format_ARGB8565_Premultiplied:
    case QImage::Format_ARGB8555_Premultiplied:
    case QImage::Format_RGB888:
        depth = 24;
        break;
    }
    return depth;
}

void QDirectFBScreenPrivate::setFlipFlags(const QStringList &args)
{
    QRegExp flipRegexp(QLatin1String("^flip=([\\w,]*)$"));
    int index = args.indexOf(flipRegexp);
    if (index >= 0) {
        const QStringList flips = flipRegexp.cap(1).split(QLatin1Char(','),
                                                          QString::SkipEmptyParts);
        flipFlags = DSFLIP_NONE;
        foreach(const QString &flip, flips) {
            if (flip == QLatin1String("wait"))
                flipFlags |= DSFLIP_WAIT;
            else if (flip == QLatin1String("blit"))
                flipFlags |= DSFLIP_BLIT;
            else if (flip == QLatin1String("onsync"))
                flipFlags |= DSFLIP_ONSYNC;
            else if (flip == QLatin1String("pipeline"))
                flipFlags |= DSFLIP_PIPELINE;
            else
                qWarning("QDirectFBScreen: Unknown flip argument: %s",
                         qPrintable(flip));
        }
    } else {
        flipFlags = DSFLIP_BLIT|DSFLIP_ONSYNC;
    }
}

#ifdef QT_DIRECTFB_WM
void QDirectFBScreenPrivate::onWindowEvent(QWSWindow *window, QWSServer::WindowEvent event)
{
    if (event == QWSServer::Raise) {
        QWSWindowSurface *windowSurface = window->windowSurface();
        if (windowSurface && windowSurface->key() == QLatin1String("directfb")) {
            static_cast<QDirectFBWindowSurface*>(windowSurface)->raise();
        }
    }
}
#endif

QPixmapData *QDirectFBScreenPrivate::createPixmapData(QPixmapData::PixelType type) const
{
    if (type == QPixmapData::BitmapType)
        return QWSGraphicsSystem::createPixmapData(type);

    return new QDirectFBPixmapData(q, type);
}

#if (Q_DIRECTFB_VERSION >= 0x000923)
#ifdef QT_NO_DEBUG
struct FlagDescription;
static const FlagDescription *accelerationDescriptions = 0;
static const FlagDescription *blitDescriptions = 0;
static const FlagDescription *drawDescriptions = 0;
#else
struct FlagDescription {
    const char *name;
    uint flag;
};

static const FlagDescription accelerationDescriptions[] = {
    { "DFXL_NONE", DFXL_NONE },
    { "DFXL_FILLRECTANGLE", DFXL_FILLRECTANGLE },
    { "DFXL_DRAWRECTANGLE", DFXL_DRAWRECTANGLE },
    { "DFXL_DRAWLINE", DFXL_DRAWLINE },
    { "DFXL_FILLTRIANGLE", DFXL_FILLTRIANGLE },
    { "DFXL_BLIT", DFXL_BLIT },
    { "DFXL_STRETCHBLIT", DFXL_STRETCHBLIT },
    { "DFXL_TEXTRIANGLES", DFXL_TEXTRIANGLES },
    { "DFXL_DRAWSTRING", DFXL_DRAWSTRING },
    { 0, 0 }
};

static const FlagDescription blitDescriptions[] = {
    { "DSBLIT_NOFX", DSBLIT_NOFX },
    { "DSBLIT_BLEND_ALPHACHANNEL", DSBLIT_BLEND_ALPHACHANNEL },
    { "DSBLIT_BLEND_COLORALPHA", DSBLIT_BLEND_COLORALPHA },
    { "DSBLIT_COLORIZE", DSBLIT_COLORIZE },
    { "DSBLIT_SRC_COLORKEY", DSBLIT_SRC_COLORKEY },
    { "DSBLIT_DST_COLORKEY", DSBLIT_DST_COLORKEY },
    { "DSBLIT_SRC_PREMULTIPLY", DSBLIT_SRC_PREMULTIPLY },
    { "DSBLIT_DST_PREMULTIPLY", DSBLIT_DST_PREMULTIPLY },
    { "DSBLIT_DEMULTIPLY", DSBLIT_DEMULTIPLY },
    { "DSBLIT_DEINTERLACE", DSBLIT_DEINTERLACE },
#if (Q_DIRECTFB_VERSION >= 0x000923)
    { "DSBLIT_SRC_PREMULTCOLOR", DSBLIT_SRC_PREMULTCOLOR },
    { "DSBLIT_XOR", DSBLIT_XOR },
#endif
#if (Q_DIRECTFB_VERSION >= 0x010000)
    { "DSBLIT_INDEX_TRANSLATION", DSBLIT_INDEX_TRANSLATION },
#endif
    { 0, 0 }
};

static const FlagDescription drawDescriptions[] = {
    { "DSDRAW_NOFX", DSDRAW_NOFX },
    { "DSDRAW_BLEND", DSDRAW_BLEND },
    { "DSDRAW_DST_COLORKEY", DSDRAW_DST_COLORKEY },
    { "DSDRAW_SRC_PREMULTIPLY", DSDRAW_SRC_PREMULTIPLY },
    { "DSDRAW_DST_PREMULTIPLY", DSDRAW_DST_PREMULTIPLY },
    { "DSDRAW_DEMULTIPLY", DSDRAW_DEMULTIPLY },
    { "DSDRAW_XOR", DSDRAW_XOR },
    { 0, 0 }
};
#endif

static const QByteArray flagDescriptions(uint mask, const FlagDescription *flags)
{
#ifdef QT_NO_DEBUG
    Q_UNUSED(mask);
    Q_UNUSED(flags);
    return QByteArray("");
#else
    if (!mask)
        return flags[0].name;

    QStringList list;
    for (int i=1; flags[i].name; ++i) {
        if (mask & flags[i].flag) {
            list.append(QString::fromLatin1(flags[i].name));
        }
    }
    Q_ASSERT(!list.isEmpty());
    return (QLatin1Char(' ') + list.join(QLatin1String("|"))).toLatin1();
#endif
}
static void printDirectFBInfo(IDirectFB *fb, IDirectFBSurface *primarySurface)
{
    DFBResult result;
    DFBGraphicsDeviceDescription dev;

    result = fb->GetDeviceDescription(fb, &dev);
    if (result != DFB_OK) {
        DirectFBError("Error reading graphics device description", result);
        return;
    }

    DFBSurfacePixelFormat pixelFormat;
    primarySurface->GetPixelFormat(primarySurface, &pixelFormat);

    qDebug("Device: %s (%s), Driver: %s v%i.%i (%s) Pixelformat: %d (%d)\n"
           "acceleration: 0x%x%s\nblit: 0x%x%s\ndraw: 0x%0x%s\nvideo: %iKB\n",
           dev.name, dev.vendor, dev.driver.name, dev.driver.major,
           dev.driver.minor, dev.driver.vendor, DFB_PIXELFORMAT_INDEX(pixelFormat),
           QDirectFBScreen::getImageFormat(primarySurface), dev.acceleration_mask,
           flagDescriptions(dev.acceleration_mask, accelerationDescriptions).constData(),
           dev.blitting_flags, flagDescriptions(dev.blitting_flags, blitDescriptions).constData(),
           dev.drawing_flags, flagDescriptions(dev.drawing_flags, drawDescriptions).constData(),
           (dev.video_memory >> 10));
}
#endif

static inline bool setIntOption(const QStringList &arguments, const QString &variable, int *value)
{
    Q_ASSERT(value);
    QRegExp rx(QString::fromLatin1("%1=?(\\d+)").arg(variable));
    rx.setCaseSensitivity(Qt::CaseInsensitive);
    if (arguments.indexOf(rx) != -1) {
        *value = rx.cap(1).toInt();
        return true;
    }
    return false;
}

static inline QColor colorFromName(const QString &name)
{
    QRegExp rx(QLatin1String("#([0-9a-f][0-9a-f])([0-9a-f][0-9a-f])([0-9a-f][0-9a-f])([0-9a-f][0-9a-f])"));
    rx.setCaseSensitivity(Qt::CaseInsensitive);
    if (rx.exactMatch(name)) {
        Q_ASSERT(rx.captureCount() == 4);
        int ints[4];
        int i;
        for (i=0; i<4; ++i) {
            bool ok;
            ints[i] = rx.cap(i + 1).toUInt(&ok, 16);
            if (!ok || ints[i] > 255)
                break;
        }
        if (i == 4)
            return QColor(ints[0], ints[1], ints[2], ints[3]);
    }
    return QColor(name);
}

bool QDirectFBScreen::connect(const QString &displaySpec)
{
    DFBResult result = DFB_OK;

    {   // pass command line arguments to DirectFB
        const QStringList args = QCoreApplication::arguments();
        int argc = args.size();
        char **argv = new char*[argc];

        for (int i = 0; i < argc; ++i)
            argv[i] = qstrdup(args.at(i).toLocal8Bit().constData());

        result = DirectFBInit(&argc, &argv);
        if (result != DFB_OK) {
            DirectFBError("QDirectFBScreen: error initializing DirectFB",
                          result);
        }
        delete[] argv;
    }

    const QStringList displayArgs = displaySpec.split(QLatin1Char(':'),
                                                      QString::SkipEmptyParts);

    d_ptr->setFlipFlags(displayArgs);

    result = DirectFBCreate(&d_ptr->dfb);
    if (result != DFB_OK) {
        DirectFBError("QDirectFBScreen: error creating DirectFB interface",
                      result);
        return false;
    }

    if (displayArgs.contains(QLatin1String("videoonly"), Qt::CaseInsensitive))
        d_ptr->directFBFlags |= VideoOnly;

    if (displayArgs.contains(QLatin1String("systemonly"), Qt::CaseInsensitive)) {
        if (d_ptr->directFBFlags & VideoOnly) {
            qWarning("QDirectFBScreen: error. videoonly and systemonly are mutually exclusive");
        } else {
            d_ptr->directFBFlags |= SystemOnly;
        }
    }

    if (displayArgs.contains(QLatin1String("boundingrectflip"), Qt::CaseInsensitive)) {
        d_ptr->directFBFlags |= BoundingRectFlip;
    } else if (displayArgs.contains(QLatin1String("nopartialflip"), Qt::CaseInsensitive)) {
        d_ptr->directFBFlags |= NoPartialFlip;
    }

#ifdef QT_DIRECTFB_IMAGECACHE
    int imageCacheSize = 4 * 1024 * 1024; // 4 MB
    setIntOption(displayArgs, QLatin1String("imagecachesize"), &imageCacheSize);
    QDirectFBPaintEngine::initImageCache(imageCacheSize);
#endif

#ifndef QT_NO_DIRECTFB_WM
    if (displayArgs.contains(QLatin1String("fullscreen")))
#endif
        d_ptr->dfb->SetCooperativeLevel(d_ptr->dfb, DFSCL_FULLSCREEN);

    const bool forcePremultiplied = displayArgs.contains(QLatin1String("forcepremultiplied"), Qt::CaseInsensitive);

    DFBSurfaceDescription description;
    memset(&description, 0, sizeof(DFBSurfaceDescription));
    IDirectFBSurface *surface;

#ifdef QT_NO_DIRECTFB_WM
    description.flags = DSDESC_CAPS;
    if (::setIntOption(displayArgs, QLatin1String("width"), &description.width))
        description.flags |= DSDESC_WIDTH;
    if (::setIntOption(displayArgs, QLatin1String("height"), &description.height))
        description.flags |= DSDESC_HEIGHT;

    description.caps = DSCAPS_PRIMARY|DSCAPS_DOUBLE;
    struct {
        const char *name;
        const DFBSurfaceCapabilities cap;
    } const capabilities[] = {
        { "static_alloc", DSCAPS_STATIC_ALLOC },
        { "triplebuffer", DSCAPS_TRIPLE },
        { "interlaced", DSCAPS_INTERLACED },
        { "separated", DSCAPS_SEPARATED },
//        { "depthbuffer", DSCAPS_DEPTH }, // only makes sense with TextureTriangles which are not supported
        { 0, DSCAPS_NONE }
    };
    for (int i=0; capabilities[i].name; ++i) {
        if (displayArgs.contains(QString::fromLatin1(capabilities[i].name), Qt::CaseInsensitive))
            description.caps |= capabilities[i].cap;
    }

    if (forcePremultiplied) {
        description.caps |= DSCAPS_PREMULTIPLIED;
    }

    // We don't track the primary surface as it's released in disconnect
    d_ptr->primarySurface = createDFBSurface(description, DontTrackSurface, &result);
    if (!d_ptr->primarySurface) {
        DirectFBError("QDirectFBScreen: error creating primary surface",
                      result);
        return false;
    }

    surface = d_ptr->primarySurface;
#else
    description.flags = DSDESC_WIDTH|DSDESC_HEIGHT;
    description.width = description.height = 1;
    surface = createDFBSurface(description, DontTrackSurface, &result);
    if (!surface) {
        DirectFBError("QDirectFBScreen: error creating surface", result);
        return false;
    }
#endif
    // Work out what format we're going to use for surfaces with an alpha channel
    QImage::Format pixelFormat = QDirectFBScreen::getImageFormat(surface);
    d_ptr->alphaPixmapFormat = pixelFormat;

    switch (pixelFormat) {
    case QImage::Format_RGB666:
        d_ptr->alphaPixmapFormat = QImage::Format_ARGB6666_Premultiplied;
        break;
    case QImage::Format_RGB444:
        d_ptr->alphaPixmapFormat = QImage::Format_ARGB4444_Premultiplied;
        break;
    case QImage::Format_RGB32:
        pixelFormat = d_ptr->alphaPixmapFormat = QImage::Format_ARGB32_Premultiplied;
        // ### Format_RGB32 doesn't work so well with Qt. Force ARGB32 for windows/pixmaps
        break;
    case QImage::Format_Indexed8:
        qWarning("QDirectFBScreen::connect(). Qt/DirectFB does not work with the LUT8  pixelformat.");
        return false;
    case QImage::NImageFormats:
    case QImage::Format_Invalid:
    case QImage::Format_Mono:
    case QImage::Format_MonoLSB:
    case QImage::Format_RGB888:
    case QImage::Format_RGB16:
    case QImage::Format_RGB555:
        d_ptr->alphaPixmapFormat = QImage::Format_ARGB32_Premultiplied;
        break;
    case QImage::Format_ARGB32:
        if (forcePremultiplied)
            d_ptr->alphaPixmapFormat = pixelFormat = QImage::Format_ARGB32_Premultiplied;
    case QImage::Format_ARGB32_Premultiplied:
    case QImage::Format_ARGB4444_Premultiplied:
    case QImage::Format_ARGB8555_Premultiplied:
    case QImage::Format_ARGB8565_Premultiplied:
    case QImage::Format_ARGB6666_Premultiplied:
        // works already
        break;
    }
    setPixelFormat(pixelFormat);
    QScreen::d = QDirectFBScreen::depth(pixelFormat);
    data = 0;
    lstep = 0;
    size = 0;

    if (result != DFB_OK) {
        DirectFBError("QDirectFBScreen::connect: "
                      "Unable to get screen!", result);
        return false;
    }
    const QString qws_size = QString::fromLatin1(qgetenv("QWS_SIZE"));
    if (!qws_size.isEmpty()) {
        QRegExp rx(QLatin1String("(\\d+)x(\\d+)"));
        if (!rx.exactMatch(qws_size)) {
            qWarning("QDirectFBScreen::connect: Can't parse QWS_SIZE=\"%s\"", qPrintable(qws_size));
        } else {
            int *ints[2] = { &w, &h };
            for (int i=0; i<2; ++i) {
                *ints[i] = rx.cap(i + 1).toInt();
                if (*ints[i] <= 0) {
                    qWarning("QDirectFBScreen::connect: %s is not a positive integer",
                             qPrintable(rx.cap(i + 1)));
                    w = h = 0;
                    break;
                }
            }
        }
    }

    setIntOption(displayArgs, QLatin1String("width"), &w);
    setIntOption(displayArgs, QLatin1String("height"), &h);

#ifndef QT_NO_DIRECTFB_LAYER
    int layerId = DLID_PRIMARY;
    setIntOption(displayArgs, QLatin1String("layerid"), &layerId);

    result = d_ptr->dfb->GetDisplayLayer(d_ptr->dfb, static_cast<DFBDisplayLayerID>(layerId),
                                         &d_ptr->dfbLayer);
    if (result != DFB_OK) {
        DirectFBError("QDirectFBScreen::connect: "
                      "Unable to get display layer!", result);
        return false;
    }
    result = d_ptr->dfbLayer->GetScreen(d_ptr->dfbLayer, &d_ptr->dfbScreen);
#else
    result = d_ptr->dfb->GetScreen(d_ptr->dfb, 0, &d_ptr->dfbScreen);
#endif

    if (w <= 0 || h <= 0) {
#ifdef QT_NO_DIRECTFB_WM
        result = d_ptr->primarySurface->GetSize(d_ptr->primarySurface, &w, &h);
#elif (Q_DIRECTFB_VERSION >= 0x010000)
        IDirectFBSurface *layerSurface;
        if (d_ptr->dfbLayer->GetSurface(d_ptr->dfbLayer, &layerSurface) == DFB_OK) {
            result = layerSurface->GetSize(layerSurface, &w, &h);
            layerSurface->Release(layerSurface);
        }
        if (w <= 0 || h <= 0) {
            result = d_ptr->dfbScreen->GetSize(d_ptr->dfbScreen, &w, &h);
        }
#else
        qWarning("QDirectFBScreen::connect: DirectFB versions prior to 1.0 do not offer a way\n"
                 "query the size of the primary surface in windowed mode. You have to specify\n"
                 "the size of the display using QWS_SIZE=[0-9]x[0-9] or\n"
                 "QWS_DISPLAY=directfb:width=[0-9]:height=[0-9]");
        return false;
#endif
        if (result != DFB_OK) {
            DirectFBError("QDirectFBScreen::connect: "
                          "Unable to get screen size!", result);
            return false;
        }
    }


    dw = w;
    dh = h;

    Q_ASSERT(dw != 0 && dh != 0);

    physWidth = physHeight = -1;
    setIntOption(displayArgs, QLatin1String("mmWidth"), &physWidth);
    setIntOption(displayArgs, QLatin1String("mmHeight"), &physHeight);
    const int dpi = 72;
    if (physWidth < 0)
        physWidth = qRound(dw * 25.4 / dpi);
    if (physHeight < 0)
        physHeight = qRound(dh * 25.4 / dpi);

    setGraphicsSystem(d_ptr);

#if (Q_DIRECTFB_VERSION >= 0x000923)
    if (displayArgs.contains(QLatin1String("debug"), Qt::CaseInsensitive))
        printDirectFBInfo(d_ptr->dfb, surface);
#endif
#ifdef QT_DIRECTFB_WM
    surface->Release(surface);
    QColor backgroundColor;
#else
    QColor &backgroundColor = d_ptr->backgroundColor;
#endif

    QRegExp backgroundColorRegExp(QLatin1String("bgcolor=(.+)"));
    backgroundColorRegExp.setCaseSensitivity(Qt::CaseInsensitive);
    if (displayArgs.indexOf(backgroundColorRegExp) != -1) {
        backgroundColor = colorFromName(backgroundColorRegExp.cap(1));
    }
#ifdef QT_NO_DIRECTFB_WM
    if (!backgroundColor.isValid())
        backgroundColor = Qt::green;
    d_ptr->primarySurface->Clear(d_ptr->primarySurface, backgroundColor.red(),
                                 backgroundColor.green(), backgroundColor.blue(),
                                 backgroundColor.alpha());
    d_ptr->primarySurface->Flip(d_ptr->primarySurface, 0, d_ptr->flipFlags);
#else
    if (backgroundColor.isValid()) {
        DFBResult result = d_ptr->dfbLayer->SetCooperativeLevel(d_ptr->dfbLayer, DLSCL_ADMINISTRATIVE);
        if (result != DFB_OK) {
            DirectFBError("QDirectFBScreen::connect "
                          "Unable to set cooperative level", result);
        }
        result = d_ptr->dfbLayer->SetBackgroundColor(d_ptr->dfbLayer, backgroundColor.red(), backgroundColor.green(),
                                                     backgroundColor.blue(), backgroundColor.alpha());
        if (result != DFB_OK) {
            DirectFBError("QDirectFBScreenCursor::connect: "
                          "Unable to set background color", result);
        }

        result = d_ptr->dfbLayer->SetBackgroundMode(d_ptr->dfbLayer, DLBM_COLOR);
        if (result != DFB_OK) {
            DirectFBError("QDirectFBScreenCursor::connect: "
                          "Unable to set background mode", result);
        }

        result = d_ptr->dfbLayer->SetCooperativeLevel(d_ptr->dfbLayer, DLSCL_SHARED);
        if (result != DFB_OK) {
            DirectFBError("QDirectFBScreen::connect "
                          "Unable to set cooperative level", result);
        }

    }
#endif

    return true;
}

void QDirectFBScreen::disconnect()
{
#if defined QT_DIRECTFB_IMAGEPROVIDER_KEEPALIVE
    if (d_ptr->imageProvider)
        d_ptr->imageProvider->Release(d_ptr->imageProvider);
#endif
#ifdef QT_NO_DIRECTFB_WM
    d_ptr->primarySurface->Release(d_ptr->primarySurface);
    d_ptr->primarySurface = 0;
#endif

    foreach (IDirectFBSurface *surf, d_ptr->allocatedSurfaces)
        surf->Release(surf);
    d_ptr->allocatedSurfaces.clear();

#ifndef QT_NO_DIRECTFB_LAYER
    d_ptr->dfbLayer->Release(d_ptr->dfbLayer);
    d_ptr->dfbLayer = 0;
#endif

    d_ptr->dfbScreen->Release(d_ptr->dfbScreen);
    d_ptr->dfbScreen = 0;

    d_ptr->dfb->Release(d_ptr->dfb);
    d_ptr->dfb = 0;
}

bool QDirectFBScreen::initDevice()
{
#ifndef QT_NO_DIRECTFB_MOUSE
    if (qgetenv("QWS_MOUSE_PROTO").isEmpty()) {
        QWSServer::instance()->setDefaultMouse("None");
        d_ptr->mouse = new QDirectFBMouseHandler;
    }
#endif
#ifndef QT_NO_DIRECTFB_KEYBOARD
    if (qgetenv("QWS_KEYBOARD").isEmpty()) {
        QWSServer::instance()->setDefaultKeyboard("None");
        d_ptr->keyboard = new QDirectFBKeyboardHandler(QString());
    }
#endif

#ifdef QT_DIRECTFB_CURSOR
    qt_screencursor = new QDirectFBScreenCursor;
#elif !defined QT_NO_QWS_CURSOR
    QScreenCursor::initSoftwareCursor();
#endif
    return true;
}

void QDirectFBScreen::shutdownDevice()
{
#ifndef QT_NO_DIRECTFB_MOUSE
    delete d_ptr->mouse;
    d_ptr->mouse = 0;
#endif
#ifndef QT_NO_DIRECTFB_KEYBOARD
    delete d_ptr->keyboard;
    d_ptr->keyboard = 0;
#endif

#ifndef QT_NO_QWS_CURSOR
    delete qt_screencursor;
    qt_screencursor = 0;
#endif
}

void QDirectFBScreen::setMode(int width, int height, int depth)
{
    d_ptr->dfb->SetVideoMode(d_ptr->dfb, width, height, depth);
}

void QDirectFBScreen::blank(bool on)
{
    d_ptr->dfbScreen->SetPowerMode(d_ptr->dfbScreen,
                                   (on ? DSPM_ON : DSPM_SUSPEND));
}

QWSWindowSurface *QDirectFBScreen::createSurface(QWidget *widget) const
{
#ifdef QT_NO_DIRECTFB_WM
    if (QApplication::type() == QApplication::GuiServer) {
        return new QDirectFBWindowSurface(d_ptr->flipFlags, const_cast<QDirectFBScreen*>(this), widget);
    } else {
        return QScreen::createSurface(widget);
    }
#else
    return new QDirectFBWindowSurface(d_ptr->flipFlags, const_cast<QDirectFBScreen*>(this), widget);
#endif
}

QWSWindowSurface *QDirectFBScreen::createSurface(const QString &key) const
{
    if (key == QLatin1String("directfb")) {
        return new QDirectFBWindowSurface(d_ptr->flipFlags, const_cast<QDirectFBScreen*>(this));
    }
    return QScreen::createSurface(key);
}

#if defined QT_NO_DIRECTFB_WM
struct PaintCommand {
    PaintCommand() : dfbSurface(0), windowOpacity(255), blittingFlags(DSBLIT_NOFX) {}
    IDirectFBSurface *dfbSurface;
    QImage image;
    QPoint windowPosition;
    QRegion source;
    quint8 windowOpacity;
    DFBSurfaceBlittingFlags blittingFlags;
};

static inline void initParameters(DFBRectangle &source, const QRect &sourceGlobal, const QPoint &pos)
{
    source.x = sourceGlobal.x() - pos.x();
    source.y = sourceGlobal.y() - pos.y();
    source.w = sourceGlobal.width();
    source.h = sourceGlobal.height();
}
#endif

void QDirectFBScreen::exposeRegion(QRegion r, int)
{
    Q_UNUSED(r);
#if defined QT_NO_DIRECTFB_WM

    r &= region();
    if (r.isEmpty()) {
        return;
    }
    r = r.boundingRect();

    IDirectFBSurface *primary = d_ptr->primarySurface;
    const QList<QWSWindow*> windows = QWSServer::instance()->clientWindows();
    QVarLengthArray<PaintCommand, 4> commands(windows.size());
    QRegion region = r;
    int idx = 0;
    for (int i=0; i<windows.size(); ++i) {
        QWSWindowSurface *surface = windows.at(i)->windowSurface();
        if (!surface)
            continue;

        const QRect windowGeometry = surface->geometry();
        const QRegion intersection = region & windowGeometry;
        if (intersection.isEmpty()) {
            continue;
        }

        PaintCommand &cmd = commands[idx];

        if (surface->key() == QLatin1String("directfb")) {
            const QDirectFBWindowSurface *ws = static_cast<QDirectFBWindowSurface*>(surface);
            cmd.dfbSurface = ws->directFBSurface();

            if (!cmd.dfbSurface) {
                continue;
            }
        } else {
            cmd.image = surface->image();
            if (cmd.image.isNull()) {
                continue;
            }
        }
        ++idx;

        cmd.windowPosition = windowGeometry.topLeft();
        cmd.source = intersection;
        if (windows.at(i)->isOpaque()) {
            region -= intersection;
            if (region.isEmpty())
                break;
        } else {
            cmd.windowOpacity = windows.at(i)->opacity();
            cmd.blittingFlags = cmd.windowOpacity == 255
                                ? DSBLIT_BLEND_ALPHACHANNEL
                                : (DSBLIT_BLEND_ALPHACHANNEL|DSBLIT_BLEND_COLORALPHA);
        }
    }

    solidFill(d_ptr->backgroundColor, region);

    while (idx > 0) {
        const PaintCommand &cmd = commands[--idx];
        Q_ASSERT(cmd.dfbSurface || !cmd.image.isNull());
        IDirectFBSurface *surface;
        if (cmd.dfbSurface) {
            surface = cmd.dfbSurface;
        } else {
            Q_ASSERT(!cmd.image.isNull());
            DFBResult result;
            surface = createDFBSurface(cmd.image, cmd.image.format(), DontTrackSurface, &result);
            Q_ASSERT((result != DFB_OK) == !surface);
            if (result != DFB_OK) {
                DirectFBError("QDirectFBScreen::exposeRegion: Can't create surface from image", result);
                continue;
            }
        }

        primary->SetBlittingFlags(primary, cmd.blittingFlags);
        if (cmd.blittingFlags & DSBLIT_BLEND_COLORALPHA) {
            primary->SetColor(primary, 0xff, 0xff, 0xff, cmd.windowOpacity);
        }
        const QRegion &region = cmd.source;
        const int rectCount = region.rectCount();
        DFBRectangle source;
        if (rectCount == 1) {
            ::initParameters(source, region.boundingRect(), cmd.windowPosition);
            primary->Blit(primary, surface, &source, cmd.windowPosition.x() + source.x, cmd.windowPosition.y() + source.y);
        } else {
            const QVector<QRect> rects = region.rects();
            for (int i=0; i<rectCount; ++i) {
                ::initParameters(source, rects.at(i), cmd.windowPosition);
                primary->Blit(primary, surface, &source, cmd.windowPosition.x() + source.x, cmd.windowPosition.y() + source.y);
            }
        }
        if (surface != cmd.dfbSurface) {
            surface->Release(surface);
        }
    }

    primary->SetColor(primary, 0xff, 0xff, 0xff, 0xff);

#if defined QT_NO_DIRECTFB_CURSOR and !defined QT_NO_QWS_CURSOR
    if (QScreenCursor *cursor = QScreenCursor::instance()) {
        const QRect cursorRectangle = cursor->boundingRect();
        if (cursor->isVisible() && !cursor->isAccelerated() && r.intersects(cursorRectangle)) {
            const QImage image = cursor->image();
            if (image.cacheKey() != d_ptr->cursorImageKey) {
                if (d_ptr->cursorSurface) {
                    releaseDFBSurface(d_ptr->cursorSurface);
                }
                d_ptr->cursorSurface = createDFBSurface(image, image.format(), QDirectFBScreen::TrackSurface);
                d_ptr->cursorImageKey = image.cacheKey();
            }

            Q_ASSERT(d_ptr->cursorSurface);
            primary->SetBlittingFlags(primary, DSBLIT_BLEND_ALPHACHANNEL);
            primary->Blit(primary, d_ptr->cursorSurface, 0, cursorRectangle.x(), cursorRectangle.y());
        }
    }
#endif
    flipSurface(primary, d_ptr->flipFlags, r, QPoint());
    primary->SetBlittingFlags(primary, DSBLIT_NOFX);
#endif
}

void QDirectFBScreen::solidFill(const QColor &color, const QRegion &region)
{
#ifdef QT_DIRECTFB_WM
    Q_UNUSED(color);
    Q_UNUSED(region);
#else
    QDirectFBScreen::solidFill(d_ptr->primarySurface, color, region);
#endif
}

static inline void clearRect(IDirectFBSurface *surface, const QColor &color, const QRect &rect)
{
    Q_ASSERT(surface);
    const DFBRegion region = { rect.left(), rect.top(), rect.right(), rect.bottom() };
    // could just reinterpret_cast this to a DFBRegion
    surface->SetClip(surface, &region);
    surface->Clear(surface, color.red(), color.green(), color.blue(), color.alpha());
}

void QDirectFBScreen::solidFill(IDirectFBSurface *surface, const QColor &color, const QRegion &region)
{
    if (region.isEmpty())
        return;

    const int n = region.rectCount();
    if (n == 1) {
        clearRect(surface, color, region.boundingRect());
    } else {
        const QVector<QRect> rects = region.rects();
        for (int i=0; i<n; ++i) {
            clearRect(surface, color, rects.at(i));
        }
    }
    surface->SetClip(surface, 0);
}

QImage::Format QDirectFBScreen::alphaPixmapFormat() const
{
    return d_ptr->alphaPixmapFormat;
}

bool QDirectFBScreen::initSurfaceDescriptionPixelFormat(DFBSurfaceDescription *description,
                                                        QImage::Format format)
{
    const DFBSurfacePixelFormat pixelformat = QDirectFBScreen::getSurfacePixelFormat(format);
    if (pixelformat == DSPF_UNKNOWN)
        return false;
    description->flags |= DSDESC_PIXELFORMAT;
    description->pixelformat = pixelformat;
    if (QDirectFBScreen::isPremultiplied(format)) {
        if (!(description->flags & DSDESC_CAPS)) {
            description->caps = DSCAPS_PREMULTIPLIED;
            description->flags |= DSDESC_CAPS;
        } else {
            description->caps |= DSCAPS_PREMULTIPLIED;
        }
    }
    return true;
}

uchar *QDirectFBScreen::lockSurface(IDirectFBSurface *surface, DFBSurfaceLockFlags flags, int *bpl)
{
    void *mem = 0;
    const DFBResult result = surface->Lock(surface, flags, &mem, bpl);
    if (result != DFB_OK) {
        DirectFBError("QDirectFBScreen::lockSurface()", result);
    }

    return reinterpret_cast<uchar*>(mem);
}

static inline bool isFullUpdate(IDirectFBSurface *surface, const QRegion &region, const QPoint &offset)
{
    if (offset == QPoint(0, 0) && region.rectCount() == 1) {
	QSize size;
	surface->GetSize(surface, &size.rwidth(), &size.rheight());
	if (region.boundingRect().size() == size)
	    return true;
    }
    return false;
}

void QDirectFBScreen::flipSurface(IDirectFBSurface *surface, DFBSurfaceFlipFlags flipFlags,
                                  const QRegion &region, const QPoint &offset)
{
    if (d_ptr->directFBFlags & NoPartialFlip
        || (!(flipFlags & DSFLIP_BLIT) && QT_PREPEND_NAMESPACE(isFullUpdate(surface, region, offset)))) {
        surface->Flip(surface, 0, flipFlags);
    } else {
        if (!(d_ptr->directFBFlags & BoundingRectFlip) && region.rectCount() > 1) {
            const QVector<QRect> rects = region.rects();
            const DFBSurfaceFlipFlags nonWaitFlags = flipFlags & ~DSFLIP_WAIT;
            for (int i=0; i<rects.size(); ++i) {
                const QRect &r = rects.at(i);
                const DFBRegion dfbReg = { r.x() + offset.x(), r.y() + offset.y(),
                                           r.right() + offset.x(),
                                           r.bottom() + offset.y() };
                surface->Flip(surface, &dfbReg, i + 1 < rects.size() ? nonWaitFlags : flipFlags);
            }
        } else {
            const QRect r = region.boundingRect();
            const DFBRegion dfbReg = { r.x() + offset.x(), r.y() + offset.y(),
                                       r.right() + offset.x(),
                                       r.bottom() + offset.y() };
            surface->Flip(surface, &dfbReg, flipFlags);
        }
    }
}

#if defined QT_DIRECTFB_IMAGEPROVIDER_KEEPALIVE
void QDirectFBScreen::setDirectFBImageProvider(IDirectFBImageProvider *provider)
{
    Q_ASSERT(provider);
    if (d_ptr->imageProvider)
        d_ptr->imageProvider->Release(d_ptr->imageProvider);
    d_ptr->imageProvider = provider;
}
#endif

void QDirectFBScreen::waitIdle()
{
    d_ptr->dfb->WaitIdle(d_ptr->dfb);
}

#ifdef QT_DIRECTFB_WM
IDirectFBWindow *QDirectFBScreen::windowForWidget(const QWidget *widget) const
{
    if (widget) {
        const QWSWindowSurface *surface = static_cast<const QWSWindowSurface*>(widget->windowSurface());
        if (surface && surface->key() == QLatin1String("directfb")) {
            return static_cast<const QDirectFBWindowSurface*>(surface)->directFBWindow();
        }
    }
    return 0;
}
#endif

IDirectFBSurface * QDirectFBScreen::surfaceForWidget(const QWidget *widget, QRect *rect) const
{
    Q_ASSERT(widget);
    if (!widget->isVisible() || widget->size().isNull())
        return 0;

    const QWSWindowSurface *surface = static_cast<const QWSWindowSurface*>(widget->windowSurface());
    if (surface && surface->key() == QLatin1String("directfb")) {
        return static_cast<const QDirectFBWindowSurface*>(surface)->surfaceForWidget(widget, rect);
    }
    return 0;
}

#ifdef QT_DIRECTFB_SUBSURFACE
IDirectFBSurface *QDirectFBScreen::subSurfaceForWidget(const QWidget *widget, const QRect &area) const
{
    Q_ASSERT(widget);
    QRect rect;
    IDirectFBSurface *surface = surfaceForWidget(widget, &rect);
    IDirectFBSurface *subSurface = 0;
    if (surface) {
        if (!area.isNull())
            rect &= area.translated(widget->mapTo(widget->window(), QPoint(0, 0)));
        if (!rect.isNull()) {
            const DFBRectangle subRect = { rect.x(), rect.y(), rect.width(), rect.height() };
            const DFBResult result = surface->GetSubSurface(surface, &subRect, &subSurface);
            if (result != DFB_OK) {
                DirectFBError("QDirectFBScreen::subSurface(): Can't get sub surface", result);
            }
        }
    }
    return subSurface;
}
#endif

Q_GUI_EXPORT IDirectFBSurface *qt_directfb_surface_for_widget(const QWidget *widget, QRect *rect)
{
    return QDirectFBScreen::instance() ? QDirectFBScreen::instance()->surfaceForWidget(widget, rect) : 0;
}
#ifdef QT_DIRECTFB_SUBSURFACE
Q_GUI_EXPORT IDirectFBSurface *qt_directfb_subsurface_for_widget(const QWidget *widget, const QRect &area)
{
    return QDirectFBScreen::instance() ? QDirectFBScreen::instance()->subSurfaceForWidget(widget, area) : 0;
}
#endif
#ifdef QT_DIRECTFB_WM
Q_GUI_EXPORT IDirectFBWindow *qt_directfb_window_for_widget(const QWidget *widget)
{
    return QDirectFBScreen::instance() ? QDirectFBScreen::instance()->windowForWidget(widget) : 0;
}

#endif

QT_END_NAMESPACE

#include "qdirectfbscreen.moc"
#endif // QT_NO_QWS_DIRECTFB

