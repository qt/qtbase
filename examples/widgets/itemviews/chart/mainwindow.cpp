// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "pieview.h"
#include "mainwindow.h"

#include <QtWidgets>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QMenu *fileMenu = new QMenu(tr("&File"), this);
    QAction *openAction = fileMenu->addAction(tr("&Open..."));
    openAction->setShortcuts(QKeySequence::Open);
    QAction *saveAction = fileMenu->addAction(tr("&Save As..."));
    saveAction->setShortcuts(QKeySequence::SaveAs);
    QAction *quitAction = fileMenu->addAction(tr("E&xit"));
    quitAction->setShortcuts(QKeySequence::Quit);

    setupModel();
    setupViews();

    connect(openAction, &QAction::triggered, this, &MainWindow::openFile);
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveFile);
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);

    menuBar()->addMenu(fileMenu);
    statusBar();

    loadFile(":/Charts/qtdata.cht");

    setWindowTitle(tr("Chart"));
    resize(870, 550);
}

void MainWindow::setupModel()
{
    model = new QStandardItemModel(8, 2, this);
    model->setHeaderData(0, Qt::Horizontal, tr("Label"));
    model->setHeaderData(1, Qt::Horizontal, tr("Quantity"));
}

void MainWindow::setupViews()
{
    QSplitter *splitter = new QSplitter;
    QTableView *table = new QTableView;
    pieChart = new PieView;
    splitter->addWidget(table);
    splitter->addWidget(pieChart);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    table->setModel(model);
    pieChart->setModel(model);

    QItemSelectionModel *selectionModel = new QItemSelectionModel(model);
    table->setSelectionModel(selectionModel);
    pieChart->setSelectionModel(selectionModel);

    QHeaderView *headerView = table->horizontalHeader();
    headerView->setStretchLastSection(true);

    setCentralWidget(splitter);
}

void MainWindow::openFile()
{
    const QString fileName =
        QFileDialog::getOpenFileName(this, tr("Choose a data file"), "", "*.cht");
    if (!fileName.isEmpty())
        loadFile(fileName);
}

void MainWindow::loadFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text))
        return;

    QTextStream stream(&file);

    model->removeRows(0, model->rowCount(QModelIndex()), QModelIndex());

    int row = 0;
    while (!stream.atEnd()) {
        const QString line = stream.readLine();
        if (!line.isEmpty()) {
            model->insertRows(row, 1, QModelIndex());

            const QStringList pieces = line.split(QLatin1Char(','), Qt::SkipEmptyParts);
            if (pieces.size() < 3)
                continue;
            model->setData(model->index(row, 0, QModelIndex()),
                           pieces.value(0));
            model->setData(model->index(row, 1, QModelIndex()),
                           pieces.value(1));
            model->setData(model->index(row, 0, QModelIndex()),
                           QColor(pieces.value(2)), Qt::DecorationRole);
            row++;
        }
    };

    file.close();
    statusBar()->showMessage(tr("Loaded %1").arg(fileName), 2000);
}

void MainWindow::saveFile()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save file as"), "", "*.cht");

    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text))
        return;

    QTextStream stream(&file);
    for (int row = 0; row < model->rowCount(QModelIndex()); ++row) {

        QStringList pieces;

        pieces.append(model->data(model->index(row, 0, QModelIndex()),
                                  Qt::DisplayRole).toString());
        pieces.append(model->data(model->index(row, 1, QModelIndex()),
                                  Qt::DisplayRole).toString());
        pieces.append(model->data(model->index(row, 0, QModelIndex()),
                                  Qt::DecorationRole).toString());

        stream << pieces.join(',') << "\n";
    }

    file.close();
    statusBar()->showMessage(tr("Saved %1").arg(fileName), 2000);
}
