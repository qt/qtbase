/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2014 Samuel Gaist <samuel.gaist@edeltech.ch>
** Copyright (C) 2014 Petroules Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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
    \since 5.3

    Constructs a new QByteArray containing a copy of the CFData \a data.

    \sa fromRawCFData(), fromRawData(), toRawCFData(), toCFData()
*/
QByteArray QByteArray::fromCFData(CFDataRef data)
{
    if (!data)
        return QByteArray();

    return QByteArray(reinterpret_cast<const char *>(CFDataGetBytePtr(data)), CFDataGetLength(data));
}

/*!
    \since 5.3

    Constructs a QByteArray that uses the bytes of the CFData \a data.

    The \a data's bytes are not copied.

    The caller guarantees that the CFData will not be deleted
    or modified as long as this QByteArray object exists.

    \sa fromCFData(), fromRawData(), toRawCFData(), toCFData()
*/
QByteArray QByteArray::fromRawCFData(CFDataRef data)
{
    if (!data)
        return QByteArray();

    return QByteArray::fromRawData(reinterpret_cast<const char *>(CFDataGetBytePtr(data)), CFDataGetLength(data));
}

/*!
    \since 5.3

    Creates a CFData from a QByteArray. The caller owns the CFData object
    and is responsible for releasing it.

    \sa toRawCFData(), fromCFData(), fromRawCFData(), fromRawData()
*/
CFDataRef QByteArray::toCFData() const
{
    return CFDataCreate(kCFAllocatorDefault, reinterpret_cast<const UInt8 *>(data()), length());
}

/*!
    \since 5.3

    Constructs a CFData that uses the bytes of the QByteArray.

    The QByteArray's bytes are not copied.

    The caller guarantees that the QByteArray will not be deleted
    or modified as long as this CFData object exists.

    \sa toCFData(), fromRawCFData(), fromCFData(), fromRawData()
*/
CFDataRef QByteArray::toRawCFData() const
{
    return CFDataCreateWithBytesNoCopy(kCFAllocatorDefault, reinterpret_cast<const UInt8 *>(data()),
                    length(), kCFAllocatorNull);
}

/*!
    \since 5.3

    Constructs a new QByteArray containing a copy of the NSData \a data.

    \sa fromRawNSData(), fromRawData(), toNSData(), toRawNSData()
*/
QByteArray QByteArray::fromNSData(const NSData *data)
{
    if (!data)
        return QByteArray();
    return QByteArray(reinterpret_cast<const char *>([data bytes]), [data length]);
}

/*!
    \since 5.3

    Constructs a QByteArray that uses the bytes of the NSData \a data.

    The \a data's bytes are not copied.

    The caller guarantees that the NSData will not be deleted
    or modified as long as this QByteArray object exists.

    \sa fromNSData(), fromRawData(), toRawNSData(), toNSData()
*/
QByteArray QByteArray::fromRawNSData(const NSData *data)
{
    if (!data)
        return QByteArray();
    return QByteArray::fromRawData(reinterpret_cast<const char *>([data bytes]), [data length]);
}

/*!
    \since 5.3

    Creates a NSData from a QByteArray. The NSData object is autoreleased.

    \sa fromNSData(), fromRawNSData(), fromRawData(), toRawNSData()
*/
NSData *QByteArray::toNSData() const
{
    return [NSData dataWithBytes:constData() length:size()];
}

/*!
    \since 5.3

    Constructs a NSData that uses the bytes of the QByteArray.

    The QByteArray's bytes are not copied.

    The caller guarantees that the QByteArray will not be deleted
    or modified as long as this NSData object exists.

    \sa fromRawNSData(), fromNSData(), fromRawData(), toNSData()
*/
NSData *QByteArray::toRawNSData() const
{
    // const_cast is fine here because NSData is immutable thus will never modify bytes we're giving it
    return [NSData dataWithBytesNoCopy:const_cast<char *>(constData()) length:size() freeWhenDone:NO];
}

// ----------------------------------------------------------------------------

/*!
    \since 5.2

    Constructs a new QString containing a copy of the \a string CFString.

    \note this function is only available on OS X and iOS.
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
    \since 5.2

    Creates a CFString from a QString. The caller owns the CFString and is
    responsible for releasing it.

    \note this function is only available on OS X and iOS.
*/
CFStringRef QString::toCFString() const
{
    return CFStringCreateWithCharacters(0, reinterpret_cast<const UniChar *>(unicode()), length());
}

