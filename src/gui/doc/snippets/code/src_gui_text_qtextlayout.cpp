// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QTextLayout>
#include <QTextLine>

namespace src_gui_text_qtextlayout {
struct Wrapper : public QPaintDevice
{
    void wrapper1();
};
QTextLayout textLayout;


void wrapper0() {
qreal lineWidth = 0;
QFont aFont;
QFontMetrics fontMetrics(aFont);

//! [0]
int leading = fontMetrics.leading();
qreal height = 0;
textLayout.setCacheEnabled(true);
textLayout.beginLayout();
while (1) {
    QTextLine line = textLayout.createLine();
    if (!line.isValid())
        break;

    line.setLineWidth(lineWidth);
    height += leading;
    line.setPosition(QPointF(0, height));
    height += line.height();
}
textLayout.endLayout();
//! [0]

} // wrapper0


void Wrapper::wrapper1() {

//! [1]
QPainter painter(this);
textLayout.draw(&painter, QPoint(0, 0));
//! [1]

} // Wrapper::wrapper1

} // src_gui_text_qtextlayout
