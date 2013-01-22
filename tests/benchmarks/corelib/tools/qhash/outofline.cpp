/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtTest module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
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
