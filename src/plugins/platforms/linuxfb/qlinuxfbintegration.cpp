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

#include "qlinuxfbintegration.h"
#include "../fb_base/fb_base.h"
#include "qgenericunixfontdatabase.h"
#include <QtGui/private/qpixmap_raster_p.h>
#include <private/qcore_unix_p.h> // overrides QT_OPEN
#include <qimage.h>
#include <qdebug.h>

#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/kd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <limits.h>
#include <signal.h>

#if !defined(Q_OS_DARWIN) && !defined(Q_OS_FREEBSD)
#include <linux/fb.h>

#ifdef __i386__
#include <asm/mtrr.h>
#endif
#endif

QT_BEGIN_NAMESPACE

class QLinuxFbIntegrationPrivate
{
public:
    QLinuxFbIntegrationPrivate();
    ~QLinuxFbIntegrationPrivate();

    void openTty();
    void closeTty();

    int fd;
    int startupw;
    int startuph;
    int startupd;
    bool blank;

    bool doGraphicsMode;
#ifdef QT_QWS_DEPTH_GENERIC
    bool doGenericColors;
#endif
    int ttyfd;
    long oldKdMode;
    QString ttyDevice;
    QString displaySpec;
};

QLinuxFbIntegrationPrivate::QLinuxFbIntegrationPrivate()
    : fd(-1), blank(true), doGraphicsMode(true),
#ifdef QT_QWS_DEPTH_GENERIC
      doGenericColors(false),
#endif
      ttyfd(-1), oldKdMode(KD_TEXT)
{
}

QLinuxFbIntegrationPrivate::~QLinuxFbIntegrationPrivate()
{
    closeTty();
}

void QLinuxFbIntegrationPrivate::openTty()
{
    const char *const devs[] = {"/dev/tty0", "/dev/tty", "/dev/console", 0};

    if (ttyDevice.isEmpty()) {
        for (const char * const *dev = devs; *dev; ++dev) {
            ttyfd = QT_OPEN(*dev, O_RDWR);
            if (ttyfd != -1)
                break;
        }
    } else {
        ttyfd = QT_OPEN(ttyDevice.toAscii().constData(), O_RDWR);
    }

    if (ttyfd == -1)
        return;

    if (doGraphicsMode) {
        ioctl(ttyfd, KDGETMODE, &oldKdMode);
        if (oldKdMode != KD_GRAPHICS) {
            int ret = ioctl(ttyfd, KDSETMODE, KD_GRAPHICS);
            if (ret == -1)
                doGraphicsMode = false;
        }
    }

    // No blankin' screen, no blinkin' cursor!, no cursor!
    const char termctl[] = "\033[9;0]\033[?33l\033[?25l\033[?1c";
    QT_WRITE(ttyfd, termctl, sizeof(termctl));
}

void QLinuxFbIntegrationPrivate::closeTty()
{
    if (ttyfd == -1)
        return;

    if (doGraphicsMode)
        ioctl(ttyfd, KDSETMODE, oldKdMode);

    // Blankin' screen, blinkin' cursor!
    const char termctl[] = "\033[9;15]\033[?33h\033[?25h\033[?0c";
    QT_WRITE(ttyfd, termctl, sizeof(termctl));

    QT_CLOSE(ttyfd);
    ttyfd = -1;
}

QLinuxFbIntegration::QLinuxFbIntegration()
    :fontDb(new QGenericUnixFontDatabase())
{
    d_ptr = new QLinuxFbIntegrationPrivate();

    // XXX
    QString displaySpec = QString::fromLatin1(qgetenv("QWS_DISPLAY"));

    if (!connect(displaySpec))
        qFatal("QLinuxFbIntegration: could not initialize screen");
    initDevice();

    // Create a QImage directly on the screen's framebuffer.
    // This is the blit target for copying windows to the screen.
    mPrimaryScreen = new QLinuxFbScreen(data, w, h, lstep,
                                                      screenFormat);
    mPrimaryScreen->setPhysicalSize(QSize(physWidth, physHeight));
    mScreens.append(mPrimaryScreen);
}

