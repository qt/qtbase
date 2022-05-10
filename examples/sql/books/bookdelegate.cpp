// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "bookdelegate.h"

#include <QtWidgets>

BookDelegate::BookDelegate(QObject *parent)
    : QSqlRelationalDelegate(parent), star(QPixmap(":images/star.png"))
{
}

void BookDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                           const QModelIndex &index) const
{
    if (index.column() != 5) {
        QSqlRelationalDelegate::paint(painter, option, index);
    } else {
        const QAbstractItemModel *model = index.model();
        QPalette::ColorGroup cg = (option.state & QStyle::State_Enabled) ?
            (option.state & QStyle::State_Active) ?
                        QPalette::Normal :
                        QPalette::Inactive :
                        QPalette::Disabled;

        if (option.state & QStyle::State_Selected)
            painter->fillRect(
                        option.rect,
                        option.palette.color(cg, QPalette::Highlight));

        int rating = model->data(index, Qt::DisplayRole).toInt();
        int width = star.width();
        int height = star.height();
        int x = option.rect.x();
        int y = option.rect.y() + (option.rect.height() / 2) - (height / 2);
        for (int i = 0; i < rating; ++i) {
            painter->drawPixmap(x, y, star);
            x += width;
        }
    }

    QPen pen = painter->pen();
    painter->setPen(option.palette.color(QPalette::Mid));
    painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());
    painter->drawLine(option.rect.topRight(), option.rect.bottomRight());
    painter->setPen(pen);
}

QSize BookDelegate::sizeHint(const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const
{
    if (index.column() == 5)
        return QSize(5 * star.width(), star.height()) + QSize(1, 1);
    // Since we draw the grid ourselves:
    return QSqlRelationalDelegate::sizeHint(option, index) + QSize(1, 1);
}

bool BookDelegate::editorEvent(QEvent *event, QAbstractItemModel *model,
                               const QStyleOptionViewItem &option,
                               const QModelIndex &index)
{
    if (index.column() != 5)
        return QSqlRelationalDelegate::editorEvent(event, model, option, index);

    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        int stars = qBound(0, int(0.7 + qreal(mouseEvent->position().toPoint().x()
            - option.rect.x()) / star.width()), 5);
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
    if (index.column() != 4)
        return QSqlRelationalDelegate::createEditor(parent, option, index);

    // For editing the year, return a spinbox with a range from -1000 to 2100.
    QSpinBox *sb = new QSpinBox(parent);
    sb->setFrame(false);
    sb->setMaximum(2100);
    sb->setMinimum(-1000);

    return sb;
}
