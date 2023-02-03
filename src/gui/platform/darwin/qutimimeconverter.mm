// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <ImageIO/ImageIO.h>
#include <CoreFoundation/CoreFoundation.h>

#include <QtCore/qsystemdetection.h>
#include <QtCore/qurl.h>
#include <QtGui/qimage.h>
#include <QtCore/qmimedata.h>
#include <QtCore/qstringconverter.h>

#if defined(Q_OS_MACOS)
#import <AppKit/AppKit.h>
#else
#include <MobileCoreServices/MobileCoreServices.h>
#endif

#if defined(QT_PLATFORM_UIKIT)
#import <UIKit/UIKit.h>
#endif

#include "qutimimeconverter.h"
#include "qmacmimeregistry_p.h"
#include "qguiapplication.h"
#include "private/qcore_mac_p.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*****************************************************************************
  QDnD debug facilities
 *****************************************************************************/
//#define DEBUG_MIME_MAPS

/*!
    \class QUtiMimeConverter
    \brief The QUtiMimeConverter class converts between a MIME type and a
    \l{https://developer.apple.com/documentation/uniformtypeidentifiers}
        {Uniform Type Identifier (UTI)} format.
    \since 6.5

    \ingroup draganddrop
    \inmodule QtGui

    Qt's drag and drop and clipboard facilities use the MIME
    standard. On X11, this maps trivially to the Xdnd protocol. On
    Mac, although some applications use MIME to describe clipboard
    contents, it is more common to use Apple's UTI format.

    QUtiMimeConverter's role is to bridge the gap between MIME and UTI;
    By subclasses this class, one can extend Qt's drag and drop
    and clipboard handling to convert to and from unsupported, or proprietary, UTI formats.

    A subclass of QUtiMimeConverter will automatically be registered, and active, upon instantiation.

    Qt has predefined support for the following UTIs:
    \list
        \li public.utf8-plain-text - converts to "text/plain"
        \li public.utf16-plain-text - converts to "text/plain"
        \li public.text - converts to "text/plain"
        \li public.html - converts to "text/html"
        \li public.url - converts to "text/uri-list"
        \li public.file-url - converts to "text/uri-list"
        \li public.tiff - converts to "application/x-qt-image"
        \li public.vcard - converts to "text/plain"
        \li com.apple.traditional-mac-plain-text - converts to "text/plain"
        \li com.apple.pict - converts to "application/x-qt-image"
    \endlist

    When working with MIME data, Qt will iterate through all instances of QUtiMimeConverter to find
    find an instance that can convert to, or from, a specific MIME type. It will do this by calling
    mimeForUti() or utiForMime() on each instance, starting with (and choosing) the last created
    instance first. The actual conversions will be done by using convertToMime() and convertFromMime().
*/

/*!
    \enum QUtiMimeConverter::HandlerScope
    \internal
*/

/*!
    \internal
    Constructs a new conversion object of type \a scope, adding it to the
    globally accessed list of available converters.
*/
QUtiMimeConverter::QUtiMimeConverter(HandlerScope scope)
    : m_scope(scope)
{
    QMacMimeRegistry::registerMimeConverter(this);
}

/*!
    Constructs a new conversion object and adds it to the
    globally accessed list of available converters.
*/
QUtiMimeConverter::QUtiMimeConverter()
    : QUtiMimeConverter(HandlerScopeFlag::All)
{
}

/*!
    Destroys a conversion object, removing it from the global
    list of available converters.
*/
QUtiMimeConverter::~QUtiMimeConverter()
{
    QMacMimeRegistry::unregisterMimeConverter(this);
}

/*!
    Returns the item count for the given \a mimeData
*/
int QUtiMimeConverter::count(const QMimeData *mimeData) const
{
    Q_UNUSED(mimeData);
    return 1;
}

