/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qwindowsmime.h"
#include "qwindowscontext.h"

#include <QtGui/private/qinternalmimedata_p.h>
#include <QtCore/qbytearraymatcher.h>
#include <QtCore/qtextcodec.h>
#include <QtCore/qmap.h>
#include <QtCore/qurl.h>
#include <QtCore/qdir.h>
#include <QtCore/qdebug.h>
#include <QtCore/qbuffer.h>
#include <QtGui/qimagereader.h>
#include <QtGui/qimagewriter.h>

#include <shlobj.h>
#include <algorithm>

QT_BEGIN_NAMESPACE

/* The MSVC compilers allows multi-byte characters, that has the behavior of
 * that each character gets shifted into position. 0x73524742 below is for MSVC
 * equivalent to doing 'sRGB', but this does of course not work
 * on conformant C++ compilers. */
#define BMP_LCS_sRGB  0x73524742
#define BMP_LCS_GM_IMAGES  0x00000004L

struct _CIEXYZ {
    long ciexyzX, ciexyzY, ciexyzZ;
};

struct _CIEXYZTRIPLE {
    _CIEXYZ  ciexyzRed, ciexyzGreen, ciexyzBlue;
};

struct BMP_BITMAPV5HEADER {
    DWORD  bV5Size;
    LONG   bV5Width;
    LONG   bV5Height;
    WORD   bV5Planes;
    WORD   bV5BitCount;
    DWORD  bV5Compression;
    DWORD  bV5SizeImage;
    LONG   bV5XPelsPerMeter;
    LONG   bV5YPelsPerMeter;
    DWORD  bV5ClrUsed;
    DWORD  bV5ClrImportant;
    DWORD  bV5RedMask;
    DWORD  bV5GreenMask;
    DWORD  bV5BlueMask;
    DWORD  bV5AlphaMask;
    DWORD  bV5CSType;
    _CIEXYZTRIPLE bV5Endpoints;
    DWORD  bV5GammaRed;
    DWORD  bV5GammaGreen;
    DWORD  bV5GammaBlue;
    DWORD  bV5Intent;
    DWORD  bV5ProfileData;
    DWORD  bV5ProfileSize;
    DWORD  bV5Reserved;
};
static const int BMP_BITFIELDS = 3;

static const char dibFormatC[] = "dib";

static inline QByteArray msgConversionError(const char *func, const char *format)
{
    QByteArray msg = func;
    msg += ": Unable to convert DIB image. The image converter plugin for '";
    msg += format;
    msg += "' is not available. Available formats: ";
    const auto &formats = QImageReader::supportedImageFormats();
    for (const QByteArray &af : formats) {
        msg += af;
        msg += ' ';
    }
    return msg;
}

static inline QImage readDib(QByteArray data)
{
    QBuffer buffer(&data);
    buffer.open(QIODevice::ReadOnly);
    QImageReader reader(&buffer, dibFormatC);
    if (!reader.canRead()) {
         qWarning("%s", msgConversionError(__FUNCTION__, dibFormatC).constData());
         return QImage();
    }
    return reader.read();
}

static QByteArray writeDib(const QImage &img)
{
    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::ReadWrite);
    QImageWriter writer(&buffer, dibFormatC);
    if (!writer.canWrite()) {
        qWarning("%s", msgConversionError(__FUNCTION__, dibFormatC).constData());
        return ba;
    }
    if (!writer.write(img))
        ba.clear();
    return ba;
}

static bool qt_write_dibv5(QDataStream &s, QImage image)
{
    QIODevice* d = s.device();
    if (!d->isWritable())
        return false;

    //depth will be always 32
    int bpl_bmp = image.width()*4;

    BMP_BITMAPV5HEADER bi;
    ZeroMemory(&bi, sizeof(bi));
    bi.bV5Size          = sizeof(BMP_BITMAPV5HEADER);
    bi.bV5Width         = image.width();
    bi.bV5Height        = image.height();
    bi.bV5Planes        = 1;
    bi.bV5BitCount      = 32;
    bi.bV5Compression   = BI_BITFIELDS;
    bi.bV5SizeImage     = DWORD(bpl_bmp * image.height());
    bi.bV5XPelsPerMeter = 0;
    bi.bV5YPelsPerMeter = 0;
    bi.bV5ClrUsed       = 0;
    bi.bV5ClrImportant  = 0;
    bi.bV5BlueMask      = 0x000000ff;
    bi.bV5GreenMask     = 0x0000ff00;
    bi.bV5RedMask       = 0x00ff0000;
    bi.bV5AlphaMask     = 0xff000000;
    bi.bV5CSType        = BMP_LCS_sRGB;         //LCS_sRGB
    bi.bV5Intent        = BMP_LCS_GM_IMAGES;    //LCS_GM_IMAGES

    d->write(reinterpret_cast<const char*>(&bi), bi.bV5Size);
    if (s.status() != QDataStream::Ok)
        return false;

    if (image.format() != QImage::Format_ARGB32)
        image = image.convertToFormat(QImage::Format_ARGB32);

    auto *buf = new uchar[bpl_bmp];

    memset(buf, 0, size_t(bpl_bmp));
    for (int y=image.height()-1; y>=0; y--) {
        // write the image bits
        const QRgb *p = reinterpret_cast<const QRgb *>(image.constScanLine(y));
        const QRgb *end = p + image.width();
        uchar *b = buf;
        while (p < end) {
            int alpha = qAlpha(*p);
            if (alpha) {
                *b++ = uchar(qBlue(*p));
                *b++ = uchar(qGreen(*p));
                *b++ = uchar(qRed(*p));
            } else {
                //white for fully transparent pixels.
                *b++ = 0xff;
                *b++ = 0xff;
                *b++ = 0xff;
            }
            *b++ = uchar(alpha);
            p++;
        }
        d->write(reinterpret_cast<const char *>(buf), bpl_bmp);
        if (s.status() != QDataStream::Ok) {
            delete[] buf;
            return false;
        }
    }
    delete[] buf;
    return true;
}

static int calc_shift(int mask)
{
    int result = 0;
    while (!(mask & 1)) {
        result++;
        mask >>= 1;
    }
    return result;
}

