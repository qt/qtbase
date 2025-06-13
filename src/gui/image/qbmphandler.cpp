// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "private/qbmphandler_p.h"

#ifndef QT_NO_IMAGEFORMAT_BMP

#include <qimage.h>
#include <qlist.h>
#include <qvariant.h>

QT_BEGIN_NAMESPACE

static void swapPixel01(QImage *image)        // 1-bpp: swap 0 and 1 pixels
{
    qsizetype i;
    if (image->depth() == 1 && image->colorCount() == 2) {
        uint *p = (uint *)image->bits();
        qsizetype nbytes = static_cast<qsizetype>(image->sizeInBytes());
        for (i=0; i<nbytes/4; i++) {
            *p = ~*p;
            p++;
        }
        uchar *p2 = (uchar *)p;
        for (i=0; i<(nbytes&3); i++) {
            *p2 = ~*p2;
            p2++;
        }
        QRgb t = image->color(0);                // swap color 0 and 1
        image->setColor(0, image->color(1));
        image->setColor(1, t);
    }
}

/*
    QImageIO::defineIOHandler("BMP", "^BM", 0,
                               read_bmp_image, write_bmp_image);
*/

/*****************************************************************************
  BMP (DIB) image read/write functions
 *****************************************************************************/

const int BMP_FILEHDR_SIZE = 14;                // size of BMP_FILEHDR data

static QDataStream &operator>>(QDataStream &s, BMP_FILEHDR &bf)
{                                                // read file header
    s.readRawData(bf.bfType, 2);
    s >> bf.bfSize >> bf.bfReserved1 >> bf.bfReserved2 >> bf.bfOffBits;
    return s;
}

static QDataStream &operator<<(QDataStream &s, const BMP_FILEHDR &bf)
{                                                // write file header
    s.writeRawData(bf.bfType, 2);
    s << bf.bfSize << bf.bfReserved1 << bf.bfReserved2 << bf.bfOffBits;
    return s;
}


const int BMP_OLD  = 12;                        // old Windows/OS2 BMP size
const int BMP_WIN  = 40;                        // Windows BMP v3 size
const int BMP_OS2  = 64;                        // new OS/2 BMP size
const int BMP_WIN4 = 108;                       // Windows BMP v4 size
const int BMP_WIN5 = 124;                       // Windows BMP v5 size

const int BMP_RGB  = 0;                                // no compression
const int BMP_RLE8 = 1;                                // run-length encoded, 8 bits
const int BMP_RLE4 = 2;                                // run-length encoded, 4 bits
const int BMP_BITFIELDS = 3;                        // RGB values encoded in data as bit-fields
const int BMP_ALPHABITFIELDS = 4;                   // RGBA values encoded in data as bit-fields


static QDataStream &operator>>(QDataStream &s, BMP_INFOHDR &bi)
{
    s >> bi.biSize;
    if (bi.biSize == BMP_WIN || bi.biSize == BMP_OS2 || bi.biSize == BMP_WIN4 || bi.biSize == BMP_WIN5) {
        s >> bi.biWidth >> bi.biHeight >> bi.biPlanes >> bi.biBitCount;
        s >> bi.biCompression >> bi.biSizeImage;
        s >> bi.biXPelsPerMeter >> bi.biYPelsPerMeter;
        s >> bi.biClrUsed >> bi.biClrImportant;
        if (bi.biSize >= BMP_WIN4) {
            s >> bi.biRedMask >> bi.biGreenMask >> bi.biBlueMask >> bi.biAlphaMask;
            s >> bi.biCSType;
            for (int i = 0; i < 9; ++i)
                s >> bi.biEndpoints[i];
            s >> bi.biGammaRed >> bi.biGammaGreen >> bi.biGammaBlue;
            if (bi.biSize == BMP_WIN5)
                s >> bi.biIntent >> bi.biProfileData >> bi.biProfileSize >> bi.biReserved;
        }
    }
    else {                                        // probably old Windows format
        qint16 w, h;
        s >> w >> h >> bi.biPlanes >> bi.biBitCount;
        bi.biWidth  = w;
        bi.biHeight = h;
        bi.biCompression = BMP_RGB;                // no compression
        bi.biSizeImage = 0;
        bi.biXPelsPerMeter = bi.biYPelsPerMeter = 0;
        bi.biClrUsed = bi.biClrImportant = 0;
    }
    return s;
}

