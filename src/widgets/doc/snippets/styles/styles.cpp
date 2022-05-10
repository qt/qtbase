// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QStyleOption>
#include <QStylePainter>
#include <QWidget>

class MyWidget : public QWidget
{
protected:
    void paintEvent(QPaintEvent *event) override;
    void paintEvent2(QPaintEvent *event);

};

//! [0] //! [1]
void MyWidget::paintEvent(QPaintEvent * /* event */)
//! [0]
{
//! [2]
    QPainter painter(this);
//! [2]

    QStyleOptionFocusRect option;
    option.initFrom(this);
    option.backgroundColor = palette().color(QPalette::Background);

//! [3]
    style()->drawPrimitive(QStyle::PE_FrameFocusRect, &option, &painter, this);
//! [3]
}
//! [1]

void MyWidget::paintEvent2(QPaintEvent * /* event */)
//! [4]
{
//! [4] //! [5] //! [6]
    QStylePainter painter(this);
//! [5]

    QStyleOptionFocusRect option;
    option.initFrom(this);
    option.backgroundColor = palette().color(QPalette::Background);

//! [7]
    painter.drawPrimitive(QStyle::PE_FrameFocusRect, option);
//! [7]
}
//! [6]

int main()
{
    return 0;
}
