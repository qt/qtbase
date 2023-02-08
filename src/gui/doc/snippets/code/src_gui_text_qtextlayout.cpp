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
    void elided();
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
while (true) {
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

void Wrapper::elided() {

QString content;

//! [elided]
QPainter painter(this);
QFontMetrics fontMetrics = painter.fontMetrics();

int lineSpacing = fontMetrics.lineSpacing();
int y = 0;

QTextLayout textLayout(content, painter.font());
textLayout.beginLayout();
while (true) {
    QTextLine line = textLayout.createLine();

    if (!line.isValid())
        break;

    line.setLineWidth(width());
    const int nextLineY = y + lineSpacing;

    if (height() >= nextLineY + lineSpacing) {
        line.draw(&painter, QPoint(0, y));
        y = nextLineY;
    } else {
        const QString lastLine = content.mid(line.textStart());
        const QString elidedLastLine = fontMetrics.elidedText(lastLine, Qt::ElideRight, width());
        painter.drawText(QPoint(0, y + fontMetrics.ascent()), elidedLastLine);
        line = textLayout.createLine();
        break;
    }
}
textLayout.endLayout();
//! [elided]
}

} // src_gui_text_qtextlayout
