/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "widget.h"
#include "ui_widget.h"

#include <QTimer>
#include <QDebug>

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    QTimer::singleShot(5000, this, SLOT(switchToFullScreen()));
    QTimer::singleShot(10000, this, SLOT(addMenuBar()));
    QTimer::singleShot(15000, this, SLOT(removeMenuBar()));
    QTimer::singleShot(20000, this, SLOT(switchToNormalScreen()));
}

Widget::~Widget()
{
    delete ui;
}

void Widget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void Widget::switchToFullScreen()
{
    ui->label->setText("entering full screen");
    showFullScreen();
}

void Widget::switchToNormalScreen()
{
    ui->label->setText("leaving full screen");
    showNormal();
}

void Widget::addMenuBar()
{
    ui->label->setText("adding menu bar");
    menuBar = new QMenuBar(this);
    menuBar->setVisible(true);
}

void Widget::removeMenuBar()
{
    ui->label->setText("removing menu bar");
    delete menuBar;
}
