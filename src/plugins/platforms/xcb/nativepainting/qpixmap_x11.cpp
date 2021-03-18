/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QGuiApplication>

#include <private/qdrawhelper_p.h>
#include <private/qimage_p.h>
#include <private/qimagepixmapcleanuphooks_p.h>

#include "qxcbnativepainting.h"
#include "qpixmap_x11_p.h"
#include "qcolormap_x11_p.h"
#include "qpaintengine_x11_p.h"

QT_BEGIN_NAMESPACE

#if QT_POINTER_SIZE == 8 // 64-bit versions

Q_ALWAYS_INLINE uint PREMUL(uint x) {
    uint a = x >> 24;
    quint64 t = (((quint64(x)) | ((quint64(x)) << 24)) & 0x00ff00ff00ff00ff) * a;
    t = (t + ((t >> 8) & 0xff00ff00ff00ff) + 0x80008000800080) >> 8;
    t &= 0x000000ff00ff00ff;
    return (uint(t)) | (uint(t >> 24)) | (a << 24);
}

#else // 32-bit versions

Q_ALWAYS_INLINE uint PREMUL(uint x) {
    uint a = x >> 24;
    uint t = (x & 0xff00ff) * a;
    t = (t + ((t >> 8) & 0xff00ff) + 0x800080) >> 8;
    t &= 0xff00ff;

    x = ((x >> 8) & 0xff) * a;
    x = (x + ((x >> 8) & 0xff) + 0x80);
    x &= 0xff00;
    x |= t | (a << 24);
    return x;
}
#endif



struct QXImageWrapper
{
    XImage *xi;
};

QPixmap qt_toX11Pixmap(const QImage &image)
{
    QPlatformPixmap *data =
        new QX11PlatformPixmap(image.depth() == 1
                           ? QPlatformPixmap::BitmapType
                           : QPlatformPixmap::PixmapType);

    data->fromImage(image, Qt::AutoColor);

    return QPixmap(data);
}

QPixmap qt_toX11Pixmap(const QPixmap &pixmap)
{
    if (pixmap.isNull())
        return QPixmap();

    if (QPixmap(pixmap).data_ptr()->classId() == QPlatformPixmap::X11Class)
        return pixmap;

    return qt_toX11Pixmap(pixmap.toImage());
}

// For thread-safety:
//   image->data does not belong to X11, so we must free it ourselves.

inline static void qSafeXDestroyImage(XImage *x)
{
    if (x->data) {
        free(x->data);
        x->data = 0;
    }
    XDestroyImage(x);
}

QBitmap QX11PlatformPixmap::mask_to_bitmap(int screen) const
{
    if (!x11_mask)
        return QBitmap();
    qt_x11SetDefaultScreen(screen);
    QBitmap bm(w, h);
    QX11PlatformPixmap *that = qt_x11Pixmap(bm);
    const QXcbX11Info *x = that->x11_info();
    GC gc = XCreateGC(x->display(), that->handle(), 0, 0);
    XCopyArea(x->display(), x11_mask, that->handle(), gc, 0, 0,
              that->width(), that->height(), 0, 0);
    XFreeGC(x->display(), gc);
    return bm;
}

void QX11PlatformPixmap::bitmapFromImage(const QImage &image)
{
    w = image.width();
    h = image.height();
    d = 1;
    is_null = (w <= 0 || h <= 0);
    hd = createBitmapFromImage(image);
#if QT_CONFIG(xrender)
    if (X11->use_xrender)
        picture = XRenderCreatePicture(xinfo.display(), hd,
                                       XRenderFindStandardFormat(xinfo.display(), PictStandardA1), 0, 0);
#endif // QT_CONFIG(xrender)
}

bool QX11PlatformPixmap::canTakeQImageFromXImage(const QXImageWrapper &xiWrapper) const
{
    XImage *xi = xiWrapper.xi;

    if (xi->format != ZPixmap)
        return false;

    // ARGB32_Premultiplied
    if (picture && depth() == 32)
        return true;

    // RGB32
    if (depth() == 24 && xi->bits_per_pixel == 32 && xi->red_mask == 0xff0000
        && xi->green_mask == 0xff00 && xi->blue_mask == 0xff)
        return true;

    // RGB16
    if (depth() == 16 && xi->bits_per_pixel == 16 && xi->red_mask == 0xf800
        && xi->green_mask == 0x7e0 && xi->blue_mask == 0x1f)
        return true;

    return false;
}

QImage QX11PlatformPixmap::takeQImageFromXImage(const QXImageWrapper &xiWrapper) const
{
    XImage *xi = xiWrapper.xi;

    QImage::Format format = QImage::Format_ARGB32_Premultiplied;
    if (depth() == 24)
        format = QImage::Format_RGB32;
    else if (depth() == 16)
        format = QImage::Format_RGB16;

    QImage image((uchar *)xi->data, xi->width, xi->height, xi->bytes_per_line, format);
    image.setDevicePixelRatio(devicePixelRatio());
    // take ownership
    image.data_ptr()->own_data = true;
    xi->data = 0;

    // we may have to swap the byte order
    if ((QSysInfo::ByteOrder == QSysInfo::LittleEndian && xi->byte_order == MSBFirst)
        || (QSysInfo::ByteOrder == QSysInfo::BigEndian && xi->byte_order == LSBFirst))
    {
        for (int i=0; i < image.height(); i++) {
            if (depth() == 16) {
                ushort *p = (ushort*)image.scanLine(i);
                ushort *end = p + image.width();
                while (p < end) {
                    *p = ((*p << 8) & 0xff00) | ((*p >> 8) & 0x00ff);
                    p++;
                }
            } else {
                uint *p = (uint*)image.scanLine(i);
                uint *end = p + image.width();
                while (p < end) {
                    *p = ((*p << 24) & 0xff000000) | ((*p << 8) & 0x00ff0000)
                         | ((*p >> 8) & 0x0000ff00) | ((*p >> 24) & 0x000000ff);
                    p++;
                }
            }
        }
    }

    // fix-up alpha channel
    if (format == QImage::Format_RGB32) {
        QRgb *p = (QRgb *)image.bits();
        for (int y = 0; y < xi->height; ++y) {
            for (int x = 0; x < xi->width; ++x)
                p[x] |= 0xff000000;
            p += xi->bytes_per_line / 4;
        }
    }

    XDestroyImage(xi);
    return image;
}

XID QX11PlatformPixmap::bitmap_to_mask(const QBitmap &bitmap, int screen)
{
    if (bitmap.isNull())
        return 0;
    QBitmap bm = bitmap;
    qt_x11SetScreen(bm, screen);

    QX11PlatformPixmap *that = qt_x11Pixmap(bm);
    const QXcbX11Info *x = that->x11_info();
    Pixmap mask = XCreatePixmap(x->display(), RootWindow(x->display(), screen),
                                that->width(), that->height(), 1);
    GC gc = XCreateGC(x->display(), mask, 0, 0);
    XCopyArea(x->display(), that->handle(), mask, gc, 0, 0,
              that->width(), that->height(), 0, 0);
    XFreeGC(x->display(), gc);
    return mask;
}

Drawable qt_x11Handle(const QPixmap &pixmap)
{
    if (pixmap.isNull())
        return XNone;

    if (pixmap.handle()->classId() != QPlatformPixmap::X11Class)
        return XNone;

    return static_cast<const QX11PlatformPixmap *>(pixmap.handle())->handle();
}


/*****************************************************************************
  Internal functions
 *****************************************************************************/

//extern const uchar *qt_get_bitflip_array();                // defined in qimage.cpp

// Returns position of highest bit set or -1 if none
static int highest_bit(uint v)
{
    int i;
    uint b = (uint)1 << 31;
    for (i=31; ((b & v) == 0) && i>=0;         i--)
        b >>= 1;
    return i;
}

// Counts the number of bits set in 'v'
static uint n_bits(uint v)
{
    int i = 0;
    while (v) {
        v = v & (v - 1);
        i++;
    }
    return i;
}

static uint *red_scale_table   = 0;
static uint *green_scale_table = 0;
static uint *blue_scale_table  = 0;

static void cleanup_scale_tables()
{
    delete[] red_scale_table;
    delete[] green_scale_table;
    delete[] blue_scale_table;
}

/*
  Could do smart bitshifting, but the "obvious" algorithm only works for
  nBits >= 4. This is more robust.
*/
static void build_scale_table(uint **table, uint nBits)
{
    if (nBits > 7) {
        qWarning("build_scale_table: internal error, nBits = %i", nBits);
        return;
    }
    if (!*table) {
        static bool firstTable = true;
        if (firstTable) {
            qAddPostRoutine(cleanup_scale_tables);
            firstTable = false;
        }
        *table = new uint[256];
    }
    int   maxVal   = (1 << nBits) - 1;
    int   valShift = 8 - nBits;
    int i;
    for (i = 0 ; i < maxVal + 1 ; i++)
        (*table)[i << valShift] = i*255/maxVal;
}

static int defaultScreen = -1;

int qt_x11SetDefaultScreen(int screen)
{
    int old = defaultScreen;
    defaultScreen = screen;
    return old;
}