/*!
    \fn bool QUtiMimeConverter::canConvert(const QString &mime, const QString &uti) const

    Returns \c true if the converter can convert (both ways) between
    \a mime and \a uti; otherwise returns \c false.
*/

/*!
    \fn QString QUtiMimeConverter::mimeForUti(const QString &uti) const

    Returns the MIME type used for Mac UTI \a uti, or an empty string if
    this converter does not support converting from \a uti.

    All subclasses must reimplement this pure virtual function.
*/

/*!
    \fn QString QUtiMimeConverter::utiForMime(const QString &mime) const

    Returns the Mac UTI used for MIME type \a mime, or an empty string if
    this converter does not support converting from \a mime.

    All subclasses must reimplement this pure virtual function.
*/

/*!
    \fn QVariant QUtiMimeConverter::convertToMime(const QString &mime,
                                         const QList<QByteArray> &data, const QString &uti) const

    Returns \a data converted from Mac UTI \a uti to MIME type \a mime.

    Note that Mac UTIs must all be self-terminating. The input \a data
    may contain trailing data.

    All subclasses must reimplement this pure virtual function.
*/

/*!
    \fn QList<QByteArray> QUtiMimeConverter::convertFromMime(const QString &mime,
                                                    const QVariant &data, const QString & uti) const

    Returns \a data converted from MIME type \a mime to Mac UTI \a uti.

    Note that Mac UTIs must all be self-terminating.  The return
    value may contain trailing data.

    All subclasses must reimplement this pure virtual function.
*/


class QMacMimeAny : public QUtiMimeConverter {
public:
    QMacMimeAny() : QUtiMimeConverter(HandlerScopeFlag::AllCompatible) {}

    QString utiForMime(const QString &mime) const override;
    QString mimeForUti(const QString &uti) const override;
    QVariant convertToMime(const QString &mime, const QList<QByteArray> &data,
                           const QString &uti) const override;
    QList<QByteArray> convertFromMime(const QString &mime, const QVariant &data,
                           const QString &uti) const override;
};

QString QMacMimeAny::utiForMime(const QString &mime) const
{
    // do not handle the mime type name in the drag pasteboard
    if (mime == "application/x-qt-mime-type-name"_L1)
        return QString();
    QString ret = "com.trolltech.anymime."_L1 + mime;
    return ret.replace(u'/', "--"_L1);
}

QString QMacMimeAny::mimeForUti(const QString &uti) const
{
    const QString any_prefix = "com.trolltech.anymime."_L1;
    if (uti.size() > any_prefix.length() && uti.startsWith(any_prefix))
        return uti.mid(any_prefix.length()).replace("--"_L1, "/"_L1);
    return QString();
}

QVariant QMacMimeAny::convertToMime(const QString &mime, const QList<QByteArray> &data,
                                    const QString &) const
{
    if (data.count() > 1)
        qWarning("QMacMimeAny: Cannot handle multiple member data");
    QVariant ret;
    if (mime == "text/plain"_L1)
        ret = QString::fromUtf8(data.first());
    else
        ret = data.first();
    return ret;
}

QList<QByteArray> QMacMimeAny::convertFromMime(const QString &mime, const QVariant &data,
                                               const QString &) const
{
    QList<QByteArray> ret;
    if (mime == "text/plain"_L1)
        ret.append(data.toString().toUtf8());
    else
        ret.append(data.toByteArray());
    return ret;
}

class QMacMimeTypeName : public QUtiMimeConverter {
private:

public:
    QMacMimeTypeName(): QUtiMimeConverter(HandlerScopeFlag::AllCompatible) {}

    QString utiForMime(const QString &mime) const override;
    QString mimeForUti(const QString &uti) const override;
    QVariant convertToMime(const QString &mime, const QList<QByteArray> &data, const QString &uti) const override;
    QList<QByteArray> convertFromMime(const QString &mime, const QVariant &data, const QString &uti) const override;
};

