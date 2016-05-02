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

#import <Foundation/Foundation.h>

QT_BEGIN_NAMESPACE

/*! \fn QByteArray QByteArray::fromCFData(CFDataRef data)
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

/*! \fn QByteArray QByteArray::fromRawCFData(CFDataRef data)
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

/*! \fn CFDataRef QByteArray::toCFData() const
    \since 5.3

    Creates a CFData from a QByteArray. The caller owns the CFData object
    and is responsible for releasing it.

    \sa toRawCFData(), fromCFData(), fromRawCFData(), fromRawData()
*/
CFDataRef QByteArray::toCFData() const
{
    return CFDataCreate(kCFAllocatorDefault, reinterpret_cast<const UInt8 *>(data()), length());
}

/*! \fn CFDataRef QByteArray::toRawCFData() const
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

/*! \fn QByteArray QByteArray::fromNSData(const NSData *data)
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

/*! \fn QByteArray QByteArray::fromRawNSData(const NSData *data)
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

/*! \fn NSData QByteArray::toNSData() const
    \since 5.3

    Creates a NSData from a QByteArray. The NSData object is autoreleased.

    \sa fromNSData(), fromRawNSData(), fromRawData(), toRawNSData()
*/
NSData *QByteArray::toNSData() const
{
    return [NSData dataWithBytes:constData() length:size()];
}

/*! \fn NSData QByteArray::toRawNSData() const
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

/*! \fn QString QString::fromCFString(CFStringRef string)
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

/*! \fn CFStringRef QString::toCFString() const
    \since 5.2

    Creates a CFString from a QString. The caller owns the CFString and is
    responsible for releasing it.

    \note this function is only available on OS X and iOS.
*/
CFStringRef QString::toCFString() const
{
    return CFStringCreateWithCharacters(0, reinterpret_cast<const UniChar *>(unicode()), length());
}

/*! \fn QString QString::fromNSString(const NSString *string)
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

/*! \fn NSString QString::toNSString() const
    \since 5.2

    Creates a NSString from a QString. The NSString is autoreleased.

    \note this function is only available on OS X and iOS.
*/
NSString *QString::toNSString() const
{
    return [NSString stringWithCharacters: reinterpret_cast<const UniChar*>(unicode()) length: length()];
}

// ----------------------------------------------------------------------------

/*! \fn QUuid QUuid::fromCFUUID(CFUUIDRef uuid)
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

/*! \fn CFUUIDRef QUuid::toCFUUID() const
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

/*! \fn QUuid QUuid::fromNSUUID(const NSUUID *uuid)
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

/*! \fn NSUUID QUuid::toNSUUID() const
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


/*! \fn QUrl QUrl::fromCFURL(CFURLRef url)
    \since 5.2

    Constructs a QUrl containing a copy of the CFURL \a url.
*/
QUrl QUrl::fromCFURL(CFURLRef url)
{
    return QUrl(QString::fromCFString(CFURLGetString(url)));
}

/*! \fn CFURLRef QUrl::toCFURL() const
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
    \fn QUrl QUrl::fromNSURL(const NSURL *url)
    \since 5.2

    Constructs a QUrl containing a copy of the NSURL \a url.
*/
QUrl QUrl::fromNSURL(const NSURL *url)
{
    return QUrl(QString::fromNSString([url absoluteString]));
}

/*!
    \fn NSURL* QUrl::toNSURL() const
    \since 5.2

    Creates a NSURL from a QUrl. The NSURL is autoreleased.
*/
NSURL *QUrl::toNSURL() const
{
    return [NSURL URLWithString:toString(FullyEncoded).toNSString()];
}

// ----------------------------------------------------------------------------


/*! \fn QDateTime QDateTime::fromCFDate(CFDateRef date)
    \since 5.5

    Constructs a new QDateTime containing a copy of the CFDate \a date.

    \sa toCFDate()
*/
QDateTime QDateTime::fromCFDate(CFDateRef date)
{
    if (!date)
        return QDateTime();
    return QDateTime::fromMSecsSinceEpoch(static_cast<qint64>((CFDateGetAbsoluteTime(date)
                                                    + kCFAbsoluteTimeIntervalSince1970) * 1000));
}

/*! \fn CFDateRef QDateTime::toCFDate() const
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

/*! \fn QDateTime QDateTime::fromNSDate(const NSDate *date)
    \since 5.5

    Constructs a new QDateTime containing a copy of the NSDate \a date.

    \sa toNSDate()
*/
QDateTime QDateTime::fromNSDate(const NSDate *date)
{
    if (!date)
        return QDateTime();
    return QDateTime::fromMSecsSinceEpoch(static_cast<qint64>([date timeIntervalSince1970] * 1000));
}

/*! \fn NSDate QDateTime::toNSDate() const
    \since 5.5

    Creates an NSDate from a QDateTime. The NSDate object is autoreleased.

    \sa fromNSDate()
*/
NSDate *QDateTime::toNSDate() const
{
    return [NSDate
            dateWithTimeIntervalSince1970:static_cast<NSTimeInterval>(toMSecsSinceEpoch()) / 1000];
}

QT_END_NAMESPACE