void qt_x11SetScreen(QPixmap &pixmap, int screen)
{
    if (pixmap.paintingActive()) {
        qWarning("qt_x11SetScreen(): Cannot change screens during painting");
        return;
    }

    if (pixmap.isNull())
        return;

    if (pixmap.handle()->classId() != QPlatformPixmap::X11Class)
        return;

    if (screen < 0)
        screen = QXcbX11Info::appScreen();

    QX11PlatformPixmap *pm = static_cast<QX11PlatformPixmap *>(pixmap.handle());
    if (screen == pm->xinfo.screen())
        return; // nothing to do

    if (pixmap.isNull()) {
        pm->xinfo = QXcbX11Info::fromScreen(screen);
        return;
    }

#if 0
    qDebug("qt_x11SetScreen for %p from %d to %d. Size is %d/%d", pm, pm->xinfo.screen(), screen, pm->width(), pm->height());
#endif

    qt_x11SetDefaultScreen(screen);
    pixmap = qt_toX11Pixmap(pixmap.toImage());
}

/*****************************************************************************
  QPixmap member functions
 *****************************************************************************/

QBasicAtomicInt qt_pixmap_serial = Q_BASIC_ATOMIC_INITIALIZER(0);
int Q_GUI_EXPORT qt_x11_preferred_pixmap_depth = 0;

QX11PlatformPixmap::QX11PlatformPixmap(PixelType pixelType)
    : QPlatformPixmap(pixelType, X11Class), hd(0),
      flags(Uninitialized), x11_mask(0), picture(0), mask_picture(0), hd2(0),
      dpr(1.0), pengine(0)
{}

QX11PlatformPixmap::~QX11PlatformPixmap()
{
    // Cleanup hooks have to be called before the handles are freed
    if (is_cached) {
        QImagePixmapCleanupHooks::executePlatformPixmapDestructionHooks(this);
        is_cached = false;
    }

    release();
}

QPlatformPixmap *QX11PlatformPixmap::createCompatiblePlatformPixmap() const
{
    QX11PlatformPixmap *p = new QX11PlatformPixmap(pixelType());
    p->setDevicePixelRatio(devicePixelRatio());
    return p;
}

void QX11PlatformPixmap::resize(int width, int height)
{
    setSerialNumber(qt_pixmap_serial.fetchAndAddRelaxed(1));

    w = width;
    h = height;
    is_null = (w <= 0 || h <= 0);

    if (defaultScreen >= 0 && defaultScreen != xinfo.screen()) {
        xinfo = QXcbX11Info::fromScreen(defaultScreen);
    }

    int dd = xinfo.depth();

    if (qt_x11_preferred_pixmap_depth)
        dd = qt_x11_preferred_pixmap_depth;

    bool make_null = w <= 0 || h <= 0;                // create null pixmap
    d = (pixelType() == BitmapType ? 1 : dd);
    if (make_null || d == 0) {
        w = 0;
        h = 0;
        is_null = true;
        hd = 0;
        picture = 0;
        d = 0;
        if (!make_null)
            qWarning("QPixmap: Invalid pixmap parameters");
        return;
    }
    hd = XCreatePixmap(xinfo.display(),
                       RootWindow(xinfo.display(), xinfo.screen()),
                       w, h, d);
#if QT_CONFIG(xrender)
    if (X11->use_xrender) {
        XRenderPictFormat *format = d == 1
                                    ? XRenderFindStandardFormat(xinfo.display(), PictStandardA1)
                                    : XRenderFindVisualFormat(xinfo.display(), (Visual *) xinfo.visual());
        picture = XRenderCreatePicture(xinfo.display(), hd, format, 0, 0);
    }
#endif // QT_CONFIG(xrender)
}

struct QX11AlphaDetector
{
    bool hasAlpha() const {
        if (checked)
            return has;
        // Will implicitly also check format and return quickly for opaque types...
        checked = true;
        has = image->isNull() ? false : const_cast<QImage *>(image)->data_ptr()->checkForAlphaPixels();
        return has;
    }

    bool hasXRenderAndAlpha() const {
        if (!X11->use_xrender)
            return false;
        return hasAlpha();
    }

    QX11AlphaDetector(const QImage *i, Qt::ImageConversionFlags flags)
        : image(i), checked(false), has(false)
    {
        if (flags & Qt::NoOpaqueDetection) {
            checked = true;
            has = image->hasAlphaChannel();
        }
    }

    const QImage *image;
    mutable bool checked;
    mutable bool has;
};

