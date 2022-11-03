// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <ImageIO/ImageIO.h>

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

#include "qmacmime_p.h"
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
  \class QMacInternalPasteboardMime
  \internal
  \brief The QMacPasteboardMime class converts between a MIME type and a
  \l{http://developer.apple.com/macosx/uniformtypeidentifiers.html}{Uniform
  Type Identifier (UTI)} format.
  \since 4.2

  \ingroup draganddrop
  \inmodule QtWidgets

  Qt's drag and drop and clipboard facilities use the MIME
  standard. On X11, this maps trivially to the Xdnd protocol. On
  Mac, although some applications use MIME to describe clipboard
  contents, it is more common to use Apple's UTI format.

  QMacPasteboardMime's role is to bridge the gap between MIME and UTI;
  By subclasses this class, one can extend Qt's drag and drop
  and clipboard handling to convert to and from unsupported, or proprietary, UTI formats.

  A subclass of QMacPasteboardMime will automatically be registered, and active, upon instantiation.

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

  When working with MIME data, Qt will iterate through all instances of QMacPasteboardMime to
  find an instance that can convert to, or from, a specific MIME type. It will do this by calling
  canConvert() on each instance, starting with (and choosing) the last created instance first.
  The actual conversions will be done by using convertToMime() and convertFromMime().

  \note The API uses the term "flavor" in some cases. This is for backwards
  compatibility reasons, and should now be understood as UTIs.
*/

/*
    \enum QMacPasteboardMime::QMacPasteboardMimeType
    \internal
*/

/*
  Constructs a new conversion object of type \a t, adding it to the
  globally accessed list of available converters.
*/
QMacInternalPasteboardMime::QMacInternalPasteboardMime(QMacPasteboardMimeType t)
    : m_type(t)
{
    QMacMimeRegistry::registerMimeConverter(this);
}

/*
  Destroys a conversion object, removing it from the global
  list of available converters.
*/
QMacInternalPasteboardMime::~QMacInternalPasteboardMime()
{
    QMacMimeRegistry::unregisterMimeConverter(this);
}

/*
  Returns the item count for the given \a mimeData
*/
int QMacInternalPasteboardMime::count(const QMimeData *mimeData) const
{
    Q_UNUSED(mimeData);
    return 1;
}

class QMacPasteboardMimeAny : public QMacInternalPasteboardMime {
public:
    QMacPasteboardMimeAny() : QMacInternalPasteboardMime(MIME_ALL_COMPATIBLE) {}

    QString flavorFor(const QString &mime) const override;
    QString mimeFor(const QString &flav) const override;
    bool canConvert(const QString &mime, const QString &flav) const override;
    QVariant convertToMime(const QString &mime, const QList<QByteArray> &data, const QString &flav) const override;
    QList<QByteArray> convertFromMime(const QString &mime, const QVariant &data, const QString &flav) const override;
};

QString QMacPasteboardMimeAny::flavorFor(const QString &mime) const
{
    // do not handle the mime type name in the drag pasteboard
    if (mime == "application/x-qt-mime-type-name"_L1)
        return QString();
    QString ret = "com.trolltech.anymime."_L1 + mime;
    return ret.replace(u'/', "--"_L1);
}

QString QMacPasteboardMimeAny::mimeFor(const QString &flav) const
{
    const QString any_prefix = "com.trolltech.anymime."_L1;
    if (flav.size() > any_prefix.length() && flav.startsWith(any_prefix))
        return flav.mid(any_prefix.length()).replace("--"_L1, "/"_L1);
    return QString();
}

bool QMacPasteboardMimeAny::canConvert(const QString &mime, const QString &flav) const
{
    return mimeFor(flav) == mime;
}

QVariant QMacPasteboardMimeAny::convertToMime(const QString &mime, const QList<QByteArray> &data, const QString &) const
{
    if (data.count() > 1)
        qWarning("QMacPasteboardMimeAny: Cannot handle multiple member data");
    QVariant ret;
    if (mime == "text/plain"_L1)
        ret = QString::fromUtf8(data.first());
    else
        ret = data.first();
    return ret;
}

