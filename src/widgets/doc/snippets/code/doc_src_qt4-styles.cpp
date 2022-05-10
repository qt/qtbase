// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
const QStyleOptionFocusRect *focusRectOption =
        qstyleoption_cast<const QStyleOptionFocusRect *>(option);
if (focusRectOption) {
    ...
}
//! [0]


//! [1]
void MyWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    ...

    QStyleOptionFocusRect option(1);
    option.init(this);
    option.backgroundColor = palette().color(QPalette::Window);

    style().drawPrimitive(QStyle::PE_FrameFocusRect, &option, &painter,
                          this);
}
//! [1]


//! [2]
void drawControl(ControlElement element,
                 QPainter *painter,
                 const QWidget *widget,
                 const QRect &rect,
                 const QColorGroup &colorGroup,
                 SFlags how = Style_Default,
                 const QStyleOption &option = QStyleOption::Default) const;
//! [2]


//! [3]
void drawControl(ControlElement element,
                 const QStyleOption *option,
                 QPainter *painter,
                 const QWidget *widget = nullptr) const;
//! [3]