void QX11PlatformPixmap::fromImage(const QImage &img, Qt::ImageConversionFlags flags)
{
    setSerialNumber(qt_pixmap_serial.fetchAndAddRelaxed(1));

    w = img.width();
    h = img.height();
    d = img.depth();
    is_null = (w <= 0 || h <= 0);
    setDevicePixelRatio(img.devicePixelRatio());

    if (is_null) {
        w = h = 0;
        return;
    }

    if (defaultScreen >= 0 && defaultScreen != xinfo.screen()) {
        xinfo = QXcbX11Info::fromScreen(defaultScreen);
    }

    if (pixelType() == BitmapType) {
        bitmapFromImage(img);
        return;
    }

    if (uint(w) >= 32768 || uint(h) >= 32768) {
        w = h = 0;
        is_null = true;
        return;
    }

    QX11AlphaDetector alphaCheck(&img, flags);
    int dd = alphaCheck.hasXRenderAndAlpha() ? 32 : xinfo.depth();

    if (qt_x11_preferred_pixmap_depth)
        dd = qt_x11_preferred_pixmap_depth;

    QImage image = img;

    // must be monochrome
    if (dd == 1 || (flags & Qt::ColorMode_Mask) == Qt::MonoOnly) {
        if (d != 1) {
            // dither
            image = image.convertToFormat(QImage::Format_MonoLSB, flags);
            d = 1;
        }
    } else { // can be both
        bool conv8 = false;
        if (d > 8 && dd <= 8) { // convert to 8 bit
            if ((flags & Qt::DitherMode_Mask) == Qt::AutoDither)
                flags = (flags & ~Qt::DitherMode_Mask)
                        | Qt::PreferDither;
            conv8 = true;
        } else if ((flags & Qt::ColorMode_Mask) == Qt::ColorOnly) {
            conv8 = (d == 1);                        // native depth wanted
        } else if (d == 1) {
            if (image.colorCount() == 2) {
                QRgb c0 = image.color(0);        // Auto: convert to best
                QRgb c1 = image.color(1);
                conv8 = qMin(c0,c1) != qRgb(0,0,0) || qMax(c0,c1) != qRgb(255,255,255);
            } else {
                // eg. 1-color monochrome images (they do exist).
                conv8 = true;
            }
        }
        if (conv8) {
            image = image.convertToFormat(QImage::Format_Indexed8, flags);
            d = 8;
        }
    }

    if (d == 1 || image.format() > QImage::Format_ARGB32_Premultiplied) {
        QImage::Format fmt = QImage::Format_RGB32;
        if (alphaCheck.hasXRenderAndAlpha() && d > 1)
            fmt = QImage::Format_ARGB32_Premultiplied;
        image = image.convertToFormat(fmt, flags);
        fromImage(image, Qt::AutoColor);
        return;
    }

    Display *dpy   = xinfo.display();
    Visual *visual = (Visual *)xinfo.visual();
    XImage *xi = 0;
    bool    trucol = (visual->c_class >= TrueColor);
    size_t  nbytes = image.sizeInBytes();
    uchar  *newbits= 0;

#if QT_CONFIG(xrender)
    if (alphaCheck.hasXRenderAndAlpha()) {
        const QImage &cimage = image;

        d = 32;

        if (QXcbX11Info::appDepth() != d) {
            xinfo.setDepth(d);
        }

        hd = XCreatePixmap(dpy, RootWindow(dpy, xinfo.screen()), w, h, d);
        picture = XRenderCreatePicture(dpy, hd,
                                       XRenderFindStandardFormat(dpy, PictStandardARGB32), 0, 0);

        xi = XCreateImage(dpy, visual, d, ZPixmap, 0, 0, w, h, 32, 0);
        Q_CHECK_PTR(xi);
        newbits = (uchar *)malloc(xi->bytes_per_line*h);
        Q_CHECK_PTR(newbits);
        xi->data = (char *)newbits;

        switch (cimage.format()) {
        case QImage::Format_Indexed8: {
            QVector<QRgb> colorTable = cimage.colorTable();
            uint *xidata = (uint *)xi->data;
            for (int y = 0; y < h; ++y) {
                const uchar *p = cimage.scanLine(y);
                for (int x = 0; x < w; ++x) {
                    const QRgb rgb = colorTable[p[x]];
                    const int a = qAlpha(rgb);
                    if (a == 0xff)
                        *xidata = rgb;
                    else
                        // RENDER expects premultiplied alpha
                        *xidata = qRgba(qt_div_255(qRed(rgb) * a),
                                        qt_div_255(qGreen(rgb) * a),
                                        qt_div_255(qBlue(rgb) * a),
                                        a);
                    ++xidata;
                }
            }
        }
            break;
        case QImage::Format_RGB32: {
            uint *xidata = (uint *)xi->data;
            for (int y = 0; y < h; ++y) {
                const QRgb *p = (const QRgb *) cimage.scanLine(y);
                for (int x = 0; x < w; ++x)
                    *xidata++ = p[x] | 0xff000000;
            }
        }
            break;
        case QImage::Format_ARGB32: {
            uint *xidata = (uint *)xi->data;
            for (int y = 0; y < h; ++y) {
                const QRgb *p = (const QRgb *) cimage.scanLine(y);
                for (int x = 0; x < w; ++x) {
                    const QRgb rgb = p[x];
                    const int a = qAlpha(rgb);
                    if (a == 0xff)
                        *xidata = rgb;
                    else
                        // RENDER expects premultiplied alpha
                        *xidata = qRgba(qt_div_255(qRed(rgb) * a),
                                        qt_div_255(qGreen(rgb) * a),
                                        qt_div_255(qBlue(rgb) * a),
                                        a);
                    ++xidata;
                }
            }

        }
            break;
        case QImage::Format_ARGB32_Premultiplied: {
            uint *xidata = (uint *)xi->data;
            for (int y = 0; y < h; ++y) {
                const QRgb *p = (const QRgb *) cimage.scanLine(y);
                memcpy(xidata, p, w*sizeof(QRgb));
                xidata += w;
            }
        }
            break;
        default:
            Q_ASSERT(false);
        }

        if ((xi->byte_order == MSBFirst) != (QSysInfo::ByteOrder == QSysInfo::BigEndian)) {
            uint *xidata = (uint *)xi->data;
            uint *xiend = xidata + w*h;
            while (xidata < xiend) {
                *xidata = (*xidata >> 24)
                          | ((*xidata >> 8) & 0xff00)
                          | ((*xidata << 8) & 0xff0000)
                          | (*xidata << 24);
                ++xidata;
            }
        }

        GC gc = XCreateGC(dpy, hd, 0, 0);
        XPutImage(dpy, hd, gc, xi, 0, 0, 0, 0, w, h);
        XFreeGC(dpy, gc);

        qSafeXDestroyImage(xi);

        return;
    }
#endif // QT_CONFIG(xrender)

    if (trucol) {                                // truecolor display
        if (image.format() == QImage::Format_ARGB32_Premultiplied)
            image = image.convertToFormat(QImage::Format_ARGB32);

        const QImage &cimage = image;
        QRgb  pix[256];                                // pixel translation table
        const bool  d8 = (d == 8);
        const uint  red_mask          = (uint)visual->red_mask;
        const uint  green_mask  = (uint)visual->green_mask;
        const uint  blue_mask          = (uint)visual->blue_mask;
        const int   red_shift          = highest_bit(red_mask)   - 7;
        const int   green_shift = highest_bit(green_mask) - 7;
        const int   blue_shift  = highest_bit(blue_mask)  - 7;
        const uint  rbits = highest_bit(red_mask) - lowest_bit(red_mask) + 1;
        const uint  gbits = highest_bit(green_mask) - lowest_bit(green_mask) + 1;
        const uint  bbits = highest_bit(blue_mask) - lowest_bit(blue_mask) + 1;

        if (d8) {                                // setup pixel translation
            QVector<QRgb> ctable = cimage.colorTable();
            for (int i=0; i < cimage.colorCount(); i++) {
                int r = qRed  (ctable[i]);
                int g = qGreen(ctable[i]);
                int b = qBlue (ctable[i]);
                r = red_shift        > 0 ? r << red_shift   : r >> -red_shift;
                g = green_shift > 0 ? g << green_shift : g >> -green_shift;
                b = blue_shift        > 0 ? b << blue_shift  : b >> -blue_shift;
                pix[i] = (b & blue_mask) | (g & green_mask) | (r & red_mask)
                         | ~(blue_mask | green_mask | red_mask);
            }
        }

        xi = XCreateImage(dpy, visual, dd, ZPixmap, 0, 0, w, h, 32, 0);
        Q_CHECK_PTR(xi);
        newbits = (uchar *)malloc(xi->bytes_per_line*h);
        Q_CHECK_PTR(newbits);
        if (!newbits)                                // no memory
            return;
        int bppc = xi->bits_per_pixel;

        bool contig_bits = n_bits(red_mask) == rbits &&
                           n_bits(green_mask) == gbits &&
                           n_bits(blue_mask) == bbits;
        bool dither_tc =
            // Want it?
            (flags & Qt::Dither_Mask) != Qt::ThresholdDither &&
            (flags & Qt::DitherMode_Mask) != Qt::AvoidDither &&
            // Need it?
            bppc < 24 && !d8 &&
            // Can do it? (Contiguous bits?)
            contig_bits;

        static bool init=false;
        static int D[16][16];
        if (dither_tc && !init) {
            // I also contributed this code to XV - WWA.
            /*
              The dither matrix, D, is obtained with this formula:

              D2 = [0 2]
              [3 1]


              D2*n = [4*Dn       4*Dn+2*Un]
              [4*Dn+3*Un  4*Dn+1*Un]
            */
            int n,i,j;
            init=1;

            /* Set D2 */
            D[0][0]=0;
            D[1][0]=2;
            D[0][1]=3;
            D[1][1]=1;

            /* Expand using recursive definition given above */
            for (n=2; n<16; n*=2) {
                for (i=0; i<n; i++) {
                    for (j=0; j<n; j++) {
                        D[i][j]*=4;
                        D[i+n][j]=D[i][j]+2;
                        D[i][j+n]=D[i][j]+3;
                        D[i+n][j+n]=D[i][j]+1;
                    }
                }
            }
            init=true;
        }

        enum { BPP8,
               BPP16_565, BPP16_555,
               BPP16_MSB, BPP16_LSB,
               BPP24_888,
               BPP24_MSB, BPP24_LSB,
               BPP32_8888,
               BPP32_MSB, BPP32_LSB
        } mode = BPP8;

        bool same_msb_lsb = (xi->byte_order == MSBFirst) == (QSysInfo::ByteOrder == QSysInfo::BigEndian);

        if (bppc == 8) // 8 bit
            mode = BPP8;
        else if (bppc == 16) { // 16 bit MSB/LSB
            if (red_shift == 8 && green_shift == 3 && blue_shift == -3 && !d8 && same_msb_lsb)
                mode = BPP16_565;
            else if (red_shift == 7 && green_shift == 2 && blue_shift == -3 && !d8 && same_msb_lsb)
                mode = BPP16_555;
            else
                mode = (xi->byte_order == LSBFirst) ? BPP16_LSB : BPP16_MSB;
        } else if (bppc == 24) { // 24 bit MSB/LSB
            if (red_shift == 16 && green_shift == 8 && blue_shift == 0 && !d8 && same_msb_lsb)
                mode = BPP24_888;
            else
                mode = (xi->byte_order == LSBFirst) ? BPP24_LSB : BPP24_MSB;
        } else if (bppc == 32) { // 32 bit MSB/LSB
            if (red_shift == 16 && green_shift == 8 && blue_shift == 0 && !d8 && same_msb_lsb)
                mode = BPP32_8888;
            else
                mode = (xi->byte_order == LSBFirst) ? BPP32_LSB : BPP32_MSB;
        } else
            qFatal("Logic error 3");

#define GET_PIXEL                                                       \
        uint pixel;                                                     \
        if (d8) pixel = pix[*src++];                                    \
        else {                                                          \
            int r = qRed  (*p);                                         \
            int g = qGreen(*p);                                         \
            int b = qBlue (*p++);                                       \
            r = red_shift   > 0                                         \
                ? r << red_shift   : r >> -red_shift;                   \
            g = green_shift > 0                                         \
                ? g << green_shift : g >> -green_shift;                 \
            b = blue_shift  > 0                                         \
                ? b << blue_shift  : b >> -blue_shift;                  \
            pixel = (r & red_mask)|(g & green_mask) | (b & blue_mask)   \
                    | ~(blue_mask | green_mask | red_mask);             \
        }

#define GET_PIXEL_DITHER_TC                                             \
        int r = qRed  (*p);                                             \
        int g = qGreen(*p);                                             \
        int b = qBlue (*p++);                                           \
        const int thres = D[x%16][y%16];                                \
        if (r <= (255-(1<<(8-rbits))) && ((r<<rbits) & 255)             \
            > thres)                                                    \
            r += (1<<(8-rbits));                                        \
        if (g <= (255-(1<<(8-gbits))) && ((g<<gbits) & 255)             \
            > thres)                                                    \
            g += (1<<(8-gbits));                                        \
        if (b <= (255-(1<<(8-bbits))) && ((b<<bbits) & 255)             \
            > thres)                                                    \
            b += (1<<(8-bbits));                                        \
        r = red_shift   > 0                                             \
            ? r << red_shift   : r >> -red_shift;                       \
        g = green_shift > 0                                             \
            ? g << green_shift : g >> -green_shift;                     \
        b = blue_shift  > 0                                             \
            ? b << blue_shift  : b >> -blue_shift;                      \
        uint pixel = (r & red_mask)|(g & green_mask) | (b & blue_mask);

// again, optimized case
// can't be optimized that much :(
#define GET_PIXEL_DITHER_TC_OPT(red_shift,green_shift,blue_shift,red_mask,green_mask,blue_mask, \
                                rbits,gbits,bbits)                      \
        const int thres = D[x%16][y%16];                                \
        int r = qRed  (*p);                                             \
        if (r <= (255-(1<<(8-rbits))) && ((r<<rbits) & 255)             \
            > thres)                                                    \
            r += (1<<(8-rbits));                                        \
        int g = qGreen(*p);                                             \
        if (g <= (255-(1<<(8-gbits))) && ((g<<gbits) & 255)             \
            > thres)                                                    \
            g += (1<<(8-gbits));                                        \
        int b = qBlue (*p++);                                           \
        if (b <= (255-(1<<(8-bbits))) && ((b<<bbits) & 255)             \
            > thres)                                                    \
            b += (1<<(8-bbits));                                        \
        uint pixel = ((r red_shift) & red_mask)                         \
                     | ((g green_shift) & green_mask)                   \
                     | ((b blue_shift) & blue_mask);

#define CYCLE(body)                                             \
        for (int y=0; y<h; y++) {                               \
            const uchar* src = cimage.scanLine(y);              \
            uchar* dst = newbits + xi->bytes_per_line*y;        \
            const QRgb* p = (const QRgb *)src;                  \
            body                                                \
                }

        if (dither_tc) {
            switch (mode) {
            case BPP16_565:
                CYCLE(
                    quint16* dst16 = (quint16*)dst;
                    for (int x=0; x<w; x++) {
                        GET_PIXEL_DITHER_TC_OPT(<<8,<<3,>>3,0xf800,0x7e0,0x1f,5,6,5)
                            *dst16++ = pixel;
                    }
                    )
                    break;
            case BPP16_555:
                CYCLE(
                    quint16* dst16 = (quint16*)dst;
                    for (int x=0; x<w; x++) {
                        GET_PIXEL_DITHER_TC_OPT(<<7,<<2,>>3,0x7c00,0x3e0,0x1f,5,5,5)
                            *dst16++ = pixel;
                    }
                    )
                    break;
            case BPP16_MSB:                        // 16 bit MSB
                CYCLE(
                    for (int x=0; x<w; x++) {
                        GET_PIXEL_DITHER_TC
                            *dst++ = (pixel >> 8);
                        *dst++ = pixel;
                    }
                    )
                    break;
            case BPP16_LSB:                        // 16 bit LSB
                CYCLE(
                    for (int x=0; x<w; x++) {
                        GET_PIXEL_DITHER_TC
                            *dst++ = pixel;
                        *dst++ = pixel >> 8;
                    }
                    )
                    break;
            default:
                qFatal("Logic error");
            }
        } else {
            switch (mode) {
            case BPP8:                        // 8 bit
                CYCLE(
                    Q_UNUSED(p);
                    for (int x=0; x<w; x++)
                        *dst++ = pix[*src++];
                    )
                    break;
            case BPP16_565:
                CYCLE(
                    quint16* dst16 = (quint16*)dst;
                    for (int x = 0; x < w; x++) {
                        *dst16++ = ((*p >> 8) & 0xf800)
                                   | ((*p >> 5) & 0x7e0)
                                   | ((*p >> 3) & 0x1f);
                        ++p;
                    }
                    )
                    break;
            case BPP16_555:
                CYCLE(
                    quint16* dst16 = (quint16*)dst;
                    for (int x=0; x<w; x++) {
                        *dst16++ = ((*p >> 9) & 0x7c00)
                                   | ((*p >> 6) & 0x3e0)
                                   | ((*p >> 3) & 0x1f);
                        ++p;
                    }
                    )
                    break;
            case BPP16_MSB:                        // 16 bit MSB
                CYCLE(
                    for (int x=0; x<w; x++) {
                        GET_PIXEL
                            *dst++ = (pixel >> 8);
                        *dst++ = pixel;
                    }
                    )
                    break;
            case BPP16_LSB:                        // 16 bit LSB
                CYCLE(
                    for (int x=0; x<w; x++) {
                        GET_PIXEL
                            *dst++ = pixel;
                        *dst++ = pixel >> 8;
                    }
                    )
                    break;
            case BPP24_888:
                CYCLE(
                    if (QSysInfo::ByteOrder == QSysInfo::BigEndian) {
                        for (int x=0; x<w; x++) {
                            *dst++ = qRed  (*p);
                            *dst++ = qGreen(*p);
                            *dst++ = qBlue (*p++);
                        }
                    } else {
                        for (int x=0; x<w; x++) {
                            *dst++ = qBlue (*p);
                            *dst++ = qGreen(*p);
                            *dst++ = qRed  (*p++);
                        }
                    }
                    )
                    break;
            case BPP24_MSB:                        // 24 bit MSB
                CYCLE(
                    for (int x=0; x<w; x++) {
                        GET_PIXEL
                            *dst++ = pixel >> 16;
                        *dst++ = pixel >> 8;
                        *dst++ = pixel;
                    }
                    )
                    break;
            case BPP24_LSB:                        // 24 bit LSB
                CYCLE(
                    for (int x=0; x<w; x++) {
                        GET_PIXEL
                            *dst++ = pixel;
                        *dst++ = pixel >> 8;
                        *dst++ = pixel >> 16;
                    }
                    )
                    break;
            case BPP32_8888:
                CYCLE(
                    memcpy(dst, p, w * 4);
                    )
                    break;
            case BPP32_MSB:                        // 32 bit MSB
                CYCLE(
                    for (int x=0; x<w; x++) {
                        GET_PIXEL
                            *dst++ = pixel >> 24;
                        *dst++ = pixel >> 16;
                        *dst++ = pixel >> 8;
                        *dst++ = pixel;
                    }
                    )
                    break;
            case BPP32_LSB:                        // 32 bit LSB
                CYCLE(
                    for (int x=0; x<w; x++) {
                        GET_PIXEL
                            *dst++ = pixel;
                        *dst++ = pixel >> 8;
                        *dst++ = pixel >> 16;
                        *dst++ = pixel >> 24;
                    }
                    )
                    break;
            default:
                qFatal("Logic error 2");
            }
        }
        xi->data = (char *)newbits;
    }

    if (d == 8 && !trucol) {                        // 8 bit pixmap
        int  pop[256];                                // pixel popularity

        if (image.colorCount() == 0)
            image.setColorCount(1);

        const QImage &cimage = image;
        memset(pop, 0, sizeof(int)*256);        // reset popularity array
        for (int i = 0; i < h; i++) {                        // for each scanline...
            const uchar* p = cimage.scanLine(i);
            const uchar *end = p + w;
            while (p < end)                        // compute popularity
                pop[*p++]++;
        }

        newbits = (uchar *)malloc(nbytes);        // copy image into newbits
        Q_CHECK_PTR(newbits);
        if (!newbits)                                // no memory
            return;
        uchar* p = newbits;
        memcpy(p, cimage.bits(), nbytes);        // copy image data into newbits

        /*
         * The code below picks the most important colors. It is based on the
         * diversity algorithm, implemented in XV 3.10. XV is (C) by John Bradley.
         */

        struct PIX {                                // pixel sort element
            uchar r,g,b,n;                        // color + pad
            int          use;                                // popularity
            int          index;                        // index in colormap
            int          mindist;
        };
        int ncols = 0;
        for (int i=0; i< cimage.colorCount(); i++) { // compute number of colors
            if (pop[i] > 0)
                ncols++;
        }
        for (int i = cimage.colorCount(); i < 256; i++) // ignore out-of-range pixels
            pop[i] = 0;

        // works since we make sure above to have at least
        // one color in the image
        if (ncols == 0)
            ncols = 1;

        PIX pixarr[256];                        // pixel array
        PIX pixarr_sorted[256];                        // pixel array (sorted)
        memset(pixarr, 0, ncols*sizeof(PIX));
        PIX *px                   = &pixarr[0];
        int  maxpop = 0;
        int  maxpix = 0;
        uint j = 0;
        QVector<QRgb> ctable = cimage.colorTable();
        for (int i = 0; i < 256; i++) {                // init pixel array
            if (pop[i] > 0) {
                px->r = qRed  (ctable[i]);
                px->g = qGreen(ctable[i]);
                px->b = qBlue (ctable[i]);
                px->n = 0;
                px->use = pop[i];
                if (pop[i] > maxpop) {        // select most popular entry
                    maxpop = pop[i];
                    maxpix = j;
                }
                px->index = i;
                px->mindist = 1000000;
                px++;
                j++;
            }
        }
        pixarr_sorted[0] = pixarr[maxpix];
        pixarr[maxpix].use = 0;

        for (int i = 1; i < ncols; i++) {                // sort pixels
            int minpix = -1, mindist = -1;
            px = &pixarr_sorted[i-1];
            int r = px->r;
            int g = px->g;
            int b = px->b;
            int dist;
            if ((i & 1) || i<10) {                // sort on max distance
                for (int j=0; j<ncols; j++) {
                    px = &pixarr[j];
                    if (px->use) {
                        dist = (px->r - r)*(px->r - r) +
                               (px->g - g)*(px->g - g) +
                               (px->b - b)*(px->b - b);
                        if (px->mindist > dist)
                            px->mindist = dist;
                        if (px->mindist > mindist) {
                            mindist = px->mindist;
                            minpix = j;
                        }
                    }
                }
            } else {                                // sort on max popularity
                for (int j=0; j<ncols; j++) {
                    px = &pixarr[j];
                    if (px->use) {
                        dist = (px->r - r)*(px->r - r) +
                               (px->g - g)*(px->g - g) +
                               (px->b - b)*(px->b - b);
                        if (px->mindist > dist)
                            px->mindist = dist;
                        if (px->use > mindist) {
                            mindist = px->use;
                            minpix = j;
                        }
                    }
                }
            }
            pixarr_sorted[i] = pixarr[minpix];
            pixarr[minpix].use = 0;
        }

        QXcbColormap cmap = QXcbColormap::instance(xinfo.screen());
        uint pix[256];                                // pixel translation table
        px = &pixarr_sorted[0];
        for (int i = 0; i < ncols; i++) {                // allocate colors
            QColor c(px->r, px->g, px->b);
            pix[px->index] = cmap.pixel(c);
            px++;
        }

        p = newbits;
        for (size_t i = 0; i < nbytes; i++) {                // translate pixels
            *p = pix[*p];
            p++;
        }
    }

    if (!xi) {                                // X image not created
        xi = XCreateImage(dpy, visual, dd, ZPixmap, 0, 0, w, h, 32, 0);
        if (xi->bits_per_pixel == 16) {        // convert 8 bpp ==> 16 bpp
            ushort *p2;
            int            p2inc = xi->bytes_per_line/sizeof(ushort);
            ushort *newerbits = (ushort *)malloc(xi->bytes_per_line * h);
            Q_CHECK_PTR(newerbits);
            if (!newerbits)                                // no memory
                return;
            uchar* p = newbits;
            for (int y = 0; y < h; y++) {                // OOPS: Do right byte order!!
                p2 = newerbits + p2inc*y;
                for (int x = 0; x < w; x++)
                    *p2++ = *p++;
            }
            free(newbits);
            newbits = (uchar *)newerbits;
        } else if (xi->bits_per_pixel != 8) {
            qWarning("QPixmap::fromImage: Display not supported "
                     "(bpp=%d)", xi->bits_per_pixel);
        }
        xi->data = (char *)newbits;
    }

    hd = XCreatePixmap(dpy,
                       RootWindow(dpy, xinfo.screen()),
                       w, h, dd);

    GC gc = XCreateGC(dpy, hd, 0, 0);
    XPutImage(dpy, hd, gc, xi, 0, 0, 0, 0, w, h);
    XFreeGC(dpy, gc);

    qSafeXDestroyImage(xi);
    d = dd;

