// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "hbwidget.h"
#include <QHBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QLineEdit>
#include <QSpinBox>
#include <QLabel>
#include <QCheckBox>

HbWidget::HbWidget(QWidget *parent) :
    QWidget(parent)
{
    QHBoxLayout *hb = new QHBoxLayout(this);
    hb->setObjectName("HbWidget");
    QComboBox *combo = new QComboBox(this);
    combo->addItem("123");
    QComboBox *combo2 = new QComboBox();
    combo2->setEditable(true);
    combo2->addItem("123");

    hb->addWidget(new QLabel("123"));
    hb->addWidget(new QLineEdit("123"));
    hb->addWidget(combo);
    hb->addWidget(combo2);
    hb->addWidget(new QCheckBox("123"));
    hb->addWidget(new QDateTimeEdit());
    hb->addWidget(new QPushButton("123"));
    hb->addWidget(new QSpinBox());

    qDebug("There should be four warnings, but no crash or freeze:");
    hb->addWidget(this); ///< This command should print a warning, but should not add "this"
    hb->addWidget(nullptr); ///< This command should print a warning, but should not add "NULL"
    hb->addLayout(hb); ///< This command should print a warning, but should not add "hb"
    hb->addLayout(nullptr); ///< This command should print a warning, but should not add "NULL"
    qDebug("Neither crashed nor frozen");
}
