// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "draglabel.h"
#include "dragwidget.h"

#include <QtWidgets>

static inline QString fridgetMagnetsMimeType() { return QStringLiteral("application/x-fridgemagnet"); }

//! [0]
DragWidget::DragWidget(QWidget *parent)
    : QWidget(parent)
{
    QFile dictionaryFile(QStringLiteral(":/dictionary/words.txt"));
    dictionaryFile.open(QFile::ReadOnly);
    QTextStream inputStream(&dictionaryFile);
//! [0]

//! [1]
    int x = 5;
    int y = 5;

    while (!inputStream.atEnd()) {
        QString word;
        inputStream >> word;
        if (!word.isEmpty()) {
            DragLabel *wordLabel = new DragLabel(word, this);
            wordLabel->move(x, y);
            wordLabel->show();
            wordLabel->setAttribute(Qt::WA_DeleteOnClose);
            x += wordLabel->width() + 2;
            if (x >= 245) {
                x = 5;
                y += wordLabel->height() + 2;
            }
        }
    }
//! [1]

//! [2]
    QPalette newPalette = palette();
    newPalette.setColor(QPalette::Window, Qt::white);
    setPalette(newPalette);

    setMinimumSize(400, qMax(200, y));
    setWindowTitle(tr("Fridge Magnets"));
//! [2] //! [3]
    setAcceptDrops(true);
}
//! [3]

//! [4]
void DragWidget::dragEnterEvent(QDragEnterEvent *event)
{
//! [4] //! [5]
    if (event->mimeData()->hasFormat(fridgetMagnetsMimeType())) {
        if (children().contains(event->source())) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else {
            event->acceptProposedAction();
//! [5] //! [6]
        }
//! [6] //! [7]
    } else if (event->mimeData()->hasText()) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}
//! [7]

//! [8]
void DragWidget::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasFormat(fridgetMagnetsMimeType())) {
        if (children().contains(event->source())) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else {
            event->acceptProposedAction();
        }
    } else if (event->mimeData()->hasText()) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}
//! [8]

//! [9]
void DragWidget::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasFormat(fridgetMagnetsMimeType())) {
        const QMimeData *mime = event->mimeData();
//! [9] //! [10]
        QByteArray itemData = mime->data(fridgetMagnetsMimeType());
        QDataStream dataStream(&itemData, QIODevice::ReadOnly);

        QString text;
        QPoint offset;
        dataStream >> text >> offset;
//! [10]
//! [11]
        DragLabel *newLabel = new DragLabel(text, this);
        newLabel->move(event->position().toPoint() - offset);
        newLabel->show();
        newLabel->setAttribute(Qt::WA_DeleteOnClose);

        if (event->source() == this) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else {
            event->acceptProposedAction();
        }
//! [11] //! [12]
    } else if (event->mimeData()->hasText()) {
        QStringList pieces = event->mimeData()->text().split(
            QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
        QPoint position = event->position().toPoint();

        for (const QString &piece : pieces) {
            DragLabel *newLabel = new DragLabel(piece, this);
            newLabel->move(position);
            newLabel->show();
            newLabel->setAttribute(Qt::WA_DeleteOnClose);

            position += QPoint(newLabel->width(), 0);
        }

        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}
//! [12]

//! [13]
void DragWidget::mousePressEvent(QMouseEvent *event)
{
//! [13]
//! [14]
    DragLabel *child = static_cast<DragLabel*>(childAt(event->position().toPoint()));
    if (!child)
        return;

    QPoint hotSpot = event->position().toPoint() - child->pos();

    QByteArray itemData;
    QDataStream dataStream(&itemData, QIODevice::WriteOnly);
    dataStream << child->labelText() << QPoint(hotSpot);
//! [14]

//! [15]
    QMimeData *mimeData = new QMimeData;
    mimeData->setData(fridgetMagnetsMimeType(), itemData);
    mimeData->setText(child->labelText());
//! [15]

//! [16]
    QDrag *drag = new QDrag(this);
    drag->setMimeData(mimeData);
    drag->setPixmap(child->pixmap());
    drag->setHotSpot(hotSpot);

    child->hide();
//! [16]

//! [17]
    if (drag->exec(Qt::MoveAction | Qt::CopyAction, Qt::CopyAction) == Qt::MoveAction)
        child->close();
    else
        child->show();
}
//! [17]
