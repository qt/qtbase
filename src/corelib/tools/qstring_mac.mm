/****************************************************************************
**
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

#include "qstring.h"

#import <Foundation/Foundation.h>

QT_BEGIN_NAMESPACE

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

CFStringRef QString::toCFString() const
{
    return CFStringCreateWithCharacters(0, reinterpret_cast<const UniChar *>(unicode()), length());
}

QString QString::fromNSString(const NSString *string)
{
    if (!string)
        return QString();
   QString qstring;
   qstring.resize([string length]);
   [string getCharacters: reinterpret_cast<unichar*>(qstring.data()) range: NSMakeRange(0, [string length])];
   return qstring;
}

NSString *QString::toNSString() const
{
    return [NSString stringWithCharacters: reinterpret_cast<const UniChar*>(unicode()) length: length()];
}

QT_END_NAMESPACE
