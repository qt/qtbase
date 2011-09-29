/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qfontengine_mac_p.h"

#include <private/qapplication_p.h>
#include <private/qfontengine_p.h>
#include <private/qpainter_p.h>
#include <private/qtextengine_p.h>
#include <qbitmap.h>
#include <private/qpaintengine_mac_p.h>
#include <private/qprintengine_mac_p.h>
#include <qglobal.h>
#include <qpixmap.h>
#include <qpixmapcache.h>
#include <qvarlengtharray.h>
#include <qdebug.h>
#include <qendian.h>
#include <qmath.h>
#include <private/qimage_p.h>

#include <ApplicationServices/ApplicationServices.h>
#include <AppKit/AppKit.h>

QT_BEGIN_NAMESPACE

/*****************************************************************************
  QFontEngine debug facilities
 *****************************************************************************/
//#define DEBUG_ADVANCES

extern int qt_antialiasing_threshold; // QApplication.cpp

#ifndef FixedToQFixed
#define FixedToQFixed(a) QFixed::fromFixed((a) >> 10)
#define QFixedToFixed(x) ((x).value() << 10)
#endif

class QMacFontPath
{
    float x, y;
    QPainterPath *path;
public:
    inline QMacFontPath(float _x, float _y, QPainterPath *_path) : x(_x), y(_y), path(_path) { }
    inline void setPosition(float _x, float _y) { x = _x; y = _y; }
    inline void advance(float _x) { x += _x; }
    static OSStatus lineTo(const Float32Point *, void *);
    static OSStatus cubicTo(const Float32Point *, const Float32Point *,
                            const Float32Point *, void *);
    static OSStatus moveTo(const Float32Point *, void *);
    static OSStatus closePath(void *);
};

OSStatus QMacFontPath::lineTo(const Float32Point *pt, void *data)

{
    QMacFontPath *p = static_cast<QMacFontPath*>(data);
    p->path->lineTo(p->x + pt->x, p->y + pt->y);
    return noErr;
}

OSStatus QMacFontPath::cubicTo(const Float32Point *cp1, const Float32Point *cp2,
                               const Float32Point *ep, void *data)

{
    QMacFontPath *p = static_cast<QMacFontPath*>(data);
    p->path->cubicTo(p->x + cp1->x, p->y + cp1->y,
                     p->x + cp2->x, p->y + cp2->y,
                     p->x + ep->x, p->y + ep->y);
    return noErr;
}

OSStatus QMacFontPath::moveTo(const Float32Point *pt, void *data)
{
    QMacFontPath *p = static_cast<QMacFontPath*>(data);
    p->path->moveTo(p->x + pt->x, p->y + pt->y);
    return noErr;
}

OSStatus QMacFontPath::closePath(void *data)
{
    static_cast<QMacFontPath*>(data)->path->closeSubpath();
    return noErr;
}

QT_END_NAMESPACE
