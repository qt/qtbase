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

#include <qapplication.h>
#include <qwidget.h>
#include <qpainter.h>
#include <qdrawutil.h>
#include "qdecorationwindows_qws.h"

QT_BEGIN_NAMESPACE

#if !defined(QT_NO_QWS_DECORATION_WINDOWS) || defined(QT_PLUGIN)

#ifndef QT_NO_IMAGEFORMAT_XPM

/* XPM */
static const char * const win_close_xpm[] = {
"16 16 4 1",
"  s None  c None",
". c #000000",
"X c #FFFFFF",
"Y c #707070",
"                ",
"                ",
"                ",
"   Y.      .Y   ",
"    ..    ..    ",
"     ..  ..     ",
"      .YY.      ",
"      Y..Y      ",
"      .YY.      ",
"     ..  ..     ",
"    ..    ..    ",
"   Y.      .Y   ",
"                ",
"                ",
"                ",
"                "};

static const char * const win_help_xpm[] = {
"16 16 3 1",
"       s None  c None",
".      c #ffffff",
"X      c #000000",
"                ",
"                ",
"                ",
"     XXXXXX     ",
"    XX    XX    ",
"    XX    XX    ",
"          XX    ",
"         XX     ",
"       XX       ",
"       XX       ",
"                ",
"       XX       ",
"       XX       ",
"                ",
"                ",
"                "};

static const char * const win_maximize_xpm[] = {
"16 16 4 1",
"  s None  c None",
". c #000000",
"X c #FFFFFF",
"Y c #707070",
"                ",
"                ",
"                ",
"   ..........   ",
"   ..........   ",
"   .        .   ",
"   .        .   ",
"   .        .   ",
"   .        .   ",
"   .        .   ",
"   .        .   ",
"   ..........   ",
"                ",
"                ",
"                ",
"                "};

static const char * const win_minimize_xpm[] = {
"16 16 4 1",
"  s None  c None",
". c #000000",
"X c #FFFFFF",
"Y c #707070",
"                ",
"                ",
"                ",
"                ",
"                ",
"                ",
"                ",
"                ",
"                ",
"                ",
"    ........    ",
"    ........    ",
"                ",
"                ",
"                ",
"                "};

static const char * const win_normalize_xpm[] = {
"16 16 4 1",
"  s None  c None",
". c #000000",
"X c #FFFFFF",
"Y c #707070",
"                ",
"                ",
"     .........  ",
"     .........  ",
"     .       .  ",
"     .       .  ",
"  .........  .  ",
"  .........  .  ",
"  .       .  .  ",
"  .       ....  ",
"  .       .     ",
"  .       .     ",
"  .........     ",
"                ",
"                ",
"                "};

#endif // QT_NO_IMAGEFORMAT_XPM


QDecorationWindows::QDecorationWindows()
    : QDecorationDefault()
{
    menu_width = 16;
    help_width = 18;
    minimize_width = 18;
    maximize_width = 18;
    close_width = 18;
}

QDecorationWindows::~QDecorationWindows()
{
}

const char **QDecorationWindows::xpmForRegion(int reg)
{
#ifdef QT_NO_IMAGEFORMAT_XPM
    Q_UNUSED(reg);
#else
    switch(reg)
    {
    case Close:
        return (const char **)win_close_xpm;
    case Help:
        return (const char **)win_help_xpm;
    case Minimize:
        return (const char **)win_minimize_xpm;
    case Maximize:
        return (const char **)win_maximize_xpm;
    case Normalize:
        return (const char **)win_normalize_xpm;
    default:
        return QDecorationDefault::xpmForRegion(reg);
    }
#endif
    return 0;
}

QRegion QDecorationWindows::region(const QWidget *widget, const QRect &rect, int type)
{
    Qt::WindowFlags flags = widget->windowFlags();
    bool hasTitle = flags & Qt::WindowTitleHint;
    bool hasSysMenu = flags & Qt::WindowSystemMenuHint;
    bool hasContextHelp = flags & Qt::WindowContextHelpButtonHint;
    bool hasMinimize = flags & Qt::WindowMinimizeButtonHint;
    bool hasMaximize = flags & Qt::WindowMaximizeButtonHint;
    const QFontMetrics fontMetrics = QApplication::fontMetrics();
    int titleHeight = hasTitle ? qMax(20, fontMetrics.height()) : 0;
    int state = widget->windowState();
    bool isMinimized = state & Qt::WindowMinimized;
    bool isMaximized = state & Qt::WindowMaximized;

    QRegion region;
    switch (type) {
        case Menu: {
                if (hasSysMenu) {
                    region = QRect(rect.left() + 2, rect.top() - titleHeight,
                                   menu_width, titleHeight);
                }
            }
            break;

        case Title: {
                QRect r(rect.left()
                        + (hasSysMenu ? menu_width + 4: 0),
                        rect.top() - titleHeight,
                        rect.width()
                        - (hasSysMenu ? menu_width : 0)
                        - close_width
                        - (hasMaximize ? maximize_width : 0)
                        - (hasMinimize ? minimize_width : 0)
                        - (hasContextHelp ? help_width : 0)
                        - 3,
                        titleHeight);
                if (r.width() > 0)
                    region = r;
            }
            break;
        case Help: {
                if (hasContextHelp) {
                    QRect r(rect.right()
                            - close_width
                            - (hasMaximize ? maximize_width : 0)
                            - (hasMinimize ? minimize_width : 0)
                            - help_width - 3, rect.top() - titleHeight,
                            help_width, titleHeight);
                    if (r.left() > rect.left() + titleHeight)
                        region = r;
                }
            }
            break;

        case Minimize: {
                if (hasMinimize && !isMinimized) {
                    QRect r(rect.right() - close_width
                            - (hasMaximize ? maximize_width : 0)
                            - minimize_width - 3, rect.top() - titleHeight,
                            minimize_width, titleHeight);
                    if (r.left() > rect.left() + titleHeight)
                        region = r;
                }
            }
            break;

        case Maximize: {
                if (hasMaximize && !isMaximized) {
                    QRect r(rect.right() - close_width - maximize_width - 3,
                            rect.top() - titleHeight, maximize_width, titleHeight);
                    if (r.left() > rect.left() + titleHeight)
                        region = r;
                }
            }
            break;

        case Normalize: {
                if (hasMinimize && isMinimized) {
                    QRect r(rect.right() - close_width
                            - (hasMaximize ? maximize_width : 0)
                            - minimize_width - 3, rect.top() - titleHeight,
                            minimize_width, titleHeight);
                    if (r.left() > rect.left() + titleHeight)
                        region = r;
                } else if (hasMaximize && isMaximized) {
                    QRect r(rect.right() - close_width - maximize_width - 3,
                            rect.top() - titleHeight, maximize_width, titleHeight);
                    if (r.left() > rect.left() + titleHeight)
                        region = r;
                }
            }
            break;

        case Close: {
                QRect r(rect.right() - close_width - 1, rect.top() - titleHeight,
                        close_width, titleHeight);
                if (r.left() > rect.left() + titleHeight)
                    region = r;
            }
            break;

        default:
            region = QDecorationDefault::region(widget, rect, type);
            break;
    }

    return region;
}

