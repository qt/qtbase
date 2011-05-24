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

#ifndef QDECORATION_QWS_H
#define QDECORATION_QWS_H

#include <QtGui/qregion.h>
#include <QtGui/qwidget.h>
#include <QtGui/qaction.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QPopupMenu;
class QMenu;

#ifndef QT_NO_ACTION
class QDecorationAction : public QAction
{
public:
    QDecorationAction(const QString &text, QObject* parent, int region)
        : QAction(text, parent), reg(region) {}
    int reg;
};
#endif // QT_NO_ACTION

/*
 Implements decoration styles
*/
class Q_GUI_EXPORT QDecoration
{
public:
    QDecoration() {}
    virtual ~QDecoration() {}

    /* AABBBBBBBBBBCC   Items in DecorationRegion:
       AijjjjjjjklmnC
       A            C   A = TopLeft      B = Top        C = TopRight
       D            E   D = Left                        E = Right
       D            E   F = BottomLeft   H = Bottom     G = BottomRight
       F            G   i = Menu         j = Title      k = Help
       FFHHHHHHHHHHGG   l = Minimize     m = Maximize   n = Close

    */

    enum DecorationRegion {
        None        = 0x0000000000, All      = 0x7fffffff,
        TopLeft     = 0x0000000001, Top      = 0x0000000002, TopRight    = 0x0000000004,
        Left        = 0x0000000008,                          Right       = 0x0000000010,
        BottomLeft  = 0x0000000020, Bottom   = 0x0000000040, BottomRight = 0x0000000080,
        Borders     = 0x00000000ff,
        Menu        = 0x0000000100, Title    = 0x0000000200, Help        = 0x0000000400,
        Minimize    = 0x0000000800, Maximize = 0x0000001000, Normalize   = 0x0000002000,
        Close       = 0x0000004000, Move     = 0x0000008000, Resize      = 0x0000010000
    };

    enum DecorationState { Normal = 0x04, Disabled = 0x08, Hover = 0x01, Pressed = 0x02 };

    virtual QRegion region(const QWidget *w, const QRect &rect, int decorationRegion = All ) = 0;
    QRegion region(const QWidget *w, int decorationRegion = All )
    { return region(w, w->rect(), decorationRegion); }
    virtual int regionAt(const QWidget *w, const QPoint &point);

    virtual void regionClicked(QWidget *widget, int region);
    virtual void regionDoubleClicked(QWidget *widget, int region);
#ifndef QT_NO_MENU
    virtual void buildSysMenu(QWidget *widget, QMenu *menu);
    void menuTriggered(QWidget *widget, QAction *action);
#endif

    static void startMove(QWidget *widget);
    static void startResize(QWidget *widget);

    virtual bool paint(QPainter *p, const QWidget *w, int decorationRegion = All,
                       DecorationState state = Normal) = 0;

};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QDECORATION_QWS_H
