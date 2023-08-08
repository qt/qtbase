// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtWidgets>
#include "dragwidget.h"

class FramedLabel : public QLabel
{
public:
    FramedLabel(const QString &text, QWidget *parent)
        : QLabel(text, parent)
    {
        setAutoFillBackground(true);
        setFrameShape(QFrame::Panel);
        setFrameShadow(QFrame::Raised);
    }
};

DragWidget::DragWidget(QString text, QWidget *parent)
    : QWidget(parent)
{
    int x = 5;
    int y = 5;

    bool createChildWindow = text.isEmpty(); // OK, yes this is a hack...
    if (text.isEmpty())
        text = "You can drag from this window and drop text here";

    QStringList words = text.split(' ');
    for (const QString &word : words) {
        if (!word.isEmpty()) {
            auto wordLabel = new FramedLabel(word, this);
            wordLabel->move(x, y);
            wordLabel->show();
            x += wordLabel->width() + 2;
            if (x >= 245) {
                x = 5;
                y += wordLabel->height() + 2;
            }
        }
    }

    /*
    QPalette newPalette = palette();
    newPalette.setColor(QPalette::Window, Qt::white);
    setPalette(newPalette);
    */

    setAcceptDrops(true);
    setMinimumSize(400, qMax(200, y));
    setWindowTitle(tr("Draggable Text Window %1").arg(createChildWindow ? 1 : 2));
    if (createChildWindow)
        otherWindow = new DragWidget("Here is a second window that accepts drops");
}

void DragWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasText()) {
        if (event->source() == this) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else {
            event->acceptProposedAction();
        }
    } else {
        event->ignore();
    }
}

void DragWidget::dragMoveEvent(QDragMoveEvent * event)
{
    dragPos = event->pos();
    dragTimer.start(500, this);
    update();
}

void DragWidget::dragLeaveEvent(QDragLeaveEvent *)
{
    dragTimer.stop();
    update();
}

void DragWidget::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasText()) {
        const QMimeData *mime = event->mimeData();
        const QStringList pieces = mime->text().split(QRegularExpression("\\s+"),
                                                      Qt::SkipEmptyParts);
        QPoint position = event->pos();
        QPoint hotSpot;

        QList<QByteArray> hotSpotPos = mime->data("application/x-hotspot").split(' ');
        if (hotSpotPos.size() == 2) {
            hotSpot.setX(hotSpotPos.first().toInt());
            hotSpot.setY(hotSpotPos.last().toInt());
        }
        dropPos = position - hotSpot;
        dropTimer.start(500, this);
        update();

        for (const QString &piece : pieces) {
            FramedLabel *newLabel = new FramedLabel(piece, this);
            newLabel->move(position - hotSpot);
            newLabel->show();

            position += QPoint(newLabel->width(), 0);
        }

        if (event->source() == this) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else {
            event->acceptProposedAction();
        }
    } else {
        event->ignore();
    }
    for (QObject *child : children()) {
        if (child->isWidgetType()) {
            auto widget = static_cast<QWidget *>(child);
            if (!widget->isVisible())
                widget->deleteLater();
        }
    }
}

void DragWidget::mousePressEvent(QMouseEvent *event)
{
    QLabel *child = static_cast<QLabel*>(childAt(event->pos()));
    if (!child)
        return;

    QPoint hotSpot = event->pos() - child->pos();

    QMimeData *mimeData = new QMimeData;
    mimeData->setText(child->text());
    mimeData->setData("application/x-hotspot",
                      QByteArray::number(hotSpot.x()) + ' ' + QByteArray::number(hotSpot.y()));

    const qreal dpr = devicePixelRatio() > 1 && !(QGuiApplication::keyboardModifiers() & Qt::ShiftModifier)
        ? devicePixelRatio() : 1;

    QPixmap pixmap(child->size() * dpr);
    pixmap.setDevicePixelRatio(dpr);
    child->render(&pixmap);

    auto drag = new QDrag(this);
    drag->setMimeData(mimeData);
    drag->setPixmap(pixmap);
    drag->setHotSpot(hotSpot);

    Qt::DropAction dropAction = drag->exec(Qt::CopyAction | Qt::MoveAction, Qt::CopyAction);

    if (dropAction == Qt::MoveAction)
        child->close();
}

void DragWidget::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == dragTimer.timerId())
        dragTimer.stop();
    if (e->timerId() == dropTimer.timerId())
        dropTimer.stop();
    update();
}

void DragWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), Qt::white);

    if (dropTimer.isActive()) {
        p.setBrush(Qt::red);
        p.drawEllipse(dropPos, 50, 50);
    }

    if (dragTimer.isActive()) {
        p.setPen(QPen(Qt::blue, 5));
        QPoint p1 = (rect().topLeft()*3 + rect().bottomRight())/4;
        QPoint p2 = (rect().topLeft() + rect().bottomRight()*3)/4;
        p.drawLine(p1, dragPos);
        p.drawLine(p2, dragPos);
    }
}

void DragWidget::showEvent(QShowEvent *)
{
    if (otherWindow)
        otherWindow->show();
}

void DragWidget::hideEvent(QHideEvent *)
{
    if (otherWindow)
        otherWindow->hide();
}