QLinuxFbIntegration::~QLinuxFbIntegration()
{
    delete mPrimaryScreen;
    delete d_ptr;
}

bool QLinuxFbIntegration::connect(const QString &displaySpec)
{
    const QStringList args = displaySpec.split(QLatin1Char(':'));

    if (args.contains(QLatin1String("nographicsmodeswitch")))
        d_ptr->doGraphicsMode = false;

#ifdef QT_QWS_DEPTH_GENERIC
    if (args.contains(QLatin1String("genericcolors")))
        d_ptr->doGenericColors = true;
#endif

    QRegExp ttyRegExp(QLatin1String("tty=(.*)"));
    if (args.indexOf(ttyRegExp) != -1)
        d_ptr->ttyDevice = ttyRegExp.cap(1);

#if 0
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
#ifndef QT_QWS_FRAMEBUFFER_LITTLE_ENDIAN
    if (args.contains(QLatin1String("littleendian")))
#endif
        QScreen::setFrameBufferLittleEndian(true);
#endif
#endif

    // Check for explicitly specified device
    const int len = 8; // "/dev/fbx"
    int m = displaySpec.indexOf(QLatin1String("/dev/fb"));

    QString dev;
    if (m > 0)
        dev = displaySpec.mid(m, len);
    else
        dev = QLatin1String("/dev/fb0");

    if (access(dev.toLatin1().constData(), R_OK|W_OK) == 0)
        d_ptr->fd = QT_OPEN(dev.toLatin1().constData(), O_RDWR);
    if (d_ptr->fd == -1) {
        if (access(dev.toLatin1().constData(), R_OK) == 0)
            d_ptr->fd = QT_OPEN(dev.toLatin1().constData(), O_RDONLY);
        if (d_ptr->fd == 1) {
            qWarning("Error opening framebuffer device %s", qPrintable(dev));
            return false;
        }
    }

    fb_fix_screeninfo finfo;
    fb_var_screeninfo vinfo;
    //#######################
    // Shut up Valgrind
    memset(&vinfo, 0, sizeof(vinfo));
    memset(&finfo, 0, sizeof(finfo));
    //#######################

    /* Get fixed screen information */
    if (d_ptr->fd != -1 && ioctl(d_ptr->fd, FBIOGET_FSCREENINFO, &finfo)) {
        perror("QLinuxFbIntegration::connect");
        qWarning("Error reading fixed information");
        return false;
    }

    if (finfo.type == FB_TYPE_VGA_PLANES) {
        qWarning("VGA16 video mode not supported");
        return false;
    }

    /* Get variable screen information */
    if (d_ptr->fd != -1 && ioctl(d_ptr->fd, FBIOGET_VSCREENINFO, &vinfo)) {
        perror("QLinuxFbIntegration::connect");
        qWarning("Error reading variable information");
        return false;
    }

    grayscale = vinfo.grayscale;
    d = vinfo.bits_per_pixel;
    if (d == 24) {
        d = vinfo.red.length + vinfo.green.length + vinfo.blue.length;
        if (d <= 0)
            d = 24; // reset if color component lengths are not reported
    } else if (d == 16) {
        d = vinfo.red.length + vinfo.green.length + vinfo.blue.length;
        if (d <= 0)
            d = 16;
    }
    lstep = finfo.line_length;

    int xoff = vinfo.xoffset;
    int yoff = vinfo.yoffset;
    const char* qwssize;
    if((qwssize=::getenv("QWS_SIZE")) && sscanf(qwssize,"%dx%d",&w,&h)==2) {
        if (d_ptr->fd != -1) {
            if ((uint)w > vinfo.xres) w = vinfo.xres;
            if ((uint)h > vinfo.yres) h = vinfo.yres;
        }
        dw=w;
        dh=h;
        int xxoff, yyoff;
        if (sscanf(qwssize, "%*dx%*d+%d+%d", &xxoff, &yyoff) == 2) {
            if (xxoff < 0 || xxoff + w > (int)(vinfo.xres))
                xxoff = vinfo.xres - w;
            if (yyoff < 0 || yyoff + h > (int)(vinfo.yres))
                yyoff = vinfo.yres - h;
            xoff += xxoff;
            yoff += yyoff;
        } else {
            xoff += (vinfo.xres - w)/2;
            yoff += (vinfo.yres - h)/2;
        }
    } else {
        dw=w=vinfo.xres;
        dh=h=vinfo.yres;
    }

    if (w == 0 || h == 0) {
        qWarning("QLinuxFbIntegration::connect(): Unable to find screen geometry, "
                 "will use 320x240.");
        dw = w = 320;
        dh = h = 240;
    }

    setPixelFormat(vinfo);

    // Handle display physical size spec.
    QStringList displayArgs = displaySpec.split(QLatin1Char(':'));
    QRegExp mmWidthRx(QLatin1String("mmWidth=?(\\d+)"));
    int dimIdxW = displayArgs.indexOf(mmWidthRx);
    QRegExp mmHeightRx(QLatin1String("mmHeight=?(\\d+)"));
    int dimIdxH = displayArgs.indexOf(mmHeightRx);
    if (dimIdxW >= 0) {
        mmWidthRx.exactMatch(displayArgs.at(dimIdxW));
        physWidth = mmWidthRx.cap(1).toInt();
        if (dimIdxH < 0)
            physHeight = dh*physWidth/dw;
    }
    if (dimIdxH >= 0) {
        mmHeightRx.exactMatch(displayArgs.at(dimIdxH));
        physHeight = mmHeightRx.cap(1).toInt();
        if (dimIdxW < 0)
            physWidth = dw*physHeight/dh;
    }
    if (dimIdxW < 0 && dimIdxH < 0) {
        if (vinfo.width != 0 && vinfo.height != 0
            && vinfo.width != UINT_MAX && vinfo.height != UINT_MAX) {
            physWidth = vinfo.width;
            physHeight = vinfo.height;
        } else {
            const int dpi = 72;
            physWidth = qRound(dw * 25.4 / dpi);
            physHeight = qRound(dh * 25.4 / dpi);
        }
    }

    dataoffset = yoff * lstep + xoff * d / 8;
    //qDebug("Using %dx%dx%d screen",w,h,d);

    /* Figure out the size of the screen in bytes */
    size = h * lstep;

    mapsize = finfo.smem_len;

    data = (unsigned char *)-1;
    if (d_ptr->fd != -1)
        data = (unsigned char *)mmap(0, mapsize, PROT_READ | PROT_WRITE,
                                     MAP_SHARED, d_ptr->fd, 0);

    if ((long)data == -1) {
        perror("QLinuxFbIntegration::connect");
        qWarning("Error: failed to map framebuffer device to memory.");
        return false;
    } else {
        data += dataoffset;
    }

#if 0
    canaccel = useOffscreen();
    if(canaccel)
        setupOffScreen();
#endif
    canaccel = false;

    // Now read in palette
    if((vinfo.bits_per_pixel==8) || (vinfo.bits_per_pixel==4)) {
        screencols= (vinfo.bits_per_pixel==8) ? 256 : 16;
        int loopc;
        fb_cmap startcmap;
        startcmap.start=0;
        startcmap.len=screencols;
        startcmap.red=(unsigned short int *)
                 malloc(sizeof(unsigned short int)*screencols);
        startcmap.green=(unsigned short int *)
                   malloc(sizeof(unsigned short int)*screencols);
        startcmap.blue=(unsigned short int *)
                  malloc(sizeof(unsigned short int)*screencols);
        startcmap.transp=(unsigned short int *)
                    malloc(sizeof(unsigned short int)*screencols);
        if (d_ptr->fd == -1 || ioctl(d_ptr->fd, FBIOGETCMAP, &startcmap)) {
            perror("QLinuxFbIntegration::connect");
            qWarning("Error reading palette from framebuffer, using default palette");
            createPalette(startcmap, vinfo, finfo);
        }
        int bits_used = 0;
        for(loopc=0;loopc<screencols;loopc++) {
            screenclut[loopc]=qRgb(startcmap.red[loopc] >> 8,
                                   startcmap.green[loopc] >> 8,
                                   startcmap.blue[loopc] >> 8);
            bits_used |= startcmap.red[loopc]
                         | startcmap.green[loopc]
                         | startcmap.blue[loopc];
        }
        // WORKAROUND: Some framebuffer drivers only return 8 bit
        // color values, so we need to not bit shift them..
        if ((bits_used & 0x00ff) && !(bits_used & 0xff00)) {
            for(loopc=0;loopc<screencols;loopc++) {
                screenclut[loopc] = qRgb(startcmap.red[loopc],
                                         startcmap.green[loopc],
                                         startcmap.blue[loopc]);
            }
            qWarning("8 bits cmap returned due to faulty FB driver, colors corrected");
        }
        free(startcmap.red);
        free(startcmap.green);
        free(startcmap.blue);
        free(startcmap.transp);
    } else {
        screencols=0;
    }

    return true;
}

