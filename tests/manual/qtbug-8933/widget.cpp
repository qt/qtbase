/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
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
