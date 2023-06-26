// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "characterwidget.h"

#include <QFontDatabase>
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>

//! [0]
CharacterWidget::CharacterWidget(QWidget *parent)
    : QWidget(parent)
{
    calculateSquareSize();
    setMouseTracking(true);
}
//! [0]

//! [1]
void CharacterWidget::updateFont(const QFont &font)
{
    displayFont.setFamily(font.family());
    calculateSquareSize();
    adjustSize();
    update();
}
//! [1]

//! [2]
void CharacterWidget::updateSize(const QString &fontSize)
{
    displayFont.setPointSize(fontSize.toInt());
    calculateSquareSize();
    adjustSize();
    update();
}
//! [2]

void CharacterWidget::updateStyle(const QString &fontStyle)
{
    const QFont::StyleStrategy oldStrategy = displayFont.styleStrategy();
    displayFont = QFontDatabase::font(displayFont.family(), fontStyle, displayFont.pointSize());
    displayFont.setStyleStrategy(oldStrategy);
    calculateSquareSize();
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

void CharacterWidget::calculateSquareSize()
{
    squareSize = qMax(16, 4 + QFontMetrics(displayFont, this).height());
}

//! [3]
QSize CharacterWidget::sizeHint() const
{
    return QSize(columns*squareSize, (65536 / columns) * squareSize);
}
//! [3]

//! [4]
void CharacterWidget::mouseMoveEvent(QMouseEvent *event)
{
    QPoint widgetPosition = mapFromGlobal(event->globalPosition().toPoint());
    uint key = (widgetPosition.y() / squareSize) * columns + widgetPosition.x() / squareSize;

    QString text = QString::fromLatin1("<p>Character: <span style=\"font-size: 24pt; font-family: %1\">").arg(displayFont.family())
                  + QChar(key)
                  + QString::fromLatin1("</span><p>Value: 0x")
                  + QString::number(key, 16);
    QToolTip::showText(event->globalPosition().toPoint(), text, this);
}
//! [4]

//! [5]
void CharacterWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        lastKey = (event->position().toPoint().y() / squareSize) * columns + event->position().toPoint().x() / squareSize;
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
    int beginRow = redrawRect.top() / squareSize;
    int endRow = redrawRect.bottom() / squareSize;
    int beginColumn = redrawRect.left() / squareSize;
    int endColumn = redrawRect.right() / squareSize;
//! [7]

//! [8]
    painter.setPen(QPen(Qt::gray));
    for (int row = beginRow; row <= endRow; ++row) {
        for (int column = beginColumn; column <= endColumn; ++column) {
            painter.drawRect(column * squareSize, row * squareSize, squareSize, squareSize);
        }
//! [8] //! [9]
    }
//! [9]

//! [10]
    QFontMetrics fontMetrics(displayFont);
    painter.setPen(QPen(Qt::black));
    for (int row = beginRow; row <= endRow; ++row) {
        for (int column = beginColumn; column <= endColumn; ++column) {
            int key = row * columns + column;
            painter.setClipRect(column * squareSize, row * squareSize, squareSize, squareSize);

            if (key == lastKey)
                painter.fillRect(column * squareSize + 1, row * squareSize + 1,
                                 squareSize, squareSize, QBrush(Qt::red));

            painter.drawText(column * squareSize + (squareSize / 2) -
                                 fontMetrics.horizontalAdvance(QChar(key)) / 2,
                             row * squareSize + 4 + fontMetrics.ascent(),
                             QString(QChar(key)));
        }
    }
}
//! [10]
