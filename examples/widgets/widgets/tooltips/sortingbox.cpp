// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "sortingbox.h"

#include <QMouseEvent>
#include <QIcon>
#include <QPainter>
#include <QRandomGenerator>
#include <QStyle>
#include <QToolButton>
#include <QToolTip>

//! [0]
SortingBox::SortingBox(QWidget *parent)
    : QWidget(parent)
{
//! [0] //! [1]
    setMouseTracking(true);
//! [1] //! [2]
    setBackgroundRole(QPalette::Base);
//! [2]

    itemInMotion = nullptr;

//! [3]
    newCircleButton = createToolButton(tr("New Circle"),
                                       QIcon(":/images/circle.png"),
                                       SLOT(createNewCircle()));

    newSquareButton = createToolButton(tr("New Square"),
                                       QIcon(":/images/square.png"),
                                       SLOT(createNewSquare()));

    newTriangleButton = createToolButton(tr("New Triangle"),
                                         QIcon(":/images/triangle.png"),
                                         SLOT(createNewTriangle()));

    circlePath.addEllipse(QRect(0, 0, 100, 100));
    squarePath.addRect(QRect(0, 0, 100, 100));

    qreal x = trianglePath.currentPosition().x();
    qreal y = trianglePath.currentPosition().y();
    trianglePath.moveTo(x + 120 / 2, y);
    trianglePath.lineTo(0, 100);
    trianglePath.lineTo(120, 100);
    trianglePath.lineTo(x + 120 / 2, y);

//! [3] //! [4]
    setWindowTitle(tr("Tool Tips"));
    resize(500, 300);

    createShapeItem(circlePath, tr("Circle"), initialItemPosition(circlePath),
                    initialItemColor());
    createShapeItem(squarePath, tr("Square"), initialItemPosition(squarePath),
                    initialItemColor());
    createShapeItem(trianglePath, tr("Triangle"),
                    initialItemPosition(trianglePath), initialItemColor());
}
//! [4]

//! [5]
bool SortingBox::event(QEvent *event)
{
//! [5] //! [6]
    if (event->type() == QEvent::ToolTip) {
        QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
        int index = itemAt(helpEvent->pos());
        if (index != -1) {
            QToolTip::showText(helpEvent->globalPos(), shapeItems[index].toolTip());
        } else {
            QToolTip::hideText();
            event->ignore();
        }

        return true;
    }
    return QWidget::event(event);
}
//! [6]

//! [7]
void SortingBox::resizeEvent(QResizeEvent * /* event */)
{
    int margin = style()->pixelMetric(QStyle::PM_LayoutTopMargin);
    int x = width() - margin;
    int y = height() - margin;

    y = updateButtonGeometry(newCircleButton, x, y);
    y = updateButtonGeometry(newSquareButton, x, y);
    updateButtonGeometry(newTriangleButton, x, y);
}
//! [7]

//! [8]
void SortingBox::paintEvent(QPaintEvent * /* event */)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    for (const ShapeItem &shapeItem : std::as_const(shapeItems)) {
//! [8] //! [9]
        painter.translate(shapeItem.position());
//! [9] //! [10]
        painter.setBrush(shapeItem.color());
        painter.drawPath(shapeItem.path());
        painter.translate(-shapeItem.position());
    }
}
//! [10]

//! [11]
void SortingBox::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        int index = itemAt(event->position().toPoint());
        if (index != -1) {
            itemInMotion = &shapeItems[index];
            previousPosition = event->position().toPoint();
            shapeItems.move(index, shapeItems.size() - 1);
            update();
        }
    }
}
//! [11]

//! [12]
void SortingBox::mouseMoveEvent(QMouseEvent *event)
{
    if ((event->buttons() & Qt::LeftButton) && itemInMotion)
        moveItemTo(event->position().toPoint());
}
//! [12]

//! [13]
void SortingBox::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && itemInMotion) {
        moveItemTo(event->position().toPoint());
        itemInMotion = nullptr;
    }
}
//! [13]

//! [14]
void SortingBox::createNewCircle()
{
    static int count = 1;
    createShapeItem(circlePath, tr("Circle <%1>").arg(++count),
                    randomItemPosition(), randomItemColor());
}
//! [14]

//! [15]
void SortingBox::createNewSquare()
{
    static int count = 1;
    createShapeItem(squarePath, tr("Square <%1>").arg(++count),
                    randomItemPosition(), randomItemColor());
}
//! [15]

//! [16]
void SortingBox::createNewTriangle()
{
    static int count = 1;
    createShapeItem(trianglePath, tr("Triangle <%1>").arg(++count),
                    randomItemPosition(), randomItemColor());
}
//! [16]

//! [17]
int SortingBox::itemAt(const QPoint &pos)
{
    for (int i = shapeItems.size() - 1; i >= 0; --i) {
        const ShapeItem &item = shapeItems[i];
        if (item.path().contains(pos - item.position()))
            return i;
    }
    return -1;
}
//! [17]

//! [18]
void SortingBox::moveItemTo(const QPoint &pos)
{
    QPoint offset = pos - previousPosition;
    itemInMotion->setPosition(itemInMotion->position() + offset);
//! [18] //! [19]
    previousPosition = pos;
    update();
}
//! [19]

//! [20]
int SortingBox::updateButtonGeometry(QToolButton *button, int x, int y)
{
    QSize size = button->sizeHint();
    button->setGeometry(x - size.rwidth(), y - size.rheight(),
                        size.rwidth(), size.rheight());

    return y - size.rheight()
           - style()->pixelMetric(QStyle::PM_LayoutVerticalSpacing);
}
//! [20]

//! [21]
void SortingBox::createShapeItem(const QPainterPath &path,
                                 const QString &toolTip, const QPoint &pos,
                                 const QColor &color)
{
    ShapeItem shapeItem;
    shapeItem.setPath(path);
    shapeItem.setToolTip(toolTip);
    shapeItem.setPosition(pos);
    shapeItem.setColor(color);
    shapeItems.append(shapeItem);
    update();
}
//! [21]

//! [22]
QToolButton *SortingBox::createToolButton(const QString &toolTip,
                                          const QIcon &icon, const char *member)
{
    QToolButton *button = new QToolButton(this);
    button->setToolTip(toolTip);
    button->setIcon(icon);
    button->setIconSize(QSize(32, 32));
    connect(button, SIGNAL(clicked()), this, member);

    return button;
}
//! [22]

//! [23]
QPoint SortingBox::initialItemPosition(const QPainterPath &path)
{
    int x;
    int y = (height() - qRound(path.controlPointRect().height()) / 2);
    if (shapeItems.size() == 0)
        x = ((3 * width()) / 2 - qRound(path.controlPointRect().width())) / 2;
    else
        x = (width() / shapeItems.size()
             - qRound(path.controlPointRect().width())) / 2;

    return QPoint(x, y);
}
//! [23]

//! [24]
QPoint SortingBox::randomItemPosition()
{
    return QPoint(QRandomGenerator::global()->bounded(width() - 120), QRandomGenerator::global()->bounded(height() - 120));
}
//! [24]

//! [25]
QColor SortingBox::initialItemColor()
{
    return QColor::fromHsv(((shapeItems.size() + 1) * 85) % 256, 255, 190);
}
//! [25]

//! [26]
QColor SortingBox::randomItemColor()
{
    return QColor::fromHsv(QRandomGenerator::global()->bounded(256), 255, 190);
}
//! [26]