QString QMacMimeTypeName::utiForMime(const QString &mime) const
{
    if (mime == "application/x-qt-mime-type-name"_L1)
        return u"com.trolltech.qt.MimeTypeName"_s;
    return QString();
}

QString QMacMimeTypeName::mimeForUti(const QString &) const
{
    return QString();
}

QVariant QMacMimeTypeName::convertToMime(const QString &, const QList<QByteArray> &, const QString &) const
{
    QVariant ret;
    return ret;
}

QList<QByteArray> QMacMimeTypeName::convertFromMime(const QString &, const QVariant &, const QString &) const
{
    QList<QByteArray> ret;
    ret.append(QString("x-qt-mime-type-name"_L1).toUtf8());
    return ret;
}

class QMacMimePlainTextFallback : public QUtiMimeConverter
{
public:
    QString utiForMime(const QString &mime) const override;
    QString mimeForUti(const QString &uti) const override;
    QVariant convertToMime(const QString &mime, const QList<QByteArray> &data,
                           const QString &uti) const override;
    QList<QByteArray> convertFromMime(const QString &mime, const QVariant &data,
                           const QString &uti) const override;
};

QString QMacMimePlainTextFallback::utiForMime(const QString &mime) const
{
    if (mime == "text/plain"_L1)
        return "public.text"_L1;
    return QString();
}

QString QMacMimePlainTextFallback::mimeForUti(const QString &uti) const
{
    if (uti == "public.text"_L1)
        return "text/plain"_L1;
    return QString();
}

QVariant
QMacMimePlainTextFallback::convertToMime(const QString &mimetype,
                                         const QList<QByteArray> &data, const QString &uti) const
{
    if (data.count() > 1)
        qWarning("QMacMimePlainTextFallback: Cannot handle multiple member data");

    if (uti == "public.text"_L1) {
        // Note that public.text is documented by Apple to have an undefined encoding. From
        // testing it seems that utf8 is normally used, at least by Safari on iOS.
        const QByteArray &firstData = data.first();
        return QString(QCFString(CFStringCreateWithBytes(kCFAllocatorDefault,
                                             reinterpret_cast<const UInt8 *>(firstData.constData()),
                                             firstData.size(), kCFStringEncodingUTF8, false)));
    } else {
        qWarning("QMime::convertToMime: unhandled mimetype: %s", qPrintable(mimetype));
    }
    return QVariant();
}

QList<QByteArray>
QMacMimePlainTextFallback::convertFromMime(const QString &, const QVariant &data,
                                           const QString &uti) const
{
    QList<QByteArray> ret;
    QString string = data.toString();
    if (uti == "public.text"_L1)
        ret.append(string.toUtf8());
    return ret;
}

class QMacMimeUnicodeText : public QUtiMimeConverter
{
public:
    QString utiForMime(const QString &mime) const override;
    QString mimeForUti(const QString &uti) const override;
    QVariant convertToMime(const QString &mime, const QList<QByteArray> &data,
                           const QString &uti) const override;
    QList<QByteArray> convertFromMime(const QString &mime, const QVariant &data,
                                      const QString &uti) const override;
};

QString QMacMimeUnicodeText::utiForMime(const QString &mime) const
{
    if (mime == "text/plain"_L1)
        return "public.utf16-plain-text"_L1;
    if (qsizetype i = mime.indexOf("charset="_L1); i >= 0) {
        QString cs(mime.mid(i + 8).toLower());
        i = cs.indexOf(u';');
        if (i >= 0)
            cs = cs.left(i);
        if (cs == "system"_L1)
            return "public.utf8-plain-text"_L1;
        else if (cs == "iso-10646-ucs-2"_L1 || cs == "utf16"_L1)
            return "public.utf16-plain-text"_L1;
    }
    return QString();
}

QString QMacMimeUnicodeText::mimeForUti(const QString &uti) const
{
    if (uti == "public.utf16-plain-text"_L1 || uti == "public.utf8-plain-text"_L1)
        return "text/plain"_L1;
    return QString();
}