QList<QByteArray> QMacPasteboardMimeAny::convertFromMime(const QString &mime, const QVariant &data, const QString &) const
{
    QList<QByteArray> ret;
    if (mime == "text/plain"_L1)
        ret.append(data.toString().toUtf8());
    else
        ret.append(data.toByteArray());
    return ret;
}

class QMacPasteboardMimeTypeName : public QMacInternalPasteboardMime {
private:

public:
    QMacPasteboardMimeTypeName(): QMacInternalPasteboardMime(MIME_ALL_COMPATIBLE) {}

    QString flavorFor(const QString &mime) const override;
    QString mimeFor(const QString &flav) const override;
    bool canConvert(const QString &mime, const QString &flav) const override;
    QVariant convertToMime(const QString &mime, const QList<QByteArray> &data, const QString &flav) const override;
    QList<QByteArray> convertFromMime(const QString &mime, const QVariant &data, const QString &flav) const override;
};

QString QMacPasteboardMimeTypeName::flavorFor(const QString &mime) const
{
    if (mime == "application/x-qt-mime-type-name"_L1)
        return u"com.trolltech.qt.MimeTypeName"_s;
    return QString();
}

QString QMacPasteboardMimeTypeName::mimeFor(const QString &) const
{
    return QString();
}

bool QMacPasteboardMimeTypeName::canConvert(const QString &, const QString &) const
{
    return false;
}

QVariant QMacPasteboardMimeTypeName::convertToMime(const QString &, const QList<QByteArray> &, const QString &) const
{
    QVariant ret;
    return ret;
}

QList<QByteArray> QMacPasteboardMimeTypeName::convertFromMime(const QString &, const QVariant &, const QString &) const
{
    QList<QByteArray> ret;
    ret.append(QString("x-qt-mime-type-name"_L1).toUtf8());
    return ret;
}

class QMacPasteboardMimePlainTextFallback : public QMacInternalPasteboardMime {
public:
    QMacPasteboardMimePlainTextFallback() : QMacInternalPasteboardMime(MIME_ALL) {}

    QString flavorFor(const QString &mime) const override;
    QString mimeFor(const QString &flav) const override;
    bool canConvert(const QString &mime, const QString &flav) const override;
    QVariant convertToMime(const QString &mime, const QList<QByteArray> &data, const QString &flav) const override;
    QList<QByteArray> convertFromMime(const QString &mime, const QVariant &data, const QString &flav) const override;
};

QString QMacPasteboardMimePlainTextFallback::flavorFor(const QString &mime) const
{
    if (mime == "text/plain"_L1)
        return "public.text"_L1;
    return QString();
}

QString QMacPasteboardMimePlainTextFallback::mimeFor(const QString &flav) const
{
    if (flav == "public.text"_L1)
        return "text/plain"_L1;
    return QString();
}

bool QMacPasteboardMimePlainTextFallback::canConvert(const QString &mime, const QString &flav) const
{
    return mime == mimeFor(flav);
}

