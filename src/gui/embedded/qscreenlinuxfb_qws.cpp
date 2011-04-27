/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qscreenlinuxfb_qws.h"

#ifndef QT_NO_QWS_LINUXFB
//#include "qmemorymanager_qws.h"
#include "qwsdisplay_qws.h"
#include "qpixmap.h"
#include <private/qwssignalhandler_p.h>
#include <private/qcore_unix_p.h> // overrides QT_OPEN

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

#include "qwindowsystem_qws.h"

#if !defined(Q_OS_DARWIN) && !defined(Q_OS_FREEBSD)
#include <linux/fb.h>

#ifdef __i386__
#include <asm/mtrr.h>
#endif
#endif

QT_BEGIN_NAMESPACE

extern int qws_client_id;

//#define DEBUG_CACHE

class QLinuxFbScreenPrivate : public QObject
{
public:
    QLinuxFbScreenPrivate();
    ~QLinuxFbScreenPrivate();

    void openTty();
    void closeTty();

    int fd;
    int startupw;
    int startuph;
    int startupd;
    bool blank;
    QLinuxFbScreen::DriverTypes driverType;

    bool doGraphicsMode;
#ifdef QT_QWS_DEPTH_GENERIC
    bool doGenericColors;
#endif
    int ttyfd;
    long oldKdMode;
    QString ttyDevice;
    QString displaySpec;
};

QLinuxFbScreenPrivate::QLinuxFbScreenPrivate()
    : fd(-1), blank(true), doGraphicsMode(true),
#ifdef QT_QWS_DEPTH_GENERIC
      doGenericColors(false),
#endif
      ttyfd(-1), oldKdMode(KD_TEXT)
{
    QWSSignalHandler::instance()->addObject(this);
}

QLinuxFbScreenPrivate::~QLinuxFbScreenPrivate()
{
    closeTty();
}

void QLinuxFbScreenPrivate::openTty()
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

void QLinuxFbScreenPrivate::closeTty()
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

/*!
    \enum QLinuxFbScreen::DriverTypes

    This enum describes the driver type.

    \value GenericDriver Generic Linux framebuffer driver
    \value EInk8Track e-Ink framebuffer driver using the 8Track chipset
 */

/*!
    \fn QLinuxFbScreen::fixupScreenInfo(fb_fix_screeninfo &finfo, fb_var_screeninfo &vinfo)

    Adjust the values returned by the framebuffer driver, to work
    around driver bugs or nonstandard behavior in certain drivers.
    \a finfo and \a vinfo specify the fixed and variable screen info
    returned by the driver.
 */
void QLinuxFbScreen::fixupScreenInfo(fb_fix_screeninfo &finfo, fb_var_screeninfo &vinfo)
{
    // 8Track e-ink devices (as found in Sony PRS-505) lie
    // about their bit depth -- they claim they're 1 bit per
    // pixel while the only supported mode is 8 bit per pixel
    // grayscale.
    // Caused by this, they also miscalculate their line length.
    if(!strcmp(finfo.id, "8TRACKFB") && vinfo.bits_per_pixel == 1) {
        vinfo.bits_per_pixel = 8;
        finfo.line_length = vinfo.xres;
    }
}

/*!
    \internal

    \class QLinuxFbScreen
    \ingroup qws

    \brief The QLinuxFbScreen class implements a screen driver for the
    Linux framebuffer.

    Note that this class is only available in \l{Qt for Embedded Linux}.
    Custom screen drivers can be added by subclassing the
    QScreenDriverPlugin class, using the QScreenDriverFactory class to
    dynamically load the driver into the application, but there should
    only be one screen object per application.

    The QLinuxFbScreen class provides the cache() function allocating
    off-screen graphics memory, and the complementary uncache()
    function releasing the allocated memory. The latter function will
    first sync the graphics card to ensure the memory isn't still
    being used by a command in the graphics card FIFO queue. The
    deleteEntry() function deletes the given memory block without such
    synchronization.  Given the screen instance and client id, the
    memory can also be released using the clearCache() function, but
    this should only be necessary if a client exits abnormally.

    In addition, when in paletted graphics modes, the set() function
    provides the possibility of setting a specified color index to a
    given RGB value.

    The QLinuxFbScreen class also acts as a factory for the
    unaccelerated screen cursor and the unaccelerated raster-based
    implementation of QPaintEngine (\c QRasterPaintEngine);
    accelerated drivers for Linux should derive from this class.

    \sa QScreen, QScreenDriverPlugin, {Running Applications}
*/