#if QT_CONFIG(xrender)
    if (X11->use_xrender) {
        XRenderPictFormat *format = d == 1
                                    ? XRenderFindStandardFormat(dpy, PictStandardA1)
                                    : XRenderFindVisualFormat(dpy, (Visual *)xinfo.visual());
        picture = XRenderCreatePicture(dpy, hd, format, 0, 0);
    }
#endif

    if (alphaCheck.hasAlpha()) {
        QBitmap m = QBitmap::fromImage(image.createAlphaMask(flags));
        setMask(m);
    }
}

void QX11PlatformPixmap::copy(const QPlatformPixmap *data, const QRect &rect)
{
    if (data->pixelType() == BitmapType) {
        fromImage(data->toImage().copy(rect), Qt::AutoColor);
        return;
    }

    const QX11PlatformPixmap *x11Data = static_cast<const QX11PlatformPixmap*>(data);

    setSerialNumber(qt_pixmap_serial.fetchAndAddRelaxed(1));

    flags &= ~Uninitialized;
    xinfo = x11Data->xinfo;
    d = x11Data->d;
    w = rect.width();
    h = rect.height();
    is_null = (w <= 0 || h <= 0);
    hd = XCreatePixmap(xinfo.display(),
                       RootWindow(xinfo.display(), x11Data->xinfo.screen()),
                       w, h, d);
#if QT_CONFIG(xrender)
    if (X11->use_xrender) {
        XRenderPictFormat *format = d == 32
                                    ? XRenderFindStandardFormat(xinfo.display(), PictStandardARGB32)
                                    : XRenderFindVisualFormat(xinfo.display(), (Visual *)xinfo.visual());
        picture = XRenderCreatePicture(xinfo.display(), hd, format, 0, 0);
    }
#endif // QT_CONFIG(xrender)
    if (x11Data->x11_mask) {
        x11_mask = XCreatePixmap(xinfo.display(), hd, w, h, 1);
#if QT_CONFIG(xrender)
        if (X11->use_xrender) {
            mask_picture = XRenderCreatePicture(xinfo.display(), x11_mask,
                                                XRenderFindStandardFormat(xinfo.display(), PictStandardA1), 0, 0);
            XRenderPictureAttributes attrs;
            attrs.alpha_map = x11Data->mask_picture;
            XRenderChangePicture(xinfo.display(), x11Data->picture, CPAlphaMap, &attrs);
        }
#endif
    }

#if QT_CONFIG(xrender)
    if (x11Data->picture && x11Data->d == 32) {
        XRenderComposite(xinfo.display(), PictOpSrc,
                         x11Data->picture, 0, picture,
                         rect.x(), rect.y(), 0, 0, 0, 0, w, h);
    } else
#endif
    {
        GC gc = XCreateGC(xinfo.display(), hd, 0, 0);
        XCopyArea(xinfo.display(), x11Data->hd, hd, gc,
                  rect.x(), rect.y(), w, h, 0, 0);
        if (x11Data->x11_mask) {
            GC monogc = XCreateGC(xinfo.display(), x11_mask, 0, 0);
            XCopyArea(xinfo.display(), x11Data->x11_mask, x11_mask, monogc,
                      rect.x(), rect.y(), w, h, 0, 0);
            XFreeGC(xinfo.display(), monogc);
        }
        XFreeGC(xinfo.display(), gc);
    }
}

