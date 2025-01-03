// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsmimeregistry.h"
#include "qwindowscontext.h"

#include <QtGui/private/qinternalmimedata_p.h>
#include <QtCore/qbytearraymatcher.h>
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

using namespace Qt::StringLiterals;

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

static inline bool readDib(QBuffer &buffer, QImage &img)
{
    QImageReader reader(&buffer, dibFormatC);
    if (!reader.canRead()) {
        qWarning("%s", msgConversionError(__FUNCTION__, dibFormatC).constData());
        return false;
    }
    img = reader.read();
    return true;
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
    qsizetype bpl_bmp = qsizetype(image.width()) * 4;
    qsizetype size = bpl_bmp * image.height();
    if (qsizetype(DWORD(size)) != size)
        return false;

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

    d->write(reinterpret_cast<const char *>(&bi.bV5RedMask), sizeof(bi.bV5RedMask));
    if (s.status() != QDataStream::Ok)
        return false;

    d->write(reinterpret_cast<const char *>(&bi.bV5GreenMask), sizeof(bi.bV5GreenMask));
    if (s.status() != QDataStream::Ok)
        return false;

    d->write(reinterpret_cast<const char *>(&bi.bV5BlueMask), sizeof(bi.bV5BlueMask));
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
        d << QWindowsMimeRegistry::clipboardFormatName(tc.cfFormat);
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

class QWindowsMimeText : public QWindowsMimeConverter
{
public:
    bool canConvertToMime(const QString &mimeType, IDataObject *pDataObj) const override;
    QVariant convertToMime(const QString &mime, LPDATAOBJECT pDataObj, QMetaType preferredType) const override;
    QString mimeForFormat(const FORMATETC &formatetc) const override;
    bool canConvertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData) const override;
    bool convertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData, STGMEDIUM *pmedium) const override;
    QList<FORMATETC> formatsForMime(const QString &mimeType, const QMimeData *mimeData) const override;
};

bool QWindowsMimeText::canConvertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData) const
{
    int cf = getCf(formatetc);
    return (cf == CF_UNICODETEXT || (cf == CF_TEXT && GetACP() != CP_UTF8)) && mimeData->hasText();
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
        return u"text/plain"_s;
    return QString();
}


QList<FORMATETC> QWindowsMimeText::formatsForMime(const QString &mimeType, const QMimeData *mimeData) const
{
    QList<FORMATETC> formatics;
    if (mimeType.startsWith(u"text/plain") && mimeData->hasText()) {
        formatics += setCf(CF_UNICODETEXT);
        if (GetACP() != CP_UTF8)
            formatics += setCf(CF_TEXT);
    }
    return formatics;
}

QVariant QWindowsMimeText::convertToMime(const QString &mime, LPDATAOBJECT pDataObj, QMetaType preferredType) const
{
    QVariant ret;

    if (canConvertToMime(mime, pDataObj)) {
        QString str;
        QByteArray data = getData(CF_UNICODETEXT, pDataObj);
        if (!data.isEmpty()) {
            str = QString::fromWCharArray(reinterpret_cast<const wchar_t *>(data.constData()));
            str.replace("\r\n"_L1, "\n"_L1);
        } else {
            data = getData(CF_TEXT, pDataObj);
            if (!data.isEmpty()) {
                const char* d = data.data();
                const unsigned s = unsigned(qstrlen(d));
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
        if (preferredType.id() == QMetaType::QString)
            ret = str;
        else
            ret = std::move(str).toUtf8();
    }
    qCDebug(lcQpaMime) << __FUNCTION__ << ret;
    return ret;
}

class QWindowsMimeURI : public QWindowsMimeConverter
{
public:
    QWindowsMimeURI();
    bool canConvertToMime(const QString &mimeType, IDataObject *pDataObj) const override;
    QVariant convertToMime(const QString &mime, LPDATAOBJECT pDataObj, QMetaType preferredType) const override;
    QString mimeForFormat(const FORMATETC &formatetc) const override;
    bool canConvertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData) const override;
    bool convertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData, STGMEDIUM *pmedium) const override;
    QList<FORMATETC> formatsForMime(const QString &mimeType, const QMimeData *mimeData) const override;
private:
    int CF_INETURL_W; // wide char version
    int CF_INETURL;
};

