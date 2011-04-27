/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui>
#include "mydecoration.h"

/* XPM */
static const char * const _close_button[] = {
"16 16 3 1",
"  c none",
". c #ffafaf",
"x c #000000",
"                ",
" xxxxxxxxxxxxxx ",
" x............x ",
" x.x........x.x ",
" x..x......x..x ",
" x...x....x...x ",
" x....x..x....x ",
" x.....xx.....x ",
" x.....xx.....x ",
" x....x..x....x ",
" x...x....x...x ",
" x..x......x..x ",
" x.x........x.x ",
" x............x ",
" xxxxxxxxxxxxxx ",
"                "};

static const char * const _normalize_button[] = {
"16 16 4 1",
"  c none",
". c #dfdfff",
"o c #7f7f7f",
"x c #000000",
"                ",
" xxxxxxxxxxxxxx ",
" xx...........x ",
" x.x..........x ",
" x..x..x......x ",
" x...xxx......x ",
" x...xxx......x ",
" x..xxxx......x ",
" x............x ",
" x.......xxxx.x ",
" x.......x..x.x ",
" x.......x..x.x ",
" x.......xxxx.x ",
" x............x ",
" xxxxxxxxxxxxxx ",
"                "};

static const char * const _maximize_button[] = {
"16 16 4 1",
"  c none",
". c #dfdfff",
"o c #7f7f7f",
"x c #000000",
"                ",
" xxxxxxxxxxxxxx ",
" x............x ",
" x.......xxxx.x ",
" x........xxx.x ",
" x........xxx.x ",
" x.......x..x.x ",
" x......x.....x ",
" x.....x......x ",
" x.oooo.......x ",
" x.o..o.......x ",
" x.o..o.......x ",
" x.oooo.......x ",
" x............x ",
" xxxxxxxxxxxxxx ",
"                "};

static const char * const _help_button[] = {
"16 16 3 1",
"  c none",
". c #afffdf",
"x c #000000",
"                ",
" xxxxxxxxxxxxxx ",
" x............x ",
" x....xxxx....x ",
" x..xx....xx..x ",
" x.xx......xx.x ",
" x.xx......xx.x ",
" x........xx..x ",
" x......xx....x ",
" x.....xx.....x ",
" x.....xx.....x ",
" x............x ",
" x.....xx.....x ",
" x............x ",
" xxxxxxxxxxxxxx ",
"                "};

//! [constructor start]
MyDecoration::MyDecoration()
    : QDecorationDefault()
{
    border = 4;
    titleHeight = 24;
    buttonWidth = 20;
    buttonHeight = 20;
    buttonMargin = 2;
    buttonHints << Qt::Window
                << Qt::WindowMaximizeButtonHint
                << Qt::WindowContextHelpButtonHint;
    //! [constructor start]

    //! [map window flags to decoration regions]
    buttonHintMap[Qt::Window] = Close;
    buttonHintMap[Qt::WindowMaximizeButtonHint] = Maximize;
    buttonHintMap[Qt::WindowContextHelpButtonHint] = Help;
    //! [map window flags to decoration regions]

    //! [map decoration regions to pixmaps]
    normalButtonPixmaps[Close] = QPixmap(_close_button);
    normalButtonPixmaps[Maximize] = QPixmap(_maximize_button);
    normalButtonPixmaps[Normalize] = QPixmap(_normalize_button);
    normalButtonPixmaps[Help] = QPixmap(_help_button);

    maximizedButtonPixmaps[Close] = QPixmap(_close_button);
    maximizedButtonPixmaps[Maximize] = QPixmap(_normalize_button);
    maximizedButtonPixmaps[Normalize] = QPixmap(_normalize_button);
    maximizedButtonPixmaps[Help] = QPixmap(_help_button);
    //! [map decoration regions to pixmaps]

    //! [constructor end]
    stateRegions << Close << Maximize << Help;
}
//! [constructor end]

