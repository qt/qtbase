/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWINDOWSCE_P_H
#define QWINDOWSCE_P_H

#include "qwindowscestyle.h"
#include <private/qwindowsstyle_p.h>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qapplication_*.cpp, qwidget*.cpp and qfiledialog.cpp.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

QT_BEGIN_NAMESPACE

class QPainter;
class QPalette;
class QPoint;
class QColor;
class QBrush;
class QRect;

// Private class
class QWindowsCEStylePrivate : public QWindowsStylePrivate
{   
    Q_DECLARE_PUBLIC(QWindowsCEStyle)
public:
    inline QWindowsCEStylePrivate()
    { }

    
static void drawWinCEButton(QPainter *p, int x, int y, int w, int h,
                            const QPalette &pal, bool sunken = false,
                            const QBrush *fill = 0);

static void drawWinCEButton(QPainter *p, const QRect &r,
                            const QPalette &pal, bool sunken = false,
                            const QBrush *fill = 0);

static void drawWinCEPanel(QPainter *p, int x, int y, int w, int h,
                           const QPalette &pal, bool sunken = false,
                           const QBrush *fill = 0);

static void drawWinCEPanel(QPainter *p, const QRect &r,
                           const QPalette &pal, bool sunken = false,
                           const QBrush *fill = 0);

static void drawWinShades(QPainter *p,
                          int x, int y, int w, int h,
                          const QColor &c1, const QColor &c2,
                          const QColor &c3, const QColor &c4,
                          const QBrush *fill);

static void drawWinCEShades(QPainter *p,
                            int x, int y, int w, int h,
                            const QColor &c1, const QColor &c2,
                            const QColor &c3, const QColor &c4,
                            const QBrush *fill);

static void drawWinCEShadesSunken(QPainter *p,
                                  int x, int y, int w, int h,
                                  const QColor &c1, const QColor &c2,
                                  const QColor &c3, const QColor &c4,
                                  const QBrush *fill);




};

QT_END_NAMESPACE

#endif //QWINDOWSCE_P_H