/*!
    \since 5.2

    Constructs a new QString containing a copy of the \a string NSString.

    \note this function is only available on OS X and iOS.
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
    \since 5.2

    Creates a NSString from a QString. The NSString is autoreleased.

    \note this function is only available on OS X and iOS.
*/
NSString *QString::toNSString() const
{
    return [NSString stringWithCharacters: reinterpret_cast<const UniChar*>(unicode()) length: length()];
}

// ----------------------------------------------------------------------------

/*!
    \since 5.7

    Constructs a new QUuid containing a copy of the \a uuid CFUUID.

    \note this function is only available on Apple platforms.
*/
QUuid QUuid::fromCFUUID(CFUUIDRef uuid)
{
    if (!uuid)
        return QUuid();
    const CFUUIDBytes bytes = CFUUIDGetUUIDBytes(uuid);
    return QUuid::fromRfc4122(QByteArray::fromRawData(reinterpret_cast<const char *>(&bytes), sizeof(bytes)));
}

/*!
    \since 5.7

    Creates a CFUUID from a QUuid. The caller owns the CFUUID and is
    responsible for releasing it.

    \note this function is only available on Apple platforms.
*/
CFUUIDRef QUuid::toCFUUID() const
{
    const QByteArray bytes = toRfc4122();
    return CFUUIDCreateFromUUIDBytes(0, *reinterpret_cast<const CFUUIDBytes *>(bytes.constData()));
}

/*!
    \since 5.7

    Constructs a new QUuid containing a copy of the \a uuid NSUUID.

    \note this function is only available on Apple platforms.
*/
QUuid QUuid::fromNSUUID(const NSUUID *uuid)
{
    if (!uuid)
        return QUuid();
    uuid_t bytes;
    [uuid getUUIDBytes:bytes];
    return QUuid::fromRfc4122(QByteArray::fromRawData(reinterpret_cast<const char *>(bytes), sizeof(bytes)));
}

/*!
    \since 5.7

    Creates a NSUUID from a QUuid. The NSUUID is autoreleased.

    \note this function is only available on Apple platforms.
*/
NSUUID *QUuid::toNSUUID() const
{
    const QByteArray bytes = toRfc4122();
    return [[[NSUUID alloc] initWithUUIDBytes:*reinterpret_cast<const uuid_t *>(bytes.constData())] autorelease];
}

// ----------------------------------------------------------------------------


/*!
    \since 5.2

    Constructs a QUrl containing a copy of the CFURL \a url.
*/
QUrl QUrl::fromCFURL(CFURLRef url)
{
    if (!url)
        return QUrl();
    return QUrl(QString::fromCFString(CFURLGetString(url)));
}

/*!
    \since 5.2

    Creates a CFURL from a QUrl. The caller owns the CFURL and is
    responsible for releasing it.
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
    \since 5.2

    Constructs a QUrl containing a copy of the NSURL \a url.
*/
QUrl QUrl::fromNSURL(const NSURL *url)
{
    if (!url)
        return QUrl();
    return QUrl(QString::fromNSString([url absoluteString]));
}

/*!
    \since 5.2

    Creates a NSURL from a QUrl. The NSURL is autoreleased.
*/
NSURL *QUrl::toNSURL() const
{
    return [NSURL URLWithString:toString(FullyEncoded).toNSString()];
}

// ----------------------------------------------------------------------------


/*!
    \since 5.5

    Constructs a new QDateTime containing a copy of the CFDate \a date.

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
    \since 5.5

    Creates a CFDate from a QDateTime. The caller owns the CFDate object
    and is responsible for releasing it.

    \sa fromCFDate()
*/
CFDateRef QDateTime::toCFDate() const
{
    return CFDateCreate(kCFAllocatorDefault, (static_cast<CFAbsoluteTime>(toMSecsSinceEpoch())
                                                    / 1000) - kCFAbsoluteTimeIntervalSince1970);
}

/*!
    \since 5.5

    Constructs a new QDateTime containing a copy of the NSDate \a date.

    \sa toNSDate()
*/
QDateTime QDateTime::fromNSDate(const NSDate *date)
{
    if (!date)
        return QDateTime();
    return QDateTime::fromMSecsSinceEpoch(qRound64([date timeIntervalSince1970] * 1000));
}

