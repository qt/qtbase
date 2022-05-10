// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "mainwindow.h"

#include <QAction>
#include <QCoreApplication>
#include <QGroupBox>
#include <QListWidget>
#include <QMenuBar>
#include <QRadioButton>
#include <QStatusBar>
#include <QVBoxLayout>

static const char * const listEntries[] = {
    QT_TRANSLATE_NOOP("MainWindow", "First"),
    QT_TRANSLATE_NOOP("MainWindow", "Second"),
    QT_TRANSLATE_NOOP("MainWindow", "Third"),
    nullptr
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    centralWidget = new QWidget;
    setCentralWidget(centralWidget);

    createGroupBox();

    listWidget = new QListWidget;
    for (const char *entry : listEntries)
        listWidget->addItem(tr(entry));

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(groupBox);
    mainLayout->addWidget(listWidget);
    centralWidget->setLayout(mainLayout);

    exitAction = new QAction(tr("E&xit"), this);
    connect(exitAction, &QAction::triggered, qApp, QCoreApplication::quit);

    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->setPalette(QPalette(Qt::red));
    fileMenu->addAction(exitAction);

    setWindowTitle(tr("Language: %1").arg(tr("English")));
    statusBar()->showMessage(tr("Internationalization Example"));

    if (tr("LTR") == "RTL")
        setLayoutDirection(Qt::RightToLeft);
}

void MainWindow::createGroupBox()
{
    groupBox = new QGroupBox(tr("View"));
    perspectiveRadioButton = new QRadioButton(tr("Perspective"));
    isometricRadioButton = new QRadioButton(tr("Isometric"));
    obliqueRadioButton = new QRadioButton(tr("Oblique"));
    perspectiveRadioButton->setChecked(true);

    QVBoxLayout *groupBoxLayout = new QVBoxLayout;
    groupBoxLayout->addWidget(perspectiveRadioButton);
    groupBoxLayout->addWidget(isometricRadioButton);
    groupBoxLayout->addWidget(obliqueRadioButton);
    groupBox->setLayout(groupBoxLayout);
}
