/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Copyright (C) 2014 Petroules Corporation.
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

#include "qdatetime.h"

#import <Foundation/Foundation.h>

QT_BEGIN_NAMESPACE

QDateTime QDateTime::fromCFDate(CFDateRef date)
{
    if (!date)
        return QDateTime();
    return QDateTime::fromMSecsSinceEpoch(static_cast<qint64>((CFDateGetAbsoluteTime(date)
                                                    + kCFAbsoluteTimeIntervalSince1970) * 1000));
}

CFDateRef QDateTime::toCFDate() const
{
    return CFDateCreate(kCFAllocatorDefault, (static_cast<CFAbsoluteTime>(toMSecsSinceEpoch())
                                                    / 1000) - kCFAbsoluteTimeIntervalSince1970);
}

QDateTime QDateTime::fromNSDate(const NSDate *date)
{
    if (!date)
        return QDateTime();
    return QDateTime::fromMSecsSinceEpoch(static_cast<qint64>([date timeIntervalSince1970] * 1000));
}

NSDate *QDateTime::toNSDate() const
{
    return [NSDate
            dateWithTimeIntervalSince1970:static_cast<NSTimeInterval>(toMSecsSinceEpoch()) / 1000];
}

QT_END_NAMESPACE