//Supports only 32 bit DIBV5
static bool qt_read_dibv5(QDataStream &s, QImage &image)
{
    BMP_BITMAPV5HEADER bi;
    QIODevice* d = s.device();
    if (d->atEnd())
        return false;

    d->read(reinterpret_cast<char *>(&bi), sizeof(bi));   // read BITMAPV5HEADER header
    if (s.status() != QDataStream::Ok)
        return false;

    const int nbits = bi.bV5BitCount;
    if (nbits != 32 || bi.bV5Planes != 1 || bi.bV5Compression != BMP_BITFIELDS)
        return false; //Unsupported DIBV5 format

    const int w = bi.bV5Width;
    int h = bi.bV5Height;
    const int red_mask = int(bi.bV5RedMask);
    const int green_mask = int(bi.bV5GreenMask);
    const int blue_mask = int(bi.bV5BlueMask);
    const int alpha_mask = int(bi.bV5AlphaMask);

    const QImage::Format format = QImage::Format_ARGB32;

    if (bi.bV5Height < 0)
        h = -h;     // support images with negative height
    if (image.size() != QSize(w, h) || image.format() != format) {
        image = QImage(w, h, format);
        if (image.isNull())     // could not create image
            return false;
    }
    image.setDotsPerMeterX(bi.bV5XPelsPerMeter);
    image.setDotsPerMeterY(bi.bV5YPelsPerMeter);

    const int red_shift = calc_shift(red_mask);
    const int green_shift = calc_shift(green_mask);
    const int blue_shift = calc_shift(blue_mask);
    const int alpha_shift =  alpha_mask ? calc_shift(alpha_mask) : 0u;

    const int  bpl = image.bytesPerLine();
    uchar *data = image.bits();

    auto *buf24 = new uchar[bpl];
    const int bpl24 = ((w * nbits + 31) / 32) * 4;

    while (--h >= 0) {
        QRgb *p = reinterpret_cast<QRgb *>(data + h * bpl);
        QRgb *end = p + w;
        if (d->read(reinterpret_cast<char *>(buf24), bpl24) != bpl24)
            break;
        const uchar *b = buf24;
        while (p < end) {
            const int c = *b | (*(b + 1)) << 8 | (*(b + 2)) << 16 | (*(b + 3)) << 24;
            *p++ = qRgba(((c & red_mask) >> red_shift) ,
                                    ((c & green_mask) >> green_shift),
                                    ((c & blue_mask) >> blue_shift),
                                    ((c & alpha_mask) >> alpha_shift));
            b += 4;
        }
    }
    delete[] buf24;

    if (bi.bV5Height < 0) {
        // Flip the image
        auto *buf = new uchar[bpl];
        h = -bi.bV5Height;
        for (int y = 0; y < h/2; ++y) {
            memcpy(buf, data + y * bpl, size_t(bpl));
            memcpy(data + y*bpl, data + (h - y -1) * bpl, size_t(bpl));
            memcpy(data + (h - y -1 ) * bpl, buf, size_t(bpl));
        }
        delete [] buf;
    }

    return true;
}

// helpers for using global memory

static int getCf(const FORMATETC &formatetc)
{
    return formatetc.cfFormat;
}

static FORMATETC setCf(int cf)
{
    FORMATETC formatetc;
    formatetc.cfFormat = CLIPFORMAT(cf);
    formatetc.dwAspect = DVASPECT_CONTENT;
    formatetc.lindex = -1;
    formatetc.ptd = nullptr;
    formatetc.tymed = TYMED_HGLOBAL;
    return formatetc;
}

static bool setData(const QByteArray &data, STGMEDIUM *pmedium)
{
    HGLOBAL hData = GlobalAlloc(0, SIZE_T(data.size()));
    if (!hData)
        return false;

    void *out = GlobalLock(hData);
    memcpy(out, data.data(), size_t(data.size()));
    GlobalUnlock(hData);
    pmedium->tymed = TYMED_HGLOBAL;
    pmedium->hGlobal = hData;
    pmedium->pUnkForRelease = nullptr;
    return true;
}

static QByteArray getData(int cf, IDataObject *pDataObj, int lindex = -1)
{
    QByteArray data;
    FORMATETC formatetc = setCf(cf);
    formatetc.lindex = lindex;
    STGMEDIUM s;
    if (pDataObj->GetData(&formatetc, &s) == S_OK) {
        const void *val = GlobalLock(s.hGlobal);
        data = QByteArray::fromRawData(reinterpret_cast<const char *>(val), int(GlobalSize(s.hGlobal)));
        data.detach();
        GlobalUnlock(s.hGlobal);
        ReleaseStgMedium(&s);
    } else  {
        //Try reading IStream data
        formatetc.tymed = TYMED_ISTREAM;
        if (pDataObj->GetData(&formatetc, &s) == S_OK) {
            char szBuffer[4096];
            ULONG actualRead = 0;
            LARGE_INTEGER pos = {{0, 0}};
            //Move to front (can fail depending on the data model implemented)
            HRESULT hr = s.pstm->Seek(pos, STREAM_SEEK_SET, nullptr);
            while(SUCCEEDED(hr)){
                hr = s.pstm->Read(szBuffer, sizeof(szBuffer), &actualRead);
                if (SUCCEEDED(hr) && actualRead > 0) {
                    data += QByteArray::fromRawData(szBuffer, int(actualRead));
                }
                if (actualRead != sizeof(szBuffer))
                    break;
            }
            data.detach();
            ReleaseStgMedium(&s);
        }
    }
    return data;
}

static bool canGetData(int cf, IDataObject * pDataObj)
{
    FORMATETC formatetc = setCf(cf);
     if (pDataObj->QueryGetData(&formatetc) != S_OK){
        formatetc.tymed = TYMED_ISTREAM;
        return pDataObj->QueryGetData(&formatetc) == S_OK;
    }
    return true;
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug d, const FORMATETC &tc)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d << "FORMATETC(cfFormat=" << tc.cfFormat << ' ';
    switch (tc.cfFormat) {
    case CF_TEXT:
        d << "CF_TEXT";
        break;
    case CF_BITMAP:
        d << "CF_BITMAP";
        break;
    case CF_TIFF:
        d << "CF_TIFF";
        break;
    case CF_OEMTEXT:
        d << "CF_OEMTEXT";
        break;
    case CF_DIB:
        d << "CF_DIB";
        break;
    case CF_DIBV5:
        d << "CF_DIBV5";
        break;
    case CF_UNICODETEXT:
        d << "CF_UNICODETEXT";
        break;
    case CF_ENHMETAFILE:
        d << "CF_ENHMETAFILE";
        break;
    default:
        d << QWindowsMimeConverter::clipboardFormatName(tc.cfFormat);
        break;
    }
    d << ", dwAspect=" << tc.dwAspect << ", lindex=" << tc.lindex
        << ", tymed=" << tc.tymed << ", ptd=" << tc.ptd << ')';
    return d;
}

QDebug operator<<(QDebug d, IDataObject *dataObj)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d.noquote();
    d << "IDataObject(";
    if (dataObj) { // Output formats contained in IDataObject.
        IEnumFORMATETC *enumFormatEtc;
        if (SUCCEEDED(dataObj->EnumFormatEtc(DATADIR_GET, &enumFormatEtc)) && enumFormatEtc) {
            FORMATETC formatEtc[1];
            ULONG fetched;
            if (SUCCEEDED(enumFormatEtc->Reset())) {
                while (SUCCEEDED(enumFormatEtc->Next(1, formatEtc, &fetched)) && fetched)
                    d << formatEtc[0] << ',';
                enumFormatEtc->Release();
            }
        }
    } else {
        d << '0';
    }
    d << ')';
    return d;
}
#endif // !QT_NO_DEBUG_STREAM