QVariant
QMacMimeUnicodeText::convertToMime(const QString &mimetype,
                                   const QList<QByteArray> &data, const QString &uti) const
{
    if (data.count() > 1)
        qWarning("QMacMimeUnicodeText: Cannot handle multiple member data");
    const QByteArray &firstData = data.first();
    // I can only handle two types (system and unicode) so deal with them that way
    QVariant ret;
    if (uti == "public.utf8-plain-text"_L1) {
        ret = QString::fromUtf8(firstData);
    } else if (uti == "public.utf16-plain-text"_L1) {
        QString str = QStringDecoder(QStringDecoder::Utf16)(firstData);
        ret = str;
    } else {
        qWarning("QMime::convertToMime: unhandled mimetype: %s", qPrintable(mimetype));
    }
    return ret;
}

QList<QByteArray>
QMacMimeUnicodeText::convertFromMime(const QString &, const QVariant &data,
                                     const QString &uti) const
{
    QList<QByteArray> ret;
    QString string = data.toString();
    if (uti == "public.utf8-plain-text"_L1)
        ret.append(string.toUtf8());
    else if (uti == "public.utf16-plain-text"_L1) {
        QStringEncoder::Flags f;
#if defined(Q_OS_MACOS)
        // Some applications such as Microsoft Excel, don't deal well with
        // a BOM present, so we follow the traditional approach of Qt on
        // macOS to not generate public.utf16-plain-text with a BOM.
        f = QStringEncoder::Flag::Default;
#else
        // Whereas iOS applications will fail to paste if we do _not_
        // include a BOM in the public.utf16-plain-text content, most
        // likely due to converting the data using NSUTF16StringEncoding
        // which assumes big-endian byte order if there is no BOM.
        f = QStringEncoder::Flag::WriteBom;
#endif
        QStringEncoder encoder(QStringEncoder::Utf16, f);
        ret.append(encoder(string));
    }
    return ret;
}

class QMacMimeHTMLText : public QUtiMimeConverter
{
public:
    QString utiForMime(const QString &mime) const override;
    QString mimeForUti(const QString &uti) const override;
    QVariant convertToMime(const QString &mime, const QList<QByteArray> &data,
                           const QString &uti) const override;
    QList<QByteArray> convertFromMime(const QString &mime, const QVariant &data,
                                      const QString &uti) const override;
};

QString QMacMimeHTMLText::utiForMime(const QString &mime) const
{
    if (mime == "text/html"_L1)
        return "public.html"_L1;
    return QString();
}

QString QMacMimeHTMLText::mimeForUti(const QString &uti) const
{
    if (uti == "public.html"_L1)
        return "text/html"_L1;
    return QString();
}

QVariant
QMacMimeHTMLText::convertToMime(const QString &mimeType,
                                const QList<QByteArray> &data, const QString &uti) const
{
    if (!canConvert(mimeType, uti))
        return QVariant();
    if (data.count() > 1)
        qWarning("QMacMimeHTMLText: Cannot handle multiple member data");
    return data.first();
}

QList<QByteArray>
QMacMimeHTMLText::convertFromMime(const QString &mime,
                                  const QVariant &data, const QString &uti) const
{
    QList<QByteArray> ret;
    if (!canConvert(mime, uti))
        return ret;
    ret.append(data.toByteArray());
    return ret;
}

class QMacMimeRtfText : public QUtiMimeConverter
{
public:
    QString utiForMime(const QString &mime) const override;
    QString mimeForUti(const QString &uti) const override;
    QVariant convertToMime(const QString &mime, const QList<QByteArray> &data,
                           const QString &uti) const override;
    QList<QByteArray> convertFromMime(const QString &mime, const QVariant &data,
                           const QString &uti) const override;
};

QString QMacMimeRtfText::utiForMime(const QString &mime) const
{
    if (mime == "text/html"_L1)
        return "public.rtf"_L1;
    return QString();
}

