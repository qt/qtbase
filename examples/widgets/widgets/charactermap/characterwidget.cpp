/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtWidgets>

#include "characterwidget.h"

//! [0]
CharacterWidget::CharacterWidget(QWidget *parent)
    : QWidget(parent)
{
    squareSize = 24;
    columns = 16;
    lastKey = -1;
    setMouseTracking(true);
}
//! [0]

//! [1]
void CharacterWidget::updateFont(const QFont &font)
{
    displayFont.setFamily(font.family());
    squareSize = qMax(24, QFontMetrics(displayFont).xHeight() * 3);
    adjustSize();
    update();
}
//! [1]

//! [2]
void CharacterWidget::updateSize(const QString &fontSize)
{
    displayFont.setPointSize(fontSize.toInt());
    squareSize = qMax(24, QFontMetrics(displayFont).xHeight() * 3);
    adjustSize();
    update();
}
//! [2]

void CharacterWidget::updateStyle(const QString &fontStyle)
{
    QFontDatabase fontDatabase;
    const QFont::StyleStrategy oldStrategy = displayFont.styleStrategy();
    displayFont = fontDatabase.font(displayFont.family(), fontStyle, displayFont.pointSize());
    displayFont.setStyleStrategy(oldStrategy);
    squareSize = qMax(24, QFontMetrics(displayFont).xHeight() * 3);
    adjustSize();
    update();
}

void CharacterWidget::updateFontMerging(bool enable)
{
    if (enable)
        displayFont.setStyleStrategy(QFont::PreferDefault);
    else
        displayFont.setStyleStrategy(QFont::NoFontMerging);
    adjustSize();
    update();
}

//! [3]
QSize CharacterWidget::sizeHint() const
{
    return QSize(columns*squareSize, (65536/columns)*squareSize);
}
//! [3]

//! [4]
void CharacterWidget::mouseMoveEvent(QMouseEvent *event)
{
    QPoint widgetPosition = mapFromGlobal(event->globalPos());
    uint key = (widgetPosition.y()/squareSize)*columns + widgetPosition.x()/squareSize;

    QString text = QString::fromLatin1("<p>Character: <span style=\"font-size: 24pt; font-family: %1\">").arg(displayFont.family())
                  + QChar(key)
                  + QString::fromLatin1("</span><p>Value: 0x")
                  + QString::number(key, 16);
    QToolTip::showText(event->globalPos(), text, this);
}
//! [4]

//! [5]
void CharacterWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        lastKey = (event->y()/squareSize)*columns + event->x()/squareSize;
        if (QChar(lastKey).category() != QChar::Other_NotAssigned)
            emit characterSelected(QString(QChar(lastKey)));
        update();
    }
    else
        QWidget::mousePressEvent(event);
}
//! [5]

//! [6]
void CharacterWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.fillRect(event->rect(), QBrush(Qt::white));
    painter.setFont(displayFont);
//! [6]

//! [7]
    QRect redrawRect = event->rect();
    int beginRow = redrawRect.top()/squareSize;
    int endRow = redrawRect.bottom()/squareSize;
    int beginColumn = redrawRect.left()/squareSize;
    int endColumn = redrawRect.right()/squareSize;
//! [7]

//! [8]
    painter.setPen(QPen(Qt::gray));
    for (int row = beginRow; row <= endRow; ++row) {
        for (int column = beginColumn; column <= endColumn; ++column) {
            painter.drawRect(column*squareSize, row*squareSize, squareSize, squareSize);
        }
//! [8] //! [9]
    }
//! [9]

//! [10]
    QFontMetrics fontMetrics(displayFont);
    painter.setPen(QPen(Qt::black));
    for (int row = beginRow; row <= endRow; ++row) {

        for (int column = beginColumn; column <= endColumn; ++column) {

            int key = row*columns + column;
            painter.setClipRect(column*squareSize, row*squareSize, squareSize, squareSize);

            if (key == lastKey)
                painter.fillRect(column*squareSize + 1, row*squareSize + 1, squareSize, squareSize, QBrush(Qt::red));

            painter.drawText(column*squareSize + (squareSize / 2) - fontMetrics.width(QChar(key))/2,
                             row*squareSize + 4 + fontMetrics.ascent(),
                             QString(QChar(key)));
        }
    }
}
//! [10]
