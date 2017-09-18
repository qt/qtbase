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