QString QMacMimeRtfText::mimeForUti(const QString &uti) const
{
    if (uti == "public.rtf"_L1)
        return "text/html"_L1;
    return QString();
}

QVariant
QMacMimeRtfText::convertToMime(const QString &mimeType,
                               const QList<QByteArray> &data, const QString &uti) const
{
    if (!canConvert(mimeType, uti))
        return QVariant();
    if (data.count() > 1)
        qWarning("QMacMimeHTMLText: Cannot handle multiple member data");

    // Read RTF into to NSAttributedString, then convert the string to HTML
    NSAttributedString *string = [[NSAttributedString alloc] initWithData:data.at(0).toNSData()
            options:@{NSDocumentTypeDocumentAttribute: NSRTFTextDocumentType}
            documentAttributes:nil
            error:nil];

    NSError *error;
    NSRange range = NSMakeRange(0, [string length]);
    NSDictionary *dict = @{NSDocumentTypeDocumentAttribute: NSHTMLTextDocumentType};
    NSData *htmlData = [string dataFromRange:range documentAttributes:dict error:&error];
    return QByteArray::fromNSData(htmlData);
}

QList<QByteArray>
QMacMimeRtfText::convertFromMime(const QString &mime,
                                 const QVariant &data, const QString &uti) const
{
    QList<QByteArray> ret;
    if (!canConvert(mime, uti))
        return ret;

    NSAttributedString *string = [[NSAttributedString alloc] initWithData:data.toByteArray().toNSData()
            options:@{NSDocumentTypeDocumentAttribute: NSHTMLTextDocumentType}
            documentAttributes:nil
            error:nil];

    NSError *error;
    NSRange range = NSMakeRange(0, [string length]);
    NSDictionary *dict = @{NSDocumentTypeDocumentAttribute: NSRTFTextDocumentType};
    NSData *rtfData = [string dataFromRange:range documentAttributes:dict error:&error];
    ret << QByteArray::fromNSData(rtfData);
    return ret;
}

class QMacMimeFileUri : public QUtiMimeConverter
{
public:
    QString utiForMime(const QString &mime) const override;
    QString mimeForUti(const QString &uti) const override;
    QVariant convertToMime(const QString &mime, const QList<QByteArray> &data,
                           const QString &uti) const override;
    QList<QByteArray> convertFromMime(const QString &mime, const QVariant &data,
                                      const QString &uti) const override;
    int count(const QMimeData *mimeData) const override;
};

QString QMacMimeFileUri::utiForMime(const QString &mime) const
{
    if (mime == "text/uri-list"_L1)
        return "public.file-url"_L1;
    return QString();
}

QString QMacMimeFileUri::mimeForUti(const QString &uti) const
{
    if (uti == "public.file-url"_L1)
        return "text/uri-list"_L1;
    return QString();
}

QVariant
QMacMimeFileUri::convertToMime(const QString &mime,
                               const QList<QByteArray> &data, const QString &uti) const
{
    if (!canConvert(mime, uti))
        return QVariant();
    QList<QVariant> ret;
    for (int i = 0; i < data.size(); ++i) {
        const QByteArray &a = data.at(i);
        NSString *urlString = [[[NSString alloc] initWithBytesNoCopy:(void *)a.data() length:a.size()
                                                 encoding:NSUTF8StringEncoding freeWhenDone:NO] autorelease];
        NSURL *nsurl = [NSURL URLWithString:urlString];
        QUrl url;
        // OS X 10.10 sends file references instead of file paths
        if ([nsurl isFileReferenceURL]) {
            url = QUrl::fromNSURL([nsurl filePathURL]);
        } else {
            url = QUrl::fromNSURL(nsurl);
        }

        if (url.host().toLower() == "localhost"_L1)
            url.setHost(QString());

        url.setPath(url.path().normalized(QString::NormalizationForm_C));
        ret.append(url);
    }
    return QVariant(ret);
}

