// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtGui>
#include <QLabel>
#include <QHBoxLayout>
#include <QApplication>

#include "dragwidget.h"

DragWidget::DragWidget(QWidget *parent)
    : QFrame(parent)
{
    setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    dragDropLabel = new QLabel("", this);
    dragDropLabel->setAlignment(Qt::AlignHCenter);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addStretch(0);
    layout->addWidget(dragDropLabel);
    layout->addStretch(0);

    setAcceptDrops(true);
}

// Accept all actions, but deal with them separately later.
//! [0]
void DragWidget::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}
//! [0]

//! [1]
void DragWidget::dropEvent(QDropEvent *event)
{
    if (event->source() == this && event->possibleActions() & Qt::MoveAction)
        return;
//! [1]

//! [2]
    if (event->proposedAction() == Qt::MoveAction) {
        event->acceptProposedAction();
        // Process the data from the event.
//! [2]
        emit dragResult(tr("The data was moved here."));
//! [3]
    } else if (event->proposedAction() == Qt::CopyAction) {
        event->acceptProposedAction();
        // Process the data from the event.
//! [3]
        emit dragResult(tr("The data was copied here."));
//! [4]
    } else {
        // Ignore the drop.
        return;
    }
//! [4]
    // End of quote

    emit mimeTypes(event->mimeData()->formats());
    setData(event->mimeData()->formats()[0],
            event->mimeData()->data(event->mimeData()->formats()[0]));
//! [5]
}
//! [5]

//! [6]
void DragWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        dragStartPosition = event->pos();
}
//! [6]

//! [7]
void DragWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton))
        return;
    if ((event->pos() - dragStartPosition).manhattanLength()
         < QApplication::startDragDistance())
        return;

    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;

    mimeData->setData(mimeType, data);
    drag->setMimeData(mimeData);

    Qt::DropAction dropAction = drag->exec(Qt::CopyAction | Qt::MoveAction);
//! [7]

    switch (dropAction) {
        case Qt::CopyAction:
            emit dragResult(tr("The text was copied."));
            break;
        case Qt::MoveAction:
            emit dragResult(tr("The text was moved."));
            break;
        default:
            emit dragResult(tr("Unknown action."));
            break;
    }
//! [8]
}
//! [8]

void DragWidget::setData(const QString &mimetype, const QByteArray &newData)
{
    mimeType = mimetype;
    data = QByteArray(newData);

    dragDropLabel->setText(tr("%1 bytes").arg(data.size()));

    QStringList formats;
    formats << mimetype;
    emit mimeTypes(formats);
}
