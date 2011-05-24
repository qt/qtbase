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

#include "qscreenahi_qws.h"

#ifndef QT_NO_QWS_AHI

#include <QtGui/qcolor.h>
#include <QtGui/qapplication.h>
#include <QtCore/qvector.h>
#include <QtCore/qvarlengtharray.h>
#include <private/qwssignalhandler_p.h>

#include <ahi.h>

//#define QAHISCREEN_DEBUG

static int depthForPixelFormat(const AhiPixelFormat_t format)
{
    switch (format) {
    case AhiPix1bpp:
        return 1;
    case AhiPix2bpp:
        return 2;
    case AhiPix4bpp:
        return 4;
    case AhiPix8bpp_332RGB:
    case AhiPix8bpp:
        return 8;
    case AhiPix16bpp_444RGB:
        return 12;
    case AhiPix16bpp_555RGB:
        return 15;
    case AhiPix16bpp_565RGB:
        return 16;
    case AhiPix32bpp_8888ARGB:
    case AhiPix32bpp_8888BGRA:
        return 32;
    default:
        return 0;
    }
}

static AhiPixelFormat_t pixelFormatForImageFormat(const QImage::Format format)
{
    switch (format) {
    case QImage::Format_Mono:
    case QImage::Format_MonoLSB:
        return AhiPix1bpp;
    case QImage::Format_Indexed8:
        return AhiPix8bpp;
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
        return AhiPix32bpp_8888ARGB;
    case QImage::Format_RGB16:
        return AhiPix16bpp_565RGB;
    case QImage::Format_RGB555:
        return AhiPix16bpp_555RGB;
    case QImage::Format_ARGB4444_Premultiplied:
    case QImage::Format_RGB444:
        return AhiPix16bpp_444RGB;
    default:
        return AhiPixelFormatMax;
    }
}

class QAhiScreenCursor : public QScreenCursor
{
public:
    QAhiScreenCursor(QScreen *screen, AhiDevCtx_t context);

    void set(const QImage &image, int hotx, int hoty);
    void move(int x, int y);
    void show();
    void hide();

private:
    QScreen *screen;
    AhiDevCtx_t context;
};

QAhiScreenCursor::QAhiScreenCursor(QScreen *s, AhiDevCtx_t c)
    : QScreenCursor(), screen(s), context(c)
{
    hwaccel = true;
    supportsAlpha = true;

    if (enable)
        show();
    else
        hide();
}

void QAhiScreenCursor::set(const QImage &image, int hotx, int hoty)
{
    if (image.isNull()) {
        QScreenCursor::set(image, hotx, hoty);
        return;
    }

    if (image.format() != QImage::Format_MonoLSB) {
        set(image.convertToFormat(QImage::Format_MonoLSB), hotx, hoty);
        return;
    }

    AhiPixelFormat_t pixFmt = pixelFormatForImageFormat(image.format());

    if (pixFmt >= AhiPixelFormatMax) { // generic fallback
        QImage::Format toFormat = screen->pixelFormat();
        if (toFormat == QImage::Format_Invalid)
            toFormat = QImage::Format_ARGB32;
        set(image.convertToFormat(toFormat), hotx, hoty);
        return;
    }

    AhiPoint_t hotSpot = { hotx, hoty };
    AhiSize_t bitmapSize = { image.width(), image.height() };
    AhiBitmap_t bitmap = { bitmapSize, (void*)(image.bits()),
                           image.bytesPerLine(), pixFmt };

    AhiSts_t status;
    status = AhiDispCursorSet(context, AhiCursor1, &bitmap, &hotSpot,
                              image.serialNumber(), 0);
    if (status != AhiStsOk)
        qWarning("QAhiScreenCursor::set(): AhiDispCursorSet failed: %x",
                 status);

    QScreenCursor::set(image, hotx, hoty);
}

void QAhiScreenCursor::move(int x, int y)
{
    AhiPoint_t pos = { x, y };
    AhiSts_t status = AhiDispCursorPos(context, AhiCursor1, &pos, 0);
    if (status != AhiStsOk)
        qWarning("QAhiScreenCursor::move(): error setting mouse position: %x",
                 status);
    QScreenCursor::move(x, y);
}

void QAhiScreenCursor::show()
{
    AhiSts_t status;
    status = AhiDispCursorState(context, AhiCursor1, AhiCursorStateOn, 0);
    if (status != AhiStsOk)
        qWarning("QAhiScreenCursor::show(): error setting state: %x", status);
    QScreenCursor::show();
}