QList<QByteArray>
QMacMimeFileUri::convertFromMime(const QString &mime,
                                 const QVariant &data, const QString &uti) const
{
    QList<QByteArray> ret;
    if (!canConvert(mime, uti))
        return ret;
    QList<QVariant> urls = data.toList();
    for (int i = 0; i < urls.size(); ++i) {
        QUrl url = urls.at(i).toUrl();
        if (url.scheme().isEmpty())
            url.setScheme("file"_L1);
        if (url.scheme() == "file"_L1) {
            if (url.host().isEmpty())
                url.setHost("localhost"_L1);
            url.setPath(url.path().normalized(QString::NormalizationForm_D));
        }
        if (url.isLocalFile())
            ret.append(url.toEncoded());
    }
    return ret;
}

int QMacMimeFileUri::count(const QMimeData *mimeData) const
{
    return mimeData->urls().count();
}

class QMacMimeUrl : public QUtiMimeConverter
{
public:
    QString utiForMime(const QString &mime) const override;
    QString mimeForUti(const QString &uti) const override;
    QVariant convertToMime(const QString &mime, const QList<QByteArray> &data,
                           const QString &uti) const override;
    QList<QByteArray> convertFromMime(const QString &mime, const QVariant &data,
                                      const QString &uti) const override;
};

QString QMacMimeUrl::utiForMime(const QString &mime) const
{
    if (mime.startsWith("text/uri-list"_L1))
        return "public.url"_L1;
    return QString();
}

QString QMacMimeUrl::mimeForUti(const QString &uti) const
{
    if (uti == "public.url"_L1)
        return "text/uri-list"_L1;
    return QString();
}

QVariant QMacMimeUrl::convertToMime(const QString &mime,
                                    const QList<QByteArray> &data, const QString &uti) const
{
    if (!canConvert(mime, uti))
        return QVariant();

    QList<QVariant> ret;
    for (int i=0; i<data.size(); ++i) {
        QUrl url = QUrl::fromEncoded(data.at(i));
        if (url.host().toLower() == "localhost"_L1)
            url.setHost(QString());
        url.setPath(url.path().normalized(QString::NormalizationForm_C));
        ret.append(url);
    }
    return QVariant(ret);
}

QList<QByteArray> QMacMimeUrl::convertFromMime(const QString &mime,
                                               const QVariant &data, const QString &uti) const
{
    QList<QByteArray> ret;
    if (!canConvert(mime, uti))
        return ret;

    QList<QVariant> urls = data.toList();
    for (int i=0; i<urls.size(); ++i) {
        QUrl url = urls.at(i).toUrl();
        if (url.scheme().isEmpty())
            url.setScheme("file"_L1);
        if (url.scheme() == "file"_L1) {
            if (url.host().isEmpty())
                url.setHost("localhost"_L1);
            url.setPath(url.path().normalized(QString::NormalizationForm_D));
        }
        ret.append(url.toEncoded());
    }
    return ret;
}

class QMacMimeVCard : public QUtiMimeConverter
{
public:
    QString utiForMime(const QString &mime) const override;
    QString mimeForUti(const QString &uti) const override;
    QVariant convertToMime(const QString &mime, const QList<QByteArray> &data,
                           const QString &uti) const override;
    QList<QByteArray> convertFromMime(const QString &mime, const QVariant &data,
                                      const QString &uti) const override;
};

QString QMacMimeVCard::utiForMime(const QString &mime) const
{
    if (mime.startsWith("text/vcard"_L1))
        return "public.vcard"_L1;
    return QString();
}

QString QMacMimeVCard::mimeForUti(const QString &uti) const
{
    if (uti == "public.vcard"_L1)
        return "text/vcard"_L1;
    return QString();
}