/*!
    \class QWindowsMime
    \brief The QWindowsMime class maps open-standard MIME to Window Clipboard formats.
    \internal

    Qt's drag-and-drop and clipboard facilities use the MIME standard.
    On X11, this maps trivially to the Xdnd protocol, but on Windows
    although some applications use MIME types to describe clipboard
    formats, others use arbitrary non-standardized naming conventions,
    or unnamed built-in formats of Windows.

    By instantiating subclasses of QWindowsMime that provide conversions
    between Windows Clipboard and MIME formats, you can convert
    proprietary clipboard formats to MIME formats.

    Qt has predefined support for the following Windows Clipboard formats:

    \table
    \header \li Windows Format \li Equivalent MIME type
    \row \li \c CF_UNICODETEXT \li \c text/plain
    \row \li \c CF_TEXT        \li \c text/plain
    \row \li \c CF_DIB         \li \c{image/xyz}, where \c xyz is
                                 a \l{QImageWriter::supportedImageFormats()}{Qt image format}
    \row \li \c CF_HDROP       \li \c text/uri-list
    \row \li \c CF_INETURL     \li \c text/uri-list
    \row \li \c CF_HTML        \li \c text/html
    \endtable

    An example use of this class would be to map the Windows Metafile
    clipboard format (\c CF_METAFILEPICT) to and from the MIME type
    \c{image/x-wmf}. This conversion might simply be adding or removing
    a header, or even just passing on the data. See \l{Drag and Drop}
    for more information on choosing and definition MIME types.

    You can check if a MIME type is convertible using canConvertFromMime() and
    can perform conversions with convertToMime() and convertFromMime().

    \sa QWindowsMimeConverter
*/

/*!
Constructs a new conversion object, adding it to the globally accessed
list of available converters.
*/
QWindowsMime::QWindowsMime() = default;

/*!
Destroys a conversion object, removing it from the global
list of available converters.
*/
QWindowsMime::~QWindowsMime() = default;

/*!
    Registers the MIME type \a mime, and returns an ID number
    identifying the format on Windows.
*/
int QWindowsMime::registerMimeType(const QString &mime)
{
    const UINT f = RegisterClipboardFormat(reinterpret_cast<const wchar_t *> (mime.utf16()));
    if (!f)
        qErrnoWarning("QWindowsMime::registerMimeType: Failed to register clipboard format");

    return int(f);
}

/*!
\fn bool QWindowsMime::canConvertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData) const

  Returns \c true if the converter can convert from the \a mimeData to
  the format specified in \a formatetc.

  All subclasses must reimplement this pure virtual function.
*/

/*!
  \fn bool QWindowsMime::canConvertToMime(const QString &mimeType, IDataObject *pDataObj) const

  Returns \c true if the converter can convert to the \a mimeType from
  the available formats in \a pDataObj.

  All subclasses must reimplement this pure virtual function.
*/

/*!
\fn QString QWindowsMime::mimeForFormat(const FORMATETC &formatetc) const

  Returns the mime type that will be created form the format specified
  in \a formatetc, or an empty string if this converter does not support
  \a formatetc.

  All subclasses must reimplement this pure virtual function.
*/

/*!
\fn QVector<FORMATETC> QWindowsMime::formatsForMime(const QString &mimeType, const QMimeData *mimeData) const

  Returns a QVector of FORMATETC structures representing the different windows clipboard
  formats that can be provided for the \a mimeType from the \a mimeData.

  All subclasses must reimplement this pure virtual function.
*/

/*!
    \fn QVariant QWindowsMime::convertToMime(const QString &mimeType, IDataObject *pDataObj,
                                             QVariant::Type preferredType) const

    Returns a QVariant containing the converted data for \a mimeType from \a pDataObj.
    If possible the QVariant should be of the \a preferredType to avoid needless conversions.

    All subclasses must reimplement this pure virtual function.
*/

/*!
\fn bool QWindowsMime::convertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData, STGMEDIUM * pmedium) const

  Convert the \a mimeData to the format specified in \a formatetc.
  The converted data should then be placed in \a pmedium structure.

  Return true if the conversion was successful.

  All subclasses must reimplement this pure virtual function.
*/

class QWindowsMimeText : public QWindowsMime
{
public:
    bool canConvertToMime(const QString &mimeType, IDataObject *pDataObj) const override;
    QVariant convertToMime(const QString &mime, LPDATAOBJECT pDataObj, QVariant::Type preferredType) const override;
    QString mimeForFormat(const FORMATETC &formatetc) const override;
    bool canConvertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData) const override;
    bool convertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData, STGMEDIUM *pmedium) const override;
    QVector<FORMATETC> formatsForMime(const QString &mimeType, const QMimeData *mimeData) const override;
};

bool QWindowsMimeText::canConvertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData) const
{
    int cf = getCf(formatetc);
    return (cf == CF_UNICODETEXT || cf == CF_TEXT) && mimeData->hasText();
}

/*
text/plain is defined as using CRLF, but so many programs don't,
and programmers just look for '\n' in strings.
Windows really needs CRLF, so we ensure it here.
*/
bool QWindowsMimeText::convertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData, STGMEDIUM *pmedium) const
{
    if (canConvertFromMime(formatetc, mimeData)) {
        QByteArray data;
        int cf = getCf(formatetc);
        if (cf == CF_TEXT) {
            data = mimeData->text().toLocal8Bit();
            // Anticipate required space for CRLFs at 1/40
            int maxsize=data.size()+data.size()/40+3;
            QByteArray r(maxsize, '\0');
            char* o = r.data();
            const char* d = data.data();
            const int s = data.size();
            bool cr=false;
            int j=0;
            for (int i=0; i<s; i++) {
                char c = d[i];
                if (c=='\r')
                    cr=true;
                else {
                    if (c=='\n') {
                        if (!cr)
                            o[j++]='\r';
                    }
                    cr=false;
                }
                o[j++]=c;
                if (j+3 >= maxsize) {
                    maxsize += maxsize/4;
                    r.resize(maxsize);
                    o = r.data();
                }
            }
            o[j]=0;
            return setData(r, pmedium);
        }
        if (cf == CF_UNICODETEXT) {
            QString str = mimeData->text();
            const QChar *u = str.unicode();
            QString res;
            const int s = str.length();
            int maxsize = s + s/40 + 3;
            res.resize(maxsize);
            int ri = 0;
            bool cr = false;
            for (int i=0; i < s; ++i) {
                if (*u == u'\r')
                    cr = true;
                else {
                    if (*u == u'\n' && !cr)
                        res[ri++] = u'\r';
                    cr = false;
                }
                res[ri++] = *u;
                if (ri+3 >= maxsize) {
                    maxsize += maxsize/4;
                    res.resize(maxsize);
                }
                ++u;
            }
            res.truncate(ri);
            const int byteLength = res.length() * int(sizeof(ushort));
            QByteArray r(byteLength + 2, '\0');
            memcpy(r.data(), res.unicode(), size_t(byteLength));
            r[byteLength] = 0;
            r[byteLength+1] = 0;
            return setData(r, pmedium);
        }
    }
    return false;
}

bool QWindowsMimeText::canConvertToMime(const QString &mimeType, IDataObject *pDataObj) const
{
    return mimeType.startsWith(u"text/plain")
           && (canGetData(CF_UNICODETEXT, pDataObj)
           || canGetData(CF_TEXT, pDataObj));
}

QString QWindowsMimeText::mimeForFormat(const FORMATETC &formatetc) const
{
    int cf = getCf(formatetc);
    if (cf == CF_UNICODETEXT || cf == CF_TEXT)
        return QStringLiteral("text/plain");
    return QString();
}


QVector<FORMATETC> QWindowsMimeText::formatsForMime(const QString &mimeType, const QMimeData *mimeData) const
{
    QVector<FORMATETC> formatics;
    if (mimeType.startsWith(u"text/plain") && mimeData->hasText()) {
        formatics += setCf(CF_UNICODETEXT);
        formatics += setCf(CF_TEXT);
    }
    return formatics;
}

