// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2014 Samuel Gaist <samuel.gaist@edeltech.ch>
// Copyright (C) 2014 Petroules Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qstring.h>
#include <QtCore/qurl.h>
#include <QtCore/qdatetime.h>
#include <QtCore/quuid.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qrect.h>

#if QT_CONFIG(timezone) && !defined(QT_NO_SYSTEMLOCALE)
#include <QtCore/qtimezone.h>
#include <QtCore/private/qtimezoneprivate_p.h>
#include <QtCore/private/qcore_mac_p.h>
#endif

#import <CoreFoundation/CoreFoundation.h>
#import <Foundation/Foundation.h>

#if defined(QT_PLATFORM_UIKIT)
#import <CoreGraphics/CoreGraphics.h>
#endif

QT_BEGIN_NAMESPACE

/*!
    \brief Constructs a new QByteArray containing a copy of the CFData \a data.

    \since 5.3
    \ingroup platform-type-conversions

    \sa fromRawCFData(), fromRawData(), toRawCFData(), toCFData()
*/
QByteArray QByteArray::fromCFData(CFDataRef data)
{
    if (!data)
        return QByteArray();

    return QByteArray(reinterpret_cast<const char *>(CFDataGetBytePtr(data)), CFDataGetLength(data));
}

/*!
    \brief Constructs a QByteArray that uses the bytes of the CFData \a data.

    The \a data's bytes are not copied.

    The caller guarantees that the CFData will not be deleted
    or modified as long as this QByteArray object exists.

    \since 5.3
    \ingroup platform-type-conversions

    \sa fromCFData(), fromRawData(), toRawCFData(), toCFData()
*/
QByteArray QByteArray::fromRawCFData(CFDataRef data)
{
    if (!data)
        return QByteArray();

    return QByteArray::fromRawData(reinterpret_cast<const char *>(CFDataGetBytePtr(data)), CFDataGetLength(data));
}

/*!
    \brief Creates a CFData from a QByteArray.

    The caller owns the CFData object and is responsible for releasing it.

    \since 5.3
    \ingroup platform-type-conversions

    \sa toRawCFData(), fromCFData(), fromRawCFData(), fromRawData()
*/
CFDataRef QByteArray::toCFData() const
{
    return CFDataCreate(kCFAllocatorDefault, reinterpret_cast<const UInt8 *>(data()), length());
}

/*!
    \brief Constructs a CFData that uses the bytes of the QByteArray.

    The QByteArray's bytes are not copied.

    The caller guarantees that the QByteArray will not be deleted
    or modified as long as this CFData object exists.

    \since 5.3
    \ingroup platform-type-conversions

    \sa toCFData(), fromRawCFData(), fromCFData(), fromRawData()
*/
CFDataRef QByteArray::toRawCFData() const
{
    return CFDataCreateWithBytesNoCopy(kCFAllocatorDefault, reinterpret_cast<const UInt8 *>(data()),
                    length(), kCFAllocatorNull);
}

/*!
    \brief Constructs a new QByteArray containing a copy of the NSData \a data.

    \since 5.3
    \ingroup platform-type-conversions

    \sa fromRawNSData(), fromRawData(), toNSData(), toRawNSData()
*/
QByteArray QByteArray::fromNSData(const NSData *data)
{
    if (!data)
        return QByteArray();
    return QByteArray(reinterpret_cast<const char *>([data bytes]), [data length]);
}

/*!
    \brief Constructs a QByteArray that uses the bytes of the NSData \a data.

    The \a data's bytes are not copied.

    The caller guarantees that the NSData will not be deleted
    or modified as long as this QByteArray object exists.

    \since 5.3
    \ingroup platform-type-conversions

    \sa fromNSData(), fromRawData(), toRawNSData(), toNSData()
*/
QByteArray QByteArray::fromRawNSData(const NSData *data)
{
    if (!data)
        return QByteArray();
    return QByteArray::fromRawData(reinterpret_cast<const char *>([data bytes]), [data length]);
}

/*!
    \brief Creates a NSData from a QByteArray.

    The NSData object is autoreleased.

    \since 5.3
    \ingroup platform-type-conversions

    \sa fromNSData(), fromRawNSData(), fromRawData(), toRawNSData()
*/
NSData *QByteArray::toNSData() const
{
    return [NSData dataWithBytes:constData() length:size()];
}