bool QX11PlatformPixmap::scroll(int dx, int dy, const QRect &rect)
{
    GC gc = XCreateGC(xinfo.display(), hd, 0, 0);
    XCopyArea(xinfo.display(), hd, hd, gc,
              rect.left(), rect.top(), rect.width(), rect.height(),
              rect.left() + dx, rect.top() + dy);
    XFreeGC(xinfo.display(), gc);
    return true;
}

int QX11PlatformPixmap::metric(QPaintDevice::PaintDeviceMetric metric) const
{
    switch (metric) {
    case QPaintDevice::PdmDevicePixelRatio:
        return devicePixelRatio();
        break;
    case QPaintDevice::PdmDevicePixelRatioScaled:
        return devicePixelRatio() * QPaintDevice::devicePixelRatioFScale();
        break;
    case QPaintDevice::PdmWidth:
        return w;
    case QPaintDevice::PdmHeight:
        return h;
    case QPaintDevice::PdmNumColors:
        return 1 << d;
    case QPaintDevice::PdmDepth:
        return d;
    case QPaintDevice::PdmWidthMM: {
        const int screen = xinfo.screen();
        const int mm = DisplayWidthMM(xinfo.display(), screen) * w
                       / DisplayWidth(xinfo.display(), screen);
        return mm;
    }
    case QPaintDevice::PdmHeightMM: {
        const int screen = xinfo.screen();
        const int mm = (DisplayHeightMM(xinfo.display(), screen) * h)
                       / DisplayHeight(xinfo.display(), screen);
        return mm;
    }
    case QPaintDevice::PdmDpiX:
    case QPaintDevice::PdmPhysicalDpiX:
        return QXcbX11Info::appDpiX(xinfo.screen());
    case QPaintDevice::PdmDpiY:
    case QPaintDevice::PdmPhysicalDpiY:
        return QXcbX11Info::appDpiY(xinfo.screen());
    default:
        qWarning("QX11PlatformPixmap::metric(): Invalid metric");
        return 0;
    }
}

void QX11PlatformPixmap::fill(const QColor &fillColor)
{
    if (fillColor.alpha() != 255) {
#if QT_CONFIG(xrender)
        if (X11->use_xrender) {
            if (!picture || d != 32)
                convertToARGB32(/*preserveContents = */false);

            ::Picture src  = X11->getSolidFill(xinfo.screen(), fillColor);
            XRenderComposite(xinfo.display(), PictOpSrc, src, 0, picture,
                             0, 0, width(), height(),
                             0, 0, width(), height());
        } else
#endif
        {
            QImage im(width(), height(), QImage::Format_ARGB32_Premultiplied);
            im.fill(PREMUL(fillColor.rgba()));
            release();
            fromImage(im, Qt::AutoColor | Qt::OrderedAlphaDither);
        }
        return;
    }

    GC gc = XCreateGC(xinfo.display(), hd, 0, 0);
    if (depth() == 1) {
        XSetForeground(xinfo.display(), gc, qGray(fillColor.rgb()) > 127 ? 0 : 1);
    } else if (X11->use_xrender && d >= 24) {
        XSetForeground(xinfo.display(), gc, fillColor.rgba());
    } else {
        XSetForeground(xinfo.display(), gc,
                       QXcbColormap::instance(xinfo.screen()).pixel(fillColor));
    }
    XFillRectangle(xinfo.display(), hd, gc, 0, 0, width(), height());
    XFreeGC(xinfo.display(), gc);
}

QBitmap QX11PlatformPixmap::mask() const
{
    QBitmap mask;
#if QT_CONFIG(xrender)
    if (picture && d == 32) {
        // #### slow - there must be a better way..
        mask = QBitmap::fromImage(toImage().createAlphaMask());
    } else
#endif
    if (d == 1) {
        QX11PlatformPixmap *that = const_cast<QX11PlatformPixmap*>(this);
        mask = QPixmap(that);
    } else {
        mask = mask_to_bitmap(xinfo.screen());
    }
    return mask;
}

