// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtWidgets>

#include "dragwidget.h"

static QLabel *createDragLabel(const QString &text, QWidget *parent)
{
    QLabel *label = new QLabel(text, parent);
    label->setAutoFillBackground(true);
    label->setFrameShape(QFrame::Panel);
    label->setFrameShadow(QFrame::Raised);
    return label;
}

static QString hotSpotMimeDataKey() { return QStringLiteral("application/x-hotspot"); }

DragWidget::DragWidget(QWidget *parent)
    : QWidget(parent)
{
    QFile dictionaryFile(QStringLiteral(":/dictionary/words.txt"));
    if (!dictionaryFile.open(QIODevice::ReadOnly)) {
        // This would be a build problem, as the dictionary is in the
        // resource system.
        qFatal("Could not open the dictionary file: %s",
               qPrintable(dictionaryFile.errorString()));
    }
    QTextStream inputStream(&dictionaryFile);

    int x = 5;
    int y = 5;

    while (!inputStream.atEnd()) {
        QString word;
        inputStream >> word;
        if (!word.isEmpty()) {
            QLabel *wordLabel = createDragLabel(word, this);
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

    setAcceptDrops(true);
    setMinimumSize(400, qMax(200, y));
    setWindowTitle(tr("Draggable Text"));
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

void DragWidget::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasText()) {
        const QMimeData *mime = event->mimeData();
        QStringList pieces = mime->text().split(QRegularExpression(QStringLiteral("\\s+")),
                                                Qt::SkipEmptyParts);
        QPoint position = event->position().toPoint();
        QPoint hotSpot;

        QByteArrayList hotSpotPos = mime->data(hotSpotMimeDataKey()).split(' ');
        if (hotSpotPos.size() == 2) {
            hotSpot.setX(hotSpotPos.first().toInt());
            hotSpot.setY(hotSpotPos.last().toInt());
        }

        for (const QString &piece : pieces) {
            QLabel *newLabel = createDragLabel(piece, this);
            newLabel->move(position - hotSpot);
            newLabel->show();
            newLabel->setAttribute(Qt::WA_DeleteOnClose);

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
    for (QWidget *widget : findChildren<QWidget *>()) {
        if (!widget->isVisible())
            widget->deleteLater();
    }
}

void DragWidget::mousePressEvent(QMouseEvent *event)
{
    QLabel *child = qobject_cast<QLabel*>(childAt(event->position().toPoint()));
    if (!child)
        return;

    QPoint hotSpot = event->position().toPoint() - child->pos();

    QMimeData *mimeData = new QMimeData;
    mimeData->setText(child->text());
    mimeData->setData(hotSpotMimeDataKey(),
                      QByteArray::number(hotSpot.x()) + ' ' + QByteArray::number(hotSpot.y()));

    qreal dpr = window()->windowHandle()->devicePixelRatio();
    QPixmap pixmap(child->size() * dpr);
    pixmap.setDevicePixelRatio(dpr);
    child->render(&pixmap);

    QDrag *drag = new QDrag(this);
    drag->setMimeData(mimeData);
    drag->setPixmap(pixmap);
    drag->setHotSpot(hotSpot);

    Qt::DropAction dropAction = drag->exec(Qt::CopyAction | Qt::MoveAction, Qt::CopyAction);

    if (dropAction == Qt::MoveAction)
        child->close();
}