QVariant QWindowsMimeText::convertToMime(const QString &mime, LPDATAOBJECT pDataObj, QVariant::Type preferredType) const
{
    QVariant ret;

    if (canConvertToMime(mime, pDataObj)) {
        QString str;
        QByteArray data = getData(CF_UNICODETEXT, pDataObj);
        if (!data.isEmpty()) {
            str = QString::fromWCharArray(reinterpret_cast<const wchar_t *>(data.constData()));
            str.replace(QLatin1String("\r\n"), QLatin1String("\n"));
        } else {
            data = getData(CF_TEXT, pDataObj);
            if (!data.isEmpty()) {
                const char* d = data.data();
                const unsigned s = qstrlen(d);
                QByteArray r(data.size()+1, '\0');
                char* o = r.data();
                int j=0;
                for (unsigned i = 0; i < s; ++i) {
                    char c = d[i];
                    if (c!='\r')
                        o[j++]=c;
                }
                o[j]=0;
                str = QString::fromLocal8Bit(r);
            }
        }
        if (preferredType == QVariant::String)
            ret = str;
        else
            ret = std::move(str).toUtf8();
    }
    qCDebug(lcQpaMime) << __FUNCTION__ << ret;
    return ret;
}

class QWindowsMimeURI : public QWindowsMime
{
public:
    QWindowsMimeURI();
    bool canConvertToMime(const QString &mimeType, IDataObject *pDataObj) const override;
    QVariant convertToMime(const QString &mime, LPDATAOBJECT pDataObj, QVariant::Type preferredType) const override;
    QString mimeForFormat(const FORMATETC &formatetc) const override;
    bool canConvertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData) const override;
    bool convertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData, STGMEDIUM *pmedium) const override;
    QVector<FORMATETC> formatsForMime(const QString &mimeType, const QMimeData *mimeData) const override;
private:
    int CF_INETURL_W; // wide char version
    int CF_INETURL;
};

QWindowsMimeURI::QWindowsMimeURI()
{
    CF_INETURL_W = QWindowsMime::registerMimeType(QStringLiteral("UniformResourceLocatorW"));
    CF_INETURL = QWindowsMime::registerMimeType(QStringLiteral("UniformResourceLocator"));
}

bool QWindowsMimeURI::canConvertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData) const
{
    if (mimeData->hasUrls() && getCf(formatetc) == CF_HDROP) {
        const auto urls = mimeData->urls();
        for (const QUrl &url : urls) {
            if (url.isLocalFile())
                return true;
        }
    }
    return (getCf(formatetc) == CF_INETURL_W || getCf(formatetc) == CF_INETURL) && mimeData->hasUrls();
}

bool QWindowsMimeURI::convertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData, STGMEDIUM *pmedium) const
{
    if (canConvertFromMime(formatetc, mimeData)) {
        if (getCf(formatetc) == CF_HDROP) {
            const auto &urls = mimeData->urls();
            QStringList fileNames;
            int size = sizeof(DROPFILES)+2;
            for (const QUrl &url : urls) {
                const QString fn = QDir::toNativeSeparators(url.toLocalFile());
                if (!fn.isEmpty()) {
                    size += sizeof(ushort) * size_t(fn.length() + 1);
                    fileNames.append(fn);
                }
            }

            QByteArray result(size, '\0');
            auto* d = reinterpret_cast<DROPFILES *>(result.data());
            d->pFiles = sizeof(DROPFILES);
            GetCursorPos(&d->pt); // try
            d->fNC = true;
            char *files = (reinterpret_cast<char*>(d)) + d->pFiles;

            d->fWide = true;
            auto *f = reinterpret_cast<wchar_t *>(files);
            for (int i=0; i<fileNames.size(); i++) {
                const auto l = size_t(fileNames.at(i).length());
                memcpy(f, fileNames.at(i).utf16(), l * sizeof(ushort));
                f += l;
                *f++ = 0;
            }
            *f = 0;

            return setData(result, pmedium);
        }
        if (getCf(formatetc) == CF_INETURL_W) {
            const auto urls = mimeData->urls();
            QByteArray result;
            if (!urls.isEmpty()) {
                QString url = urls.at(0).toString();
                result = QByteArray(reinterpret_cast<const char *>(url.utf16()),
                                    url.length() * int(sizeof(ushort)));
            }
            result.append('\0');
            result.append('\0');
            return setData(result, pmedium);
        }
        if (getCf(formatetc) == CF_INETURL) {
            const auto urls = mimeData->urls();
            QByteArray result;
            if (!urls.isEmpty())
                result = urls.at(0).toString().toLocal8Bit();
            return setData(result, pmedium);
        }
    }

    return false;
}

bool QWindowsMimeURI::canConvertToMime(const QString &mimeType, IDataObject *pDataObj) const
{
    return mimeType == u"text/uri-list"
           && (canGetData(CF_HDROP, pDataObj) || canGetData(CF_INETURL_W, pDataObj) || canGetData(CF_INETURL, pDataObj));
}

QString QWindowsMimeURI::mimeForFormat(const FORMATETC &formatetc) const
{
    QString format;
    if (getCf(formatetc) == CF_HDROP || getCf(formatetc) == CF_INETURL_W || getCf(formatetc) == CF_INETURL)
        format = QStringLiteral("text/uri-list");
    return format;
}

QVector<FORMATETC> QWindowsMimeURI::formatsForMime(const QString &mimeType, const QMimeData *mimeData) const
{
    QVector<FORMATETC> formatics;
    if (mimeType == u"text/uri-list") {
        if (canConvertFromMime(setCf(CF_HDROP), mimeData))
            formatics += setCf(CF_HDROP);
        if (canConvertFromMime(setCf(CF_INETURL_W), mimeData))
            formatics += setCf(CF_INETURL_W);
        if (canConvertFromMime(setCf(CF_INETURL), mimeData))
            formatics += setCf(CF_INETURL);
    }
    return formatics;
}

QVariant QWindowsMimeURI::convertToMime(const QString &mimeType, LPDATAOBJECT pDataObj, QVariant::Type preferredType) const
{
    if (mimeType == u"text/uri-list") {
        if (canGetData(CF_HDROP, pDataObj)) {
            QList<QVariant> urls;

            QByteArray data = getData(CF_HDROP, pDataObj);
            if (data.isEmpty())
                return QVariant();

            const auto *hdrop = reinterpret_cast<const DROPFILES *>(data.constData());
            if (hdrop->fWide) {
                const auto *filesw = reinterpret_cast<const wchar_t *>(data.constData() + hdrop->pFiles);
                int i = 0;
                while (filesw[i]) {
                    QString fileurl = QString::fromWCharArray(filesw + i);
                    urls += QUrl::fromLocalFile(fileurl);
                    i += fileurl.length()+1;
                }
            } else {
                const char* files = reinterpret_cast<const char *>(data.constData() + hdrop->pFiles);
                int i=0;
                while (files[i]) {
                    urls += QUrl::fromLocalFile(QString::fromLocal8Bit(files+i));
                    i += int(strlen(files+i))+1;
                }
            }

            if (preferredType == QVariant::Url && urls.size() == 1)
                return urls.at(0);
            if (!urls.isEmpty())
                return urls;
        } else if (canGetData(CF_INETURL_W, pDataObj)) {
            QByteArray data = getData(CF_INETURL_W, pDataObj);
            if (data.isEmpty())
                return QVariant();
            return QUrl(QString::fromWCharArray(reinterpret_cast<const wchar_t *>(data.constData())));
         } else if (canGetData(CF_INETURL, pDataObj)) {
            QByteArray data = getData(CF_INETURL, pDataObj);
            if (data.isEmpty())
                return QVariant();
            return QUrl(QString::fromLocal8Bit(data.constData()));
        }
    }
    return QVariant();
}