//! [region start]
QRegion MyDecoration::region(const QWidget *widget, const QRect &insideRect,
                             int decorationRegion)
{
    //! [region start]
    //! [calculate the positions of buttons based on the window flags used]
    QHash<DecorationRegion, int> buttons;
    Qt::WindowFlags flags = widget->windowFlags();
    int dx = -buttonMargin - buttonWidth;

    foreach (Qt::WindowType button, buttonHints) {
        if (flags & button) {
            int x = (buttons.size() + 1) * dx;
            buttons[buttonHintMap[button]] = x;
        }
    }
    //! [calculate the positions of buttons based on the window flags used]

    //! [calculate the extent of the title]
    int titleRightMargin = buttons.size() * dx;

    QRect outsideRect(insideRect.left() - border,
                      insideRect.top() - titleHeight - border,
                      insideRect.width() + 2 * border,
                      insideRect.height() + titleHeight + 2 * border);
    //! [calculate the extent of the title]

    //! [check for all regions]
    QRegion region;

    if (decorationRegion == All) {
        region += QRegion(outsideRect) - QRegion(insideRect);
        return region;
    }
    //! [check for all regions]

    //! [compose a region based on the decorations specified]
    if (decorationRegion & Title) {
        QRect rect = outsideRect.adjusted(border, border, -border, 0);
        rect.setHeight(titleHeight);

        // Adjust the width to accommodate buttons.
        rect.setWidth(qMax(0, rect.width() + titleRightMargin));
        region += rect;
    }
    if (decorationRegion & Top) {
        QRect rect = outsideRect.adjusted(border, 0, -border, 0);
        rect.setHeight(border);
        region += rect;
    }
    if (decorationRegion & Left) {
        QRect rect = outsideRect.adjusted(0, border, 0, -border);
        rect.setWidth(border);
        region += rect;
    }
    if (decorationRegion & Right) {
        QRect rect = outsideRect.adjusted(0, border, 0, -border);
        rect.setLeft(rect.right() + 1 - border);
        region += rect;
    }
    if (decorationRegion & Bottom) {
        QRect rect = outsideRect.adjusted(border, 0, -border, 0);
        rect.setTop(rect.bottom() + 1 - border);
        region += rect;
    }
    if (decorationRegion & TopLeft) {
        QRect rect = outsideRect;
        rect.setWidth(border);
        rect.setHeight(border);
        region += rect;
    }
    if (decorationRegion & TopRight) {
        QRect rect = outsideRect;
        rect.setLeft(rect.right() + 1 - border);
        rect.setHeight(border);
        region += rect;
    }
    if (decorationRegion & BottomLeft) {
        QRect rect = outsideRect;
        rect.setWidth(border);
        rect.setTop(rect.bottom() + 1 - border);
        region += rect;
    }
    if (decorationRegion & BottomRight) {
        QRect rect = outsideRect;
        rect.setLeft(rect.right() + 1 - border);
        rect.setTop(rect.bottom() + 1 - border);
        region += rect;
    }
    //! [compose a region based on the decorations specified]

    //! [add a region for each button only if it is present]
    foreach (QDecoration::DecorationRegion testRegion, stateRegions) {
        if (decorationRegion & testRegion and buttons.contains(testRegion)) {
            // Inside the title rectangle
            QRect rect = outsideRect.adjusted(border, border, -border, 0);
            rect.setHeight(titleHeight);

            dx = buttons[testRegion];
            rect.setLeft(rect.right() + 1 + dx);
            rect.setWidth(buttonWidth + buttonMargin);
            region += rect;
        }
    }
    //! [add a region for each button only if it is present]

    //! [region end]
    return region;
}
//! [region end]

//! [paint start]
bool MyDecoration::paint(QPainter *painter, const QWidget *widget,
                         int decorationRegion, DecorationState state)
{
    if (decorationRegion == None)
        return false;
    //! [paint start]

    //! [paint different regions]
    bool handled = false;

    QPalette palette = QApplication::palette();
    QHash<DecorationRegion, QPixmap> buttonPixmaps;

    if (widget->windowState() == Qt::WindowMaximized)
        buttonPixmaps = maximizedButtonPixmaps;
    else
        buttonPixmaps = normalButtonPixmaps;

    if (decorationRegion & Title) {
        QRect rect = QDecoration::region(widget, Title).boundingRect();
        painter->fillRect(rect, palette.brush(QPalette::Base));
        painter->save();
        painter->setPen(QPen(palette.color(QPalette::Text)));
        painter->drawText(rect, Qt::AlignCenter, widget->windowTitle());
        painter->restore();
        handled = true;
    }
    if (decorationRegion & Top) {
        QRect rect = QDecoration::region(widget, Top).boundingRect();
        painter->fillRect(rect, palette.brush(QPalette::Dark));
        handled = true;
    }
    if (decorationRegion & Left) {
        QRect rect = QDecoration::region(widget, Left).boundingRect();
        painter->fillRect(rect, palette.brush(QPalette::Dark));
        handled = true;
    }
    if (decorationRegion & Right) {
        QRect rect = QDecoration::region(widget, Right).boundingRect();
        painter->fillRect(rect, palette.brush(QPalette::Dark));
        handled = true;
    }
    if (decorationRegion & Bottom) {
        QRect rect = QDecoration::region(widget, Bottom).boundingRect();
        painter->fillRect(rect, palette.brush(QPalette::Dark));
        handled = true;
    }
    if (decorationRegion & TopLeft) {
        QRect rect = QDecoration::region(widget, TopLeft).boundingRect();
        painter->fillRect(rect, palette.brush(QPalette::Dark));
        handled = true;
    }
    if (decorationRegion & TopRight) {
        QRect rect = QDecoration::region(widget, TopRight).boundingRect();
        painter->fillRect(rect, palette.brush(QPalette::Dark));
        handled = true;
    }
    if (decorationRegion & BottomLeft) {
        QRect rect = QDecoration::region(widget, BottomLeft).boundingRect();
        painter->fillRect(rect, palette.brush(QPalette::Dark));
        handled = true;
    }
    if (decorationRegion & BottomRight) {
        QRect rect = QDecoration::region(widget, BottomRight).boundingRect();
        painter->fillRect(rect, palette.brush(QPalette::Dark));
        handled = true;
    }
    //! [paint different regions]

    //! [paint buttons]
    int margin = (titleHeight - 16) / 2;
    Qt::WindowFlags flags = widget->windowFlags();

    foreach (DecorationRegion testRegion, stateRegions) {
        if (decorationRegion & testRegion && flags & buttonHintMap.key(testRegion)) {
            QRect rect = QDecoration::region(
                widget, testRegion).boundingRect();
            painter->fillRect(rect, palette.brush(QPalette::Base));

            QRect buttonRect = rect.adjusted(0, margin, -buttonMargin - margin,
                                             -buttonMargin);
            painter->drawPixmap(buttonRect.topLeft(), buttonPixmaps[testRegion]);
            handled = true;
        }
    }
    //! [paint buttons]

    //! [paint end]
    return handled;
}
//! [paint end]
