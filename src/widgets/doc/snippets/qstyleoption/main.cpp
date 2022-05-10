// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtGui>
#include <QApplication>

class MyPushButton : public QPushButton
{
public:
    MyPushButton(QWidget *parent = nullptr);

    void paintEvent(QPaintEvent *) override;
};

MyPushButton::MyPushButton(QWidget *parent)
    : QPushButton(parent)
{
}

//! [0]
void MyPushButton::paintEvent(QPaintEvent *)
{
    QStyleOptionButton option;
    option.initFrom(this);
    option.state = isDown() ? QStyle::State_Sunken : QStyle::State_Raised;
    if (isDefault())
        option.features |= QStyleOptionButton::DefaultButton;
    option.text = text();
    option.icon = icon();

    QPainter painter(this);
    style()->drawControl(QStyle::CE_PushButton, &option, &painter, this);
}
//! [0]



class MyStyle : public QStyle
{
public:

    void drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                       QPainter *painter, const QWidget *widget) override;
};

//! [4]
void MyStyle::drawPrimitive(PrimitiveElement element,
                            const QStyleOption *option,
                            QPainter *painter,
                            const QWidget *widget)
{
    if (element == PE_FrameFocusRect) {
        const QStyleOptionFocusRect *focusRectOption =
                qstyleoption_cast<const QStyleOptionFocusRect *>(option);
        if (focusRectOption) {
            // ...
        }
    }
    // ...
}
//! [4]

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MyPushButton button;
    button.show();
    return app.exec();
}