bool QLinuxFbIntegration::initDevice()
{
    d_ptr->openTty();

    // Grab current mode so we can reset it
    fb_var_screeninfo vinfo;
    fb_fix_screeninfo finfo;
    //#######################
    // Shut up Valgrind
    memset(&vinfo, 0, sizeof(vinfo));
    memset(&finfo, 0, sizeof(finfo));
    //#######################

    if (ioctl(d_ptr->fd, FBIOGET_VSCREENINFO, &vinfo)) {
        perror("QLinuxFbScreen::initDevice");
        qFatal("Error reading variable information in card init");
        return false;
    }

#ifdef DEBUG_VINFO
    qDebug("Greyscale %d",vinfo.grayscale);
    qDebug("Nonstd %d",vinfo.nonstd);
    qDebug("Red %d %d %d",vinfo.red.offset,vinfo.red.length,
           vinfo.red.msb_right);
    qDebug("Green %d %d %d",vinfo.green.offset,vinfo.green.length,
           vinfo.green.msb_right);
    qDebug("Blue %d %d %d",vinfo.blue.offset,vinfo.blue.length,
           vinfo.blue.msb_right);
    qDebug("Transparent %d %d %d",vinfo.transp.offset,vinfo.transp.length,
           vinfo.transp.msb_right);
#endif

    d_ptr->startupw=vinfo.xres;
    d_ptr->startuph=vinfo.yres;
    d_ptr->startupd=vinfo.bits_per_pixel;
    grayscale = vinfo.grayscale;

    if (ioctl(d_ptr->fd, FBIOGET_FSCREENINFO, &finfo)) {
        perror("QLinuxFbScreen::initDevice");
        qCritical("Error reading fixed information in card init");
        // It's not an /error/ as such, though definitely a bad sign
        // so we return true
        return true;
    }

#ifdef __i386__
    // Now init mtrr
    if(!::getenv("QWS_NOMTRR")) {
        int mfd=QT_OPEN("/proc/mtrr",O_WRONLY,0);
        // MTRR entry goes away when file is closed - i.e.
        // hopefully when QWS is killed
        if(mfd != -1) {
            mtrr_sentry sentry;
            sentry.base=(unsigned long int)finfo.smem_start;
            //qDebug("Physical framebuffer address %p",(void*)finfo.smem_start);
            // Size needs to be in 4k chunks, but that's not always
            // what we get thanks to graphics card registers. Write combining
            // these is Not Good, so we write combine what we can
            // (which is not much - 4 megs on an 8 meg card, it seems)
            unsigned int size=finfo.smem_len;
            size=size >> 22;
            size=size << 22;
            sentry.size=size;
            sentry.type=MTRR_TYPE_WRCOMB;
            if(ioctl(mfd,MTRRIOC_ADD_ENTRY,&sentry)==-1) {
                //printf("Couldn't add mtrr entry for %lx %lx, %s\n",
                //sentry.base,sentry.size,strerror(errno));
            }
        }

        // Should we close mfd here?
        //QT_CLOSE(mfd);
    }
#endif
    if ((vinfo.bits_per_pixel==8) || (vinfo.bits_per_pixel==4) || (finfo.visual==FB_VISUAL_DIRECTCOLOR))
    {
        fb_cmap cmap;
        createPalette(cmap, vinfo, finfo);
        if (ioctl(d_ptr->fd, FBIOPUTCMAP, &cmap)) {
            perror("QLinuxFbScreen::initDevice");
            qWarning("Error writing palette to framebuffer");
        }
        free(cmap.red);
        free(cmap.green);
        free(cmap.blue);
        free(cmap.transp);
    }

#if 0
    if (canaccel) {
        *entryp=0;
        *lowest = mapsize;
        insert_entry(*entryp, *lowest, *lowest);  // dummy entry to mark start
    }

    shared->fifocount = 0;
    shared->buffer_offset = 0xffffffff;  // 0 would be a sensible offset (screen)
    shared->linestep = 0;
    shared->cliptop = 0xffffffff;
    shared->clipleft = 0xffffffff;
    shared->clipright = 0xffffffff;
    shared->clipbottom = 0xffffffff;
    shared->rop = 0xffffffff;
#endif

#ifdef QT_QWS_DEPTH_GENERIC
    if (pixelFormat() == QImage::Format_Invalid && screencols == 0
        && d_ptr->doGenericColors)
    {
        qt_set_generic_blit(this, vinfo.bits_per_pixel,
                            vinfo.red.length, vinfo.green.length,
                            vinfo.blue.length, vinfo.transp.length,
                            vinfo.red.offset, vinfo.green.offset,
                            vinfo.blue.offset, vinfo.transp.offset);
    }
#endif

#if 0
#ifndef QT_NO_QWS_CURSOR
    QScreenCursor::initSoftwareCursor();
#endif
#endif
    blank(false);

    return true;
}