void QX11PlatformPixmap::setMask(const QBitmap &newmask)
{
    if (newmask.isNull()) { // clear mask
#if QT_CONFIG(xrender)
        if (picture && d == 32) {
            QX11PlatformPixmap newData(pixelType());
            newData.resize(w, h);
            newData.fill(Qt::black);
            XRenderComposite(xinfo.display(), PictOpOver,
                             picture, 0, newData.picture,
                             0, 0, 0, 0, 0, 0, w, h);
            release();
            *this = newData;
            // the new QX11PlatformPixmap object isn't referenced yet, so
            // ref it
            ref.ref();

            // the below is to make sure the QX11PlatformPixmap destructor
            // doesn't delete our newly created render picture
            newData.hd = 0;
            newData.x11_mask = 0;
            newData.picture = 0;
            newData.mask_picture = 0;
            newData.hd2 = 0;
        } else
#endif
            if (x11_mask) {
#if QT_CONFIG(xrender)
                if (picture) {
                    XRenderPictureAttributes attrs;
                    attrs.alpha_map = 0;
                    XRenderChangePicture(xinfo.display(), picture, CPAlphaMap,
                                         &attrs);
                }
                if (mask_picture)
                    XRenderFreePicture(xinfo.display(), mask_picture);
                mask_picture = 0;
#endif
                XFreePixmap(xinfo.display(), x11_mask);
                x11_mask = 0;
            }
        return;
    }

#if QT_CONFIG(xrender)
    if (picture && d == 32) {
        XRenderComposite(xinfo.display(), PictOpSrc,
                         picture, qt_x11Pixmap(newmask)->x11PictureHandle(),
                         picture, 0, 0, 0, 0, 0, 0, w, h);
    } else
#endif
        if (depth() == 1) {
            XGCValues vals;
            vals.function = GXand;
            GC gc = XCreateGC(xinfo.display(), hd, GCFunction, &vals);
            XCopyArea(xinfo.display(), qt_x11Pixmap(newmask)->handle(), hd, gc, 0, 0,
                      width(), height(), 0, 0);
            XFreeGC(xinfo.display(), gc);
        } else {
            // ##### should or the masks together
            if (x11_mask) {
                XFreePixmap(xinfo.display(), x11_mask);
#if QT_CONFIG(xrender)
                if (mask_picture)
                    XRenderFreePicture(xinfo.display(), mask_picture);
#endif
            }
            x11_mask = QX11PlatformPixmap::bitmap_to_mask(newmask, xinfo.screen());
#if QT_CONFIG(xrender)
            if (picture) {
                mask_picture = XRenderCreatePicture(xinfo.display(), x11_mask,
                                                    XRenderFindStandardFormat(xinfo.display(), PictStandardA1), 0, 0);
                XRenderPictureAttributes attrs;
                attrs.alpha_map = mask_picture;
                XRenderChangePicture(xinfo.display(), picture, CPAlphaMap, &attrs);
            }
#endif
        }
}

bool QX11PlatformPixmap::hasAlphaChannel() const
{
    if (picture && d == 32)
        return true;

    if (x11_mask && d == 1)
        return true;

    return false;
}

QPixmap QX11PlatformPixmap::transformed(const QTransform &transform, Qt::TransformationMode mode) const
{
    if (mode == Qt::SmoothTransformation || transform.type() >= QTransform::TxProject) {
        QImage image = toImage();
        return QPixmap::fromImage(image.transformed(transform, mode));
    }

    uint   w = 0;
    uint   h = 0;                               // size of target pixmap
    uint   ws, hs;                              // size of source pixmap
    uchar *dptr;                                // data in target pixmap
    uint   dbpl, dbytes;                        // bytes per line/bytes total
    uchar *sptr;                                // data in original pixmap
    int    sbpl;                                // bytes per line in original
    int    bpp;                                 // bits per pixel
    bool   depth1 = depth() == 1;
    Display *dpy = xinfo.display();

    ws = width();
    hs = height();

    QTransform mat(transform.m11(), transform.m12(), transform.m13(),
                   transform.m21(), transform.m22(), transform.m23(),
                   0., 0., 1);
    bool complex_xform = false;
    qreal scaledWidth;
    qreal scaledHeight;

    if (mat.type() <= QTransform::TxScale) {
        scaledHeight = qAbs(mat.m22()) * hs + 0.9999;
        scaledWidth = qAbs(mat.m11()) * ws + 0.9999;
        h = qAbs(int(scaledHeight));
        w = qAbs(int(scaledWidth));
    } else {                                        // rotation or shearing
        QPolygonF a(QRectF(0, 0, ws, hs));
        a = mat.map(a);
        QRect r = a.boundingRect().toAlignedRect();
        w = r.width();
        h = r.height();
        scaledWidth = w;
        scaledHeight = h;
        complex_xform = true;
    }
    mat = QPixmap::trueMatrix(mat, ws, hs); // true matrix

    bool invertible;
    mat = mat.inverted(&invertible);  // invert matrix

    if (h == 0 || w == 0 || !invertible
        || qAbs(scaledWidth) >= 32768 || qAbs(scaledHeight) >= 32768 )
    // error, return null pixmap
        return QPixmap();

    XImage *xi = XGetImage(xinfo.display(), handle(), 0, 0, ws, hs, AllPlanes,
                           depth1 ? XYPixmap : ZPixmap);

    if (!xi)
        return QPixmap();

    sbpl = xi->bytes_per_line;
    sptr = (uchar *)xi->data;
    bpp         = xi->bits_per_pixel;

    if (depth1)
        dbpl = (w+7)/8;
    else
        dbpl = ((w*bpp+31)/32)*4;
    dbytes = dbpl*h;

    dptr = (uchar *)malloc(dbytes);        // create buffer for bits
    Q_CHECK_PTR(dptr);
    if (depth1)                                // fill with zeros
        memset(dptr, 0, dbytes);
    else if (bpp == 8)                        // fill with background color
        memset(dptr, WhitePixel(xinfo.display(), xinfo.screen()), dbytes);
    else
        memset(dptr, 0, dbytes);

    // #define QT_DEBUG_XIMAGE
#if defined(QT_DEBUG_XIMAGE)
    qDebug("----IMAGE--INFO--------------");
    qDebug("width............. %d", xi->width);
    qDebug("height............ %d", xi->height);
    qDebug("xoffset........... %d", xi->xoffset);
    qDebug("format............ %d", xi->format);
    qDebug("byte order........ %d", xi->byte_order);
    qDebug("bitmap unit....... %d", xi->bitmap_unit);
    qDebug("bitmap bit order.. %d", xi->bitmap_bit_order);
    qDebug("depth............. %d", xi->depth);
    qDebug("bytes per line.... %d", xi->bytes_per_line);
    qDebug("bits per pixel.... %d", xi->bits_per_pixel);
#endif

    int type;
    if (xi->bitmap_bit_order == MSBFirst)
        type = QT_XFORM_TYPE_MSBFIRST;
    else
        type = QT_XFORM_TYPE_LSBFIRST;
    int        xbpl, p_inc;
    if (depth1) {
        xbpl  = (w+7)/8;
        p_inc = dbpl - xbpl;
    } else {
        xbpl  = (w*bpp)/8;
        p_inc = dbpl - xbpl;
    }

    if (!qt_xForm_helper(mat, xi->xoffset, type, bpp, dptr, xbpl, p_inc, h, sptr, sbpl, ws, hs)){
        qWarning("QPixmap::transform: display not supported (bpp=%d)",bpp);
        QPixmap pm;
        free(dptr);
        return pm;
    }

    qSafeXDestroyImage(xi);

    if (depth1) {                                // mono bitmap
        QBitmap bm = QBitmap::fromData(QSize(w, h), dptr,
                                       BitmapBitOrder(xinfo.display()) == MSBFirst
                                       ? QImage::Format_Mono
                                       : QImage::Format_MonoLSB);
        free(dptr);
        return bm;
    } else {                                        // color pixmap
        QX11PlatformPixmap *x11Data = new QX11PlatformPixmap(QPlatformPixmap::PixmapType);
        QPixmap pm(x11Data);
        x11Data->flags &= ~QX11PlatformPixmap::Uninitialized;
        x11Data->xinfo = xinfo;
        x11Data->d = d;
        x11Data->w = w;
        x11Data->h = h;
        x11Data->is_null = (w <= 0 || h <= 0);
        x11Data->hd = XCreatePixmap(xinfo.display(),
                                    RootWindow(xinfo.display(), xinfo.screen()),
                                    w, h, d);
        x11Data->setSerialNumber(qt_pixmap_serial.fetchAndAddRelaxed(1));

#if QT_CONFIG(xrender)
        if (X11->use_xrender) {
            XRenderPictFormat *format = x11Data->d == 32
                                        ? XRenderFindStandardFormat(xinfo.display(), PictStandardARGB32)
                                        : XRenderFindVisualFormat(xinfo.display(), (Visual *) x11Data->xinfo.visual());
            x11Data->picture = XRenderCreatePicture(xinfo.display(), x11Data->hd, format, 0, 0);
        }
#endif // QT_CONFIG(xrender)

        GC gc = XCreateGC(xinfo.display(), x11Data->hd, 0, 0);
        xi = XCreateImage(dpy, (Visual*)x11Data->xinfo.visual(),
                          x11Data->d,
                          ZPixmap, 0, (char *)dptr, w, h, 32, 0);
        XPutImage(dpy, qt_x11Pixmap(pm)->handle(), gc, xi, 0, 0, 0, 0, w, h);
        qSafeXDestroyImage(xi);
        XFreeGC(xinfo.display(), gc);

        if (x11_mask) { // xform mask, too
            pm.setMask(mask_to_bitmap(xinfo.screen()).transformed(transform));
        } else if (d != 32 && complex_xform) { // need a mask!
            QBitmap mask(ws, hs);
            mask.fill(Qt::color1);
            pm.setMask(mask.transformed(transform));
        }
        return pm;
    }
}