QWindowsMimeURI::QWindowsMimeURI()
{
    CF_INETURL_W = registerMimeType(u"UniformResourceLocatorW"_s);
    CF_INETURL = registerMimeType(u"UniformResourceLocator"_s);
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
            size_t size = sizeof(DROPFILES) + 2;
            for (const QUrl &url : urls) {
                const QString fn = QDir::toNativeSeparators(url.toLocalFile());
                if (!fn.isEmpty()) {
                    size += sizeof(ushort) * size_t(fn.length() + 1);
                    fileNames.append(fn);
                }
            }

            QByteArray result(int(size), '\0');
            auto* d = reinterpret_cast<DROPFILES *>(result.data());
            d->pFiles = sizeof(DROPFILES);
            GetCursorPos(&d->pt); // try
            d->fNC = true;
            char *files = (reinterpret_cast<char*>(d)) + d->pFiles;

            d->fWide = true;
            auto *f = reinterpret_cast<wchar_t *>(files);
            for (int i=0; i<fileNames.size(); i++) {
                const auto l = size_t(fileNames.at(i).length());
                memcpy(f, fileNames.at(i).data(), l * sizeof(ushort));
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
                const QString url = urls.at(0).toString();
                result = QByteArray(reinterpret_cast<const char *>(url.data()),
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
        format = u"text/uri-list"_s;
    return format;
}

QList<FORMATETC> QWindowsMimeURI::formatsForMime(const QString &mimeType, const QMimeData *mimeData) const
{
    QList<FORMATETC> formatics;
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

QVariant QWindowsMimeURI::convertToMime(const QString &mimeType, LPDATAOBJECT pDataObj, QMetaType preferredType) const
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

            if (preferredType.id() == QMetaType::QUrl && urls.size() == 1)
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

class QWindowsMimeHtml : public QWindowsMimeConverter
{
public:
    QWindowsMimeHtml();

    // for converting from Qt
    bool canConvertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData) const override;
    bool convertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData, STGMEDIUM * pmedium) const override;
    QList<FORMATETC> formatsForMime(const QString &mimeType, const QMimeData *mimeData) const override;

    // for converting to Qt
    bool canConvertToMime(const QString &mimeType, IDataObject *pDataObj) const override;
    QVariant convertToMime(const QString &mime, IDataObject *pDataObj, QMetaType preferredType) const override;
    QString mimeForFormat(const FORMATETC &formatetc) const override;

private:
    int CF_HTML;
};

QWindowsMimeHtml::QWindowsMimeHtml()
{
    CF_HTML = registerMimeType(u"HTML Format"_s);
}

QList<FORMATETC> QWindowsMimeHtml::formatsForMime(const QString &mimeType, const QMimeData *mimeData) const
{
    QList<FORMATETC> formatetcs;
    if (mimeType == u"text/html" && (!mimeData->html().isEmpty()))
        formatetcs += setCf(CF_HTML);
    return formatetcs;
}

QString QWindowsMimeHtml::mimeForFormat(const FORMATETC &formatetc) const
{
    if (getCf(formatetc) == CF_HTML)
        return u"text/html"_s;
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
QVariant QWindowsMimeHtml::convertToMime(const QString &mime, IDataObject *pDataObj, QMetaType preferredType) const
{
    Q_UNUSED(preferredType);
    QVariant result;
    if (canConvertToMime(mime, pDataObj)) {
        QByteArray html = getData(CF_HTML, pDataObj);
        static constexpr auto startMatcher = qMakeStaticByteArrayMatcher("StartHTML:");
        static constexpr auto endMatcher   = qMakeStaticByteArrayMatcher("EndHTML:");
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

        static constexpr auto startFragmentMatcher = qMakeStaticByteArrayMatcher("<!--StartFragment-->");
        static constexpr auto endFragmentMatcher   = qMakeStaticByteArrayMatcher("<!--EndFragment-->");

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
class QWindowsMimeImage : public QWindowsMimeConverter
{
public:
    QWindowsMimeImage();
    // for converting from Qt
    bool canConvertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData) const override;
    bool convertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData, STGMEDIUM * pmedium) const override;
    QList<FORMATETC> formatsForMime(const QString &mimeType, const QMimeData *mimeData) const override;

    // for converting to Qt
    bool canConvertToMime(const QString &mimeType, IDataObject *pDataObj) const override;
    QVariant convertToMime(const QString &mime, IDataObject *pDataObj, QMetaType preferredType) const override;
    QString mimeForFormat(const FORMATETC &formatetc) const override;
private:
    bool hasOriginalDIBV5(IDataObject *pDataObj) const;
    UINT CF_PNG;
};

QWindowsMimeImage::QWindowsMimeImage()
{
    CF_PNG = RegisterClipboardFormat(L"PNG");
}

QList<FORMATETC> QWindowsMimeImage::formatsForMime(const QString &mimeType, const QMimeData *mimeData) const
{
    QList<FORMATETC> formatetcs;
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
       return u"application/x-qt-image"_s;
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
                isSynthesized = false;
                break;
            }
        }
        pEnum->Release();
    }
    return !isSynthesized;
}

