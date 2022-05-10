// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
static void paintLayout(QPainter *painter, QLayoutItem *item)
{
    QLayout *layout = item->layout();
    if (layout) {
        for (int i = 0; i < layout->count(); ++i)
            paintLayout(painter, layout->itemAt(i));
    }
    painter->drawRect(item->geometry());
}

void MyWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    if (layout())
        paintLayout(&painter, layout());
}
//! [0]


//! [1]
QLayoutItem *child;
while ((child = layout->takeAt(0)) != nullptr) {
    ...
    delete child->widget(); // delete the widget
    delete child;   // delete the layout item
}
//! [1]