class QWindowsMimeHtml : public QWindowsMime
{
public:
    QWindowsMimeHtml();

    // for converting from Qt
    bool canConvertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData) const override;
    bool convertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData, STGMEDIUM * pmedium) const override;
    QVector<FORMATETC> formatsForMime(const QString &mimeType, const QMimeData *mimeData) const override;

    // for converting to Qt
    bool canConvertToMime(const QString &mimeType, IDataObject *pDataObj) const override;
    QVariant convertToMime(const QString &mime, IDataObject *pDataObj, QVariant::Type preferredType) const override;
    QString mimeForFormat(const FORMATETC &formatetc) const override;

private:
    int CF_HTML;
};

QWindowsMimeHtml::QWindowsMimeHtml()
{
    CF_HTML = QWindowsMime::registerMimeType(QStringLiteral("HTML Format"));
}

QVector<FORMATETC> QWindowsMimeHtml::formatsForMime(const QString &mimeType, const QMimeData *mimeData) const
{
    QVector<FORMATETC> formatetcs;
    if (mimeType == u"text/html" && (!mimeData->html().isEmpty()))
        formatetcs += setCf(CF_HTML);
    return formatetcs;
}

QString QWindowsMimeHtml::mimeForFormat(const FORMATETC &formatetc) const
{
    if (getCf(formatetc) == CF_HTML)
        return QStringLiteral("text/html");
    return QString();
}

bool QWindowsMimeHtml::canConvertToMime(const QString &mimeType, IDataObject *pDataObj) const
{
    return mimeType == u"text/html" && canGetData(CF_HTML, pDataObj);
}


bool QWindowsMimeHtml::canConvertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData) const
{
    return getCf(formatetc) == CF_HTML && (!mimeData->html().isEmpty());
}

/*
The windows HTML clipboard format is as follows (xxxxxxxxxx is a 10 integer number giving the positions
in bytes). Charset used is mostly utf8, but can be different, ie. we have to look for the <meta> charset tag

  Version: 1.0
  StartHTML:xxxxxxxxxx
  EndHTML:xxxxxxxxxx
  StartFragment:xxxxxxxxxx
  EndFragment:xxxxxxxxxx
  ...html...

*/
QVariant QWindowsMimeHtml::convertToMime(const QString &mime, IDataObject *pDataObj, QVariant::Type preferredType) const
{
    Q_UNUSED(preferredType);
    QVariant result;
    if (canConvertToMime(mime, pDataObj)) {
        QByteArray html = getData(CF_HTML, pDataObj);
        static Q_RELAXED_CONSTEXPR auto startMatcher = qMakeStaticByteArrayMatcher("StartHTML:");
        static Q_RELAXED_CONSTEXPR auto endMatcher   = qMakeStaticByteArrayMatcher("EndHTML:");
        qCDebug(lcQpaMime) << __FUNCTION__ << "raw:" << html;
        int start = startMatcher.indexIn(html);
        int end = endMatcher.indexIn(html);

        if (start != -1) {
            int startOffset = start + 10;
            int i = startOffset;
            while (html.at(i) != '\r' && html.at(i) != '\n')
                ++i;
            QByteArray bytecount = html.mid(startOffset, i - startOffset);
            start = bytecount.toInt();
        }

        if (end != -1) {
            int endOffset = end + 8;
            int i = endOffset ;
            while (html.at(i) != '\r' && html.at(i) != '\n')
                ++i;
            QByteArray bytecount = html.mid(endOffset , i - endOffset);
            end = bytecount.toInt();
        }

        if (end > start && start > 0) {
            html = html.mid(start, end - start);
            html.replace('\r', "");
            result = QString::fromUtf8(html);
        }
    }
    return result;
}

bool QWindowsMimeHtml::convertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData, STGMEDIUM * pmedium) const
{
    if (canConvertFromMime(formatetc, mimeData)) {
        QByteArray data = mimeData->html().toUtf8();
        QByteArray result =
            "Version:1.0\r\n"                    // 0-12
            "StartHTML:0000000107\r\n"           // 13-35
            "EndHTML:0000000000\r\n"             // 36-55
            "StartFragment:0000000000\r\n"       // 56-81
            "EndFragment:0000000000\r\n\r\n";    // 82-107

        static Q_RELAXED_CONSTEXPR auto startFragmentMatcher = qMakeStaticByteArrayMatcher("<!--StartFragment-->");
        static Q_RELAXED_CONSTEXPR auto endFragmentMatcher   = qMakeStaticByteArrayMatcher("<!--EndFragment-->");

        if (startFragmentMatcher.indexIn(data) == -1)
            result += "<!--StartFragment-->";
        result += data;
        if (endFragmentMatcher.indexIn(data) == -1)
            result += "<!--EndFragment-->";

        // set the correct number for EndHTML
        QByteArray pos = QByteArray::number(result.size());
        memcpy(reinterpret_cast<char *>(result.data() + 53 - pos.length()), pos.constData(), size_t(pos.length()));

        // set correct numbers for StartFragment and EndFragment
        pos = QByteArray::number(startFragmentMatcher.indexIn(result) + 20);
        memcpy(reinterpret_cast<char *>(result.data() + 79 - pos.length()), pos.constData(), size_t(pos.length()));
        pos = QByteArray::number(endFragmentMatcher.indexIn(result));
        memcpy(reinterpret_cast<char *>(result.data() + 103 - pos.length()), pos.constData(), size_t(pos.length()));

        return setData(result, pmedium);
    }
    return false;
}


#ifndef QT_NO_IMAGEFORMAT_BMP
class QWindowsMimeImage : public QWindowsMime
{
public:
    QWindowsMimeImage();
    // for converting from Qt
    bool canConvertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData) const override;
    bool convertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData, STGMEDIUM * pmedium) const override;
    QVector<FORMATETC> formatsForMime(const QString &mimeType, const QMimeData *mimeData) const override;

    // for converting to Qt
    bool canConvertToMime(const QString &mimeType, IDataObject *pDataObj) const override;
    QVariant convertToMime(const QString &mime, IDataObject *pDataObj, QVariant::Type preferredType) const override;
    QString mimeForFormat(const FORMATETC &formatetc) const override;
private:
    bool hasOriginalDIBV5(IDataObject *pDataObj) const;
    UINT CF_PNG;
};

QWindowsMimeImage::QWindowsMimeImage()
{
    CF_PNG = RegisterClipboardFormat(L"PNG");
}