QVariant QMacMimeVCard::convertToMime(const QString &mime,
                                      const QList<QByteArray> &data, const QString &uti) const
{
    if (!canConvert(mime, uti))
        return QVariant();
    QByteArray cards;
    if (uti == "public.vcard"_L1) {
        for (int i=0; i<data.size(); ++i)
            cards += data[i];
    }
    return QVariant(cards);
}

QList<QByteArray> QMacMimeVCard::convertFromMime(const QString &mime,
                                                 const QVariant &data, const QString &uti) const
{
    QList<QByteArray> ret;
    if (!canConvert(mime, uti))
        return ret;

    if (mime == "text/vcard"_L1)
        ret.append(data.toString().toUtf8());
    return ret;
}

extern QImage qt_mac_toQImage(CGImageRef image);
extern CGImageRef qt_mac_toCGImage(const QImage &qImage);

class QMacMimeTiff : public QUtiMimeConverter
{
public:
    QString utiForMime(const QString &mime) const override;
    QString mimeForUti(const QString &uti) const override;
    QVariant convertToMime(const QString &mime, const QList<QByteArray> &data,
                           const QString &uti) const override;
    QList<QByteArray> convertFromMime(const QString &mime, const QVariant &data,
                                      const QString &uti) const override;
};

QString QMacMimeTiff::utiForMime(const QString &mime) const
{
    if (mime.startsWith("application/x-qt-image"_L1))
        return "public.tiff"_L1;
    return QString();
}

QString QMacMimeTiff::mimeForUti(const QString &uti) const
{
    if (uti == "public.tiff"_L1)
        return "application/x-qt-image"_L1;
    return QString();
}

QVariant QMacMimeTiff::convertToMime(const QString &mime,
                                     const QList<QByteArray> &data, const QString &uti) const
{
    if (data.count() > 1)
        qWarning("QMacMimeTiff: Cannot handle multiple member data");

    if (!canConvert(mime, uti))
        return QVariant();

    QCFType<CFDataRef> tiffData = data.first().toRawCFData();
    QCFType<CGImageSourceRef> imageSource = CGImageSourceCreateWithData(tiffData, 0);

    if (QCFType<CGImageRef> image = CGImageSourceCreateImageAtIndex(imageSource, 0, 0))
        return QVariant(qt_mac_toQImage(image));

    return QVariant();
}

QList<QByteArray> QMacMimeTiff::convertFromMime(const QString &mime,
                                                const QVariant &variant, const QString &uti) const
{
    if (!canConvert(mime, uti))
        return QList<QByteArray>();

    QCFType<CFMutableDataRef> data = CFDataCreateMutable(0, 0);
    QCFType<CGImageDestinationRef> imageDestination = CGImageDestinationCreateWithData(data,
                                                                        kUTTypeTIFF, 1, 0);

    if (!imageDestination)
        return QList<QByteArray>();

    QImage img = qvariant_cast<QImage>(variant);
    NSDictionary *props = @{
        static_cast<NSString *>(kCGImagePropertyPixelWidth): @(img.width()),
        static_cast<NSString *>(kCGImagePropertyPixelHeight): @(img.height())
    };

    CGImageDestinationAddImage(imageDestination, qt_mac_toCGImage(img),
                               static_cast<CFDictionaryRef>(props));
    CGImageDestinationFinalize(imageDestination);

    return QList<QByteArray>() << QByteArray::fromCFData(data);
}

namespace QMacMimeRegistry {

void registerBuiltInTypes()
{
    // Create QMacMimeAny first to put it at the end of globalMimeList
    // with lowest priority. (the constructor prepends to the list)
    new QMacMimeAny;

    //standard types that we wrap
    new QMacMimeTiff;
    new QMacMimePlainTextFallback;
    new QMacMimeUnicodeText;
    new QMacMimeRtfText;
    new QMacMimeHTMLText;
    new QMacMimeFileUri;
    new QMacMimeUrl;
    new QMacMimeTypeName;
    new QMacMimeVCard;
}

}

QT_END_NAMESPACE
