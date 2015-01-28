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

#include "mainwindow.h"
#include "hbwidget.h"
#include "vbwidget.h"
#include "gridwidget.h"
#include <QGridLayout>
#include <QComboBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QWidget *widget = new QWidget(this);
    setCentralWidget(widget);
    m_mainLayout = new QGridLayout(widget);

    QComboBox *combo = new QComboBox();
    combo->addItem("HBox Layout");
    combo->addItem("VBox Layout");
    combo->addItem("Grid Layout");
    connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(layoutIndexChanged(int)));
    HbWidget *hbWidget = new HbWidget(this);

    m_mainLayout->addWidget(combo);
    m_mainLayout->addWidget(hbWidget);
}

MainWindow::~MainWindow()
{

}

void MainWindow::layoutIndexChanged(int index)
{
    delete m_mainLayout->takeAt(1)->widget();

    switch (index) {
    case 0: {
        HbWidget *hbWidget = new HbWidget(this);
        m_mainLayout->addWidget(hbWidget);
        break;
        }
    case 1: {
        VbWidget *vbWidget = new VbWidget(this);
        m_mainLayout->addWidget(vbWidget);
        break;
        }
    default: {
        GridWidget *gridW = new GridWidget(this);
        m_mainLayout->addWidget(gridW);
        break;
        }
    }
}