/*!
    \brief Constructs a NSData that uses the bytes of the QByteArray.

    The QByteArray's bytes are not copied.

    The caller guarantees that the QByteArray will not be deleted
    or modified as long as this NSData object exists.

    \since 5.3
    \ingroup platform-type-conversions

    \sa fromRawNSData(), fromNSData(), fromRawData(), toNSData()
*/
NSData *QByteArray::toRawNSData() const
{
    // const_cast is fine here because NSData is immutable thus will never modify bytes we're giving it
    return [NSData dataWithBytesNoCopy:const_cast<char *>(constData()) length:size() freeWhenDone:NO];
}

// ----------------------------------------------------------------------------

/*!
    \brief Constructs a new QString containing a copy of the \a string CFString.

    \note this function is only available on \macos and iOS.

    \since 5.2
    \ingroup platform-type-conversions
*/
QString QString::fromCFString(CFStringRef string)
{
    if (!string)
        return QString();
    CFIndex length = CFStringGetLength(string);

    // Fast path: CFStringGetCharactersPtr does not copy but may
    // return null for any and no reason.
    const UniChar *chars = CFStringGetCharactersPtr(string);
    if (chars)
        return QString(reinterpret_cast<const QChar *>(chars), length);

    QString ret(length, Qt::Uninitialized);
    CFStringGetCharacters(string, CFRangeMake(0, length), reinterpret_cast<UniChar *>(ret.data()));
    return ret;
}

/*!
    \brief Creates a CFString from a QString.

    The caller owns the CFString and is responsible for releasing it.

    \note this function is only available on \macos and iOS.

    \since 5.2
    \ingroup platform-type-conversions
*/
CFStringRef QString::toCFString() const
{
    return QStringView{*this}.toCFString();
}

/*!
    \brief Creates a CFString from this QStringView.

    The caller owns the CFString and is responsible for releasing it.

    \note this function is only available on \macos and iOS.

    \since 6.0
    \ingroup platform-type-conversions
*/
CFStringRef QStringView::toCFString() const
{
    return CFStringCreateWithCharacters(0, reinterpret_cast<const UniChar *>(data()), size());
}

/*!
    \brief Constructs a new QString containing a copy of the \a string NSString.

    \note this function is only available on \macos and iOS.

    \since 5.2
    \ingroup platform-type-conversions
*/
QString QString::fromNSString(const NSString *string)
{
    if (!string)
        return QString();
   QString qstring;
   qstring.resize([string length]);
   [string getCharacters: reinterpret_cast<unichar*>(qstring.data()) range: NSMakeRange(0, [string length])];
   return qstring;
}

/*!
    \brief Creates a NSString from a QString.

    The NSString is autoreleased.

    \note this function is only available on \macos and iOS.

    \since 5.2
    \ingroup platform-type-conversions
*/
NSString *QString::toNSString() const
{
    return QStringView{*this}.toNSString();
}

/*!
    \brief Creates an NSString from this QStringView.

    The NSString is autoreleased.

    \note this function is only available on \macos and iOS.

    \since 6.0
    \ingroup platform-type-conversions
*/
NSString *QStringView::toNSString() const
{
    return [NSString stringWithCharacters:reinterpret_cast<const UniChar*>(data()) length:size()];
}

// ----------------------------------------------------------------------------

/*!
    \brief Constructs a new QUuid containing a copy of the \a uuid CFUUID.

    \note this function is only available on Apple platforms.

    \since 5.7
    \ingroup platform-type-conversions
*/
QUuid QUuid::fromCFUUID(CFUUIDRef uuid)
{
    if (!uuid)
        return QUuid();
    const CFUUIDBytes bytes = CFUUIDGetUUIDBytes(uuid);
    return QUuid::fromRfc4122(QByteArrayView(reinterpret_cast<const char *>(&bytes), sizeof(bytes)));
}

/*!
    \brief Creates a CFUUID from a QUuid.

    The caller owns the CFUUID and is responsible for releasing it.

    \note this function is only available on Apple platforms.

    \since 5.7
    \ingroup platform-type-conversions
*/
CFUUIDRef QUuid::toCFUUID() const
{
    const QByteArray bytes = toRfc4122();
    return CFUUIDCreateFromUUIDBytes(0, *reinterpret_cast<const CFUUIDBytes *>(bytes.constData()));
}

