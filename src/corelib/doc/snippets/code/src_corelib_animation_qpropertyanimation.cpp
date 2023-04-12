// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause


//! [0]
#include <QApplication>
#include <QPushButton>
#include <QPropertyAnimation>

class MyButtonWidget : public QWidget
{
public:
    MyButtonWidget(QWidget *parent = nullptr);
};

MyButtonWidget::MyButtonWidget(QWidget *parent) : QWidget(parent)
{
    QPushButton *button = new QPushButton(tr("Animated Button"), this);
    QPropertyAnimation *anim = new QPropertyAnimation(button, "pos", this);
    anim->setDuration(10000);
    anim->setStartValue(QPoint(0, 0));
    anim->setEndValue(QPoint(100, 250));
    anim->start();
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MyButtonWidget buttonAnimWidget;
    buttonAnimWidget.resize(QSize(800, 600));
    buttonAnimWidget.show();
    return a.exec();
}
//! [0]


//! [easing-curve]
MyButtonWidget::MyButtonWidget(QWidget *parent) : QWidget(parent)
{
    QPushButton *button = new QPushButton(tr("Animated Button"), this);
    QPropertyAnimation *anim = new QPropertyAnimation(button, "pos", this);
    anim->setDuration(10000);
    anim->setStartValue(QPoint(0, 0));
    anim->setEndValue(QPoint(100, 250));
    anim->setEasingCurve(QEasingCurve::OutBounce);
    anim->start();
}
//! [easing-curve]


//! [animation-group1]
MyButtonWidget::MyButtonWidget(QWidget *parent) : QWidget(parent)
{
    QPushButton *bonnie = new QPushButton(tr("Bonnie"), this);
    QPushButton *clyde = new QPushButton(tr("Clyde"), this);

    QPropertyAnimation *anim1 = new QPropertyAnimation(bonnie, "pos", this);
    anim1->setDuration(3000);
    anim1->setStartValue(QPoint(0, 0));
    anim1->setEndValue(QPoint(100, 250));

    QPropertyAnimation *anim2 = new QPropertyAnimation(clyde, "pos", this);
    anim2->setDuration(3000);
    anim2->setStartValue(QPoint(100, 250));
    anim2->setEndValue(QPoint(500, 500));

    QParallelAnimationGroup *parallelAnim = new QParallelAnimationGroup;
    parallelAnim->addAnimation(anim1);
    parallelAnim->addAnimation(anim2);
    parallelAnim->start();
}
//! [animation-group1]

//! [animation-group2]
MyButtonWidget::MyButtonWidget(QWidget *parent) : QWidget(parent)
{
    QPushButton *bonnie = new QPushButton(tr("Bonnie"), this);
    QPushButton *clyde = new QPushButton(tr("Clyde"), this);

    QPropertyAnimation *anim1 = new QPropertyAnimation(bonnie, "pos", this);
    anim1->setDuration(3000);
    anim1->setStartValue(QPoint(0, 0));
    anim1->setEndValue(QPoint(100, 250));

    QPropertyAnimation *anim2 = new QPropertyAnimation(clyde, "pos", this);
    anim2->setDuration(3000);
    anim2->setStartValue(QPoint(0, 0));
    anim2->setEndValue(QPoint(200, 250));

    QSequentialAnimationGroup *sequenceAnim = new QSequentialAnimationGroup;
    sequenceAnim->addAnimation(anim1);
    sequenceAnim->addAnimation(anim2);
    sequenceAnim->start();
}
//! [animation-group2]
