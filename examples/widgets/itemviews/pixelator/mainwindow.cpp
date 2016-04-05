/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "imagemodel.h"
#include "mainwindow.h"
#include "pixeldelegate.h"

#include <QtWidgets>
#ifndef QT_NO_PRINTER
#include <QPrinter>
#include <QPrintDialog>
#endif

//! [0]
MainWindow::MainWindow()
{
//! [0]
    currentPath = QDir::homePath();
    model = new ImageModel(this);

    QWidget *centralWidget = new QWidget;

//! [1]
    view = new QTableView;
    view->setShowGrid(false);
    view->horizontalHeader()->hide();
    view->verticalHeader()->hide();
    view->horizontalHeader()->setMinimumSectionSize(1);
    view->verticalHeader()->setMinimumSectionSize(1);
    view->setModel(model);
//! [1]

//! [2]
    PixelDelegate *delegate = new PixelDelegate(this);
    view->setItemDelegate(delegate);
//! [2]

//! [3]
    QLabel *pixelSizeLabel = new QLabel(tr("Pixel size:"));
    QSpinBox *pixelSizeSpinBox = new QSpinBox;
    pixelSizeSpinBox->setMinimum(4);
    pixelSizeSpinBox->setMaximum(32);
    pixelSizeSpinBox->setValue(12);
//! [3]

    QMenu *fileMenu = new QMenu(tr("&File"), this);
    QAction *openAction = fileMenu->addAction(tr("&Open..."));
    openAction->setShortcuts(QKeySequence::Open);

    printAction = fileMenu->addAction(tr("&Print..."));
    printAction->setEnabled(false);
    printAction->setShortcut(QKeySequence::Print);

    QAction *quitAction = fileMenu->addAction(tr("E&xit"));
    quitAction->setShortcuts(QKeySequence::Quit);

    QMenu *helpMenu = new QMenu(tr("&Help"), this);
    QAction *aboutAction = helpMenu->addAction(tr("&About"));

    menuBar()->addMenu(fileMenu);
    menuBar()->addSeparator();
    menuBar()->addMenu(helpMenu);

    connect(openAction, &QAction::triggered, this, &MainWindow::chooseImage);
    connect(printAction, &QAction::triggered, this, &MainWindow::printImage);
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAboutBox);
//! [4]
    typedef void (QSpinBox::*QSpinBoxIntSignal)(int);
    connect(pixelSizeSpinBox, static_cast<QSpinBoxIntSignal>(&QSpinBox::valueChanged),
            delegate, &PixelDelegate::setPixelSize);
    connect(pixelSizeSpinBox, static_cast<QSpinBoxIntSignal>(&QSpinBox::valueChanged),
            this, &MainWindow::updateView);
//! [4]

    QHBoxLayout *controlsLayout = new QHBoxLayout;
    controlsLayout->addWidget(pixelSizeLabel);
    controlsLayout->addWidget(pixelSizeSpinBox);
    controlsLayout->addStretch(1);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(view);
    mainLayout->addLayout(controlsLayout);
    centralWidget->setLayout(mainLayout);

    setCentralWidget(centralWidget);

    setWindowTitle(tr("Pixelator"));
    resize(640, 480);
//! [5]
}
//! [5]

void MainWindow::chooseImage()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Choose an image"), currentPath, "*");

    if (!fileName.isEmpty())
        openImage(fileName);
}

void MainWindow::openImage(const QString &fileName)
{
    QImage image;

    if (image.load(fileName)) {
        model->setImage(image);
        if (!fileName.startsWith(":/")) {
            currentPath = fileName;
            setWindowTitle(tr("%1 - Pixelator").arg(currentPath));
        }

        printAction->setEnabled(true);
        updateView();
    }
}

void MainWindow::printImage()
{
#if !defined(QT_NO_PRINTER) && !defined(QT_NO_PRINTDIALOG)
    if (model->rowCount(QModelIndex())*model->columnCount(QModelIndex()) > 90000) {
        QMessageBox::StandardButton answer;
        answer = QMessageBox::question(this, tr("Large Image Size"),
                tr("The printed image may be very large. Are you sure that "
                   "you want to print it?"),
        QMessageBox::Yes | QMessageBox::No);
        if (answer == QMessageBox::No)
            return;
    }

    QPrinter printer(QPrinter::HighResolution);

    QPrintDialog dlg(&printer, this);
    dlg.setWindowTitle(tr("Print Image"));

    if (dlg.exec() != QDialog::Accepted) {
        return;
    }

    QPainter painter;
    painter.begin(&printer);

    int rows = model->rowCount(QModelIndex());
    int columns = model->columnCount(QModelIndex());
    int sourceWidth = (columns + 1) * ItemSize;
    int sourceHeight = (rows + 1) * ItemSize;

    painter.save();

    double xscale = printer.pageRect().width() / double(sourceWidth);
    double yscale = printer.pageRect().height() / double(sourceHeight);
    double scale = qMin(xscale, yscale);

    painter.translate(printer.paperRect().x() + printer.pageRect().width() / 2,
                      printer.paperRect().y() + printer.pageRect().height() / 2);
    painter.scale(scale, scale);
    painter.translate(-sourceWidth / 2, -sourceHeight / 2);

    QStyleOptionViewItem option;
    QModelIndex parent = QModelIndex();

    QProgressDialog progress(tr("Printing..."), tr("Cancel"), 0, rows, this);
    progress.setWindowModality(Qt::ApplicationModal);
    float y = ItemSize / 2;

    for (int row = 0; row < rows; ++row) {
        progress.setValue(row);
        qApp->processEvents();
        if (progress.wasCanceled())
            break;

        float x = ItemSize / 2;

        for (int column = 0; column < columns; ++column) {
            option.rect = QRect(int(x), int(y), ItemSize, ItemSize);
            view->itemDelegate()->paint(&painter, option,
                                        model->index(row, column, parent));
            x = x + ItemSize;
        }
        y = y + ItemSize;
    }
    progress.setValue(rows);

    painter.restore();
    painter.end();

    if (progress.wasCanceled()) {
        QMessageBox::information(this, tr("Printing canceled"),
            tr("The printing process was canceled."), QMessageBox::Cancel);
    }
#else
    QMessageBox::information(this, tr("Printing canceled"),
        tr("Printing is not supported on this Qt build"), QMessageBox::Cancel);
#endif
}

void MainWindow::showAboutBox()
{
    QMessageBox::about(this, tr("About the Pixelator example"),
        tr("This example demonstrates how a standard view and a custom\n"
           "delegate can be used to produce a specialized representation\n"
           "of data in a simple custom model."));
}

//! [6]
void MainWindow::updateView()
{
    view->resizeColumnsToContents();
    view->resizeRowsToContents();
}
//! [6]