QVariant QWindowsMimeImage::convertToMime(const QString &mimeType, IDataObject *pDataObj, QMetaType preferredType) const
{
    Q_UNUSED(preferredType);
    QVariant result;
    if (mimeType != u"application/x-qt-image")
        return result;
    // Try to convert from DIBV5 as it is the most widespread format that supports transparency,
    // but avoid synthesizing it, as that typically loses transparency, e.g. from Office
    const bool canGetDibV5 = canGetData(CF_DIBV5, pDataObj);
    const bool hasOrigDibV5 = canGetDibV5 ? hasOriginalDIBV5(pDataObj) : false;
    qCDebug(lcQpaMime) << "canGetDibV5:" << canGetDibV5 << "hasOrigDibV5:" << hasOrigDibV5;
    if (hasOrigDibV5) {
        qCDebug(lcQpaMime) << "Decoding DIBV5";
        QImage img;
        QByteArray data = getData(CF_DIBV5, pDataObj);
        QBuffer buffer(&data);
        if (readDib(buffer, img))
            return img;
    }
    //PNG, MS Office place this (undocumented)
    if (canGetData(CF_PNG, pDataObj)) {
        qCDebug(lcQpaMime) << "Decoding PNG";
        QImage img;
        QByteArray data = getData(CF_PNG, pDataObj);
        if (img.loadFromData(data, "PNG")) {
            return img;
        }
    }
    //Fallback to DIB
    if (canGetData(CF_DIB, pDataObj)) {
        qCDebug(lcQpaMime) << "Decoding DIB";
        QImage img;
        QByteArray data = getData(CF_DIBV5, pDataObj);
        QBuffer buffer(&data);
        if (readDib(buffer, img))
            return img;
    }
    // Failed
    return result;
}
#endif

class QBuiltInMimes : public QWindowsMimeConverter
{
public:
    QBuiltInMimes();

    // for converting from Qt
    bool canConvertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData) const override;
    bool convertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData, STGMEDIUM * pmedium) const override;
    QList<FORMATETC> formatsForMime(const QString &mimeType, const QMimeData *mimeData) const override;

    // for converting to Qt
    bool canConvertToMime(const QString &mimeType, IDataObject *pDataObj) const override;
    QVariant convertToMime(const QString &mime, IDataObject *pDataObj, QMetaType preferredType) const override;
    QString mimeForFormat(const FORMATETC &formatetc) const override;

private:
    QMap<int, QString> outFormats;
    QMap<int, QString> inFormats;
};

