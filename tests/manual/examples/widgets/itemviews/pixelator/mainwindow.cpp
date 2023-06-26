// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "mainwindow.h"
#include "imagemodel.h"
#include "pixeldelegate.h"

#include <QtWidgets>
#if defined(QT_PRINTSUPPORT_LIB)
#include <QtPrintSupport/qtprintsupportglobal.h>
#if QT_CONFIG(printdialog)
#include <QPrinter>
#include <QPrintDialog>
#endif
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
    connect(pixelSizeSpinBox, &QSpinBox::valueChanged,
            delegate, &PixelDelegate::setPixelSize);
    connect(pixelSizeSpinBox, &QSpinBox::valueChanged,
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
#if defined(QT_PRINTSUPPORT_LIB) && QT_CONFIG(printdialog)
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

    double xscale = printer.pageRect(QPrinter::DevicePixel).width() / double(sourceWidth);
    double yscale = printer.pageRect(QPrinter::DevicePixel).height() / double(sourceHeight);
    double scale = qMin(xscale, yscale);

    painter.translate(printer.paperRect(QPrinter::DevicePixel).x() + printer.pageRect(QPrinter::DevicePixel).width() / 2,
                      printer.paperRect(QPrinter::DevicePixel).y() + printer.pageRect(QPrinter::DevicePixel).height() / 2);
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