void QAhiScreenCursor::hide()
{
    AhiDispCursorState(context, AhiCursor1, AhiCursorStateOff, 0);
    QScreenCursor::hide();
}

class QAhiScreenPrivate : public QObject
{
public:
    QAhiScreenPrivate();
    ~QAhiScreenPrivate();

    bool setMode(AhiDispMode_t mode);

    AhiDevCtx_t context;
    AhiSurf_t surface;
    QAhiScreenCursor *cursor;
};

QT_BEGIN_NAMESPACE

QAhiScreenPrivate::QAhiScreenPrivate()
    : context(0), surface(0), cursor(0)
{
#ifndef QT_NO_QWS_SIGNALHANDLER
    QWSSignalHandler::instance()->addObject(this);
#endif
}

QAhiScreenPrivate::~QAhiScreenPrivate()
{
    delete cursor;

    if (surface) {
        AhiSurfFree(context, surface);
        surface = 0;
    }
    if (context) {
        AhiDevClose(context);
        context = 0;
    }
    AhiTerm();
}

bool QAhiScreenPrivate::setMode(AhiDispMode_t mode)
{
    AhiSts_t status;

    status = AhiDispModeSet(context, &mode, 0);
    if (status != AhiStsOk) {
        qCritical("QAhiScreenPrivate::setMode(): AhiDispModeSet failed: %x",
                  status);
        return false;
    }

    if (surface) {
        AhiSurfFree(context, surface);
        surface = 0;
    }
    status = AhiSurfAlloc(context, &surface, &mode.size, mode.pixFmt,
                          AHIFLAG_SURFFIXED);
    if (status != AhiStsOk) {
        qCritical("QAhiScreenPrivate::setMode(): AhisurfAlloc failed: %x",
                  status);
        return false;
    }

    status = AhiDispSurfSet(context, surface, 0);
    if (status != AhiStsOk) {
        qCritical("QAhiScreenPrivate::setMode(): AhiDispSurfSet failed: %x",
                  status);
        return false;
    }

    return true;
}

QAhiScreen::QAhiScreen(int displayId)
    : QScreen(displayId), d_ptr(new QAhiScreenPrivate)
{
}

QAhiScreen::~QAhiScreen()
{
    delete d_ptr;
}

bool QAhiScreen::configure()
{
    AhiSurfInfo_t surfaceInfo;
    AhiSts_t status;

    status = AhiSurfInfo(d_ptr->context, d_ptr->surface, &surfaceInfo);
    if (status != AhiStsOk) {
        qCritical("QAhiScreen::configure(): AhiSurfInfo failed: %x", status);
        return false;
    }

    QScreen::data = 0;
    QScreen::w = QScreen::dw = surfaceInfo.size.cx;
    QScreen::h = QScreen::dh = surfaceInfo.size.cy;
    QScreen::lstep = surfaceInfo.stride;
    QScreen::size = surfaceInfo.sizeInBytes;

    switch (surfaceInfo.pixFmt) {
    case AhiPix1bpp:
        setPixelFormat(QImage::Format_Mono);
        QScreen::d = 1;
        break;
    case AhiPix4bpp:
        QScreen::d = 4;
        break;
    case AhiPix8bpp_332RGB:
    case AhiPix8bpp:
        QScreen::d = 8;
        break;
    case AhiPix16bpp_444RGB:
        setPixelFormat(QImage::Format_RGB444);
        QScreen::d = 12;
        break;
    case AhiPix16bpp_555RGB:
        setPixelFormat(QImage::Format_RGB555);
        QScreen::d = 15;
        break;
    case AhiPix16bpp_565RGB:
        setPixelFormat(QImage::Format_RGB16);
        QScreen::d = 16;
        break;
    case AhiPix2bpp:
        QScreen::d = 2;
        break;
    case AhiPix32bpp_8888ARGB:
        setPixelFormat(QImage::Format_ARGB32);
        // fallthrough
    case AhiPix32bpp_8888BGRA:
        QScreen::d = 32;
        break;
    default:
        qCritical("QAhiScreen::configure(): Unknown pixel format: %x",
                  surfaceInfo.pixFmt);
        return false;
    }

    const int dpi = 72;
    QScreen::physWidth = qRound(QScreen::dw * 25.4 / dpi);
    QScreen::physHeight = qRound(QScreen::dh * 25.4 / dpi);

    return true;
}

