// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include <qplatformdefs.h>
#include "private/qxbmhandler_p.h"

#ifndef QT_NO_IMAGEFORMAT_XBM

#include <qimage.h>
#include <qiodevice.h>
#include <qloggingcategory.h>
#include <qvariant.h>
#include <private/qtools_p.h>
#include <private/qimage_p.h>

#include <cstdio>

#include <stdio.h>

QT_BEGIN_NAMESPACE

using namespace QtMiscUtils;

/*****************************************************************************
  X bitmap image read/write functions
 *****************************************************************************/

static inline int hex2byte(char *p)
{
    return QtMiscUtils::fromHex(p[0]) * 16 | QtMiscUtils::fromHex(p[1]);
}

static bool read_xbm_header(QIODevice *device, int& w, int& h)
{
    const int buflen = 300;
    const int maxlen = 4096;
    char buf[buflen + 1];

    qint64 readBytes = 0;
    qint64 totalReadBytes = 0;

    buf[0] = '\0';

    // skip initial comment, if any
    while (buf[0] != '#') {
        readBytes = device->readLine(buf, buflen);

        // if readBytes >= buflen, it's very probably not a C file
        if (readBytes <= 0 || readBytes >= buflen -1)
            return false;

        // limit xbm headers to the first 4k in the file to prevent
        // excessive reads on non-xbm files
        totalReadBytes += readBytes;
        if (totalReadBytes >= maxlen)
            return false;
    }

    auto parseDefine = [] (const char *buf, int len) -> int {
        auto checkChar = [] (char ch) -> bool {
            return isAsciiLetterOrNumber(ch)
                || ch == '_' || ch == '.';
        };
        auto isAsciiSpace = [] (char ch) -> bool {
            return ch == ' ' || ch == '\t';
        };
        const char define[] = "#define";
        constexpr size_t defineLen = sizeof(define) - 1;
        if (strncmp(buf, define, defineLen) != 0)
            return 0;
        int index = defineLen;
        while (buf[index] && isAsciiSpace(buf[index]))
            ++index;
        while (buf[index] && checkChar(buf[index]))
            ++index;
        while (buf[index] && isAsciiSpace(buf[index]))
            ++index;

        return QByteArray(buf + index, len - index).toInt();
    };


    // "#define .._width <num>"
    w = parseDefine(buf, readBytes - 1);

    readBytes = device->readLine(buf, buflen);
    // "#define .._height <num>"
    h = parseDefine(buf, readBytes - 1);

    // format error
    if (w <= 0 || w > 32767 || h <= 0 || h > 32767)
        return false;

    return true;
}

static bool read_xbm_body(QIODevice *device, int w, int h, QImage *outImage)
{
    const int buflen = 300;
    char buf[buflen + 1];

    qint64 readBytes = 0;

    char *p;

    // scan for database
    do {
        if ((readBytes = device->readLine(buf, buflen)) <= 0) {
            // end of file
            return false;
        }

        buf[readBytes] = '\0';
        p = strstr(buf, "0x");
    } while (!p);

    if (!QImageIOHandler::allocateImage(QSize(w, h), QImage::Format_MonoLSB, outImage))
        return false;

    outImage->fill(Qt::color0);       // in case the image data does not cover the full image

    outImage->setColorCount(2);
    outImage->setColor(0, qRgb(255,255,255));        // white
    outImage->setColor(1, qRgb(0,0,0));                // black

    int           x = 0, y = 0;
    uchar *b = outImage->scanLine(0);
    w = (w+7)/8;                                // byte width

    while (y < h) {                                // for all encoded bytes...
        if (p && p < (buf + readBytes - 3)) {      // p = "0x.."
            const int byte = hex2byte(p + 2);
            if (byte < 0) // non-hex char encountered
                return false;
            *b++ = byte;
            p += 2;
            if (++x == w && ++y < h) {
                b = outImage->scanLine(y);
                x = 0;
            }
            p = strstr(p, "0x");
        } else {                                // read another line
            if ((readBytes = device->readLine(buf,buflen)) <= 0)        // EOF ==> truncated image
                break;
            buf[readBytes] = '\0';
            p = strstr(buf, "0x");
        }
    }

    return true;
}

static bool read_xbm_image(QIODevice *device, QImage *outImage)
{
    int w = 0, h = 0;
    if (!read_xbm_header(device, w, h))
        return false;
    return read_xbm_body(device, w, h, outImage);
}

