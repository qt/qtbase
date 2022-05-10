// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "scribblearea.h"

#include <QMouseEvent>
#include <QPainter>

#if defined(QT_PRINTSUPPORT_LIB)
#include <QtPrintSupport/qtprintsupportglobal.h>
#if QT_CONFIG(printdialog)
#include <QPrinter>
#include <QPrintDialog>
#endif
#endif

//! [0]
ScribbleArea::ScribbleArea(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_StaticContents);
}
//! [0]

//! [1]
bool ScribbleArea::openImage(const QString &fileName)
//! [1] //! [2]
{
    QImage loadedImage;
    if (!loadedImage.load(fileName))
        return false;

    QSize newSize = loadedImage.size().expandedTo(size());
    resizeImage(&loadedImage, newSize);
    image = loadedImage;
    modified = false;
    update();
    return true;
}
//! [2]

//! [3]
bool ScribbleArea::saveImage(const QString &fileName, const char *fileFormat)
//! [3] //! [4]
{
    QImage visibleImage = image;
    resizeImage(&visibleImage, size());

    if (visibleImage.save(fileName, fileFormat)) {
        modified = false;
        return true;
    }
    return false;
}
//! [4]

//! [5]
void ScribbleArea::setPenColor(const QColor &newColor)
//! [5] //! [6]
{
    myPenColor = newColor;
}
//! [6]

//! [7]
void ScribbleArea::setPenWidth(int newWidth)
//! [7] //! [8]
{
    myPenWidth = newWidth;
}
//! [8]

//! [9]
void ScribbleArea::clearImage()
//! [9] //! [10]
{
    image.fill(qRgb(255, 255, 255));
    modified = true;
    update();
}
//! [10]

//! [11]
void ScribbleArea::mousePressEvent(QMouseEvent *event)
//! [11] //! [12]
{
    if (event->button() == Qt::LeftButton) {
        lastPoint = event->position().toPoint();
        scribbling = true;
    }
}

void ScribbleArea::mouseMoveEvent(QMouseEvent *event)
{
    if ((event->buttons() & Qt::LeftButton) && scribbling)
        drawLineTo(event->position().toPoint());
}

void ScribbleArea::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && scribbling) {
        drawLineTo(event->position().toPoint());
        scribbling = false;
    }
}

//! [12] //! [13]
void ScribbleArea::paintEvent(QPaintEvent *event)
//! [13] //! [14]
{
    QPainter painter(this);
    QRect dirtyRect = event->rect();
    painter.drawImage(dirtyRect, image, dirtyRect);
}
//! [14]

//! [15]
void ScribbleArea::resizeEvent(QResizeEvent *event)
//! [15] //! [16]
{
    if (width() > image.width() || height() > image.height()) {
        int newWidth = qMax(width() + 128, image.width());
        int newHeight = qMax(height() + 128, image.height());
        resizeImage(&image, QSize(newWidth, newHeight));
        update();
    }
    QWidget::resizeEvent(event);
}
//! [16]

//! [17]
void ScribbleArea::drawLineTo(const QPoint &endPoint)
//! [17] //! [18]
{
    QPainter painter(&image);
    painter.setPen(QPen(myPenColor, myPenWidth, Qt::SolidLine, Qt::RoundCap,
                        Qt::RoundJoin));
    painter.drawLine(lastPoint, endPoint);
    modified = true;

    int rad = (myPenWidth / 2) + 2;
    update(QRect(lastPoint, endPoint).normalized()
                                     .adjusted(-rad, -rad, +rad, +rad));
    lastPoint = endPoint;
}
//! [18]

//! [19]
void ScribbleArea::resizeImage(QImage *image, const QSize &newSize)
//! [19] //! [20]
{
    if (image->size() == newSize)
        return;

    QImage newImage(newSize, QImage::Format_RGB32);
    newImage.fill(qRgb(255, 255, 255));
    QPainter painter(&newImage);
    painter.drawImage(QPoint(0, 0), *image);
    *image = newImage;
}
//! [20]

//! [21]
void ScribbleArea::print()
{
#if defined(QT_PRINTSUPPORT_LIB) && QT_CONFIG(printdialog)
    QPrinter printer(QPrinter::HighResolution);

    QPrintDialog printDialog(&printer, this);
//! [21] //! [22]
    if (printDialog.exec() == QDialog::Accepted) {
        QPainter painter(&printer);
        QRect rect = painter.viewport();
        QSize size = image.size();
        size.scale(rect.size(), Qt::KeepAspectRatio);
        painter.setViewport(rect.x(), rect.y(), size.width(), size.height());
        painter.setWindow(image.rect());
        painter.drawImage(0, 0, image);
    }
#endif // QT_CONFIG(printdialog)
}
//! [22]