bool QAhiScreen::connect(const QString &displaySpec)
{
    Q_UNUSED(displaySpec);

    AhiSts_t status;

    status = AhiInit(0);
    if (status != AhiStsOk) {
        qCritical("QAhiScreen::connect(): AhiInit failed: %x", status);
        return false;
    }

    AhiDev_t device;
    AhiDevInfo_t info;

    status = AhiDevEnum(&device, &info, 0);
    if (status != AhiStsOk) {
        qCritical("QAhiScreen::connect(): AhiDevEnum failed: %x", status);
        return false;
    }
#ifdef QAHISCREEN_DEBUG
    {
        int displayNo = 0;
        AhiDevInfo_t dispInfo = info;
        qDebug("AHI supported devices:");
        do {
            qDebug("  %2i: %s, sw version: %s (rev %u)\n"
                   "       chip: 0x%x (rev %u), mem: %i (%i/%i), bus: 0x%x",
                   displayNo, dispInfo.name,
                   dispInfo.swVersion, uint(dispInfo.swRevision),
                   uint(dispInfo.chipId), uint(dispInfo.revisionId),
                   uint(dispInfo.totalMemory),
                   uint(dispInfo.internalMemSize),
                   uint(dispInfo.externalMemSize),
                   uint(dispInfo.cpuBusInterfaceMode));
            status = AhiDevEnum(&device, &info, ++displayNo);
        } while (status == AhiStsOk);
    }
#endif

    status = AhiDevOpen(&d_ptr->context, device, "qscreenahi",
                        AHIFLAG_USERLEVEL);
    if (status != AhiStsOk) {
        qCritical("QAhiScreen::connect(): AhiDevOpen failed: %x", status);
        return false;
    }

    AhiDispMode_t mode;

    status = AhiDispModeEnum(d_ptr->context, &mode, 0);
    if (status != AhiStsOk) {
        qCritical("QAhiScreen::connect(): AhiDispModeEnum failed: %x", status);
        return false;
    }

#ifdef QAHISCREEN_DEBUG
    {
        int modeNo = 0;
        AhiDispMode_t modeInfo = mode;
        qDebug("AHI supported modes:");
        do {
            qDebug("  %2i: %ux%u, fmt: %i, %u Hz, rot: %i, mirror: %i",
                   modeNo, uint(modeInfo.size.cx), uint(modeInfo.size.cy),
                   modeInfo.pixFmt, uint(modeInfo.frequency),
                   modeInfo.rotation, modeInfo.mirror);
            status = AhiDispModeEnum(d_ptr->context, &modeInfo, ++modeNo);
        } while (status == AhiStsOk);
    }
#endif

    if (QApplication::type() == QApplication::GuiServer) {
        if (!d_ptr->setMode(mode))
            return false;
    } else {
        status = AhiDispSurfGet(d_ptr->context, &d_ptr->surface);
        if (status != AhiStsOk) {
            qCritical("QAhiScreen::connect(): AhiDispSurfGet failed: %x",
                      status);
            return false;
        }

        status = AhiDispModeGet(d_ptr->context, &mode);
        if (status != AhiStsOk) {
            qCritical("QAhiScreen::context(): AhiDispModeGet failed: %x",
                      status);
            return false;
        }
    }

    return configure();
}

void QAhiScreen::disconnect()
{
    AhiSurfFree(d_ptr->context, d_ptr->surface);
    d_ptr->surface = 0;
    AhiDevClose(d_ptr->context);
    d_ptr->context = 0;
    AhiTerm();
}

bool QAhiScreen::initDevice()
{
    QScreenCursor::initSoftwareCursor();

    AhiSts_t status = AhiDispState(d_ptr->context, AhiDispStateOn, 0);
    if (status != AhiStsOk) {
        qCritical("QAhiScreen::connect(): AhiDispState failed: %x", status);
        return false;
    }

    return true;
}

void QAhiScreen::shutdownDevice()
{
    AhiDispState(d_ptr->context, AhiDispStateOff, 0);
}

void QAhiScreen::setMode(int width, int height, int depth)
{
    int modeNo = 0;
    AhiDispMode_t mode;
    AhiSts_t status = AhiStsOk;

    while (status == AhiStsOk) {
        status = AhiDispModeEnum(d_ptr->context, &mode, modeNo);
        if (mode.size.cx == uint(width) &&
            mode.size.cy == uint(height) &&
            depthForPixelFormat(mode.pixFmt) == depth)
        {
            d_ptr->setMode(mode);
            configure();
            return;
        }
    }
}

