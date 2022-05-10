// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "pixeldelegate.h"

#include <QPainter>

//! [0]
PixelDelegate::PixelDelegate(QObject *parent)
    : QAbstractItemDelegate(parent), pixelSize(12)
{}
//! [0]

//! [1]
void PixelDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const
{
//! [2]
    if (option.state & QStyle::State_Selected)
        painter->fillRect(option.rect, option.palette.highlight());
//! [1]

//! [3]
    const int size = qMin(option.rect.width(), option.rect.height());
//! [3] //! [4]
    const int brightness = index.model()->data(index, Qt::DisplayRole).toInt();
    const double radius = (size / 2.0) - (brightness / 255.0 * size / 2.0);
    if (qFuzzyIsNull(radius))
        return;
//! [4]

//! [5]
    painter->save();
//! [5] //! [6]
    painter->setRenderHint(QPainter::Antialiasing, true);
//! [6] //! [7]
    painter->setPen(Qt::NoPen);
//! [7] //! [8]
    if (option.state & QStyle::State_Selected)
//! [8] //! [9]
        painter->setBrush(option.palette.highlightedText());
    else
//! [2]
        painter->setBrush(option.palette.text());
//! [9]

//! [10]
    painter->drawEllipse(QRectF(option.rect.x() + option.rect.width() / 2 - radius,
                                option.rect.y() + option.rect.height() / 2 - radius,
                                2 * radius, 2 * radius));
    painter->restore();
}
//! [10]

//! [11]
QSize PixelDelegate::sizeHint(const QStyleOptionViewItem & /* option */,
                              const QModelIndex & /* index */) const
{
    return QSize(pixelSize, pixelSize);
}
//! [11]

//! [12]
void PixelDelegate::setPixelSize(int size)
{
    pixelSize = size;
}
//! [12]
