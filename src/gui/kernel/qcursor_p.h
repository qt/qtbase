/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QCURSOR_P_H
#define QCURSOR_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of a number of Qt sources files.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>
#include "QtCore/qatomic.h"
#include "QtCore/qnamespace.h"
#include "QtGui/qpixmap.h"


QT_BEGIN_NAMESPACE


class QBitmap;
class QCursorData {
public:
    QCursorData(Qt::CursorShape s = Qt::ArrowCursor);
    ~QCursorData();

    static void initialize();
    static void cleanup();

    QAtomicInt ref;
    Qt::CursorShape cshape;
    QBitmap  *bm, *bmm;
    QPixmap pixmap;
    short     hx, hy;
    static bool initialized;
    void update();
    static QCursorData *setBitmap(const QBitmap &bitmap, const QBitmap &mask, int hotX, int hotY,
                                  qreal devicePixelRatio);
};

extern QCursorData *qt_cursorTable[Qt::LastCursor + 1]; // qcursor.cpp

QT_END_NAMESPACE

#endif // QCURSOR_P_H