void QLinuxFbIntegration::setPixelFormat(struct fb_var_screeninfo info)
{
    const fb_bitfield rgba[4] = { info.red, info.green,
                                  info.blue, info.transp };

    QImage::Format format = QImage::Format_Invalid;

    switch (d) {
    case 32: {
        const fb_bitfield argb8888[4] = {{16, 8, 0}, {8, 8, 0},
                                         {0, 8, 0}, {24, 8, 0}};
        const fb_bitfield abgr8888[4] = {{0, 8, 0}, {8, 8, 0},
                                         {16, 8, 0}, {24, 8, 0}};
        if (memcmp(rgba, argb8888, 4 * sizeof(fb_bitfield)) == 0) {
            format = QImage::Format_ARGB32;
        } else if (memcmp(rgba, argb8888, 3 * sizeof(fb_bitfield)) == 0) {
            format = QImage::Format_RGB32;
        } else if (memcmp(rgba, abgr8888, 3 * sizeof(fb_bitfield)) == 0) {
            format = QImage::Format_RGB32;
            pixeltype = BGRPixel;
        }
        break;
    }
    case 24: {
        const fb_bitfield rgb888[4] = {{16, 8, 0}, {8, 8, 0},
                                       {0, 8, 0}, {0, 0, 0}};
        const fb_bitfield bgr888[4] = {{0, 8, 0}, {8, 8, 0},
                                       {16, 8, 0}, {0, 0, 0}};
        if (memcmp(rgba, rgb888, 3 * sizeof(fb_bitfield)) == 0) {
            format = QImage::Format_RGB888;
        } else if (memcmp(rgba, bgr888, 3 * sizeof(fb_bitfield)) == 0) {
            format = QImage::Format_RGB888;
            pixeltype = BGRPixel;
        }
        break;
    }
    case 18: {
        const fb_bitfield rgb666[4] = {{12, 6, 0}, {6, 6, 0},
                                       {0, 6, 0}, {0, 0, 0}};
        if (memcmp(rgba, rgb666, 3 * sizeof(fb_bitfield)) == 0)
            format = QImage::Format_RGB666;
        break;
    }
    case 16: {
        const fb_bitfield rgb565[4] = {{11, 5, 0}, {5, 6, 0},
                                       {0, 5, 0}, {0, 0, 0}};
        const fb_bitfield bgr565[4] = {{0, 5, 0}, {5, 6, 0},
                                       {11, 5, 0}, {0, 0, 0}};
        if (memcmp(rgba, rgb565, 3 * sizeof(fb_bitfield)) == 0) {
            format = QImage::Format_RGB16;
        } else if (memcmp(rgba, bgr565, 3 * sizeof(fb_bitfield)) == 0) {
            format = QImage::Format_RGB16;
            pixeltype = BGRPixel;
        }
        break;
    }
    case 15: {
        const fb_bitfield rgb1555[4] = {{10, 5, 0}, {5, 5, 0},
                                        {0, 5, 0}, {15, 1, 0}};
        const fb_bitfield bgr1555[4] = {{0, 5, 0}, {5, 5, 0},
                                        {10, 5, 0}, {15, 1, 0}};
        if (memcmp(rgba, rgb1555, 3 * sizeof(fb_bitfield)) == 0) {
            format = QImage::Format_RGB555;
        } else if (memcmp(rgba, bgr1555, 3 * sizeof(fb_bitfield)) == 0) {
            format = QImage::Format_RGB555;
            pixeltype = BGRPixel;
        }
        break;
    }
    case 12: {
        const fb_bitfield rgb444[4] = {{8, 4, 0}, {4, 4, 0},
                                       {0, 4, 0}, {0, 0, 0}};
        if (memcmp(rgba, rgb444, 3 * sizeof(fb_bitfield)) == 0)
            format = QImage::Format_RGB444;
        break;
    }
    case 8:
        break;
    case 1:
        format = QImage::Format_Mono; //###: LSB???
        break;
    default:
        break;
    }

    screenFormat = format;
}