static QDataStream &operator<<(QDataStream &s, const BMP_INFOHDR &bi)
{
    s << bi.biSize;
    s << bi.biWidth << bi.biHeight;
    s << bi.biPlanes;
    s << bi.biBitCount;
    s << bi.biCompression;
    s << bi.biSizeImage;
    s << bi.biXPelsPerMeter << bi.biYPelsPerMeter;
    s << bi.biClrUsed << bi.biClrImportant;

    if (bi.biSize >= BMP_WIN4) {
        s << bi.biRedMask << bi.biGreenMask << bi.biBlueMask << bi.biAlphaMask;
        s << bi.biCSType;

        for (int i = 0; i < 9; i++)
            s << bi.biEndpoints[i];

        s << bi.biGammaRed;
        s << bi.biGammaGreen;
        s << bi.biGammaBlue;
    }

    if (bi.biSize >= BMP_WIN5) {
        s << bi.biIntent;
        s << bi.biProfileData;
        s << bi.biProfileSize;
        s << bi.biReserved;
    }

    return s;
}

static uint calc_shift(uint mask)
{
    uint result = 0;
    while ((mask >= 0x100) || (!(mask & 1) && mask)) {
        result++;
        mask >>= 1;
    }
    return result;
}

static uint calc_scale(uint low_mask)
{
    uint result = 8;
    while (low_mask && result) {
        result--;
        low_mask >>= 1;
    }
    return result;
}

static inline uint apply_scale(uint value, uint scale)
{
    if (!(scale & 0x07)) // return immediately if scale == 8 or 0
        return value;

    uint filled = 8 - scale;
    uint result = value << scale;

    do {
        result |= result >> filled;
        filled <<= 1;
    } while (filled < 8);

    return result;
}

static bool read_dib_fileheader(QDataStream &s, BMP_FILEHDR &bf)
{
    // read BMP file header
    if (!(s >> bf))
        return false;

    // check header
    if (qstrncmp(bf.bfType,"BM",2) != 0)
        return false;

    return true;
}

static bool read_dib_infoheader(QDataStream &s, BMP_INFOHDR &bi)
{
    if (!(s >> bi))                                       // read BMP info header
        return false;

    int nbits = bi.biBitCount;
    int comp = bi.biCompression;
    if (!(nbits == 1 || nbits == 4 || nbits == 8 || nbits == 16 || nbits == 24 || nbits == 32) ||
        bi.biPlanes != 1 || comp > BMP_BITFIELDS)
        return false;                                        // weird BMP image
    if (!(comp == BMP_RGB || (nbits == 4 && comp == BMP_RLE4) ||
        (nbits == 8 && comp == BMP_RLE8) || ((nbits == 16 || nbits == 32) && comp == BMP_BITFIELDS)))
         return false;                                // weird compression type
    if (bi.biHeight == INT_MIN)
        return false; // out of range for positive int
    if (bi.biWidth <= 0 || !bi.biHeight || quint64(bi.biWidth) * qAbs(bi.biHeight) > 16384 * 16384)
        return false;

    return true;
}