/*!
    \brief Constructs a new QUuid containing a copy of the \a uuid NSUUID.

    \note this function is only available on Apple platforms.

    \since 5.7
    \ingroup platform-type-conversions
*/
QUuid QUuid::fromNSUUID(const NSUUID *uuid)
{
    if (!uuid)
        return QUuid();
    uuid_t bytes;
    [uuid getUUIDBytes:bytes];
    return QUuid::fromRfc4122(QByteArrayView(reinterpret_cast<const char *>(bytes), sizeof(bytes)));
}

/*!
    \brief Creates a NSUUID from a QUuid.

    The NSUUID is autoreleased.

    \note this function is only available on Apple platforms.

    \since 5.7
    \ingroup platform-type-conversions
*/
NSUUID *QUuid::toNSUUID() const
{
    const QByteArray bytes = toRfc4122();
    return [[[NSUUID alloc] initWithUUIDBytes:*reinterpret_cast<const uuid_t *>(bytes.constData())] autorelease];
}

// ----------------------------------------------------------------------------


/*!
    \brief Constructs a QUrl containing a copy of the CFURL \a url.

    \since 5.2
    \ingroup platform-type-conversions
*/
QUrl QUrl::fromCFURL(CFURLRef url)
{
    if (!url)
        return QUrl();
    return QUrl(QString::fromCFString(CFURLGetString(url)));
}

/*!
    \brief Creates a CFURL from a QUrl.

    The caller owns the CFURL and is responsible for releasing it.

    \since 5.2
    \ingroup platform-type-conversions
*/
CFURLRef QUrl::toCFURL() const
{
    CFURLRef url = 0;
    CFStringRef str = toString(FullyEncoded).toCFString();
    if (str) {
        url = CFURLCreateWithString(0, str, 0);
        CFRelease(str);
    }
    return url;
}

/*!
    \brief Constructs a QUrl containing a copy of the NSURL \a url.

    \since 5.2
    \ingroup platform-type-conversions
*/
QUrl QUrl::fromNSURL(const NSURL *url)
{
    if (!url)
        return QUrl();
    return QUrl(QString::fromNSString([url absoluteString]));
}

/*!
    \brief Creates a NSURL from a QUrl.

    The NSURL is autoreleased.

    \since 5.2
    \ingroup platform-type-conversions
*/
NSURL *QUrl::toNSURL() const
{
    return [NSURL URLWithString:toString(FullyEncoded).toNSString()];
}

// ----------------------------------------------------------------------------


/*!
    \brief Constructs a new QDateTime containing a copy of the CFDate \a date.

    \since 5.5
    \ingroup platform-type-conversions

    \sa toCFDate()
*/
QDateTime QDateTime::fromCFDate(CFDateRef date)
{
    if (!date)
        return QDateTime();
    CFAbsoluteTime sSinceEpoch = kCFAbsoluteTimeIntervalSince1970 + CFDateGetAbsoluteTime(date);
    return QDateTime::fromMSecsSinceEpoch(qRound64(sSinceEpoch * 1000));
}

/*!
    \brief Creates a CFDate from a QDateTime.

    The caller owns the CFDate object and is responsible for releasing it.

    \since 5.5
    \ingroup platform-type-conversions

    \sa fromCFDate()
*/
CFDateRef QDateTime::toCFDate() const
{
    return CFDateCreate(kCFAllocatorDefault, (static_cast<CFAbsoluteTime>(toMSecsSinceEpoch())
                                                    / 1000) - kCFAbsoluteTimeIntervalSince1970);
}

/*!
    \brief Constructs a new QDateTime containing a copy of the NSDate \a date.

    \since 5.5
    \ingroup platform-type-conversions

    \sa toNSDate()
*/
QDateTime QDateTime::fromNSDate(const NSDate *date)
{
    if (!date)
        return QDateTime();
    return QDateTime::fromMSecsSinceEpoch(qRound64([date timeIntervalSince1970] * 1000));
}

/*!
    \brief Creates an NSDate from a QDateTime.

    The NSDate object is autoreleased.

    \since 5.5
    \ingroup platform-type-conversions

    \sa fromNSDate()
*/
NSDate *QDateTime::toNSDate() const
{
    return [NSDate
            dateWithTimeIntervalSince1970:static_cast<NSTimeInterval>(toMSecsSinceEpoch()) / 1000];
}

// ----------------------------------------------------------------------------

