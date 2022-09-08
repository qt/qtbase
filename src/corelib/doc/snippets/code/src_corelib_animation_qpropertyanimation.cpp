/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/


//! [0]
#include <QApplication>
#include <QPushButton>
#include <QPropertyAnimation>
//! [1]
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
//! [1]
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