static bool read_dib_body(QDataStream &s, const BMP_INFOHDR &bi, qint64 datapos, qint64 startpos, QImage &image)
{
    QIODevice* d = s.device();
    if (d->atEnd())                                // end of stream/file
        return false;
#if 0
    qDebug("offset...........%lld", datapos);
    qDebug("startpos.........%lld", startpos);
    qDebug("biSize...........%d", bi.biSize);
    qDebug("biWidth..........%d", bi.biWidth);
    qDebug("biHeight.........%d", bi.biHeight);
    qDebug("biPlanes.........%d", bi.biPlanes);
    qDebug("biBitCount.......%d", bi.biBitCount);
    qDebug("biCompression....%d", bi.biCompression);
    qDebug("biSizeImage......%d", bi.biSizeImage);
    qDebug("biXPelsPerMeter..%d", bi.biXPelsPerMeter);
    qDebug("biYPelsPerMeter..%d", bi.biYPelsPerMeter);
    qDebug("biClrUsed........%d", bi.biClrUsed);
    qDebug("biClrImportant...%d", bi.biClrImportant);
#endif
    int w = bi.biWidth,         h = bi.biHeight,  nbits = bi.biBitCount;
    int t = bi.biSize,         comp = bi.biCompression;
    uint red_mask = 0;
    uint green_mask = 0;
    uint blue_mask = 0;
    uint alpha_mask = 0;
    uint red_shift = 0;
    uint green_shift = 0;
    uint blue_shift = 0;
    uint alpha_shift = 0;
    uint red_scale = 0;
    uint green_scale = 0;
    uint blue_scale = 0;
    uint alpha_scale = 0;
    bool bitfields = comp == BMP_BITFIELDS || comp == BMP_ALPHABITFIELDS;

    if (!d->isSequential())
        d->seek(startpos + bi.biSize); // goto start of colormap or masks

    if (bi.biSize >= BMP_WIN4) {
        red_mask = bi.biRedMask;
        green_mask = bi.biGreenMask;
        blue_mask = bi.biBlueMask;
        alpha_mask = bi.biAlphaMask;
    } else if (bitfields && (nbits == 16 || nbits == 32)) {
        if (d->read((char *)&red_mask, sizeof(red_mask)) != sizeof(red_mask))
            return false;
        if (d->read((char *)&green_mask, sizeof(green_mask)) != sizeof(green_mask))
            return false;
        if (d->read((char *)&blue_mask, sizeof(blue_mask)) != sizeof(blue_mask))
            return false;
        if (comp == BMP_ALPHABITFIELDS && d->read((char *)&alpha_mask, sizeof(alpha_mask)) != sizeof(alpha_mask))
            return false;
    }

    bool transp = bitfields || (comp == BMP_RGB && nbits == 32 && alpha_mask == 0xff000000);
    transp = transp && alpha_mask;

    int ncols = 0;
    int depth = 0;
    QImage::Format format;
    switch (nbits) {
        case 32:
        case 24:
        case 16:
            depth = 32;
            format = transp ? QImage::Format_ARGB32 : QImage::Format_RGB32;
            break;
        case 8:
        case 4:
            depth = 8;
            format = QImage::Format_Indexed8;
            break;
        case 1:
            depth = 1;
            format = QImage::Format_Mono;
            break;
        default:
            return false;
            break;
    }

    if (depth != 32) {
        ncols = bi.biClrUsed ? bi.biClrUsed : 1 << nbits;
        if (ncols < 1 || ncols > 256) // sanity check - don't run out of mem if color table is broken
            return false;
    }

    if (bi.biHeight < 0)
        h = -h;                  // support images with negative height

    if (!QImageIOHandler::allocateImage(QSize(w, h), format, &image))
        return false;
    if (ncols > 0) {                                // read color table
        image.setColorCount(ncols);
        uchar rgb[4];
        int   rgb_len = t == BMP_OLD ? 3 : 4;
        for (int i=0; i<ncols; i++) {
            if (d->read((char *)rgb, rgb_len) != rgb_len)
                return false;
            image.setColor(i, qRgb(rgb[2],rgb[1],rgb[0]));
            if (d->atEnd())                        // truncated file
                return false;
        }
    } else if (bitfields && (nbits == 16 || nbits == 32)) {
        red_shift = calc_shift(red_mask);
        if (((red_mask >> red_shift) + 1) == 0)
            return false;
        red_scale = calc_scale(red_mask >> red_shift);
        green_shift = calc_shift(green_mask);
        if (((green_mask >> green_shift) + 1) == 0)
            return false;
        green_scale = calc_scale(green_mask >> green_shift);
        blue_shift = calc_shift(blue_mask);
        if (((blue_mask >> blue_shift) + 1) == 0)
            return false;
        blue_scale = calc_scale(blue_mask >> blue_shift);
        alpha_shift = calc_shift(alpha_mask);
        if (((alpha_mask >> alpha_shift) + 1) == 0)
            return false;
        alpha_scale = calc_scale(alpha_mask >> alpha_shift);
    } else if (comp == BMP_RGB && (nbits == 24 || nbits == 32)) {
        blue_mask = 0x000000ff;
        green_mask = 0x0000ff00;
        red_mask = 0x00ff0000;
        blue_shift = 0;
        green_shift = 8;
        red_shift = 16;
        blue_scale = green_scale = red_scale = 0;
        if (transp) {
            alpha_shift = calc_shift(alpha_mask);
            if (((alpha_mask >> alpha_shift) + 1) == 0)
                return false;
            alpha_scale = calc_scale(alpha_mask >> alpha_shift);
        }
    } else if (comp == BMP_RGB && nbits == 16) {
        blue_mask = 0x001f;
        green_mask = 0x03e0;
        red_mask = 0x7c00;
        blue_shift = 0;
        green_shift = 5;
        red_shift = 10;
        blue_scale = green_scale = red_scale = 3;
    }

    image.setDotsPerMeterX(bi.biXPelsPerMeter);
    image.setDotsPerMeterY(bi.biYPelsPerMeter);

#if 0
    qDebug("Rmask: %08x Rshift: %08x Rscale:%08x", red_mask, red_shift, red_scale);
    qDebug("Gmask: %08x Gshift: %08x Gscale:%08x", green_mask, green_shift, green_scale);
    qDebug("Bmask: %08x Bshift: %08x Bscale:%08x", blue_mask, blue_shift, blue_scale);
    qDebug("Amask: %08x Ashift: %08x Ascale:%08x", alpha_mask, alpha_shift, alpha_scale);
#endif

    if (datapos >= 0 && datapos > d->pos()) {
        if (!d->isSequential())
            d->seek(datapos); // start of image data
    }

    int             bpl = image.bytesPerLine();
    uchar *data = image.bits();

    if (nbits == 1) {                                // 1 bit BMP image
        while (--h >= 0) {
            if (d->read((char*)(data + h*bpl), bpl) != bpl)
                break;
        }
        if (ncols == 2 && qGray(image.color(0)) < qGray(image.color(1)))
            swapPixel01(&image);                // pixel 0 is white!
    }

    else if (nbits == 4) {                        // 4 bit BMP image
        int    buflen = ((w+7)/8)*4;
        uchar *buf    = new uchar[buflen];
        if (comp == BMP_RLE4) {                // run length compression
            int x=0, y=0, c, i;
            quint8 b;
            uchar *p = data + (h-1)*bpl;
            const uchar *endp = p + w;
            while (y < h) {
                if (!d->getChar((char *)&b))
                    break;
                if (b == 0) {                        // escape code
                    if (!d->getChar((char *)&b) || b == 1) {
                        y = h;                // exit loop
                    } else switch (b) {
                        case 0:                        // end of line
                            x = 0;
                            y++;
                            p = data + (h-y-1)*bpl;
                            break;
                        case 2:                        // delta (jump)
                        {
                            quint8 tmp;
                            d->getChar((char *)&tmp);
                            x += tmp;
                            d->getChar((char *)&tmp);
                            y += tmp;
                        }

                            // Protection
                            if ((uint)x >= (uint)w)
                                x = w-1;
                            if ((uint)y >= (uint)h)
                                y = h-1;

                            p = data + (h-y-1)*bpl + x;
                            break;
                        default:                // absolute mode
                            // Protection
                            if (p + b > endp)
                                b = endp-p;

                            i = (c = b)/2;
                            while (i--) {
                                d->getChar((char *)&b);
                                *p++ = b >> 4;
                                *p++ = b & 0x0f;
                            }
                            if (c & 1) {
                                unsigned char tmp;
                                d->getChar((char *)&tmp);
                                *p++ = tmp >> 4;
                            }
                            if ((((c & 3) + 1) & 2) == 2)
                                d->getChar(nullptr);        // align on word boundary
                            x += c;
                    }
                } else {                        // encoded mode
                    // Protection
                    if (p + b > endp)
                        b = endp-p;

                    i = (c = b)/2;
                    d->getChar((char *)&b);                // 2 pixels to be repeated
                    while (i--) {
                        *p++ = b >> 4;
                        *p++ = b & 0x0f;
                    }
                    if (c & 1)
                        *p++ = b >> 4;
                    x += c;
                }
            }
        } else if (comp == BMP_RGB) {                // no compression
            memset(data, 0, h*bpl);
            while (--h >= 0) {
                if (d->read((char*)buf,buflen) != buflen)
                    break;
                uchar *p = data + h*bpl;
                uchar *b = buf;
                for (int i=0; i<w/2; i++) {        // convert nibbles to bytes
                    *p++ = *b >> 4;
                    *p++ = *b++ & 0x0f;
                }
                if (w & 1)                        // the last nibble
                    *p = *b >> 4;
            }
        }
        delete [] buf;
    }

    else if (nbits == 8) {                        // 8 bit BMP image
        if (comp == BMP_RLE8) {                // run length compression
            int x=0, y=0;
            quint8 b;
            uchar *p = data + (h-1)*bpl;
            const uchar *endp = p + w;
            while (y < h) {
                if (!d->getChar((char *)&b))
                    break;
                if (b == 0) {                        // escape code
                    if (!d->getChar((char *)&b) || b == 1) {
                            y = h;                // exit loop
                    } else switch (b) {
                        case 0:                        // end of line
                            x = 0;
                            y++;
                            p = data + (h-y-1)*bpl;
                            break;
                        case 2:                        // delta (jump)
                            {
                                quint8 tmp;
                                d->getChar((char *)&tmp);
                                x += tmp;
                                d->getChar((char *)&tmp);
                                y += tmp;
                            }

                            // Protection
                            if ((uint)x >= (uint)w)
                                x = w-1;
                            if ((uint)y >= (uint)h)
                                y = h-1;

                            p = data + (h-y-1)*bpl + x;
                            break;
                        default:                // absolute mode
                            // Protection
                            if (p + b > endp)
                                b = endp-p;

                            if (d->read((char *)p, b) != b)
                                return false;
                            if ((b & 1) == 1)
                                d->getChar(nullptr);        // align on word boundary
                            x += b;
                            p += b;
                    }
                } else {                        // encoded mode
                    // Protection
                    if (p + b > endp)
                        b = endp-p;

                    char tmp;
                    d->getChar(&tmp);
                    memset(p, tmp, b); // repeat pixel
                    x += b;
                    p += b;
                }
            }
        } else if (comp == BMP_RGB) {                // uncompressed
            while (--h >= 0) {
                if (d->read((char *)data + h*bpl, bpl) != bpl)
                    break;
            }
        }
    }

    else if (nbits == 16 || nbits == 24 || nbits == 32) { // 16,24,32 bit BMP image
        QRgb *p;
        QRgb  *end;
        uchar *buf24 = new uchar[bpl];
        int    bpl24 = ((w*nbits+31)/32)*4;
        uchar *b;
        int c;

        while (--h >= 0) {
            p = (QRgb *)(data + h*bpl);
            end = p + w;
            if (d->read((char *)buf24,bpl24) != bpl24)
                break;
            b = buf24;
            while (p < end) {
                c = *(uchar*)b | (*(uchar*)(b+1)<<8);
                if (nbits > 16)
                    c |= *(uchar*)(b+2)<<16;
                if (nbits > 24)
                    c |= *(uchar*)(b+3)<<24;
                *p++ = qRgba(apply_scale((c & red_mask) >> red_shift, red_scale),
                             apply_scale((c & green_mask) >> green_shift, green_scale),
                             apply_scale((c & blue_mask) >> blue_shift, blue_scale),
                             transp ? apply_scale((c & alpha_mask) >> alpha_shift, alpha_scale) : 0xff);
                b += nbits/8;
            }
        }
        delete[] buf24;
    }

    if (bi.biHeight < 0) {
        // Flip the image
        uchar *buf = new uchar[bpl];
        h = -bi.biHeight;
        for (int y = 0; y < h/2; ++y) {
            memcpy(buf, data + y*bpl, bpl);
            memcpy(data + y*bpl, data + (h-y-1)*bpl, bpl);
            memcpy(data + (h-y-1)*bpl, buf, bpl);
        }
        delete [] buf;
    }

    return true;
}

