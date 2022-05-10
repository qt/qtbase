// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "basictoolsplugin.h"

#include <QInputDialog>
#include <QPainter>
#include <QRandomGenerator>
#include <QtMath>

//! [0]
QStringList BasicToolsPlugin::brushes() const
{
    return {tr("Pencil"), tr("Air Brush"), tr("Random Letters")};
}
//! [0]

//! [1]
QRect BasicToolsPlugin::mousePress(const QString &brush, QPainter &painter,
                                   const QPoint &pos)
{
    return mouseMove(brush, painter, pos, pos);
}
//! [1]

//! [2]
QRect BasicToolsPlugin::mouseMove(const QString &brush, QPainter &painter,
                                  const QPoint &oldPos, const QPoint &newPos)
{
    painter.save();

    int rad = painter.pen().width() / 2;
    QRect boundingRect = QRect(oldPos, newPos).normalized()
                                              .adjusted(-rad, -rad, +rad, +rad);
    QColor color = painter.pen().color();
    int thickness = painter.pen().width();
    QColor transparentColor(color.red(), color.green(), color.blue(), 0);
//! [2] //! [3]

    if (brush == tr("Pencil")) {
        painter.drawLine(oldPos, newPos);
    } else if (brush == tr("Air Brush")) {
        int numSteps = 2 + (newPos - oldPos).manhattanLength() / 2;

        painter.setBrush(QBrush(color, Qt::Dense6Pattern));
        painter.setPen(Qt::NoPen);

        for (int i = 0; i < numSteps; ++i) {
            int x = oldPos.x() + i * (newPos.x() - oldPos.x()) / (numSteps - 1);
            int y = oldPos.y() + i * (newPos.y() - oldPos.y()) / (numSteps - 1);

            painter.drawEllipse(x - (thickness / 2), y - (thickness / 2),
                                thickness, thickness);
        }
    } else if (brush == tr("Random Letters")) {
        QChar ch(QRandomGenerator::global()->bounded('A', 'Z' + 1));

        QFont biggerFont = painter.font();
        biggerFont.setBold(true);
        biggerFont.setPointSize(biggerFont.pointSize() + thickness);
        painter.setFont(biggerFont);

        painter.drawText(newPos, QString(ch));

        QFontMetrics metrics(painter.font());
        boundingRect = metrics.boundingRect(ch);
        boundingRect.translate(newPos);
        boundingRect.adjust(-10, -10, +10, +10);
    }
    painter.restore();
    return boundingRect;
}
//! [3]

//! [4]
QRect BasicToolsPlugin::mouseRelease(const QString & /* brush */,
                                     QPainter & /* painter */,
                                     const QPoint & /* pos */)
{
    return QRect(0, 0, 0, 0);
}
//! [4]

//! [5]
QStringList BasicToolsPlugin::shapes() const
{
    return {tr("Circle"), tr("Star"), tr("Text...")};
}
//! [5]

//! [6]
QPainterPath BasicToolsPlugin::generateShape(const QString &shape,
                                             QWidget *parent)
{
    QPainterPath path;

    if (shape == tr("Circle")) {
        path.addEllipse(0, 0, 50, 50);
    } else if (shape == tr("Star")) {
        path.moveTo(90, 50);
        for (int i = 1; i < 5; ++i) {
            path.lineTo(50 + 40 * std::cos(0.8 * i * M_PI),
                        50 + 40 * std::sin(0.8 * i * M_PI));
        }
        path.closeSubpath();
    } else if (shape == tr("Text...")) {
        QString text = QInputDialog::getText(parent, tr("Text Shape"),
                                             tr("Enter text:"),
                                             QLineEdit::Normal, tr("Qt"));
        if (!text.isEmpty()) {
            QFont timesFont("Times", 50);
            timesFont.setStyleStrategy(QFont::ForceOutline);
            path.addText(0, 0, timesFont, text);
        }
    }

    return path;
}
//! [6]

//! [7]
QStringList BasicToolsPlugin::filters() const
{
    return {tr("Invert Pixels"), tr("Swap RGB"), tr("Grayscale")};
}
//! [7]

//! [8]
QImage BasicToolsPlugin::filterImage(const QString &filter, const QImage &image,
                                     QWidget * /* parent */)
{
    QImage result = image.convertToFormat(QImage::Format_RGB32);

    if (filter == tr("Invert Pixels")) {
        result.invertPixels();
    } else if (filter == tr("Swap RGB")) {
        result = result.rgbSwapped();
    } else if (filter == tr("Grayscale")) {
        for (int y = 0; y < result.height(); ++y) {
            for (int x = 0; x < result.width(); ++x) {
                QRgb pixel = result.pixel(x, y);
                int gray = qGray(pixel);
                int alpha = qAlpha(pixel);
                result.setPixel(x, y, qRgba(gray, gray, gray, alpha));
            }
        }
    }
    return result;
}
//! [8]