/*!
    \fn bool QLinuxFbScreen::useOffscreen()
    \internal
*/

// Unaccelerated screen/driver setup. Can be overridden by accelerated
// drivers

/*!
    \fn QLinuxFbScreen::QLinuxFbScreen(int displayId)

    Constructs a QLinuxFbScreen object. The \a displayId argument
    identifies the Qt for Embedded Linux server to connect to.
*/

QLinuxFbScreen::QLinuxFbScreen(int display_id)
    : QScreen(display_id, LinuxFBClass), d_ptr(new QLinuxFbScreenPrivate)
{
    canaccel=false;
    clearCacheFunc = &clearCache;
#ifdef QT_QWS_CLIENTBLIT
    setSupportsBlitInClients(true);
#endif
}

/*!
    Destroys this QLinuxFbScreen object.
*/

QLinuxFbScreen::~QLinuxFbScreen()
{
}

/*!
    \reimp

    This is called by \l{Qt for Embedded Linux} clients to map in the framebuffer.
    It should be reimplemented by accelerated drivers to map in
    graphics card registers; those drivers should then call this
    function in order to set up offscreen memory management. The
    device is specified in \a displaySpec; e.g. "/dev/fb".

    \sa disconnect()
*/

bool QLinuxFbScreen::connect(const QString &displaySpec)
{
    d_ptr->displaySpec = displaySpec;

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

#if Q_BYTE_ORDER == Q_BIG_ENDIAN
#ifndef QT_QWS_FRAMEBUFFER_LITTLE_ENDIAN
    if (args.contains(QLatin1String("littleendian")))
#endif
        QScreen::setFrameBufferLittleEndian(true);
#endif

    QString dev = QLatin1String("/dev/fb0");
    foreach(QString d, args) {
	if (d.startsWith(QLatin1Char('/'))) {
	    dev = d;
	    break;
	}
    }

    if (access(dev.toLatin1().constData(), R_OK|W_OK) == 0)
        d_ptr->fd = QT_OPEN(dev.toLatin1().constData(), O_RDWR);
    if (d_ptr->fd == -1) {
        if (QApplication::type() == QApplication::GuiServer) {
            perror("QScreenLinuxFb::connect");
            qCritical("Error opening framebuffer device %s", qPrintable(dev));
            return false;
        }
        if (access(dev.toLatin1().constData(), R_OK) == 0)
            d_ptr->fd = QT_OPEN(dev.toLatin1().constData(), O_RDONLY);
    }

    ::fb_fix_screeninfo finfo;
    ::fb_var_screeninfo vinfo;
    //#######################
    // Shut up Valgrind
    memset(&vinfo, 0, sizeof(vinfo));
    memset(&finfo, 0, sizeof(finfo));
    //#######################

    /* Get fixed screen information */
    if (d_ptr->fd != -1 && ioctl(d_ptr->fd, FBIOGET_FSCREENINFO, &finfo)) {
        perror("QLinuxFbScreen::connect");
        qWarning("Error reading fixed information");
        return false;
    }

    d_ptr->driverType = strcmp(finfo.id, "8TRACKFB") ? GenericDriver : EInk8Track;

    if (finfo.type == FB_TYPE_VGA_PLANES) {
        qWarning("VGA16 video mode not supported");
        return false;
    }

    /* Get variable screen information */
    if (d_ptr->fd != -1 && ioctl(d_ptr->fd, FBIOGET_VSCREENINFO, &vinfo)) {
        perror("QLinuxFbScreen::connect");
        qWarning("Error reading variable information");
        return false;
    }

    fixupScreenInfo(finfo, vinfo);

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
            if (xxoff < 0 || xxoff + w > vinfo.xres)
                xxoff = vinfo.xres - w;
            if (yyoff < 0 || yyoff + h > vinfo.yres)
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
        qWarning("QScreenLinuxFb::connect(): Unable to find screen geometry, "
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
        if (QApplication::type() == QApplication::GuiServer) {
            perror("QLinuxFbScreen::connect");
            qWarning("Error: failed to map framebuffer device to memory.");
            return false;
        }
        data = 0;
    } else {
        data += dataoffset;
    }

    canaccel = useOffscreen();
    if(canaccel)
        setupOffScreen();

    // Now read in palette
    if((vinfo.bits_per_pixel==8) || (vinfo.bits_per_pixel==4)) {
        screencols= (vinfo.bits_per_pixel==8) ? 256 : 16;
        int loopc;
        ::fb_cmap startcmap;
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
            perror("QLinuxFbScreen::connect");
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

/*!
    \reimp

    This unmaps the framebuffer.

    \sa connect()
*/

void QLinuxFbScreen::disconnect()
{
    data -= dataoffset;
    if (data)
        munmap((char*)data,mapsize);
    close(d_ptr->fd);
}

// #define DEBUG_VINFO

void QLinuxFbScreen::createPalette(fb_cmap &cmap, fb_var_screeninfo &vinfo, fb_fix_screeninfo &finfo)
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

/*!
    \reimp

    This is called by the \l{Qt for Embedded Linux} server at startup time.
    It turns off console blinking, sets up the color palette, enables write
    combining on the framebuffer and initialises the off-screen memory
    manager.
*/

bool QLinuxFbScreen::initDevice()
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

    if (ioctl(d_ptr->fd, FBIOGET_FSCREENINFO, &finfo)) {
        perror("QLinuxFbScreen::initDevice");
        qCritical("Error reading fixed information in card init");
        // It's not an /error/ as such, though definitely a bad sign
        // so we return true
        return true;
    }

    fixupScreenInfo(finfo, vinfo);

    d_ptr->startupw=vinfo.xres;
    d_ptr->startuph=vinfo.yres;
    d_ptr->startupd=vinfo.bits_per_pixel;
    grayscale = vinfo.grayscale;

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

#ifndef QT_NO_QWS_CURSOR
    QScreenCursor::initSoftwareCursor();
#endif
    blank(false);

    return true;
}

/*
  The offscreen memory manager's list of entries is stored at the bottom
  of the offscreen memory area and consistes of a series of QPoolEntry's,
  each of which keep track of a block of allocated memory. Unallocated memory
  is implicitly indicated by the gap between blocks indicated by QPoolEntry's.
  The memory manager looks through any unallocated memory before the end
  of currently-allocated memory to see if a new block will fit in the gap;
  if it doesn't it allocated it from the end of currently-allocated memory.
  Memory is allocated from the top of the framebuffer downwards; if it hits
  the list of entries then offscreen memory is full and further allocations
  are made from main RAM (and hence unaccelerated). Allocated memory can
  be seen as a sort of upside-down stack; lowest keeps track of the
  bottom of the stack.
*/

void QLinuxFbScreen::delete_entry(int pos)
{
    if (pos > *entryp || pos < 0) {
        qWarning("Attempt to delete odd pos! %d %d", pos, *entryp);
        return;
    }

#ifdef DEBUG_CACHE
    qDebug("Remove entry: %d", pos);
#endif

    QPoolEntry *qpe = &entries[pos];
    if (qpe->start <= *lowest) {
        // Lowest goes up again
        *lowest = entries[pos-1].start;
#ifdef DEBUG_CACHE
        qDebug("   moved lowest to %d", *lowest);
#endif
    }

    (*entryp)--;
    if (pos == *entryp)
        return;

    int size = (*entryp)-pos;
    memmove(&entries[pos], &entries[pos+1], size*sizeof(QPoolEntry));
}

void QLinuxFbScreen::insert_entry(int pos, int start, int end)
{
    if (pos > *entryp) {
        qWarning("Attempt to insert odd pos! %d %d",pos,*entryp);
        return;
    }

#ifdef DEBUG_CACHE
    qDebug("Insert entry: %d, %d -> %d", pos, start, end);
#endif

    if (start < (int)*lowest) {
        *lowest = start;
#ifdef DEBUG_CACHE
        qDebug("    moved lowest to %d", *lowest);
#endif
    }

    if (pos == *entryp) {
        entries[pos].start = start;
        entries[pos].end = end;
        entries[pos].clientId = qws_client_id;
        (*entryp)++;
        return;
    }

    int size=(*entryp)-pos;
    memmove(&entries[pos+1],&entries[pos],size*sizeof(QPoolEntry));
    entries[pos].start=start;
    entries[pos].end=end;
    entries[pos].clientId=qws_client_id;
    (*entryp)++;
}

/*!
    \fn uchar * QLinuxFbScreen::cache(int amount)

    Requests the specified \a amount of offscreen graphics card memory
    from the memory manager, and returns a pointer to the data within
    the framebuffer (or 0 if there is no free memory).

    Note that the display is locked while memory is allocated in order to
    preserve the memory pool's integrity.

    Use the QScreen::onCard() function to retrieve an offset (in
    bytes) from the start of graphics card memory for the returned
    pointer.

    \sa uncache(), clearCache(), deleteEntry()
*/

uchar * QLinuxFbScreen::cache(int amount)
{
    if (!canaccel || entryp == 0)
        return 0;

    qt_fbdpy->grab();

    int startp = cacheStart + (*entryp+1) * sizeof(QPoolEntry);
    if (startp >= (int)*lowest) {
        // We don't have room for another cache QPoolEntry.
#ifdef DEBUG_CACHE
        qDebug("No room for pool entry in VRAM");
#endif
        qt_fbdpy->ungrab();
        return 0;
    }

    int align = pixmapOffsetAlignment();

    if (*entryp > 1) {
        // Try to find a gap in the allocated blocks.
        for (int loopc = 0; loopc < *entryp-1; loopc++) {
            int freestart = entries[loopc+1].end;
            int freeend = entries[loopc].start;
            if (freestart != freeend) {
                while (freestart % align) {
                    freestart++;
                }
                int len=freeend-freestart;
                if (len >= amount) {
                    insert_entry(loopc+1, freestart, freestart+amount);
                    qt_fbdpy->ungrab();
                    return data+freestart;
                }
            }
        }
    }

    // No free blocks in already-taken memory; get some more
    // if we can
    int newlowest = (*lowest)-amount;
    if (newlowest % align) {
        newlowest -= align;
        while (newlowest % align) {
            newlowest++;
        }
    }
    if (startp >= newlowest) {
        qt_fbdpy->ungrab();
#ifdef DEBUG_CACHE
        qDebug("No VRAM available for %d bytes", amount);
#endif
        return 0;
    }
    insert_entry(*entryp, newlowest, *lowest);
    qt_fbdpy->ungrab();

    return data + newlowest;
}

/*!
    \fn void QLinuxFbScreen::uncache(uchar * memoryBlock)

    Deletes the specified \a memoryBlock allocated from the graphics
    card memory.

    Note that the display is locked while memory is unallocated in
    order to preserve the memory pool's integrity.

    This function will first sync the graphics card to ensure the
    memory isn't still being used by a command in the graphics card
    FIFO queue. It is possible to speed up a driver by overriding this
    function to avoid syncing. For example, the driver might delay
    deleting the memory until it detects that all commands dealing
    with the memory are no longer in the queue. Note that it will then
    be up to the driver to ensure that the specified \a memoryBlock no
    longer is being used.

    \sa cache(), deleteEntry(), clearCache()
 */
void QLinuxFbScreen::uncache(uchar * c)
{
    // need to sync graphics card

    deleteEntry(c);
}

/*!
    \fn void QLinuxFbScreen::deleteEntry(uchar * memoryBlock)

    Deletes the specified \a memoryBlock allocated from the graphics
    card memory.

    \sa uncache(), cache(), clearCache()
*/
void QLinuxFbScreen::deleteEntry(uchar * c)
{
    qt_fbdpy->grab();
    unsigned long pos=(unsigned long)c;
    pos-=((unsigned long)data);
    unsigned int hold=(*entryp);
    for(unsigned int loopc=1;loopc<hold;loopc++) {
        if (entries[loopc].start==pos) {
            if (entries[loopc].clientId == qws_client_id)
                delete_entry(loopc);
            else
                qWarning("Attempt to delete client id %d cache entry",
                         entries[loopc].clientId);
            qt_fbdpy->ungrab();
            return;
        }
    }
    qt_fbdpy->ungrab();
    qWarning("Attempt to delete unknown offset %ld",pos);
}

/*!
    Removes all entries from the cache for the specified screen \a
    instance and client identified by the given \a clientId.

    Calling this function should only be necessary if a client exits
    abnormally.

    \sa cache(), uncache(), deleteEntry()
*/
void QLinuxFbScreen::clearCache(QScreen *instance, int clientId)
{
    QLinuxFbScreen *screen = (QLinuxFbScreen *)instance;
    if (!screen->canaccel || !screen->entryp)
        return;
    qt_fbdpy->grab();
    for (int loopc = 0; loopc < *(screen->entryp); loopc++) {
        if (screen->entries[loopc].clientId == clientId) {
            screen->delete_entry(loopc);
            loopc--;
        }
    }
    qt_fbdpy->ungrab();
}


void QLinuxFbScreen::setupOffScreen()
{
    // Figure out position of offscreen memory
    // Set up pool entries pointer table and 64-bit align it
    int psize = size;

    // hw: this causes the limitation of cursors to 64x64
    // the cursor should rather use the normal pixmap mechanism
    psize += 4096;  // cursor data
    psize += 8;     // for alignment
    psize &= ~0x7;  // align

    unsigned long pos = (unsigned long)data;
    pos += psize;
    entryp = ((int *)pos);
    lowest = ((unsigned int *)pos)+1;
    pos += (sizeof(int))*4;
    entries = (QPoolEntry *)pos;

    // beginning of offscreen memory available for pixmaps.
    cacheStart = psize + 4*sizeof(int) + sizeof(QPoolEntry);
}

/*!
    \reimp

    This is called by the \l{Qt for Embedded Linux} server when it shuts
    down, and should be inherited if you need to do any card-specific cleanup.
    The default version hides the screen cursor and reenables the blinking
    cursor and screen blanking.
*/

void QLinuxFbScreen::shutdownDevice()
{
    // Causing crashes. Not needed.
    //setMode(startupw,startuph,startupd);
/*
    if (startupd == 8) {
        ioctl(fd,FBIOPUTCMAP,startcmap);
        free(startcmap->red);
        free(startcmap->green);
        free(startcmap->blue);
        free(startcmap->transp);
        delete startcmap;
        startcmap = 0;
    }
*/
    d_ptr->closeTty();
}

/*!
    \fn void QLinuxFbScreen::set(unsigned int index,unsigned int red,unsigned int green,unsigned int blue)

    Sets the specified color \a index to the specified RGB value, (\a
    red, \a green, \a blue), when in paletted graphics modes.
*/

void QLinuxFbScreen::set(unsigned int i,unsigned int r,unsigned int g,unsigned int b)
{
    if (d_ptr->fd != -1) {
        fb_cmap cmap;
        cmap.start=i;
        cmap.len=1;
        cmap.red=(unsigned short int *)
                 malloc(sizeof(unsigned short int)*256);
        cmap.green=(unsigned short int *)
                   malloc(sizeof(unsigned short int)*256);
        cmap.blue=(unsigned short int *)
                  malloc(sizeof(unsigned short int)*256);
        cmap.transp=(unsigned short int *)
                    malloc(sizeof(unsigned short int)*256);
        cmap.red[0]=r << 8;
        cmap.green[0]=g << 8;
        cmap.blue[0]=b << 8;
        cmap.transp[0]=0;
        ioctl(d_ptr->fd, FBIOPUTCMAP, &cmap);
        free(cmap.red);
        free(cmap.green);
        free(cmap.blue);
        free(cmap.transp);
    }
    screenclut[i] = qRgb(r, g, b);
}

/*!
    \reimp

    Sets the framebuffer to a new resolution and bit depth. The width is
    in \a nw, the height is in \a nh, and the depth is in \a nd. After
    doing this any currently-existing paint engines will be invalid and the
    screen should be completely redrawn. In a multiple-process
    Embedded Qt situation you must signal all other applications to
    call setMode() to the same mode and redraw.
*/

void QLinuxFbScreen::setMode(int nw,int nh,int nd)
{
    if (d_ptr->fd == -1)
        return;

    fb_fix_screeninfo finfo;
    fb_var_screeninfo vinfo;
    //#######################
    // Shut up Valgrind
    memset(&vinfo, 0, sizeof(vinfo));
    memset(&finfo, 0, sizeof(finfo));
    //#######################

    if (ioctl(d_ptr->fd, FBIOGET_VSCREENINFO, &vinfo)) {
        perror("QLinuxFbScreen::setMode");
        qFatal("Error reading variable information in mode change");
    }

    vinfo.xres=nw;
    vinfo.yres=nh;
    vinfo.bits_per_pixel=nd;

    if (ioctl(d_ptr->fd, FBIOPUT_VSCREENINFO, &vinfo)) {
        perror("QLinuxFbScreen::setMode");
        qCritical("Error writing variable information in mode change");
    }

    if (ioctl(d_ptr->fd, FBIOGET_VSCREENINFO, &vinfo)) {
        perror("QLinuxFbScreen::setMode");
        qFatal("Error reading changed variable information in mode change");
    }

    if (ioctl(d_ptr->fd, FBIOGET_FSCREENINFO, &finfo)) {
        perror("QLinuxFbScreen::setMode");
        qFatal("Error reading fixed information");
    }

    fixupScreenInfo(finfo, vinfo);
    disconnect();
    connect(d_ptr->displaySpec);
    exposeRegion(region(), 0);
}

// save the state of the graphics card
// This is needed so that e.g. we can restore the palette when switching
// between linux virtual consoles.

/*!
    \reimp

    This doesn't do anything; accelerated drivers may wish to reimplement
    it to save graphics cards registers. It's called by the
    \l{Qt for Embedded Linux} server when the virtual console is switched.
*/

void QLinuxFbScreen::save()
{
    // nothing to do.
}


// restore the state of the graphics card.
/*!
    \reimp

    This is called when the virtual console is switched back to
    \l{Qt for Embedded Linux} and restores the palette.
*/
void QLinuxFbScreen::restore()
{
    if (d_ptr->fd == -1)
        return;

    if ((d == 8) || (d == 4)) {
        fb_cmap cmap;
        cmap.start=0;
        cmap.len=screencols;
        cmap.red=(unsigned short int *)
                 malloc(sizeof(unsigned short int)*256);
        cmap.green=(unsigned short int *)
                   malloc(sizeof(unsigned short int)*256);
        cmap.blue=(unsigned short int *)
                  malloc(sizeof(unsigned short int)*256);
        cmap.transp=(unsigned short int *)
                    malloc(sizeof(unsigned short int)*256);
        for (int loopc = 0; loopc < screencols; loopc++) {
            cmap.red[loopc] = qRed(screenclut[loopc]) << 8;
            cmap.green[loopc] = qGreen(screenclut[loopc]) << 8;
            cmap.blue[loopc] = qBlue(screenclut[loopc]) << 8;
            cmap.transp[loopc] = 0;
        }
        ioctl(d_ptr->fd, FBIOPUTCMAP, &cmap);
        free(cmap.red);
        free(cmap.green);
        free(cmap.blue);
        free(cmap.transp);
    }
}

/*!
    \fn int QLinuxFbScreen::sharedRamSize(void * end)
    \internal
*/

// This works like the QScreenCursor code. end points to the end
// of our shared structure, we return the amount of memory we reserved
int QLinuxFbScreen::sharedRamSize(void * end)
{
    shared=(QLinuxFb_Shared *)end;
    shared--;
    return sizeof(QLinuxFb_Shared);
}

/*!
    \reimp
*/
void QLinuxFbScreen::setDirty(const QRect &r)
{
    if(d_ptr->driverType == EInk8Track) {
        // e-Ink displays need a trigger to actually show what is
        // in their framebuffer memory. The 8-Track driver does this
        // by adding custom IOCTLs - FBIO_EINK_DISP_PIC (0x46a2) takes
        // an argument specifying whether or not to flash the screen
        // while updating.
        // There doesn't seem to be a way to tell it to just update
        // a subset of the screen.
        if(r.left() == 0 && r.top() == 0 && r.width() == dw && r.height() == dh)
            ioctl(d_ptr->fd, 0x46a2, 1);
        else
            ioctl(d_ptr->fd, 0x46a2, 0);
    }
}

/*!
    \reimp
*/
void QLinuxFbScreen::blank(bool on)
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

void QLinuxFbScreen::setPixelFormat(struct fb_var_screeninfo info)
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
            pixeltype = QScreen::BGRPixel;
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
            pixeltype = QScreen::BGRPixel;
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
            pixeltype = QScreen::BGRPixel;
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
            pixeltype = QScreen::BGRPixel;
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

    QScreen::setPixelFormat(format);
}

bool QLinuxFbScreen::useOffscreen()
{
    // Not done for 8Track because on e-Ink displays,
    // everything is offscreen anyway
    if (d_ptr->driverType == EInk8Track || ((mapsize - size) < 16*1024))
        return false;

    return true;
}

QT_END_NAMESPACE

#endif // QT_NO_QWS_LINUXFB
