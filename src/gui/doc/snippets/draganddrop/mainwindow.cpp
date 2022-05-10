// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtWidgets>

#include "dragwidget.h"
#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QFrame *centralWidget = new QFrame(this);

    QLabel *mimeTypeLabel = new QLabel(tr("MIME types:"), centralWidget);
    mimeTypeCombo = new QComboBox(centralWidget);

    QLabel *dataLabel = new QLabel(tr("Amount of data (bytes):"), centralWidget);
    dragWidget = new DragWidget(centralWidget);

    connect(dragWidget, &DragWidget::mimeTypes,
            this, &MainWindow::setMimeTypes);
    connect(dragWidget, &DragWidget::dragResult,
            this, &MainWindow::setDragResult);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->addWidget(mimeTypeLabel);
    mainLayout->addWidget(mimeTypeCombo);
    mainLayout->addSpacing(32);
    mainLayout->addWidget(dataLabel);
    mainLayout->addWidget(dragWidget);

    statusBar();
    dragWidget->setData(QString("text/plain"), QByteArray("Hello world"));
    setCentralWidget(centralWidget);
    setWindowTitle(tr("Drag and Drop"));
}

void MainWindow::setDragResult(const QString &actionText)
{
    statusBar()->showMessage(actionText);
}

void MainWindow::setMimeTypes(const QStringList &types)
{
    mimeTypeCombo->clear();
    mimeTypeCombo->addItems(types);
}
