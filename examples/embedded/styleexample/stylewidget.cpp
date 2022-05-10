// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QApplication>
#include <QString>
#include <QFile>

#include "stylewidget.h"



StyleWidget::StyleWidget(QWidget *parent)
 : QFrame(parent)
{
    m_ui.setupUi(this);
}


void StyleWidget::on_close_clicked()
{
    close();
}

void StyleWidget::on_blueStyle_clicked()
{
    QFile styleSheet(":/files/blue.qss");

    if (!styleSheet.open(QIODevice::ReadOnly)) {
        qWarning("Unable to open :/files/blue.qss");
        return;
    }

    qApp->setStyleSheet(styleSheet.readAll());
}

void StyleWidget::on_khakiStyle_clicked()
{
    QFile styleSheet(":/files/khaki.qss");

    if (!styleSheet.open(QIODevice::ReadOnly)) {
        qWarning("Unable to open :/files/khaki.qss");
        return;
    }

    qApp->setStyleSheet(styleSheet.readAll());
}


void StyleWidget::on_noStyle_clicked()
{
    QFile styleSheet(":/files/nostyle.qss");

    if (!styleSheet.open(QIODevice::ReadOnly)) {
        qWarning("Unable to open :/files/nostyle.qss");
        return;
    }

    qApp->setStyleSheet(styleSheet.readAll());
}


void StyleWidget::on_transparentStyle_clicked()
{
    QFile styleSheet(":/files/transparent.qss");

    if (!styleSheet.open(QIODevice::ReadOnly)) {
        qWarning("Unable to open :/files/transparent.qss");
        return;
    }

    qApp->setStyleSheet(styleSheet.readAll());
}