QBuiltInMimes::QBuiltInMimes()
: QWindowsMimeConverter()
{
    outFormats.insert(registerMimeType(u"application/x-color"_s), u"application/x-color"_s);
    inFormats.insert(registerMimeType(u"application/x-color"_s), u"application/x-color"_s);
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

QList<FORMATETC> QBuiltInMimes::formatsForMime(const QString &mimeType, const QMimeData *mimeData) const
{
    QList<FORMATETC> formatetcs;
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

QVariant QBuiltInMimes::convertToMime(const QString &mimeType, IDataObject *pDataObj, QMetaType preferredType) const
{
    QVariant val;
    if (canConvertToMime(mimeType, pDataObj)) {
        QByteArray data = getData(inFormats.key(mimeType), pDataObj);
        if (!data.isEmpty()) {
            qCDebug(lcQpaMime) << __FUNCTION__;
            if (mimeType == u"text/html" && preferredType == QMetaType(QMetaType::QString)) {
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


class QLastResortMimes : public QWindowsMimeConverter
{
public:

    QLastResortMimes();
    // for converting from Qt
    bool canConvertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData) const override;
    bool convertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData, STGMEDIUM * pmedium) const override;
    QList<FORMATETC> formatsForMime(const QString &mimeType, const QMimeData *mimeData) const override;

    // for converting to Qt
    bool canConvertToMime(const QString &mimeType, IDataObject *pDataObj) const override;
    QVariant convertToMime(const QString &mime, IDataObject *pDataObj, QMetaType preferredType) const override;
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
        ianaTypes.append(u"application/"_s);
        ianaTypes.append(u"audio/"_s);
        ianaTypes.append(u"example/"_s);
        ianaTypes.append(u"image/"_s);
        ianaTypes.append(u"message/"_s);
        ianaTypes.append(u"model/"_s);
        ianaTypes.append(u"multipart/"_s);
        ianaTypes.append(u"text/"_s);
        ianaTypes.append(u"video/"_s);
    }
    //Types handled by other classes
    if (excludeList.isEmpty()) {
        excludeList.append(u"HTML Format"_s);
        excludeList.append(u"UniformResourceLocator"_s);
        excludeList.append(u"text/html"_s);
        excludeList.append(u"text/plain"_s);
        excludeList.append(u"text/uri-list"_s);
        excludeList.append(u"application/x-qt-image"_s);
        excludeList.append(u"application/x-color"_s);
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

QList<FORMATETC> QLastResortMimes::formatsForMime(const QString &mimeType, const QMimeData * /*mimeData*/) const
{
    QList<FORMATETC> formatetcs;
    auto mit = std::find(formats.begin(), formats.end(), mimeType);
    // register any other available formats
    if (mit == formats.end() && !excludeList.contains(mimeType, Qt::CaseInsensitive))
        mit = formats.insert(registerMimeType(mimeType), mimeType);
    if (mit != formats.end())
        formatetcs += setCf(mit.key());

    if (!formatetcs.isEmpty())
        qCDebug(lcQpaMime) << __FUNCTION__ << mimeType << formatetcs;
    return formatetcs;
}
static const char x_qt_windows_mime[] = "application/x-qt-windows-mime;value=\"";

static bool isCustomMimeType(const QString &mimeType)
{
    return mimeType.startsWith(QLatin1StringView(x_qt_windows_mime), Qt::CaseInsensitive);
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
            *lindex = QStringView{mimeType}.mid(indexStartPos, endPos == -1 ? endPos : endPos - indexStartPos).toInt();
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
    const int cf = mit != formats.cend() ? mit.key() : registerMimeType(mimeType);
    return canGetData(cf, pDataObj);
}

QVariant QLastResortMimes::convertToMime(const QString &mimeType, IDataObject *pDataObj, QMetaType preferredType) const
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
            const int cf = mit != formats.cend() ? mit.key() : registerMimeType(mimeType);
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

    const QString clipFormat = QWindowsMimeRegistry::clipboardFormatName(getCf(formatetc));
    if (!clipFormat.isEmpty()) {
#if QT_CONFIG(draganddrop)
        if (QInternalMimeData::canReadData(clipFormat))
            format = clipFormat;
        else if ((formatetc.cfFormat >= 0xC000)){
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
                    format = QLatin1StringView(x_qt_windows_mime) + clipFormat + u'"';
                else
                    format = clipFormat;
            }
        }
#endif // QT_CONFIG(draganddrop)
    }

    return format;
}

/*!
    \class QWindowsMimeRegistry
    \brief Manages the list of QWindowsMimeConverter instances.
    \internal
    \sa QWindowsMimeConverter
*/

QWindowsMimeRegistry::QWindowsMimeRegistry() = default;

QWindowsMimeRegistry::~QWindowsMimeRegistry()
{
    qDeleteAll(m_mimes.begin(), m_mimes.begin() + m_internalMimeCount);
}

QWindowsMimeRegistry::QWindowsMimeConverter *QWindowsMimeRegistry::converterToMime(const QString &mimeType, IDataObject *pDataObj) const
{
    ensureInitialized();
    for (int i = m_mimes.size()-1; i >= 0; --i) {
        if (m_mimes.at(i)->canConvertToMime(mimeType, pDataObj))
            return m_mimes.at(i);
    }
    return nullptr;
}

QStringList QWindowsMimeRegistry::allMimesForFormats(IDataObject *pDataObj) const
{
    qCDebug(lcQpaMime) << "QWindowsMimeConverter::allMimesForFormats()";
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

QWindowsMimeRegistry::QWindowsMimeConverter *QWindowsMimeRegistry::converterFromMime(const FORMATETC &formatetc, const QMimeData *mimeData) const
{
    ensureInitialized();
    qCDebug(lcQpaMime) << __FUNCTION__ << formatetc;
    for (int i = m_mimes.size()-1; i >= 0; --i) {
        if (m_mimes.at(i)->canConvertFromMime(formatetc, mimeData))
            return m_mimes.at(i);
    }
    return nullptr;
}

QList<FORMATETC> QWindowsMimeRegistry::allFormatsForMime(const QMimeData *mimeData) const
{
    ensureInitialized();
    QList<FORMATETC> formatics;
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

void QWindowsMimeRegistry::ensureInitialized() const
{
    if (m_internalMimeCount == 0) {
        m_internalMimeCount = -1; // prevent reentrancy when types register themselves
#ifndef QT_NO_IMAGEFORMAT_BMP
        (void)new QWindowsMimeImage;
#endif //QT_NO_IMAGEFORMAT_BMP
        (void)new QLastResortMimes;
        (void)new QWindowsMimeText;
        (void)new QWindowsMimeURI;
        (void)new QWindowsMimeHtml;
        (void)new QBuiltInMimes;
        m_internalMimeCount = m_mimes.size();
        Q_ASSERT(m_internalMimeCount > 0);
    }
}

QString QWindowsMimeRegistry::clipboardFormatName(int cf)
{
    wchar_t buf[256] = {0};
    return GetClipboardFormatName(UINT(cf), buf, 255)
        ? QString::fromWCharArray(buf) : QString();
}

QVariant QWindowsMimeRegistry::convertToMime(const QStringList &mimeTypes,
                                              IDataObject *pDataObj,
                                              QMetaType preferredType,
                                              QString *formatIn /* = 0 */) const
{
    for (const QString &format : mimeTypes) {
        if (const QWindowsMimeConverter *converter = converterToMime(format, pDataObj)) {
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
    qCDebug(lcQpaMime) << __FUNCTION__ << "fails" << mimeTypes << pDataObj << preferredType.id();
    return QVariant();
}

void QWindowsMimeRegistry::registerMime(QWindowsMimeConverter *mime)
{
    ensureInitialized();
    m_mimes.append(mime);
}

/*!
    Registers the MIME type \a mime, and returns an ID number
    identifying the format on Windows.

    A mime type \c {application/x-qt-windows-mime;value="WindowsType"} will be
    registered as the clipboard format for \c WindowsType.
*/
int QWindowsMimeRegistry::registerMimeType(const QString &mime)
{
    const QString mimeType = isCustomMimeType(mime) ? customMimeType(mime) : mime;
    const UINT f = RegisterClipboardFormat(reinterpret_cast<const wchar_t *> (mimeType.utf16()));
    if (!f) {
        qErrnoWarning("QWindowsMimeRegistry::registerMimeType: Failed to register clipboard format "
                      "for %s", qPrintable(mime));
    }

    return int(f);
}

QT_END_NAMESPACE
