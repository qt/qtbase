// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "bookdelegate.h"

#include <QMouseEvent>
#include <QPainter>
#include <QSpinBox>
#include <QTableView>

BookDelegate::BookDelegate(int ratingColumn, QObject *parent)
    : QSqlRelationalDelegate(parent), ratingColumn(ratingColumn)
{}

void BookDelegate::paint(QPainter *painter,
                         const QStyleOptionViewItem &option,
                         const QModelIndex &index) const
{
    if (index.column() != ratingColumn) {
        QSqlRelationalDelegate::paint(painter, option, index);
    } else {
        if (option.state & QStyle::State_Selected) {
            painter->fillRect(option.rect,
                              option.palette.color(QPalette::Highlight));
        }

        const int rating = index.data(Qt::DisplayRole).toInt();
        const int height = qMin(option.rect.height(), iconDimension);
        const int width = height;
        // add cellPadding / 2 to center the stars in the cell
        int x = option.rect.x() + cellPadding / 2;
        int y = option.rect.y() + (option.rect.height() / 2) - (height / 2);

        for (int i = 0; i < 5; ++i) {
            if (i < rating)
                starFilledIcon.paint(painter, QRect(x, y, width, height));
            else
                starIcon.paint(painter, QRect(x, y, width, height));
            x += width;
        }
    }
}

QSize BookDelegate::sizeHint(const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    if (index.column() == ratingColumn)
        return QSize(5 * iconDimension, iconDimension) + QSize(cellPadding, cellPadding);
    // Since we draw the grid ourselves:
    return QSqlRelationalDelegate::sizeHint(option, index) + QSize(cellPadding, cellPadding);
}

bool BookDelegate::editorEvent(QEvent *event, QAbstractItemModel *model,
                               const QStyleOptionViewItem &option,
                               const QModelIndex &index)
{
    if (!ratingColumn || index.column() != ratingColumn)
        return QSqlRelationalDelegate::editorEvent(event, model, option, index);

    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        int stars = qBound(0, int(0.7 + qreal(mouseEvent->position().x()
            - option.rect.x()) / iconDimension), 5);
        model->setData(index, QVariant(stars));
        // So that the selection can change:
        return false;
    }

    return true;
}

QWidget *BookDelegate::createEditor(QWidget *parent,
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const
{
    if (!ratingColumn || index.column() != 4)
        return QSqlRelationalDelegate::createEditor(parent, option, index);

    // For editing the year, return a spinbox with a range from -1000 to 2100.
    QSpinBox *sb = new QSpinBox(parent);
    sb->setFrame(false);
    sb->setMaximum(2100);
    sb->setMinimum(-1000);

    return sb;
}