QImage QX11PlatformPixmap::toImage() const
{
    return toImage(QRect(0, 0, w, h));
}

QImage QX11PlatformPixmap::toImage(const QRect &rect) const
{
    Window root_return;
    int x_return;
    int y_return;
    unsigned int width_return;
    unsigned int height_return;
    unsigned int border_width_return;
    unsigned int depth_return;

    XGetGeometry(xinfo.display(), hd, &root_return, &x_return, &y_return, &width_return, &height_return, &border_width_return, &depth_return);

    QXImageWrapper xiWrapper;
    xiWrapper.xi = XGetImage(xinfo.display(), hd, rect.x(), rect.y(), rect.width(), rect.height(),
                             AllPlanes, (depth() == 1) ? XYPixmap : ZPixmap);

    Q_CHECK_PTR(xiWrapper.xi);
    if (!xiWrapper.xi)
        return QImage();

    if (!x11_mask && canTakeQImageFromXImage(xiWrapper))
        return takeQImageFromXImage(xiWrapper);

    QImage image = toImage(xiWrapper, rect);
    qSafeXDestroyImage(xiWrapper.xi);
    return image;
}

#if QT_CONFIG(xrender)
static XRenderPictFormat *qt_renderformat_for_depth(const QXcbX11Info &xinfo, int depth)
{
    if (depth == 1)
        return XRenderFindStandardFormat(xinfo.display(), PictStandardA1);
    else if (depth == 32)
        return XRenderFindStandardFormat(xinfo.display(), PictStandardARGB32);
    else
        return XRenderFindVisualFormat(xinfo.display(), (Visual *)xinfo.visual());
}
#endif

Q_GLOBAL_STATIC(QX11PaintEngine, qt_x11_paintengine)

QPaintEngine *QX11PlatformPixmap::paintEngine() const
{
    QX11PlatformPixmap *that = const_cast<QX11PlatformPixmap*>(this);

    if ((flags & Readonly)/* && share_mode == QPixmap::ImplicitlyShared*/) {
        // if someone wants to draw onto us, copy the shared contents
        // and turn it into a fully fledged QPixmap
        ::Pixmap hd_copy = XCreatePixmap(xinfo.display(), RootWindow(xinfo.display(), xinfo.screen()),
                                         w, h, d);
#if QT_CONFIG(xrender)
        if (picture && d == 32) {
            XRenderPictFormat *format = qt_renderformat_for_depth(xinfo, d);
            ::Picture picture_copy = XRenderCreatePicture(xinfo.display(),
                                                          hd_copy, format,
                                                          0, 0);

            XRenderComposite(xinfo.display(), PictOpSrc, picture, 0, picture_copy,
                             0, 0, 0, 0, 0, 0, w, h);
            XRenderFreePicture(xinfo.display(), picture);
            that->picture = picture_copy;
        } else
#endif
        {
            GC gc = XCreateGC(xinfo.display(), hd_copy, 0, 0);
            XCopyArea(xinfo.display(), hd, hd_copy, gc, 0, 0, w, h, 0, 0);
            XFreeGC(xinfo.display(), gc);
        }
        that->hd = hd_copy;
        that->flags &= ~QX11PlatformPixmap::Readonly;
    }

    if (qt_x11_paintengine->isActive()) {
        if (!that->pengine)
            that->pengine = new QX11PaintEngine;

        return that->pengine;
    }

    return qt_x11_paintengine();
}

qreal QX11PlatformPixmap::devicePixelRatio() const
{
    return dpr;
}

void QX11PlatformPixmap::setDevicePixelRatio(qreal scaleFactor)
{
    dpr = scaleFactor;
}

Pixmap QX11PlatformPixmap::x11ConvertToDefaultDepth()
{
#if QT_CONFIG(xrender)
    if (d == xinfo.appDepth() || !X11->use_xrender)
        return hd;
    if (!hd2) {
        hd2 = XCreatePixmap(xinfo.display(), hd, w, h, xinfo.appDepth());
        XRenderPictFormat *format = XRenderFindVisualFormat(xinfo.display(),
                                                            (Visual*) xinfo.visual());
        Picture pic = XRenderCreatePicture(xinfo.display(), hd2, format, 0, 0);
        XRenderComposite(xinfo.display(), PictOpSrc, picture,
                         XNone, pic, 0, 0, 0, 0, 0, 0, w, h);
        XRenderFreePicture(xinfo.display(), pic);
    }
    return hd2;
#else
    return hd;
#endif
}

XID QX11PlatformPixmap::createBitmapFromImage(const QImage &image)
{
    QImage img = image.convertToFormat(QImage::Format_MonoLSB);
    const QRgb c0 = QColor(Qt::black).rgb();
    const QRgb c1 = QColor(Qt::white).rgb();
    if (img.color(0) == c0 && img.color(1) == c1) {
        img.invertPixels();
        img.setColor(0, c1);
        img.setColor(1, c0);
    }

    char  *bits;
    uchar *tmp_bits;
    int w = img.width();
    int h = img.height();
    int bpl = (w + 7) / 8;
    int ibpl = img.bytesPerLine();
    if (bpl != ibpl) {
        tmp_bits = new uchar[bpl*h];
        bits = (char *)tmp_bits;
        uchar *p, *b;
        int y;
        b = tmp_bits;
        p = img.scanLine(0);
        for (y = 0; y < h; y++) {
            memcpy(b, p, bpl);
            b += bpl;
            p += ibpl;
        }
    } else {
        bits = (char *)img.bits();
        tmp_bits = 0;
    }
    XID hd = XCreateBitmapFromData(QXcbX11Info::display(),
                                   QXcbX11Info::appRootWindow(),
                                   bits, w, h);
    if (tmp_bits)                                // Avoid purify complaint
        delete [] tmp_bits;
    return hd;
}

bool QX11PlatformPixmap::isBackingStore() const
{
    return (flags & IsBackingStore);
}

void QX11PlatformPixmap::setIsBackingStore(bool on)
{
    if (on)
        flags |= IsBackingStore;
    else {
        flags &= ~IsBackingStore;
    }
}

#if QT_CONFIG(xrender)
void QX11PlatformPixmap::convertToARGB32(bool preserveContents)
{
    if (!X11->use_xrender)
        return;

    // Q_ASSERT(count == 1);
    if ((flags & Readonly)/* && share_mode == QPixmap::ExplicitlyShared*/)
        return;

    Pixmap pm = XCreatePixmap(xinfo.display(), RootWindow(xinfo.display(), xinfo.screen()),
                              w, h, 32);
    Picture p = XRenderCreatePicture(xinfo.display(), pm,
                                     XRenderFindStandardFormat(xinfo.display(), PictStandardARGB32), 0, 0);
    if (picture) {
        if (preserveContents)
            XRenderComposite(xinfo.display(), PictOpSrc, picture, 0, p, 0, 0, 0, 0, 0, 0, w, h);
        if (!(flags & Readonly))
            XRenderFreePicture(xinfo.display(), picture);
    }
    if (hd && !(flags & Readonly))
        XFreePixmap(xinfo.display(), hd);
    if (x11_mask) {
        XFreePixmap(xinfo.display(), x11_mask);
        if (mask_picture)
            XRenderFreePicture(xinfo.display(), mask_picture);
        x11_mask = 0;
        mask_picture = 0;
    }
    hd = pm;
    picture = p;

    d = 32;
    xinfo.setDepth(32);

    XVisualInfo visinfo;
    if (XMatchVisualInfo(xinfo.display(), xinfo.screen(), 32, TrueColor, &visinfo))
        xinfo.setVisual(visinfo.visual);
}
#endif

void QX11PlatformPixmap::release()
{
    delete pengine;
    pengine = 0;

    if (/*!X11*/ QCoreApplication::closingDown()) {
        // At this point, the X server will already have freed our resources,
        // so there is nothing to do.
        return;
    }

    if (x11_mask) {
#if QT_CONFIG(xrender)
        if (mask_picture)
            XRenderFreePicture(xinfo.display(), mask_picture);
        mask_picture = 0;
#endif
        XFreePixmap(xinfo.display(), x11_mask);
        x11_mask = 0;
    }

    if (hd) {
#if QT_CONFIG(xrender)
        if (picture) {
            XRenderFreePicture(xinfo.display(), picture);
            picture = 0;
        }
#endif // QT_CONFIG(xrender)

        if (hd2) {
            XFreePixmap(xinfo.display(), hd2);
            hd2 = 0;
        }
        if (!(flags & Readonly))
            XFreePixmap(xinfo.display(), hd);
        hd = 0;
    }
}

