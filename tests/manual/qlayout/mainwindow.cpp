// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