QVector<FORMATETC> QWindowsMimeImage::formatsForMime(const QString &mimeType, const QMimeData *mimeData) const
{
    QVector<FORMATETC> formatetcs;
    if (mimeData->hasImage() && mimeType == u"application/x-qt-image") {
        //add DIBV5 if image has alpha channel. Do not add CF_PNG here as it will confuse MS Office (QTBUG47656).
        auto image = qvariant_cast<QImage>(mimeData->imageData());
        if (!image.isNull() && image.hasAlphaChannel())
            formatetcs += setCf(CF_DIBV5);
        formatetcs += setCf(CF_DIB);
    }
    if (!formatetcs.isEmpty())
        qCDebug(lcQpaMime) << __FUNCTION__ << mimeType << formatetcs;
    return formatetcs;
}

QString QWindowsMimeImage::mimeForFormat(const FORMATETC &formatetc) const
{
    int cf = getCf(formatetc);
    if (cf == CF_DIB || cf == CF_DIBV5 || cf == int(CF_PNG))
       return QStringLiteral("application/x-qt-image");
    return QString();
}

bool QWindowsMimeImage::canConvertToMime(const QString &mimeType, IDataObject *pDataObj) const
{
    return mimeType == u"application/x-qt-image"
        && (canGetData(CF_DIB, pDataObj) || canGetData(CF_PNG, pDataObj));
}

bool QWindowsMimeImage::canConvertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData) const
{
    int cf = getCf(formatetc);
    if (!mimeData->hasImage())
        return false;
    const QImage image = qvariant_cast<QImage>(mimeData->imageData());
    if (image.isNull())
        return false;
    // QTBUG-64322: Use PNG only for transparent images as otherwise MS PowerPoint
    // cannot handle it.
    return cf == CF_DIBV5 || cf == CF_DIB
        || (cf == int(CF_PNG) && image.hasAlphaChannel());
}

bool QWindowsMimeImage::convertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData, STGMEDIUM * pmedium) const
{
    int cf = getCf(formatetc);
    if ((cf == CF_DIB || cf == CF_DIBV5 || cf == int(CF_PNG)) && mimeData->hasImage()) {
        auto img = qvariant_cast<QImage>(mimeData->imageData());
        if (img.isNull())
            return false;
        QByteArray ba;
        if (cf == CF_DIB) {
            if (img.format() > QImage::Format_ARGB32)
                img = img.convertToFormat(QImage::Format_RGB32);
            const QByteArray ba = writeDib(img);
            if (!ba.isEmpty())
                return setData(ba, pmedium);
        } else if (cf == int(CF_PNG)) {
            QBuffer buffer(&ba);
            const bool written = buffer.open(QIODevice::WriteOnly) && img.save(&buffer, "PNG");
            buffer.close();
            if (written)
                return setData(ba, pmedium);
        } else {
            QDataStream s(&ba, QIODevice::WriteOnly);
            s.setByteOrder(QDataStream::LittleEndian);// Intel byte order ####
            if (qt_write_dibv5(s, img))
                return setData(ba, pmedium);
        }
    }
    return false;
}

bool QWindowsMimeImage::hasOriginalDIBV5(IDataObject *pDataObj) const
{
    bool isSynthesized = true;
    IEnumFORMATETC *pEnum = nullptr;
    HRESULT res = pDataObj->EnumFormatEtc(1, &pEnum);
    if (res == S_OK && pEnum) {
        FORMATETC fc;
        while ((res = pEnum->Next(1, &fc, nullptr)) == S_OK) {
            if (fc.ptd)
                CoTaskMemFree(fc.ptd);
            if (fc.cfFormat == CF_DIB)
                break;
            if (fc.cfFormat == CF_DIBV5) {
                isSynthesized  = false;
                break;
            }
        }
        pEnum->Release();
    }
    return !isSynthesized;
}

QVariant QWindowsMimeImage::convertToMime(const QString &mimeType, IDataObject *pDataObj, QVariant::Type preferredType) const
{
    Q_UNUSED(preferredType);
    QVariant result;
    if (mimeType != u"application/x-qt-image")
        return result;
    //Try to convert from a format which has more data
    //DIBV5, use only if its is not synthesized
    if (canGetData(CF_DIBV5, pDataObj) && hasOriginalDIBV5(pDataObj)) {
        QImage img;
        QByteArray data = getData(CF_DIBV5, pDataObj);
        QDataStream s(&data, QIODevice::ReadOnly);
        s.setByteOrder(QDataStream::LittleEndian);
        if (qt_read_dibv5(s, img)) { // #### supports only 32bit DIBV5
            return img;
        }
    }
    //PNG, MS Office place this (undocumented)
    if (canGetData(CF_PNG, pDataObj)) {
        QImage img;
        QByteArray data = getData(CF_PNG, pDataObj);
        if (img.loadFromData(data, "PNG")) {
            return img;
        }
    }
    //Fallback to DIB
    if (canGetData(CF_DIB, pDataObj)) {
        const QImage img = readDib(getData(CF_DIB, pDataObj));
        if (!img.isNull())
            return img;
    }
    // Failed
    return result;
}
#endif

class QBuiltInMimes : public QWindowsMime
{
public:
    QBuiltInMimes();

    // for converting from Qt
    bool canConvertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData) const override;
    bool convertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData, STGMEDIUM * pmedium) const override;
    QVector<FORMATETC> formatsForMime(const QString &mimeType, const QMimeData *mimeData) const override;

    // for converting to Qt
    bool canConvertToMime(const QString &mimeType, IDataObject *pDataObj) const override;
    QVariant convertToMime(const QString &mime, IDataObject *pDataObj, QVariant::Type preferredType) const override;
    QString mimeForFormat(const FORMATETC &formatetc) const override;

private:
    QMap<int, QString> outFormats;
    QMap<int, QString> inFormats;
};

QBuiltInMimes::QBuiltInMimes()
: QWindowsMime()
{
    outFormats.insert(QWindowsMime::registerMimeType(QStringLiteral("application/x-color")), QStringLiteral("application/x-color"));
    inFormats.insert(QWindowsMime::registerMimeType(QStringLiteral("application/x-color")), QStringLiteral("application/x-color"));
}

bool QBuiltInMimes::canConvertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData) const
{
    // really check
    return formatetc.tymed & TYMED_HGLOBAL
        && outFormats.contains(formatetc.cfFormat)
        && mimeData->formats().contains(outFormats.value(formatetc.cfFormat));
}

bool QBuiltInMimes::convertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData, STGMEDIUM * pmedium) const
{
    if (canConvertFromMime(formatetc, mimeData)) {
        QByteArray data;
        if (outFormats.value(getCf(formatetc)) == u"text/html") {
            // text/html is in wide chars on windows (compatible with mozillia)
            QString html = mimeData->html();
            // same code as in the text converter up above
            const QChar *u = html.unicode();
            QString res;
            const int s = html.length();
            int maxsize = s + s/40 + 3;
            res.resize(maxsize);
            int ri = 0;
            bool cr = false;
            for (int i=0; i < s; ++i) {
                if (*u == u'\r')
                    cr = true;
                else {
                    if (*u == u'\n' && !cr)
                        res[ri++] = u'\r';
                    cr = false;
                }
                res[ri++] = *u;
                if (ri+3 >= maxsize) {
                    maxsize += maxsize/4;
                    res.resize(maxsize);
                }
                ++u;
            }
            res.truncate(ri);
            const int byteLength = res.length() * int(sizeof(ushort));
            QByteArray r(byteLength + 2, '\0');
            memcpy(r.data(), res.unicode(), size_t(byteLength));
            r[byteLength] = 0;
            r[byteLength+1] = 0;
            data = r;
        } else {
#if QT_CONFIG(draganddrop)
            data = QInternalMimeData::renderDataHelper(outFormats.value(getCf(formatetc)), mimeData);
#endif // QT_CONFIG(draganddrop)
        }
        return setData(data, pmedium);
    }
    return false;
}