QVariant
QMacPasteboardMimePlainTextFallback::convertToMime(const QString &mimetype,
                                                   const QList<QByteArray> &data, const QString &flavor) const
{
    if (data.count() > 1)
        qWarning("QMacPasteboardMimePlainTextFallback: Cannot handle multiple member data");

    if (flavor == "public.text"_L1) {
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
QMacPasteboardMimePlainTextFallback::convertFromMime(const QString &, const QVariant &data,
                                                     const QString &flavor) const
{
    QList<QByteArray> ret;
    QString string = data.toString();
    if (flavor == "public.text"_L1)
        ret.append(string.toUtf8());
    return ret;
}

class QMacPasteboardMimeUnicodeText : public QMacInternalPasteboardMime {
public:
    QMacPasteboardMimeUnicodeText() : QMacInternalPasteboardMime(MIME_ALL) {}

    QString flavorFor(const QString &mime) const override;
    QString mimeFor(const QString &flav) const override;
    bool canConvert(const QString &mime, const QString &flav) const override;
    QVariant convertToMime(const QString &mime, const QList<QByteArray> &data, const QString &flav) const override;
    QList<QByteArray> convertFromMime(const QString &mime, const QVariant &data, const QString &flav) const override;
};

QString QMacPasteboardMimeUnicodeText::flavorFor(const QString &mime) const
{
    if (mime == "text/plain"_L1)
        return "public.utf16-plain-text"_L1;
    int i = mime.indexOf("charset="_L1);
    if (i >= 0) {
        QString cs(mime.mid(i+8).toLower());
        i = cs.indexOf(u';');
        if (i>=0)
            cs = cs.left(i);
        if (cs == "system"_L1)
            return "public.utf8-plain-text"_L1;
        else if (cs == "iso-10646-ucs-2"_L1 || cs == "utf16"_L1)
            return "public.utf16-plain-text"_L1;
    }
    return QString();
}

QString QMacPasteboardMimeUnicodeText::mimeFor(const QString &flav) const
{
    if (flav == "public.utf16-plain-text"_L1 || flav == "public.utf8-plain-text"_L1)
        return "text/plain"_L1;
    return QString();
}

bool QMacPasteboardMimeUnicodeText::canConvert(const QString &mime, const QString &flav) const
{
    return (mime == "text/plain"_L1
            && (flav == "public.utf8-plain-text"_L1 || (flav == "public.utf16-plain-text"_L1)));
}

QVariant
QMacPasteboardMimeUnicodeText::convertToMime(const QString &mimetype,
                                             const QList<QByteArray> &data, const QString &flavor) const
{
    if (data.count() > 1)
        qWarning("QMacPasteboardMimeUnicodeText: Cannot handle multiple member data");
    const QByteArray &firstData = data.first();
    // I can only handle two types (system and unicode) so deal with them that way
    QVariant ret;
    if (flavor == "public.utf8-plain-text"_L1) {
        ret = QString::fromUtf8(firstData);
    } else if (flavor == "public.utf16-plain-text"_L1) {
        QString str = QStringDecoder(QStringDecoder::Utf16)(firstData);
        ret = str;
    } else {
        qWarning("QMime::convertToMime: unhandled mimetype: %s", qPrintable(mimetype));
    }
    return ret;
}

QList<QByteArray>
QMacPasteboardMimeUnicodeText::convertFromMime(const QString &, const QVariant &data, const QString &flavor) const
{
    QList<QByteArray> ret;
    QString string = data.toString();
    if (flavor == "public.utf8-plain-text"_L1)
        ret.append(string.toUtf8());
    else if (flavor == "public.utf16-plain-text"_L1) {
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

class QMacPasteboardMimeHTMLText : public QMacInternalPasteboardMime {
public:
    QMacPasteboardMimeHTMLText() : QMacInternalPasteboardMime(MIME_ALL) {}

    QString flavorFor(const QString &mime) const override;
    QString mimeFor(const QString &flav) const override;
    bool canConvert(const QString &mime, const QString &flav) const override;
    QVariant convertToMime(const QString &mime, const QList<QByteArray> &data, const QString &flav) const override;
    QList<QByteArray> convertFromMime(const QString &mime, const QVariant &data, const QString &flav) const override;
};

QString QMacPasteboardMimeHTMLText::flavorFor(const QString &mime) const
{
    if (mime == "text/html"_L1)
        return "public.html"_L1;
    return QString();
}

QString QMacPasteboardMimeHTMLText::mimeFor(const QString &flav) const
{
    if (flav == "public.html"_L1)
        return "text/html"_L1;
    return QString();
}

bool QMacPasteboardMimeHTMLText::canConvert(const QString &mime, const QString &flav) const
{
    return flavorFor(mime) == flav;
}

QVariant
QMacPasteboardMimeHTMLText::convertToMime(const QString &mimeType,
                                          const QList<QByteArray> &data, const QString &flavor) const
{
    if (!canConvert(mimeType, flavor))
        return QVariant();
    if (data.count() > 1)
        qWarning("QMacPasteboardMimeHTMLText: Cannot handle multiple member data");
    return data.first();
}

QList<QByteArray>
QMacPasteboardMimeHTMLText::convertFromMime(const QString &mime,
                                            const QVariant &data, const QString &flavor) const
{
    QList<QByteArray> ret;
    if (!canConvert(mime, flavor))
        return ret;
    ret.append(data.toByteArray());
    return ret;
}

class QMacPasteboardMimeRtfText : public QMacInternalPasteboardMime {
public:
    QMacPasteboardMimeRtfText() : QMacInternalPasteboardMime(MIME_ALL) {}

    QString flavorFor(const QString &mime) const override;
    QString mimeFor(const QString &flav) const override;
    bool canConvert(const QString &mime, const QString &flav) const override;
    QVariant convertToMime(const QString &mime, const QList<QByteArray> &data, const QString &flav) const override;
    QList<QByteArray> convertFromMime(const QString &mime, const QVariant &data, const QString &flav) const override;
};

QString QMacPasteboardMimeRtfText::flavorFor(const QString &mime) const
{
    if (mime == "text/html"_L1)
        return "public.rtf"_L1;
    return QString();
}

QString QMacPasteboardMimeRtfText::mimeFor(const QString &flav) const
{
    if (flav == "public.rtf"_L1)
        return "text/html"_L1;
    return QString();
}

bool QMacPasteboardMimeRtfText::canConvert(const QString &mime, const QString &flav) const
{
    return mime == mimeFor(flav);
}

QVariant
QMacPasteboardMimeRtfText::convertToMime(const QString &mimeType,
                                         const QList<QByteArray> &data, const QString &flavor) const
{
    if (!canConvert(mimeType, flavor))
        return QVariant();
    if (data.count() > 1)
        qWarning("QMacPasteboardMimeHTMLText: Cannot handle multiple member data");

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
QMacPasteboardMimeRtfText::convertFromMime(const QString &mime,
                                           const QVariant &data, const QString &flavor) const
{
    QList<QByteArray> ret;
    if (!canConvert(mime, flavor))
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

class QMacPasteboardMimeFileUri : public QMacInternalPasteboardMime {
public:
    QMacPasteboardMimeFileUri() : QMacInternalPasteboardMime(MIME_ALL) {}

    QString flavorFor(const QString &mime) const override;
    QString mimeFor(const QString &flav) const override;
    bool canConvert(const QString &mime, const QString &flav) const override;
    QVariant convertToMime(const QString &mime, const QList<QByteArray> &data, const QString &flav) const override;
    QList<QByteArray> convertFromMime(const QString &mime, const QVariant &data, const QString &flav) const override;
    int count(const QMimeData *mimeData) const override;
};

QString QMacPasteboardMimeFileUri::flavorFor(const QString &mime) const
{
    if (mime == "text/uri-list"_L1)
        return "public.file-url"_L1;
    return QString();
}

QString QMacPasteboardMimeFileUri::mimeFor(const QString &flav) const
{
    if (flav == "public.file-url"_L1)
        return "text/uri-list"_L1;
    return QString();
}

bool QMacPasteboardMimeFileUri::canConvert(const QString &mime, const QString &flav) const
{
    return mime == "text/uri-list"_L1 && flav == "public.file-url"_L1;
}

QVariant
QMacPasteboardMimeFileUri::convertToMime(const QString &mime,
                                         const QList<QByteArray> &data, const QString &flav) const
{
    if (!canConvert(mime, flav))
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
QMacPasteboardMimeFileUri::convertFromMime(const QString &mime,
                                           const QVariant &data, const QString &flav) const
{
    QList<QByteArray> ret;
    if (!canConvert(mime, flav))
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

int QMacPasteboardMimeFileUri::count(const QMimeData *mimeData) const
{
    return mimeData->urls().count();
}

class QMacPasteboardMimeUrl : public QMacInternalPasteboardMime {
public:
    QMacPasteboardMimeUrl() : QMacInternalPasteboardMime(MIME_ALL) {}

    QString flavorFor(const QString &mime) const override;
    QString mimeFor(const QString &flav) const override;
    bool canConvert(const QString &mime, const QString &flav) const override;
    QVariant convertToMime(const QString &mime, const QList<QByteArray> &data, const QString &flav) const override;
    QList<QByteArray> convertFromMime(const QString &mime, const QVariant &data, const QString &flav) const override;
};

QString QMacPasteboardMimeUrl::flavorFor(const QString &mime) const
{
    if (mime.startsWith("text/uri-list"_L1))
        return "public.url"_L1;
    return QString();
}

QString QMacPasteboardMimeUrl::mimeFor(const QString &flav) const
{
    if (flav == "public.url"_L1)
        return "text/uri-list"_L1;
    return QString();
}

bool QMacPasteboardMimeUrl::canConvert(const QString &mime, const QString &flav) const
{
    return flav == "public.url"_L1
            && mime == "text/uri-list"_L1;
}

QVariant QMacPasteboardMimeUrl::convertToMime(const QString &mime,
                                              const QList<QByteArray> &data, const QString &flav) const
{
    if (!canConvert(mime, flav))
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

QList<QByteArray> QMacPasteboardMimeUrl::convertFromMime(const QString &mime,
                                                         const QVariant &data, const QString &flav) const
{
    QList<QByteArray> ret;
    if (!canConvert(mime, flav))
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

class QMacPasteboardMimeVCard : public QMacInternalPasteboardMime
{
public:
    QMacPasteboardMimeVCard() : QMacInternalPasteboardMime(MIME_ALL) {}

    QString flavorFor(const QString &mime) const override;
    QString mimeFor(const QString &flav) const override;
    bool canConvert(const QString &mime, const QString &flav) const override;
    QVariant convertToMime(const QString &mime, const QList<QByteArray> &data, const QString &flav) const override;
    QList<QByteArray> convertFromMime(const QString &mime, const QVariant &data, const QString &flav) const override;
};

bool QMacPasteboardMimeVCard::canConvert(const QString &mime, const QString &flav) const
{
    return mimeFor(flav) == mime;
}

QString QMacPasteboardMimeVCard::flavorFor(const QString &mime) const
{
    if (mime.startsWith("text/vcard"_L1))
        return "public.vcard"_L1;
    return QString();
}

QString QMacPasteboardMimeVCard::mimeFor(const QString &flav) const
{
    if (flav == "public.vcard"_L1)
        return "text/vcard"_L1;
    return QString();
}

QVariant QMacPasteboardMimeVCard::convertToMime(const QString &mime,
                                                const QList<QByteArray> &data, const QString &) const
{
    QByteArray cards;
    if (mime == "text/vcard"_L1) {
        for (int i=0; i<data.size(); ++i)
            cards += data[i];
    }
    return QVariant(cards);
}

QList<QByteArray> QMacPasteboardMimeVCard::convertFromMime(const QString &mime,
                                                           const QVariant &data, const QString &) const
{
    QList<QByteArray> ret;
    if (mime == "text/vcard"_L1)
        ret.append(data.toString().toUtf8());
    return ret;
}

extern QImage qt_mac_toQImage(CGImageRef image);
extern CGImageRef qt_mac_toCGImage(const QImage &qImage);

class QMacPasteboardMimeTiff : public QMacInternalPasteboardMime {
public:
    QMacPasteboardMimeTiff() : QMacInternalPasteboardMime(MIME_ALL) {}

    QString flavorFor(const QString &mime) const override;
    QString mimeFor(const QString &flav) const override;
    bool canConvert(const QString &mime, const QString &flav) const override;
    QVariant convertToMime(const QString &mime, const QList<QByteArray> &data, const QString &flav) const override;
    QList<QByteArray> convertFromMime(const QString &mime, const QVariant &data, const QString &flav) const override;
};

QString QMacPasteboardMimeTiff::flavorFor(const QString &mime) const
{
    if (mime.startsWith("application/x-qt-image"_L1))
        return "public.tiff"_L1;
    return QString();
}

QString QMacPasteboardMimeTiff::mimeFor(const QString &flav) const
{
    if (flav == "public.tiff"_L1)
        return "application/x-qt-image"_L1;
    return QString();
}

bool QMacPasteboardMimeTiff::canConvert(const QString &mime, const QString &flav) const
{
    return flav == "public.tiff"_L1 && mime == "application/x-qt-image"_L1;
}

QVariant QMacPasteboardMimeTiff::convertToMime(const QString &mime,
                                               const QList<QByteArray> &data, const QString &flav) const
{
    if (data.count() > 1)
        qWarning("QMacPasteboardMimeTiff: Cannot handle multiple member data");

    if (!canConvert(mime, flav))
        return QVariant();

    QCFType<CFDataRef> tiffData = data.first().toRawCFData();
    QCFType<CGImageSourceRef> imageSource = CGImageSourceCreateWithData(tiffData, 0);

    if (QCFType<CGImageRef> image = CGImageSourceCreateImageAtIndex(imageSource, 0, 0))
        return QVariant(qt_mac_toQImage(image));

    return QVariant();
}

QList<QByteArray> QMacPasteboardMimeTiff::convertFromMime(const QString &mime,
                                                          const QVariant &variant, const QString &flav) const
{
    if (!canConvert(mime, flav))
        return QList<QByteArray>();

    QCFType<CFMutableDataRef> data = CFDataCreateMutable(0, 0);
    QCFType<CGImageDestinationRef> imageDestination = CGImageDestinationCreateWithData(data, kUTTypeTIFF, 1, 0);

    if (!imageDestination)
        return QList<QByteArray>();

    QImage img = qvariant_cast<QImage>(variant);
    NSDictionary *props = @{
        static_cast<NSString *>(kCGImagePropertyPixelWidth): @(img.width()),
        static_cast<NSString *>(kCGImagePropertyPixelHeight): @(img.height())
    };

    CGImageDestinationAddImage(imageDestination, qt_mac_toCGImage(img), static_cast<CFDictionaryRef>(props));
    CGImageDestinationFinalize(imageDestination);

    return QList<QByteArray>() << QByteArray::fromCFData(data);
}

namespace QMacMimeRegistry {

void registerBuiltInTypes()
{
    // Create QMacPasteboardMimeAny first to put it at the end of globalMimeList
    // with lowest priority. (the constructor prepends to the list)
    new QMacPasteboardMimeAny;

    //standard types that we wrap
    new QMacPasteboardMimeTiff;
    new QMacPasteboardMimePlainTextFallback;
    new QMacPasteboardMimeUnicodeText;
    new QMacPasteboardMimeRtfText;
    new QMacPasteboardMimeHTMLText;
    new QMacPasteboardMimeFileUri;
    new QMacPasteboardMimeUrl;
    new QMacPasteboardMimeTypeName;
    new QMacPasteboardMimeVCard;
}

}

/*
  \fn bool QMacPasteboardMime::canConvert(const QString &mime, QString flav)

  Returns \c true if the converter can convert (both ways) between
  \a mime and \a flav; otherwise returns \c false.

  All subclasses must reimplement this pure virtual function.
*/

/*
  \fn QString QMacPasteboardMime::mimeFor(QString flav)

  Returns the MIME UTI used for Mac flavor \a flav, or 0 if this
  converter does not support \a flav.

  All subclasses must reimplement this pure virtual function.
*/

/*
  \fn QString QMacPasteboardMime::flavorFor(const QString &mime)

  Returns the Mac UTI used for MIME type \a mime, or 0 if this
  converter does not support \a mime.

  All subclasses must reimplement this pure virtual function.
*/

/*
    \fn QVariant QMacPasteboardMime::convertToMime(const QString &mime, QList<QByteArray> data, QString flav)

    Returns \a data converted from Mac UTI \a flav to MIME type \a
    mime.

    Note that Mac flavors must all be self-terminating. The input \a
    data may contain trailing data.

    All subclasses must reimplement this pure virtual function.
*/

/*
  \fn QList<QByteArray> QMacPasteboardMime::convertFromMime(const QString &mime, const QVariant &data, const QString & flav)

  Returns \a data converted from MIME type \a mime
    to Mac UTI \a flav.

  Note that Mac flavors must all be self-terminating.  The return
  value may contain trailing data.

  All subclasses must reimplement this pure virtual function.
*/

QT_END_NAMESPACE