bool qt_write_dib(QDataStream &s, const QImage &image, int bpl, int bpl_bmp, int nbits)
{
    QIODevice* d = s.device();
    if (!d->isWritable())
        return false;

    BMP_INFOHDR bi;
    bi.biSize               = BMP_WIN;                // build info header
    bi.biWidth               = image.width();
    bi.biHeight               = image.height();
    bi.biPlanes               = 1;
    bi.biBitCount      = nbits;
    bi.biCompression   = BMP_RGB;
    bi.biSizeImage     = bpl_bmp*image.height();
    bi.biXPelsPerMeter = image.dotsPerMeterX() ? image.dotsPerMeterX()
                                                : 2834; // 72 dpi default
    bi.biYPelsPerMeter = image.dotsPerMeterY() ? image.dotsPerMeterY() : 2834;
    bi.biClrUsed       = image.colorCount();
    bi.biClrImportant  = image.colorCount();
    if (!(s << bi))                                        // write info header
        return false;

    if (image.depth() != 32) {                // write color table
        uchar *color_table = new uchar[4*image.colorCount()];
        uchar *rgb = color_table;
        const QList<QRgb> c = image.colorTable();
        for (int i = 0; i < image.colorCount(); i++) {
            *rgb++ = qBlue (c[i]);
            *rgb++ = qGreen(c[i]);
            *rgb++ = qRed  (c[i]);
            *rgb++ = 0;
        }
        if (d->write((char *)color_table, 4*image.colorCount()) == -1) {
            delete [] color_table;
            return false;
        }
        delete [] color_table;
    }

    int y;

    if (nbits == 1 || nbits == 8) {                // direct output
        for (y=image.height()-1; y>=0; y--) {
            if (d->write((const char*)image.constScanLine(y), bpl) == -1)
                return false;
        }
        return true;
    }

    uchar *buf        = new uchar[bpl_bmp];
    uchar *b, *end;
    const uchar *p;

    memset(buf, 0, bpl_bmp);
    for (y=image.height()-1; y>=0; y--) {        // write the image bits
        if (nbits == 4) {                        // convert 8 -> 4 bits
            p = image.constScanLine(y);
            b = buf;
            end = b + image.width()/2;
            while (b < end) {
                *b++ = (*p << 4) | (*(p+1) & 0x0f);
                p += 2;
            }
            if (image.width() & 1)
                *b = *p << 4;
        } else {                                // 32 bits
            const QRgb *p   = (const QRgb *)image.constScanLine(y);
            const QRgb *end = p + image.width();
            b = buf;
            while (p < end) {
                *b++ = qBlue(*p);
                *b++ = qGreen(*p);
                *b++ = qRed(*p);
                p++;
            }
        }
        if (bpl_bmp != d->write((char*)buf, bpl_bmp)) {
            delete[] buf;
            return false;
        }
    }
    delete[] buf;
    return true;
}