void QLinuxFbIntegration::createPalette(fb_cmap &cmap, fb_var_screeninfo &vinfo, fb_fix_screeninfo &finfo)
{
    if((vinfo.bits_per_pixel==8) || (vinfo.bits_per_pixel==4)) {
        screencols= (vinfo.bits_per_pixel==8) ? 256 : 16;
        cmap.start=0;
        cmap.len=screencols;
        cmap.red=(unsigned short int *)
                 malloc(sizeof(unsigned short int)*screencols);
        cmap.green=(unsigned short int *)
                   malloc(sizeof(unsigned short int)*screencols);
        cmap.blue=(unsigned short int *)
                  malloc(sizeof(unsigned short int)*screencols);
        cmap.transp=(unsigned short int *)
                    malloc(sizeof(unsigned short int)*screencols);

        if (screencols==16) {
            if (finfo.type == FB_TYPE_PACKED_PIXELS) {
                // We'll setup a grayscale cmap for 4bpp linear
                int val = 0;
                for (int idx = 0; idx < 16; ++idx, val += 17) {
                    cmap.red[idx] = (val<<8)|val;
                    cmap.green[idx] = (val<<8)|val;
                    cmap.blue[idx] = (val<<8)|val;
                    screenclut[idx]=qRgb(val, val, val);
                }
            } else {
                // Default 16 colour palette
                // Green is now trolltech green so certain images look nicer
                //                             black  d_gray l_gray white  red  green  blue cyan magenta yellow
                unsigned char reds[16]   = { 0x00, 0x7F, 0xBF, 0xFF, 0xFF, 0xA2, 0x00, 0xFF, 0xFF, 0x00, 0x7F, 0x7F, 0x00, 0x00, 0x00, 0x82 };
                unsigned char greens[16] = { 0x00, 0x7F, 0xBF, 0xFF, 0x00, 0xC5, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x7F, 0x7F, 0x7F };
                unsigned char blues[16]  = { 0x00, 0x7F, 0xBF, 0xFF, 0x00, 0x11, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x7F, 0x7F, 0x7F, 0x00, 0x00 };

                for (int idx = 0; idx < 16; ++idx) {
                    cmap.red[idx] = ((reds[idx]) << 8)|reds[idx];
                    cmap.green[idx] = ((greens[idx]) << 8)|greens[idx];
                    cmap.blue[idx] = ((blues[idx]) << 8)|blues[idx];
                    cmap.transp[idx] = 0;
                    screenclut[idx]=qRgb(reds[idx], greens[idx], blues[idx]);
                }
            }
        } else {
            if (grayscale) {
                // Build grayscale palette
                int i;
                for(i=0;i<screencols;++i) {
                    int bval = screencols == 256 ? i : (i << 4);
                    ushort val = (bval << 8) | bval;
                    cmap.red[i] = val;
                    cmap.green[i] = val;
                    cmap.blue[i] = val;
                    cmap.transp[i] = 0;
                    screenclut[i] = qRgb(bval,bval,bval);
                }
            } else {
                // 6x6x6 216 color cube
                int idx = 0;
                for(int ir = 0x0; ir <= 0xff; ir+=0x33) {
                    for(int ig = 0x0; ig <= 0xff; ig+=0x33) {
                        for(int ib = 0x0; ib <= 0xff; ib+=0x33) {
                            cmap.red[idx] = (ir << 8)|ir;
                            cmap.green[idx] = (ig << 8)|ig;
                            cmap.blue[idx] = (ib << 8)|ib;
                            cmap.transp[idx] = 0;
                            screenclut[idx]=qRgb(ir, ig, ib);
                            ++idx;
                        }
                    }
                }
                // Fill in rest with 0
                for (int loopc=0; loopc<40; ++loopc) {
                    screenclut[idx]=0;
                    ++idx;
                }
                screencols=idx;
            }
        }
    } else if(finfo.visual==FB_VISUAL_DIRECTCOLOR) {
        cmap.start=0;
        int rbits=0,gbits=0,bbits=0;
        switch (vinfo.bits_per_pixel) {
        case 8:
            rbits=vinfo.red.length;
            gbits=vinfo.green.length;
            bbits=vinfo.blue.length;
            if(rbits==0 && gbits==0 && bbits==0) {
                // cyber2000 driver bug hack
                rbits=3;
                gbits=3;
                bbits=2;
            }
            break;
        case 15:
            rbits=5;
            gbits=5;
            bbits=5;
            break;
        case 16:
            rbits=5;
            gbits=6;
            bbits=5;
            break;
        case 18:
        case 19:
            rbits=6;
            gbits=6;
            bbits=6;
            break;
        case 24: case 32:
            rbits=gbits=bbits=8;
            break;
        }
        screencols=cmap.len=1<<qMax(rbits,qMax(gbits,bbits));
        cmap.red=(unsigned short int *)
                 malloc(sizeof(unsigned short int)*256);
        cmap.green=(unsigned short int *)
                   malloc(sizeof(unsigned short int)*256);
        cmap.blue=(unsigned short int *)
                  malloc(sizeof(unsigned short int)*256);
        cmap.transp=(unsigned short int *)
                    malloc(sizeof(unsigned short int)*256);
        for(unsigned int i = 0x0; i < cmap.len; i++) {
            cmap.red[i] = i*65535/((1<<rbits)-1);
            cmap.green[i] = i*65535/((1<<gbits)-1);
            cmap.blue[i] = i*65535/((1<<bbits)-1);
            cmap.transp[i] = 0;
        }
    }
}