QVector<FORMATETC> QBuiltInMimes::formatsForMime(const QString &mimeType, const QMimeData *mimeData) const
{
    QVector<FORMATETC> formatetcs;
    const auto mit = std::find(outFormats.cbegin(), outFormats.cend(), mimeType);
    if (mit != outFormats.cend() && mimeData->formats().contains(mimeType))
        formatetcs += setCf(mit.key());
    return formatetcs;
}

bool QBuiltInMimes::canConvertToMime(const QString &mimeType, IDataObject *pDataObj) const
{
    const auto mit = std::find(inFormats.cbegin(), inFormats.cend(), mimeType);
    return mit != inFormats.cend() && canGetData(mit.key(), pDataObj);
}

QVariant QBuiltInMimes::convertToMime(const QString &mimeType, IDataObject *pDataObj, QVariant::Type preferredType) const
{
    QVariant val;
    if (canConvertToMime(mimeType, pDataObj)) {
        QByteArray data = getData(inFormats.key(mimeType), pDataObj);
        if (!data.isEmpty()) {
            qCDebug(lcQpaMime) << __FUNCTION__;
            if (mimeType == u"text/html" && preferredType == QVariant::String) {
                // text/html is in wide chars on windows (compatible with Mozilla)
                val = QString::fromWCharArray(reinterpret_cast<const wchar_t *>(data.constData()));
            } else {
                val = data; // it should be enough to return the data and let QMimeData do the rest.
            }
        }
    }
    return val;
}

QString QBuiltInMimes::mimeForFormat(const FORMATETC &formatetc) const
{
    return inFormats.value(getCf(formatetc));
}


class QLastResortMimes : public QWindowsMime
{
public:

    QLastResortMimes();
    // for converting from Qt
    bool canConvertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData) const override;
    bool convertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData, STGMEDIUM * pmedium) const override;
    QVector<FORMATETC> formatsForMime(const QString &mimeType, const QMimeData *mimeData) const override;

    // for converting to Qt
    bool canConvertToMime(const QString &mimeType, IDataObject *pDataObj) const override;
    QVariant convertToMime(const QString &mime, IDataObject *pDataObj, QVariant::Type preferredType) const override;
    QString mimeForFormat(const FORMATETC &formatetc) const override;

private:
    mutable QMap<int, QString> formats;
    static QStringList ianaTypes;
    static QStringList excludeList;
};

QStringList QLastResortMimes::ianaTypes;
QStringList QLastResortMimes::excludeList;

QLastResortMimes::QLastResortMimes()
{
    //MIME Media-Types
    if (ianaTypes.isEmpty()) {
        ianaTypes.append(QStringLiteral("application/"));
        ianaTypes.append(QStringLiteral("audio/"));
        ianaTypes.append(QStringLiteral("example/"));
        ianaTypes.append(QStringLiteral("image/"));
        ianaTypes.append(QStringLiteral("message/"));
        ianaTypes.append(QStringLiteral("model/"));
        ianaTypes.append(QStringLiteral("multipart/"));
        ianaTypes.append(QStringLiteral("text/"));
        ianaTypes.append(QStringLiteral("video/"));
    }
    //Types handled by other classes
    if (excludeList.isEmpty()) {
        excludeList.append(QStringLiteral("HTML Format"));
        excludeList.append(QStringLiteral("UniformResourceLocator"));
        excludeList.append(QStringLiteral("text/html"));
        excludeList.append(QStringLiteral("text/plain"));
        excludeList.append(QStringLiteral("text/uri-list"));
        excludeList.append(QStringLiteral("application/x-qt-image"));
        excludeList.append(QStringLiteral("application/x-color"));
    }
}

bool QLastResortMimes::canConvertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData) const
{
    // really check
#if QT_CONFIG(draganddrop)
    return formatetc.tymed & TYMED_HGLOBAL
        && (formats.contains(formatetc.cfFormat)
        && QInternalMimeData::hasFormatHelper(formats.value(formatetc.cfFormat), mimeData));
#else
    Q_UNUSED(mimeData);
    Q_UNUSED(formatetc);
    return formatetc.tymed & TYMED_HGLOBAL
        && formats.contains(formatetc.cfFormat);
#endif // QT_CONFIG(draganddrop)
}

bool QLastResortMimes::convertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData, STGMEDIUM * pmedium) const
{
#if QT_CONFIG(draganddrop)
    return canConvertFromMime(formatetc, mimeData)
        && setData(QInternalMimeData::renderDataHelper(formats.value(getCf(formatetc)), mimeData), pmedium);
#else
    Q_UNUSED(mimeData);
    Q_UNUSED(formatetc);
    Q_UNUSED(pmedium);
    return false;
#endif // QT_CONFIG(draganddrop)
}

QVector<FORMATETC> QLastResortMimes::formatsForMime(const QString &mimeType, const QMimeData * /*mimeData*/) const
{
    QVector<FORMATETC> formatetcs;
    auto mit = std::find(formats.begin(), formats.end(), mimeType);
    // register any other available formats
    if (mit == formats.end() && !excludeList.contains(mimeType, Qt::CaseInsensitive))
        mit = formats.insert(QWindowsMime::registerMimeType(mimeType), mimeType);
    if (mit != formats.end())
        formatetcs += setCf(mit.key());

    if (!formatetcs.isEmpty())
        qCDebug(lcQpaMime) << __FUNCTION__ << mimeType << formatetcs;
    return formatetcs;
}
static const char x_qt_windows_mime[] = "application/x-qt-windows-mime;value=\"";

static bool isCustomMimeType(const QString &mimeType)
{
    return mimeType.startsWith(QLatin1String(x_qt_windows_mime), Qt::CaseInsensitive);
}

static QString customMimeType(const QString &mimeType, int *lindex = nullptr)
{
    int len = sizeof(x_qt_windows_mime) - 1;
    int n = mimeType.lastIndexOf(u'\"') - len;
    QString ret = mimeType.mid(len, n);

    const int beginPos = mimeType.indexOf(u";index=");
    if (beginPos > -1) {
        const int endPos = mimeType.indexOf(u';', beginPos + 1);
        const int indexStartPos = beginPos + 7;
        if (lindex)
            *lindex = mimeType.midRef(indexStartPos, endPos == -1 ? endPos : endPos - indexStartPos).toInt();
    } else {
        if (lindex)
            *lindex = -1;
    }
    return ret;
}

bool QLastResortMimes::canConvertToMime(const QString &mimeType, IDataObject *pDataObj) const
{
    if (isCustomMimeType(mimeType)) {
        // MSDN documentation for QueryGetData says only -1 is supported, so ignore lindex here.
        QString clipFormat = customMimeType(mimeType);
        const UINT cf = RegisterClipboardFormat(reinterpret_cast<const wchar_t *> (clipFormat.utf16()));
        return canGetData(int(cf), pDataObj);
    }
    // if it is not in there then register it and see if we can get it
    const auto mit = std::find(formats.cbegin(), formats.cend(), mimeType);
    const int cf = mit != formats.cend() ? mit.key() : QWindowsMime::registerMimeType(mimeType);
    return canGetData(cf, pDataObj);
}

