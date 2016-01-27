/****************************************************************************
 **
 ** Copyright (C) 2015 The Qt Company Ltd.
 ** Contact: http://www.qt.io/licensing/
 **
 ** This file is part of the test suite of the Qt Toolkit.
 **
 ** $QT_BEGIN_LICENSE:LGPL21$
 ** Commercial License Usage
 ** Licensees holding valid commercial Qt licenses may use this file in
 ** accordance with the commercial license agreement provided with the
 ** Software or, alternatively, in accordance with the terms contained in
 ** a written agreement between you and The Qt Company. For licensing terms
 ** and conditions see http://www.qt.io/terms-conditions. For further
 ** information use the contact form at http://www.qt.io/contact-us.
 **
 ** GNU Lesser General Public License Usage
 ** Alternatively, this file may be used under the terms of the GNU Lesser
 ** General Public License version 2.1 or version 3 as published by the Free
 ** Software Foundation and appearing in the file LICENSE.LGPLv21 and
 ** LICENSE.LGPLv3 included in the packaging of this file. Please review the
 ** following information to ensure the GNU Lesser General Public License
 ** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
 ** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
 **
 ** As a special exception, The Qt Company gives you certain additional
 ** rights. These rights are described in The Qt Company LGPL Exception
 ** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
 **
 ** $QT_END_LICENSE$
 **
 ****************************************************************************/

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
    : QWidget(parent), otherWindow(0)
{
    int x = 5;
    int y = 5;

    bool createChildWindow = text.isEmpty(); // OK, yes this is a hack...
    if (text.isEmpty())
        text = "You can drag from this window and drop text here";

    QStringList words = text.split(' ');
    foreach (QString word, words) {
        if (!word.isEmpty()) {
            FramedLabel *wordLabel = new FramedLabel(word, this);
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
        QStringList pieces = mime->text().split(QRegExp("\\s+"),
                             QString::SkipEmptyParts);
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

        foreach (QString piece, pieces) {
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
    foreach (QObject *child, children()) {
        if (child->inherits("QWidget")) {
            QWidget *widget = static_cast<QWidget *>(child);
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
                      QByteArray::number(hotSpot.x()) + " " + QByteArray::number(hotSpot.y()));

    QPixmap pixmap(child->size());
    child->render(&pixmap);

    QDrag *drag = new QDrag(this);
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