#if QT_CONFIG(timezone) && !defined(QT_NO_SYSTEMLOCALE)
/*!
    \brief Constructs a new QTimeZone containing a copy of the CFTimeZone \a timeZone.

    \since 5.9
    \ingroup platform-type-conversions

    \sa toCFTimeZone()
*/
QTimeZone QTimeZone::fromCFTimeZone(CFTimeZoneRef timeZone)
{
    if (!timeZone)
        return QTimeZone();
    return QTimeZone(QString::fromCFString(CFTimeZoneGetName(timeZone)).toLatin1());
}

/*!
    \brief Creates a CFTimeZone from a QTimeZone.

    The caller owns the CFTimeZone object and is responsible for releasing it.

    \since 5.9
    \ingroup platform-type-conversions

    \sa fromCFTimeZone()
*/
CFTimeZoneRef QTimeZone::toCFTimeZone() const
{
#ifndef QT_NO_DYNAMIC_CAST
    Q_ASSERT(dynamic_cast<const QMacTimeZonePrivate *>(d.d));
#endif
    const QMacTimeZonePrivate *p = static_cast<const QMacTimeZonePrivate *>(d.d);
    return reinterpret_cast<CFTimeZoneRef>([p->nsTimeZone() copy]);
}

/*!
    \brief Constructs a new QTimeZone containing a copy of the NSTimeZone \a timeZone.

    \since 5.9
    \ingroup platform-type-conversions

    \sa toNSTimeZone()
*/
QTimeZone QTimeZone::fromNSTimeZone(const NSTimeZone *timeZone)
{
    if (!timeZone)
        return QTimeZone();
    return QTimeZone(QString::fromNSString(timeZone.name).toLatin1());
}

/*!
    \brief Creates an NSTimeZone from a QTimeZone.

    The NSTimeZone object is autoreleased.

    \since 5.9
    \ingroup platform-type-conversions

    \sa fromNSTimeZone()
*/
NSTimeZone *QTimeZone::toNSTimeZone() const
{
    return [static_cast<NSTimeZone *>(toCFTimeZone()) autorelease];
}
#endif

// ----------------------------------------------------------------------------

/*!
    \brief Creates a CGRect from a QRect.

    \since 5.8
    \ingroup platform-type-conversions

    \sa QRectF::fromCGRect()
*/
CGRect QRect::toCGRect() const noexcept
{
    return CGRectMake(x(), y(), width(), height());
}

/*!
    \brief Creates a CGRect from a QRectF.

    \since 5.8
    \ingroup platform-type-conversions

    \sa fromCGRect()
*/
CGRect QRectF::toCGRect() const noexcept
{
    return CGRectMake(x(), y(), width(), height());
}

/*!
    \brief Creates a QRectF from CGRect \a rect.

    \since 5.8
    \ingroup platform-type-conversions

    \sa toCGRect()
*/
QRectF QRectF::fromCGRect(CGRect rect) noexcept
{
    return QRectF(rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
}

// ----------------------------------------------------------------------------

/*!
    \brief Creates a CGPoint from a QPoint.

    \since 5.8
    \ingroup platform-type-conversions

    \sa QPointF::fromCGPoint()
*/
CGPoint QPoint::toCGPoint() const noexcept
{
    return CGPointMake(x(), y());
}

/*!
    \brief Creates a CGPoint from a QPointF.

    \since 5.8
    \ingroup platform-type-conversions

    \sa fromCGPoint()
*/
CGPoint QPointF::toCGPoint() const noexcept
{
    return CGPointMake(x(), y());
}

/*!
    \brief Creates a QRectF from CGPoint \a point.

    \since 5.8
    \ingroup platform-type-conversions

    \sa toCGPoint()
*/
QPointF QPointF::fromCGPoint(CGPoint point) noexcept
{
    return QPointF(point.x, point.y);
}

// ----------------------------------------------------------------------------

/*!
    \brief Creates a CGSize from a QSize.

    \since 5.8
    \ingroup platform-type-conversions

    \sa QSizeF::fromCGSize()
*/
CGSize QSize::toCGSize() const noexcept
{
    return CGSizeMake(width(), height());
}

/*!
    \brief Creates a CGSize from a QSizeF.

    \since 5.8
    \ingroup platform-type-conversions

    \sa fromCGSize()
*/
CGSize QSizeF::toCGSize() const noexcept
{
    return CGSizeMake(width(), height());
}

/*!
    \brief Creates a QRectF from \a size.

    \since 5.8
    \ingroup platform-type-conversions

    \sa toCGSize()
*/
QSizeF QSizeF::fromCGSize(CGSize size) noexcept
{
    return QSizeF(size.width, size.height);
}

QT_END_NAMESPACE