bool QDecorationWindows::paint(QPainter *painter, const QWidget *widget, int decorationRegion,
                               DecorationState state)
{
    if (decorationRegion == None)
        return false;

    const QRect titleRect = QDecoration::region(widget, Title).boundingRect();
    const QPalette pal = QApplication::palette();
    QRegion oldClipRegion = painter->clipRegion();

    bool paintAll = (decorationRegion == int(All));
    if ((paintAll || decorationRegion & Title && titleRect.width() > 0) && state == Normal
        && (widget->windowFlags() & Qt::WindowTitleHint) ) {
        painter->setClipRegion(oldClipRegion);
        QColor fromBrush, toBrush;
        QPen   titlePen;

        if (widget == qApp->activeWindow() || qApp->activeWindow() == qApp->activePopupWidget()) {
            fromBrush = pal.color(QPalette::Highlight);
            titlePen  = pal.color(QPalette::HighlightedText);
        } else {
            fromBrush = pal.color(QPalette::Window);
            titlePen  = pal.color(QPalette::Text);
        }
        toBrush = fromBrush.lighter(300);

        painter->setPen(Qt::NoPen);
        QPoint p1(titleRect.x(), titleRect.y() + titleRect.height()/2);
        QPoint p2(titleRect.right(), titleRect.y() + titleRect.height()/2);
        QLinearGradient lg(p1, p2);
        lg.setColorAt(0, fromBrush);
        lg.setColorAt(1, toBrush);
        painter->fillRect(titleRect, lg);

        painter->setPen(titlePen);
        painter->drawText(titleRect, Qt::AlignVCenter, windowTitleFor(widget));
        decorationRegion ^= Title;
    }

    return QDecorationDefault::paint(painter, widget, decorationRegion, state);
}

void QDecorationWindows::paintButton(QPainter *painter, const QWidget *widget, int buttonRegion,
                                     DecorationState state, const QPalette &pal)
{
    QBrush fromBrush, toBrush;
    QPen   titlePen;

    if (widget == qApp->activeWindow() || qApp->activeWindow() == qApp->activePopupWidget()) {
        fromBrush = pal.brush(QPalette::Highlight);
        titlePen  = pal.color(QPalette::HighlightedText);
    } else {
        fromBrush = pal.brush(QPalette::Window);
        titlePen  = pal.color(QPalette::Text);
    }
    toBrush = fromBrush.color().lighter(300);

    QRect brect(QDecoration::region(widget, buttonRegion).boundingRect());
    if (buttonRegion != Close && buttonRegion != Menu)
        painter->fillRect(brect, toBrush);
    else
        painter->fillRect(brect.x() - 2, brect.y(), brect.width() + 4, brect.height(),
                          buttonRegion == Menu ? fromBrush : toBrush);

    int xoff = 1;
    int yoff = 2;
    const QPixmap pm = pixmapFor(widget, buttonRegion, xoff, yoff);
    if (buttonRegion != Menu) {
        if (state & Normal) {
            qDrawWinPanel(painter, brect.x(), brect.y() + 2, brect.width(),
                          brect.height() - 4, pal, false, &pal.brush(QPalette::Window));
        } else if (state & Pressed) {
            qDrawWinPanel(painter, brect.x(), brect.y() + 2, brect.width(),
                          brect.height() - 4, pal, true, &pal.brush(QPalette::Window));
            ++xoff;
            ++yoff;
        }
    } else {
        xoff = 0;
        yoff = 2;
    }

    if (!pm.isNull())
        painter->drawPixmap(brect.x() + xoff, brect.y() + yoff, pm);
}

#endif // QT_NO_QWS_DECORATION_WINDOWS || QT_PLUGIN

QT_END_NAMESPACE
