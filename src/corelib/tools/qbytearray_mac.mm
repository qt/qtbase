/****************************************************************************
**
** Copyright (C) 2014 Samuel Gaist <samuel.gaist@edeltech.ch>
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qbytearray.h"

#import <Foundation/Foundation.h>

QT_BEGIN_NAMESPACE

QByteArray QByteArray::fromCFData(CFDataRef data)
{
    if (!data)
        return QByteArray();

    return QByteArray(reinterpret_cast<const char *>(CFDataGetBytePtr(data)), CFDataGetLength(data));
}

QByteArray QByteArray::fromRawCFData(CFDataRef data)
{
    if (!data)
        return QByteArray();

    return QByteArray::fromRawData(reinterpret_cast<const char *>(CFDataGetBytePtr(data)), CFDataGetLength(data));
}

CFDataRef QByteArray::toCFData() const
{
    return CFDataCreate(kCFAllocatorDefault, reinterpret_cast<const UInt8 *>(data()), length());
}

CFDataRef QByteArray::toRawCFData() const
{
    return CFDataCreateWithBytesNoCopy(kCFAllocatorDefault, reinterpret_cast<const UInt8 *>(data()),
                    length(), kCFAllocatorNull);
}

QByteArray QByteArray::fromNSData(const NSData *data)
{
    if (!data)
        return QByteArray();
    return QByteArray(reinterpret_cast<const char *>([data bytes]), [data length]);
}

QByteArray QByteArray::fromRawNSData(const NSData *data)
{
    if (!data)
        return QByteArray();
    return QByteArray::fromRawData(reinterpret_cast<const char *>([data bytes]), [data length]);
}

NSData *QByteArray::toNSData() const
{
    return [NSData dataWithBytes:constData() length:size()];
}

NSData *QByteArray::toRawNSData() const
{
    // const_cast is fine here because NSData is immutable thus will never modify bytes we're giving it
    return [NSData dataWithBytesNoCopy:const_cast<char *>(constData()) length:size() freeWhenDone:NO];
}

QT_END_NAMESPACE
