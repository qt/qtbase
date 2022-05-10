// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "window.h"
#include "layoutitem.h"

#include <QGraphicsLinearLayout>
#include <QGraphicsGridLayout>

Window::Window(QGraphicsWidget *parent) : QGraphicsWidget(parent, Qt::Window)
{
//! [0]
    QGraphicsLinearLayout *windowLayout = new QGraphicsLinearLayout(Qt::Vertical);
    QGraphicsLinearLayout *linear = new QGraphicsLinearLayout(windowLayout);
    LayoutItem *item = new LayoutItem;
    linear->addItem(item);
    linear->setStretchFactor(item, 1);
//! [0]

//! [1]
    item = new LayoutItem;
    linear->addItem(item);
    linear->setStretchFactor(item, 3);
    windowLayout->addItem(linear);
//! [1]

//! [2]
    QGraphicsGridLayout *grid = new QGraphicsGridLayout(windowLayout);
    item = new LayoutItem;
    grid->addItem(item, 0, 0, 4, 1);
    item = new LayoutItem;
    item->setMaximumHeight(item->minimumHeight());
    grid->addItem(item, 0, 1, 2, 1, Qt::AlignVCenter);
    item = new LayoutItem;
    item->setMaximumHeight(item->minimumHeight());
    grid->addItem(item, 2, 1, 2, 1, Qt::AlignVCenter);
    item = new LayoutItem;
    grid->addItem(item, 0, 2);
    item = new LayoutItem;
    grid->addItem(item, 1, 2);
    item = new LayoutItem;
    grid->addItem(item, 2, 2);
    item = new LayoutItem;
    grid->addItem(item, 3, 2);
    windowLayout->addItem(grid);
//! [2]

//! [3]
    setLayout(windowLayout);
    setWindowTitle(tr("Basic Graphics Layouts Example"));
//! [3]

}