QVariant QLastResortMimes::convertToMime(const QString &mimeType, IDataObject *pDataObj, QVariant::Type preferredType) const
{
    Q_UNUSED(preferredType);
    QVariant val;
    if (canConvertToMime(mimeType, pDataObj)) {
        QByteArray data;
        if (isCustomMimeType(mimeType)) {
            int lindex;
            QString clipFormat = customMimeType(mimeType, &lindex);
            const UINT cf = RegisterClipboardFormat(reinterpret_cast<const wchar_t *> (clipFormat.utf16()));
            data = getData(int(cf), pDataObj, lindex);
        } else {
            const auto mit = std::find(formats.cbegin(), formats.cend(), mimeType);
            const int cf = mit != formats.cend() ? mit.key() : QWindowsMime::registerMimeType(mimeType);
            data = getData(cf, pDataObj);
        }
        if (!data.isEmpty())
            val = data; // it should be enough to return the data and let QMimeData do the rest.
    }
    return val;
}

QString QLastResortMimes::mimeForFormat(const FORMATETC &formatetc) const
{
    QString format = formats.value(getCf(formatetc));
    if (!format.isEmpty())
        return format;

    const QString clipFormat = QWindowsMimeConverter::clipboardFormatName(getCf(formatetc));
    if (!clipFormat.isEmpty()) {
#if QT_CONFIG(draganddrop)
        if (QInternalMimeData::canReadData(clipFormat))
            format = clipFormat;
        else if((formatetc.cfFormat >= 0xC000)){
            //create the mime as custom. not registered.
            if (!excludeList.contains(clipFormat, Qt::CaseInsensitive)) {
                //check if this is a mime type
                bool ianaType = false;
                int sz = ianaTypes.size();
                for (int i = 0; i < sz; i++) {
                    if (clipFormat.startsWith(ianaTypes[i], Qt::CaseInsensitive)) {
                        ianaType =  true;
                        break;
                    }
                }
                if (!ianaType)
                    format = QLatin1String(x_qt_windows_mime) + clipFormat + u'"';
                else
                    format = clipFormat;
            }
        }
#endif // QT_CONFIG(draganddrop)
    }

    return format;
}

/*!
    \class QWindowsMimeConverter
    \brief Manages the list of QWindowsMime instances.
    \internal
    \sa QWindowsMime
*/

QWindowsMimeConverter::QWindowsMimeConverter() = default;

QWindowsMimeConverter::~QWindowsMimeConverter()
{
    qDeleteAll(m_mimes.begin(), m_mimes.begin() + m_internalMimeCount);
}

QWindowsMime * QWindowsMimeConverter::converterToMime(const QString &mimeType, IDataObject *pDataObj) const
{
    ensureInitialized();
    for (int i = m_mimes.size()-1; i >= 0; --i) {
        if (m_mimes.at(i)->canConvertToMime(mimeType, pDataObj))
            return m_mimes.at(i);
    }
    return nullptr;
}

QStringList QWindowsMimeConverter::allMimesForFormats(IDataObject *pDataObj) const
{
    qCDebug(lcQpaMime) << "QWindowsMime::allMimesForFormats()";
    ensureInitialized();
    QStringList formats;
    LPENUMFORMATETC FAR fmtenum;
    HRESULT hr = pDataObj->EnumFormatEtc(DATADIR_GET, &fmtenum);

    if (hr == NOERROR) {
        FORMATETC fmtetc;
        while (S_OK == fmtenum->Next(1, &fmtetc, nullptr)) {
            for (int i= m_mimes.size() - 1; i >= 0; --i) {
                QString format = m_mimes.at(i)->mimeForFormat(fmtetc);
                if (!format.isEmpty() && !formats.contains(format)) {
                    formats += format;
                    if (QWindowsContext::verbose > 1 && lcQpaMime().isDebugEnabled())
                        qCDebug(lcQpaMime) << __FUNCTION__ << fmtetc << format;
                }
            }
            // as documented in MSDN to avoid possible memleak
            if (fmtetc.ptd)
                CoTaskMemFree(fmtetc.ptd);
        }
        fmtenum->Release();
    }
    qCDebug(lcQpaMime) << pDataObj << formats;
    return formats;
}

QWindowsMime * QWindowsMimeConverter::converterFromMime(const FORMATETC &formatetc, const QMimeData *mimeData) const
{
    ensureInitialized();
    qCDebug(lcQpaMime) << __FUNCTION__ << formatetc;
    for (int i = m_mimes.size()-1; i >= 0; --i) {
        if (m_mimes.at(i)->canConvertFromMime(formatetc, mimeData))
            return m_mimes.at(i);
    }
    return nullptr;
}

QVector<FORMATETC> QWindowsMimeConverter::allFormatsForMime(const QMimeData *mimeData) const
{
    ensureInitialized();
    QVector<FORMATETC> formatics;
#if !QT_CONFIG(draganddrop)
    Q_UNUSED(mimeData);
#else
    formatics.reserve(20);
    const QStringList formats = QInternalMimeData::formatsHelper(mimeData);
    for (int f = 0; f < formats.size(); ++f) {
        for (int i = m_mimes.size() - 1; i >= 0; --i)
            formatics += m_mimes.at(i)->formatsForMime(formats.at(f), mimeData);
    }
#endif // QT_CONFIG(draganddrop)
    return formatics;
}

void QWindowsMimeConverter::ensureInitialized() const
{
    if (m_mimes.isEmpty()) {
        m_mimes
#ifndef QT_NO_IMAGEFORMAT_BMP
                << new QWindowsMimeImage
#endif //QT_NO_IMAGEFORMAT_BMP
                << new QLastResortMimes
                << new QWindowsMimeText << new QWindowsMimeURI
                << new QWindowsMimeHtml << new QBuiltInMimes;
        m_internalMimeCount = m_mimes.size();
    }
}

QString QWindowsMimeConverter::clipboardFormatName(int cf)
{
    wchar_t buf[256] = {0};
    return GetClipboardFormatName(UINT(cf), buf, 255)
        ? QString::fromWCharArray(buf) : QString();
}

QVariant QWindowsMimeConverter::convertToMime(const QStringList &mimeTypes,
                                              IDataObject *pDataObj,
                                              QVariant::Type preferredType,
                                              QString *formatIn /* = 0 */) const
{
    for (const QString &format : mimeTypes) {
        if (const QWindowsMime *converter = converterToMime(format, pDataObj)) {
            if (converter->canConvertToMime(format, pDataObj)) {
                const QVariant dataV = converter->convertToMime(format, pDataObj, preferredType);
                if (dataV.isValid()) {
                    qCDebug(lcQpaMime) << __FUNCTION__ << mimeTypes << "\nFormat: "
                        << format << pDataObj << " returns " << dataV;
                    if (formatIn)
                        *formatIn = format;
                    return dataV;
                }
            }
        }
    }
    qCDebug(lcQpaMime) << __FUNCTION__ << "fails" << mimeTypes << pDataObj << preferredType;
    return QVariant();
}

void QWindowsMimeConverter::registerMime(QWindowsMime *mime)
{
    ensureInitialized();
    m_mimes.append(mime);
}

QT_END_NAMESPACE