QBmpHandler::QBmpHandler(InternalFormat fmt) :
    m_format(fmt), state(Ready)
{
}

QByteArray QBmpHandler::formatName() const
{
    return m_format == BmpFormat ? "bmp" : "dib";
}

bool QBmpHandler::readHeader()
{
    state = Error;

    QIODevice *d = device();
    QDataStream s(d);
    startpos = d->pos();

    // Intel byte order
    s.setByteOrder(QDataStream::LittleEndian);

    // read BMP file header
    if (m_format == BmpFormat && !read_dib_fileheader(s, fileHeader))
        return false;

    // read BMP info header
    if (!read_dib_infoheader(s, infoHeader))
        return false;

    state = ReadHeader;
    return true;
}

bool QBmpHandler::canRead() const
{
    if (m_format == BmpFormat && state == Ready && !canRead(device()))
        return false;

    if (state != Error) {
        setFormat(formatName());
        return true;
    }

    return false;
}

bool QBmpHandler::canRead(QIODevice *device)
{
    if (!device) {
        qWarning("QBmpHandler::canRead() called with 0 pointer");
        return false;
    }

    char head[2];
    if (device->peek(head, sizeof(head)) != sizeof(head))
        return false;

    return (qstrncmp(head, "BM", 2) == 0);
}

