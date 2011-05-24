/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtTest module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qhash_string.h"

static void doHash(const unsigned short *p, uint &h)
{
#if 1
    // Copied from static uint hash(const QChar *p, int n).
    // Possibly not the cheapest way.
    h = (h << 4) + (*p++);
    h ^= (h & 0xf0000000) >> 23;
    h &= 0x0fffffff;

    h = (h << 4) + (*p++);
    h ^= (h & 0xf0000000) >> 23;
    h &= 0x0fffffff;

    h = (h << 4) + (*p++);
    h ^= (h & 0xf0000000) >> 23;
    h &= 0x0fffffff;

    h = (h << 4) + (*p++);
    h ^= (h & 0xf0000000) >> 23;
    h &= 0x0fffffff;
#else
    // Faster, but probably less spread.
    h ^= *(unsigned int *)p;
#endif
}

QT_BEGIN_NAMESPACE

uint qHash(const String &str)
{
    const unsigned short *p = (unsigned short *)str.constData();
    const int s = str.size();
    switch (s) {
        case 0: return 0;
        case 1: return *p;
        case 2: return *(unsigned int *)p;
        case 3: return (*(unsigned int *)p) ^ *(p + 2);
        //case 3: return (*p << 11) + (*(p + 1) << 22) + *(p + 2);
    }
    uint h = 0;
    doHash(p, h);
    doHash(p + s / 2 - 2, h);
    doHash(p + s - 4, h);
    return h;
}

QT_END_NAMESPACE