QImage QX11PlatformPixmap::toImage(const QXImageWrapper &xiWrapper, const QRect &rect) const
{
    XImage *xi = xiWrapper.xi;

    int d = depth();
    Visual *visual = (Visual *)xinfo.visual();
    bool trucol = (visual->c_class >= TrueColor) && d > 1;

    QImage::Format format = QImage::Format_Mono;
    if (d > 1 && d <= 8) {
        d = 8;
        format = QImage::Format_Indexed8;
    }
    // we could run into the situation where d == 8 AND trucol is true, which can
    // cause problems when converting to and from images.  in this case, always treat
    // the depth as 32...
    if (d > 8 || trucol) {
        d = 32;
        format = QImage::Format_RGB32;
    }

    if (d == 1 && xi->bitmap_bit_order == LSBFirst)
        format = QImage::Format_MonoLSB;
    if (x11_mask && format == QImage::Format_RGB32)
        format = QImage::Format_ARGB32;

    QImage image(xi->width, xi->height, format);
    image.setDevicePixelRatio(devicePixelRatio());
    if (image.isNull())                        // could not create image
        return image;

    QImage alpha;
    if (x11_mask) {
        if (rect.contains(QRect(0, 0, w, h)))
            alpha = mask().toImage();
        else
            alpha = mask().toImage().copy(rect);
    }
    bool ale = alpha.format() == QImage::Format_MonoLSB;

    if (trucol) {                                // truecolor
        const uint red_mask = (uint)visual->red_mask;
        const uint green_mask = (uint)visual->green_mask;
        const uint blue_mask = (uint)visual->blue_mask;
        const int  red_shift = highest_bit(red_mask) - 7;
        const int  green_shift = highest_bit(green_mask) - 7;
        const int  blue_shift = highest_bit(blue_mask) - 7;

        const uint red_bits = n_bits(red_mask);
        const uint green_bits = n_bits(green_mask);
        const uint blue_bits = n_bits(blue_mask);

        static uint red_table_bits = 0;
        static uint green_table_bits = 0;
        static uint blue_table_bits = 0;

        if (red_bits < 8 && red_table_bits != red_bits) {
            build_scale_table(&red_scale_table, red_bits);
            red_table_bits = red_bits;
        }
        if (blue_bits < 8 && blue_table_bits != blue_bits) {
            build_scale_table(&blue_scale_table, blue_bits);
            blue_table_bits = blue_bits;
        }
        if (green_bits < 8 && green_table_bits != green_bits) {
            build_scale_table(&green_scale_table, green_bits);
            green_table_bits = green_bits;
        }

        int  r, g, b;

        QRgb *dst;
        uchar *src;
        uint pixel;
        int bppc = xi->bits_per_pixel;

        if (bppc > 8 && xi->byte_order == LSBFirst)
            bppc++;

        for (int y = 0; y < xi->height; ++y) {
            uchar* asrc = x11_mask ? alpha.scanLine(y) : 0;
            dst = (QRgb *)image.scanLine(y);
            src = (uchar *)xi->data + xi->bytes_per_line*y;
            for (int x = 0; x < xi->width; x++) {
                switch (bppc) {
                case 8:
                    pixel = *src++;
                    break;
                case 16:                        // 16 bit MSB
                    pixel = src[1] | (uint)src[0] << 8;
                    src += 2;
                    break;
                case 17:                        // 16 bit LSB
                    pixel = src[0] | (uint)src[1] << 8;
                    src += 2;
                    break;
                case 24:                        // 24 bit MSB
                    pixel = src[2] | (uint)src[1] << 8 | (uint)src[0] << 16;
                    src += 3;
                    break;
                case 25:                        // 24 bit LSB
                    pixel = src[0] | (uint)src[1] << 8 | (uint)src[2] << 16;
                    src += 3;
                    break;
                case 32:                        // 32 bit MSB
                    pixel = src[3] | (uint)src[2] << 8 | (uint)src[1] << 16 | (uint)src[0] << 24;
                    src += 4;
                    break;
                case 33:                        // 32 bit LSB
                    pixel = src[0] | (uint)src[1] << 8 | (uint)src[2] << 16 | (uint)src[3] << 24;
                    src += 4;
                    break;
                default:                        // should not really happen
                    x = xi->width;                        // leave loop
                    y = xi->height;
                    pixel = 0;                // eliminate compiler warning
                    qWarning("QPixmap::convertToImage: Invalid depth %d", bppc);
                }
                if (red_shift > 0)
                    r = (pixel & red_mask) >> red_shift;
                else
                    r = (pixel & red_mask) << -red_shift;
                if (green_shift > 0)
                    g = (pixel & green_mask) >> green_shift;
                else
                    g = (pixel & green_mask) << -green_shift;
                if (blue_shift > 0)
                    b = (pixel & blue_mask) >> blue_shift;
                else
                    b = (pixel & blue_mask) << -blue_shift;

                if (red_bits < 8)
                    r = red_scale_table[r];
                if (green_bits < 8)
                    g = green_scale_table[g];
                if (blue_bits < 8)
                    b = blue_scale_table[b];

                if (x11_mask) {
                    if (ale) {
                        *dst++ = (asrc[x >> 3] & (1 << (x & 7))) ? qRgba(r, g, b, 0xff) : 0;
                    } else {
                        *dst++ = (asrc[x >> 3] & (0x80 >> (x & 7))) ? qRgba(r, g, b, 0xff) : 0;
                    }
                } else {
                    *dst++ = qRgb(r, g, b);
                }
            }
        }
    } else if (xi->bits_per_pixel == d) {        // compatible depth
        char *xidata = xi->data;                // copy each scanline
        int bpl = qMin(int(image.bytesPerLine()),xi->bytes_per_line);
        for (int y=0; y<xi->height; y++) {
            memcpy(image.scanLine(y), xidata, bpl);
            xidata += xi->bytes_per_line;
        }
    } else {
        /* Typically 2 or 4 bits display depth */
        qWarning("QPixmap::convertToImage: Display not supported (bpp=%d)",
                 xi->bits_per_pixel);
        return QImage();
    }

    if (d == 1) {                                // bitmap
        image.setColorCount(2);
        image.setColor(0, qRgb(255,255,255));
        image.setColor(1, qRgb(0,0,0));
    } else if (!trucol) {                        // pixmap with colormap
        uchar *p;
        uchar *end;
        uchar  use[256];                        // pixel-in-use table
        uchar  pix[256];                        // pixel translation table
        int    ncols, bpl;
        memset(use, 0, 256);
        memset(pix, 0, 256);
        bpl = image.bytesPerLine();

        if (x11_mask) {                         // which pixels are used?
            for (int i = 0; i < xi->height; i++) {
                uchar* asrc = alpha.scanLine(i);
                p = image.scanLine(i);
                if (ale) {
                    for (int x = 0; x < xi->width; x++) {
                        if (asrc[x >> 3] & (1 << (x & 7)))
                            use[*p] = 1;
                        ++p;
                    }
                } else {
                    for (int x = 0; x < xi->width; x++) {
                        if (asrc[x >> 3] & (0x80 >> (x & 7)))
                            use[*p] = 1;
                        ++p;
                    }
                }
            }
        } else {
            for (int i = 0; i < xi->height; i++) {
                p = image.scanLine(i);
                end = p + bpl;
                while (p < end)
                    use[*p++] = 1;
            }
        }
        ncols = 0;
        for (int i = 0; i < 256; i++) {                // build translation table
            if (use[i])
                pix[i] = ncols++;
        }
        for (int i = 0; i < xi->height; i++) {                        // translate pixels
            p = image.scanLine(i);
            end = p + bpl;
            while (p < end) {
                *p = pix[*p];
                p++;
            }
        }
        if (x11_mask) {
            int trans;
            if (ncols < 256) {
                trans = ncols++;
                image.setColorCount(ncols);        // create color table
                image.setColor(trans, 0x00000000);
            } else {
                image.setColorCount(ncols);        // create color table
                // oh dear... no spare "transparent" pixel.
                // use first pixel in image (as good as any).
                trans = image.scanLine(0)[0];
            }
            for (int i = 0; i < xi->height; i++) {
                uchar* asrc = alpha.scanLine(i);
                p = image.scanLine(i);
                if (ale) {
                    for (int x = 0; x < xi->width; x++) {
                        if (!(asrc[x >> 3] & (1 << (x & 7))))
                            *p = trans;
                        ++p;
                    }
                } else {
                    for (int x = 0; x < xi->width; x++) {
                        if (!(asrc[x >> 3] & (1 << (7 -(x & 7)))))
                            *p = trans;
                        ++p;
                    }
                }
            }
        } else {
            image.setColorCount(ncols);        // create color table
        }
        QVector<QColor> colors = QXcbColormap::instance(xinfo.screen()).colormap();
        int j = 0;
        for (int i=0; i<colors.size(); i++) {                // translate pixels
            if (use[i])
                image.setColor(j++, 0xff000000 | colors.at(i).rgb());
        }
    }

    return image;
}

QT_END_NAMESPACE