bool QBmpHandler::read(QImage *image)
{
    if (state == Error)
        return false;

    if (!image) {
        qWarning("QBmpHandler::read: cannot read into null pointer");
        return false;
    }

    if (state == Ready && !readHeader()) {
        state = Error;
        return false;
    }

    QIODevice *d = device();
    QDataStream s(d);

    // Intel byte order
    s.setByteOrder(QDataStream::LittleEndian);

    // read image
    qint64 datapos = startpos;
    if (m_format == BmpFormat) {
        datapos += fileHeader.bfOffBits;
    } else {
        // QTBUG-100351: We have no file header when reading dib format so we have to depend on the size of the
        // buffer and the biSizeImage value to find where the pixel data starts since there's sometimes optional
        // color mask values after biSize, like for example when pasting from the windows snipping tool.
        if (infoHeader.biSizeImage > 0 && infoHeader.biSizeImage < d->size()) {
            datapos = d->size() - infoHeader.biSizeImage;
        } else {
            // And sometimes biSizeImage is not filled in like when pasting from Microsoft Edge, so then we just
            // have to assume the optional color mask values are there.
            datapos += infoHeader.biSize;

            if (infoHeader.biBitCount == 16 || infoHeader.biBitCount == 32) {
                if (infoHeader.biCompression == BMP_BITFIELDS) {
                    datapos += 12;
                } else if (infoHeader.biCompression == BMP_ALPHABITFIELDS) {
                    datapos += 16;
                }
            }
        }
    }
    const bool readSuccess = m_format == BmpFormat ?
        read_dib_body(s, infoHeader, datapos, startpos + BMP_FILEHDR_SIZE, *image) :
        read_dib_body(s, infoHeader, datapos, startpos, *image);
    if (!readSuccess)
        return false;

    state = Ready;
    return true;
}

