/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qdatetime.h"

#import <Foundation/Foundation.h>

QT_BEGIN_NAMESPACE

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
