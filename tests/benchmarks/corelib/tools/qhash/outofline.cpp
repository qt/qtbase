/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtTest module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "main.h"

QT_BEGIN_NAMESPACE

uint qHash(const Qt4String &str)
{
    int n = str.length();
    const QChar *p = str.unicode();
    uint h = 0;

    while (n--) {
        h = (h << 4) + (*p++).unicode();
        h ^= (h & 0xf0000000) >> 23;
        h &= 0x0fffffff;
    }
    return h;
}

uint qHash(const Qt50String &key, uint seed)
{
    const QChar *p = key.unicode();
    int len = key.size();
    uint h = seed;
    for (int i = 0; i < len; ++i)
        h = 31 * h + p[i].unicode();
    return h;
}

// The Java's hashing algorithm for strings is a variation of D. J. Bernstein
// hashing algorithm appeared here http://cr.yp.to/cdb/cdb.txt
// and informally known as DJB33XX - DJB's 33 Times Xor.
// Java uses DJB31XA, that is, 31 Times Add.
// The original algorithm was a loop around  "(h << 5) + h ^ c",
// which is indeed "h * 33 ^ c"; it was then changed to
// "(h << 5) - h ^ c", so "h * 31 ^ c", and the XOR changed to a sum:
// "(h << 5) - h + c", which can save some assembly instructions.
// Still, we can avoid writing the multiplication as "(h << 5) - h"
// -- the compiler will turn it into a shift and an addition anyway
// (for instance, gcc 4.4 does that even at -O0).
uint qHash(const JavaString &str)
{
    const unsigned short *p = (unsigned short *)str.constData();
    const int len = str.size();

    uint h = 0;

    for (int i = 0; i < len; ++i)
        h = 31 * h + p[i];

    return h;
}

QT_END_NAMESPACE