/*!
    \since 5.5

    Creates an NSDate from a QDateTime. The NSDate object is autoreleased.

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
    \since 5.9

    Constructs a new QTimeZone containing a copy of the CFTimeZone \a timeZone.

    \sa toCFTimeZone()
*/
QTimeZone QTimeZone::fromCFTimeZone(CFTimeZoneRef timeZone)
{
    if (!timeZone)
        return QTimeZone();
    return QTimeZone(QString::fromCFString(CFTimeZoneGetName(timeZone)).toLatin1());
}

/*!
    \since 5.9

    Creates a CFTimeZone from a QTimeZone. The caller owns the CFTimeZone object
    and is responsible for releasing it.

    \sa fromCFTimeZone()
*/
CFTimeZoneRef QTimeZone::toCFTimeZone() const
{
#ifndef QT_NO_DYNAMIC_CAST
    Q_ASSERT(dynamic_cast<const QMacTimeZonePrivate *>(d.data()));
#endif
    const QMacTimeZonePrivate *p = static_cast<const QMacTimeZonePrivate *>(d.data());
    return reinterpret_cast<CFTimeZoneRef>([p->nsTimeZone() copy]);
}

/*!
    \since 5.9

    Constructs a new QTimeZone containing a copy of the NSTimeZone \a timeZone.

    \sa toNSTimeZone()
*/
QTimeZone QTimeZone::fromNSTimeZone(const NSTimeZone *timeZone)
{
    if (!timeZone)
        return QTimeZone();
    return QTimeZone(QString::fromNSString(timeZone.name).toLatin1());
}

/*!
    \since 5.9

    Creates an NSTimeZone from a QTimeZone. The NSTimeZone object is autoreleased.

    \sa fromNSTimeZone()
*/
NSTimeZone *QTimeZone::toNSTimeZone() const
{
    return [static_cast<NSTimeZone *>(toCFTimeZone()) autorelease];
}
#endif

// ----------------------------------------------------------------------------

/*!
    \since 5.8

    Creates a CGRect from a QRect.

    \sa QRectF::fromCGRect()
*/
CGRect QRect::toCGRect() const noexcept
{
    return CGRectMake(x(), y(), width(), height());
}

/*!
    \since 5.8

    Creates a CGRect from a QRectF.

    \sa fromCGRect()
*/
CGRect QRectF::toCGRect() const noexcept
{
    return CGRectMake(x(), y(), width(), height());
}

/*!
    \since 5.8

    Creates a QRectF from CGRect \a rect.

    \sa toCGRect()
*/
QRectF QRectF::fromCGRect(CGRect rect) noexcept
{
    return QRectF(rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
}

// ----------------------------------------------------------------------------

/*!
    \since 5.8

    Creates a CGPoint from a QPoint.

    \sa QPointF::fromCGPoint()
*/
CGPoint QPoint::toCGPoint() const noexcept
{
    return CGPointMake(x(), y());
}

/*!
    \since 5.8

    Creates a CGPoint from a QPointF.

    \sa fromCGPoint()
*/
CGPoint QPointF::toCGPoint() const noexcept
{
    return CGPointMake(x(), y());
}

/*!
    \since 5.8

    Creates a QRectF from CGPoint \a point.

    \sa toCGPoint()
*/
QPointF QPointF::fromCGPoint(CGPoint point) noexcept
{
    return QPointF(point.x, point.y);
}

// ----------------------------------------------------------------------------

/*!
    \since 5.8

    Creates a CGSize from a QSize.

    \sa QSizeF::fromCGSize()
*/
CGSize QSize::toCGSize() const noexcept
{
    return CGSizeMake(width(), height());
}

/*!
    \since 5.8

    Creates a CGSize from a QSizeF.

    \sa fromCGSize()
*/
CGSize QSizeF::toCGSize() const noexcept
{
    return CGSizeMake(width(), height());
}

/*!
    \since 5.8

    Creates a QRectF from \a size.

    \sa toCGSize()
*/
QSizeF QSizeF::fromCGSize(CGSize size) noexcept
{
    return QSizeF(size.width, size.height);
}

QT_END_NAMESPACE