void QAhiScreen::blit(const QImage &image, const QPoint &topLeft,
                      const QRegion &reg)
{
    AhiPixelFormat_t pixFmt = pixelFormatForImageFormat(image.format());

    if (pixFmt >= AhiPixelFormatMax) { // generic fallback
        QImage::Format toFormat = pixelFormat();
        if (toFormat == QImage::Format_Invalid)
            toFormat = QImage::Format_ARGB32;
        blit(image.convertToFormat(toFormat), topLeft, reg);
        return;
    }

    AhiSts_t status;

    status = AhiDrawSurfDstSet(d_ptr->context, d_ptr->surface, 0);
    if (status != AhiStsOk) {
        qWarning("QAhiScreen::blit(): AhiDrawSurfDstSet failed: %x", status);
        return;
    }

    const QVector<QRect> rects = (reg & region()).rects();
    const int numRects = rects.size();
    QVarLengthArray<AhiPoint_t, 8> src(numRects);
    QVarLengthArray<AhiRect_t, 8> dest(numRects);

    for (int i = 0; i < numRects; ++i) {
        const QRect rect = rects.at(i);

        src[i].x = rect.x() - topLeft.x();
        src[i].y = rect.y() - topLeft.y();
        dest[i].left = rect.left();
        dest[i].top = rect.top();
        dest[i].right = rect.x() + rect.width();
        dest[i].bottom = rect.y() + rect.height();
    }

    AhiSize_t bitmapSize = { image.width(), image.height() };
    AhiBitmap_t bitmap = { bitmapSize, (void*)(image.bits()),
                           image.bytesPerLine(), pixFmt };

    status = AhiDrawRopSet(d_ptr->context, AHIMAKEROP3(AHIROPSRCCOPY));
    if (status != AhiStsOk) {
        qWarning("QAhiScreen::blit(): AhiDrawRopSet failed: %x", status);
        return;
    }

    for (int i = 0; i < numRects; ++i) {
        status = AhiDrawBitmapBlt(d_ptr->context, &dest[i], &src[i],
                                  &bitmap, 0, 0);
        if (status != AhiStsOk) {
            qWarning("QAhiScreen::blit(): AhiDrawBitmapBlt failed: %x",
                     status);
            break;
        }
    }
}

void QAhiScreen::solidFill(const QColor &color, const QRegion &reg)
{
    AhiSts_t status = AhiStsOk;

    switch (pixelFormat()) {
    case QImage::Format_ARGB32_Premultiplied:
    case QImage::Format_ARGB32:
    case QImage::Format_RGB32:
        status = AhiDrawBrushFgColorSet(d_ptr->context, color.rgba());
        break;
    case QImage::Format_RGB16:
        status = AhiDrawBrushFgColorSet(d_ptr->context, qt_convRgbTo16(color.rgb()));
        break;
    default:
        qFatal("QAhiScreen::solidFill(): Not implemented for pixel format %d",
               int(pixelFormat()));
        break;
    }

    if (status != AhiStsOk) {
        qWarning("QAhiScreen::solidFill(): AhiDrawBrushFgColorSet failed: %x",
                 status);
        return;
    }

    status = AhiDrawBrushSet(d_ptr->context, 0, 0, 0, AHIFLAG_BRUSHSOLID);
    if (status != AhiStsOk) {
        qWarning("QAhiScreen::solidFill(): AhiDrawBrushSet failed: %x",
                 status);
        return;
    }

    status = AhiDrawRopSet(d_ptr->context, AHIMAKEROP3(AHIROPPATCOPY));
    if (status != AhiStsOk) {
        qWarning("QAhiScreen::solidFill(): AhiDrawRopSet failed: %x", status);
        return;
    }

    status = AhiDrawSurfDstSet(d_ptr->context, d_ptr->surface, 0);
    if (status != AhiStsOk) {
        qWarning("QAhiScreen::solidFill(): AhiDrawSurfDst failed: %x", status);
        return;
    }

    const QVector<QRect> rects = (reg & region()).rects();
    QVarLengthArray<AhiRect_t> ahiRects(rects.size());

    for (int i = 0; i < rects.size(); ++i) {
        const QRect rect = rects.at(i);
        ahiRects[i].left = rect.left();
        ahiRects[i].top = rect.top();
        ahiRects[i].right = rect.x() + rect.width();
        ahiRects[i].bottom = rect.y() + rect.height();
    }

    status = AhiDrawBitBltMulti(d_ptr->context, ahiRects.data(),
                                0, ahiRects.size());
    if (status != AhiStsOk)
        qWarning("QAhiScreen::solidFill(): AhiDrawBitBlt failed: %x", status);
}

QT_END_NAMESPACE

#endif // QT_NO_QWS_AHI
