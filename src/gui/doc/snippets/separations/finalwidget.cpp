// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

/*
finalwidget.cpp

A widget to display an image and a label containing a description.
*/

#include "finalwidget.h"

#include <QApplication>
#include <QBuffer>
#include <QDrag>
#include <QLabel>
#include <QMimeData>
#include <QMouseEvent>
#include <QVBoxLayout>

FinalWidget::FinalWidget(QWidget *parent, const QString &name,
                         const QSize &labelSize)
    : QFrame(parent)
{
    hasImage = false;
    imageLabel = new QLabel;
    imageLabel->setFrameShadow(QFrame::Sunken);
    imageLabel->setFrameShape(QFrame::StyledPanel);
    imageLabel->setMinimumSize(labelSize);
    nameLabel = new QLabel(name);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(imageLabel, 1);
    layout->addWidget(nameLabel, 0);
}

/*!
    If the mouse moves far enough when the left mouse button is held down,
    start a drag and drop operation.
*/

void FinalWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton))
        return;
    if ((event->pos() - dragStartPosition).manhattanLength()
         < QApplication::startDragDistance())
        return;
    if (!hasImage)
        return;

    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;

//! [0]
    QByteArray output;
    QBuffer outputBuffer(&output);
    outputBuffer.open(QIODevice::WriteOnly);
    imageLabel->pixmap().toImage().save(&outputBuffer, "PNG");
    mimeData->setData("image/png", output);
//! [0]
/*
//! [1]
    mimeData->setImageData(QVariant(*imageLabel->pixmap()));
//! [1]
*/
    drag->setMimeData(mimeData);
    drag->setPixmap(imageLabel->pixmap().scaled(64, 64, Qt::KeepAspectRatio));
//! [2]
    drag->setHotSpot(QPoint(drag->pixmap().width()/2,
                            drag->pixmap().height()));
//! [2]
}

/*!
    Check for left mouse button presses in order to enable drag and drop.
*/

void FinalWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        dragStartPosition = event->pos();
}

QPixmap FinalWidget::pixmap() const
{
    return imageLabel->pixmap();
}

void FinalWidget::setPixmap(const QPixmap &pixmap)
{
    imageLabel->setPixmap(pixmap);
    hasImage = true;
}
