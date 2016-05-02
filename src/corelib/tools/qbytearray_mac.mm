/****************************************************************************
**
** Copyright (C) 2014 Samuel Gaist <samuel.gaist@edeltech.ch>
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qbytearray.h"

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

QT_END_NAMESPACE