bool QBmpHandler::write(const QImage &img)
{
    QImage image;
    switch (img.format()) {
    case QImage::Format_Mono:
    case QImage::Format_Indexed8:
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
        image = img;
        break;
    case QImage::Format_MonoLSB:
        image = img.convertToFormat(QImage::Format_Mono);
        break;
    case QImage::Format_Alpha8:
    case QImage::Format_Grayscale8:
        image = img.convertToFormat(QImage::Format_Indexed8);
        break;
    default:
        if (img.hasAlphaChannel())
            image = img.convertToFormat(QImage::Format_ARGB32);
        else
            image = img.convertToFormat(QImage::Format_RGB32);
        break;
    }

    int nbits;
    qsizetype bpl_bmp;
    // Calculate a minimum bytes-per-line instead of using whatever value this QImage is using internally.
    qsizetype bpl = ((image.width() * image.depth() + 31) >> 5) << 2;

    if (image.depth() == 8 && image.colorCount() <= 16) {
        bpl_bmp = (((bpl+1)/2+3)/4)*4;
        nbits = 4;
   } else if (image.depth() == 32) {
        bpl_bmp = ((image.width()*24+31)/32)*4;
        nbits = 24;
    } else {
        bpl_bmp = bpl;
        nbits = image.depth();
    }
    if (qsizetype(int(bpl_bmp)) != bpl_bmp)
        return false;

    if (m_format == DibFormat) {
        QDataStream dibStream(device());
        dibStream.setByteOrder(QDataStream::LittleEndian); // Intel byte order
        return qt_write_dib(dibStream, img, bpl, bpl_bmp, nbits);
    }

    QIODevice *d = device();
    QDataStream s(d);
    BMP_FILEHDR bf;

    // Intel byte order
    s.setByteOrder(QDataStream::LittleEndian);

    // build file header
    memcpy(bf.bfType, "BM", 2);

    // write file header
    bf.bfReserved1 = 0;
    bf.bfReserved2 = 0;
    bf.bfOffBits = BMP_FILEHDR_SIZE + BMP_WIN + image.colorCount() * 4;
    bf.bfSize = bf.bfOffBits + bpl_bmp*image.height();
    if (qsizetype(bf.bfSize) != bf.bfOffBits + bpl_bmp*image.height())
        return false;
    s << bf;

    // write image
    return qt_write_dib(s, image, bpl, bpl_bmp, nbits);
}

bool QBmpHandler::supportsOption(ImageOption option) const
{
    return option == Size
            || option == ImageFormat;
}

QVariant QBmpHandler::option(ImageOption option) const
{
    if (option == Size) {
        if (state == Error)
            return QVariant();
        if (state == Ready && !const_cast<QBmpHandler*>(this)->readHeader())
            return QVariant();
        return QSize(infoHeader.biWidth, infoHeader.biHeight);
    } else if (option == ImageFormat) {
        if (state == Error)
            return QVariant();
        if (state == Ready && !const_cast<QBmpHandler*>(this)->readHeader())
            return QVariant();
        QImage::Format format;
        switch (infoHeader.biBitCount) {
            case 32:
            case 24:
            case 16:
                if ((infoHeader.biCompression == BMP_BITFIELDS || infoHeader.biCompression == BMP_ALPHABITFIELDS) && infoHeader.biSize >= BMP_WIN4 && infoHeader.biAlphaMask)
                    format = QImage::Format_ARGB32;
                else
                    format = QImage::Format_RGB32;
                break;
            case 8:
            case 4:
                format = QImage::Format_Indexed8;
                break;
            default:
                format = QImage::Format_Mono;
            }
        return format;
    }
    return QVariant();
}

void QBmpHandler::setOption(ImageOption option, const QVariant &value)
{
    Q_UNUSED(option);
    Q_UNUSED(value);
}

QT_END_NAMESPACE

#endif // QT_NO_IMAGEFORMAT_BMP
