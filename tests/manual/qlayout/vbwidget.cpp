/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "vbwidget.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QLineEdit>
#include <QSpinBox>
#include <QLabel>
#include <QCheckBox>

VbWidget::VbWidget(QWidget *parent) :
    QWidget(parent)
{
    QVBoxLayout *hb = new QVBoxLayout(this);
    hb->setObjectName("VbWidget");
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
    hb->addWidget(Q_NULLPTR); ///< This command should print a warning, but should not add "NULL"
    hb->addLayout(hb); ///< This command should print a warning, but should not add "hb"
    hb->addLayout(Q_NULLPTR); ///< This command should print a warning, but should not add "NULL"
    qDebug("Neither crashed nor frozen");
}