void QLinuxFbIntegration::blank(bool on)
{
    if (d_ptr->blank == on)
        return;

#if defined(QT_QWS_IPAQ)
    if (on)
        system("apm -suspend");
#else
    if (d_ptr->fd == -1)
        return;
// Some old kernel versions don't have this.  These defines should go
// away eventually
#if defined(FBIOBLANK)
#if defined(VESA_POWERDOWN) && defined(VESA_NO_BLANKING)
    ioctl(d_ptr->fd, FBIOBLANK, on ? VESA_POWERDOWN : VESA_NO_BLANKING);
#else
    ioctl(d_ptr->fd, FBIOBLANK, on ? 1 : 0);
#endif
#endif
#endif

    d_ptr->blank = on;
}

bool QLinuxFbIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps: return true;
    default: return QPlatformIntegration::hasCapability(cap);
    }
}


QPixmapData *QLinuxFbIntegration::createPixmapData(QPixmapData::PixelType type) const
{
    return new QRasterPixmapData(type);
}

QWindowSurface *QLinuxFbIntegration::createWindowSurface(QWidget *widget, WId) const
{
    QFbWindowSurface * surface =
        new QFbWindowSurface(mPrimaryScreen, widget);
    return surface;
}

QPlatformWindow *QLinuxFbIntegration::createPlatformWindow(QWidget *widget, WId /*winId*/) const
{
    QFbWindow *w = new QFbWindow(widget);
    mPrimaryScreen->addWindow(w);
    return w;
}