static bool write_xbm_image(const QImage &sourceImage, QIODevice *device, const QString &fileName)
{
    QImage image = sourceImage;
    int        w = image.width();
    int        h = image.height();
    int        i;
    const QByteArray s = fileName.toUtf8(); // get file base name
    const auto msize = s.size() + 100;
    char *buf = new char[msize];

    std::snprintf(buf, msize, "#define %s_width %d\n", s.data(), w);
    device->write(buf, qstrlen(buf));
    std::snprintf(buf, msize, "#define %s_height %d\n", s.data(), h);
    device->write(buf, qstrlen(buf));
    std::snprintf(buf, msize, "static char %s_bits[] = {\n ", s.data());
    device->write(buf, qstrlen(buf));

    if (image.format() != QImage::Format_MonoLSB)
        image = image.convertToFormat(QImage::Format_MonoLSB);

    bool invert = qGray(image.color(0)) < qGray(image.color(1));
    char hexrep[16];
    for (i=0; i<10; i++)
        hexrep[i] = '0' + i;
    for (i=10; i<16; i++)
        hexrep[i] = 'a' -10 + i;
    if (invert) {
        char t;
        for (i=0; i<8; i++) {
            t = hexrep[15-i];
            hexrep[15-i] = hexrep[i];
            hexrep[i] = t;
        }
    }
    int bcnt = 0;
    char *p = buf;
    int bpl = (w+7)/8;
    for (int y = 0; y < h; ++y) {
        const uchar *b = image.constScanLine(y);
        for (i = 0; i < bpl; ++i) {
            *p++ = '0'; *p++ = 'x';
            *p++ = hexrep[*b >> 4];
            *p++ = hexrep[*b++ & 0xf];

            if (i < bpl - 1 || y < h - 1) {
                *p++ = ',';
                if (++bcnt > 14) {
                    *p++ = '\n';
                    *p++ = ' ';
                    *p   = '\0';
                    if ((int)qstrlen(buf) != device->write(buf, qstrlen(buf))) {
                        delete [] buf;
                        return false;
                    }
                    p = buf;
                    bcnt = 0;
                }
            }
        }
    }
#ifdef Q_CC_MSVC
    strcpy_s(p, sizeof(" };\n"), " };\n");
#else
    strcpy(p, " };\n");
#endif
    if ((int)qstrlen(buf) != device->write(buf, qstrlen(buf))) {
        delete [] buf;
        return false;
    }

    delete [] buf;
    return true;
}

QXbmHandler::QXbmHandler()
    : state(Ready)
{
}

bool QXbmHandler::readHeader()
{
    state = Error;
    if (!read_xbm_header(device(), width, height))
        return false;
    state = ReadHeader;
    return true;
}

bool QXbmHandler::canRead() const
{
    if (state == Ready && !canRead(device()))
        return false;

    if (state != Error) {
        setFormat("xbm");
        return true;
    }

    return false;
}

bool QXbmHandler::canRead(QIODevice *device)
{
    if (!device) {
        qCWarning(lcImageIo, "QXbmHandler::canRead() called with no device");
        return false;
    }

    // it's impossible to tell whether we can load an XBM or not when
    // it's from a sequential device, as the only way to do it is to
    // attempt to parse the whole image.
    if (device->isSequential())
        return false;

    QImage image;
    qint64 oldPos = device->pos();
    bool success = read_xbm_image(device, &image);
    device->seek(oldPos);

    return success;
}

bool QXbmHandler::read(QImage *image)
{
    if (state == Error)
        return false;

    if (state == Ready && !readHeader()) {
        state = Error;
        return false;
    }

    if (!read_xbm_body(device(), width, height, image)) {
        state = Error;
        return false;
    }

    state = Ready;
    return true;
}

bool QXbmHandler::write(const QImage &image)
{
    return write_xbm_image(image, device(), fileName);
}

bool QXbmHandler::supportsOption(ImageOption option) const
{
    return option == Name
        || option == Size
        || option == ImageFormat;
}

QVariant QXbmHandler::option(ImageOption option) const
{
    if (option == Name) {
        return fileName;
    } else if (option == Size) {
        if (state == Error)
            return QVariant();
        if (state == Ready && !const_cast<QXbmHandler*>(this)->readHeader())
            return QVariant();
        return QSize(width, height);
    } else if (option == ImageFormat) {
        return QImage::Format_MonoLSB;
    }
    return QVariant();
}

void QXbmHandler::setOption(ImageOption option, const QVariant &value)
{
    if (option == Name)
        fileName = value.toString();
}

QT_END_NAMESPACE

#endif // QT_NO_IMAGEFORMAT_XBM