QPlatformFontDatabase *QLinuxFbIntegration::fontDatabase() const
{
    return fontDb;
}

QLinuxFbScreen::QLinuxFbScreen(uchar * d, int w,
    int h, int lstep, QImage::Format screenFormat) : compositePainter(0)
{
    data = d;
    mGeometry = QRect(0,0,w,h);
    bytesPerLine = lstep;
    mFormat = screenFormat;
    mDepth = 16;
    mScreenImage = new QImage(mGeometry.width(), mGeometry.height(),
                              mFormat);
    mFbScreenImage = new QImage(data, mGeometry.width(), mGeometry.height(),
                              bytesPerLine, mFormat);
    cursor = new QPlatformSoftwareCursor(this);
}

void QLinuxFbScreen::setGeometry(QRect rect)
{
    mGeometry = rect;
    delete mFbScreenImage;
    mFbScreenImage = new QImage(data, mGeometry.width(), mGeometry.height(),
                           bytesPerLine, mFormat);
    delete compositePainter;
    compositePainter = 0;

    delete mScreenImage;
    mScreenImage = new QImage(mGeometry.width(), mGeometry.height(),
                              mFormat);
}

void QLinuxFbScreen::setFormat(QImage::Format format)
{
    mFormat = format;
    delete mFbScreenImage;
    mFbScreenImage = new QImage(data, mGeometry.width(), mGeometry.height(),
                             bytesPerLine, mFormat);
    delete compositePainter;
    compositePainter = 0;

    delete mScreenImage;
    mScreenImage = new QImage(mGeometry.width(), mGeometry.height(),
                              mFormat);
}

QRegion QLinuxFbScreen::doRedraw()
{
    QRegion touched;
    touched = QFbScreen::doRedraw();

    if (!compositePainter) {
        compositePainter = new QPainter(mFbScreenImage);
    }

    QVector<QRect> rects = touched.rects();
    for (int i = 0; i < rects.size(); i++)
        compositePainter->drawImage(rects[i], *mScreenImage, rects[i]);
    return touched;
}
QT_END_NAMESPACE
