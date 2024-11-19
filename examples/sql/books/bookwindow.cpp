// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "bookwindow.h"
#include "bookdelegate.h"
#include "initdb.h"

#include <QApplication>
#include <QComboBox>
#include <QDataWidgetMapper>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QScrollBar>
#include <QSpinBox>
#include <QSqlDatabase>
#include <QTableView>

BookWindow::BookWindow()
{
    if (!QSqlDatabase::drivers().contains("QSQLITE"))
        QMessageBox::critical(this, tr("Unable to load database"),
                              tr("This demo needs the SQLITE driver"));

    // Initialize the database:
    QSqlError err = initDb();
    if (err.type() != QSqlError::NoError) {
        showError(err);
        return;
    }

    // create the central widget for the window
    window = new QWidget(this);
    setCentralWidget(window);

    createLayout();

    createModel();

    // Populate the model
    if (!model->select()) {
        showError(model->lastError());
        return;
    }

    configureWidgets();

    // create the mappings between the UI elements and the SQL model
    createMappings();

    tableView->setCurrentIndex(model->index(0, 0));
    tableView->selectRow(0);

    createMenuBar();
}

void BookWindow::showError(const QSqlError &err)
{
    QMessageBox::critical(this, tr("Unable to initialize Database"),
                          tr("Error initializing database: %1").arg(err.text()));
}

void BookWindow::createLayout()
{
    tableView = new QTableView(window);

    gridLayout = new QGridLayout(window);

    titleLabel = new QLabel(tr("Title:"), window);
    titleLineEdit = new QLineEdit(window);
    authorLabel = new QLabel(tr("Author:"), window);
    authorComboBox = new QComboBox(window);
    genreLabel = new QLabel(tr("Genre:"), window);
    genreComboBox = new QComboBox(window);
    yearLabel = new QLabel(tr("Year:"), window);
    yearSpinBox = new QSpinBox(window);
    ratingLabel = new QLabel(tr("Rating:"), window);
    ratingComboBox = new QComboBox(window);

    gridLayout->addWidget(titleLabel, 0, 0, Qt::AlignRight);
    gridLayout->addWidget(titleLineEdit, 0, 1, 1, 3);
    gridLayout->addWidget(authorLabel, 1, 0, Qt::AlignRight);
    gridLayout->addWidget(authorComboBox, 1, 1);
    gridLayout->addWidget(yearLabel, 1, 2, Qt::AlignRight);
    gridLayout->addWidget(yearSpinBox, 1, 3);
    gridLayout->addWidget(genreLabel, 2, 0, Qt::AlignRight);
    gridLayout->addWidget(genreComboBox, 2, 1);
    gridLayout->addWidget(ratingLabel, 2, 2, Qt::AlignRight);
    gridLayout->addWidget(ratingComboBox, 2, 3);
    gridLayout->addWidget(tableView, 3, 0, 1, 4, Qt::AlignCenter);
    gridLayout->setColumnStretch(1, 1000);
    gridLayout->setColumnStretch(3, 1000);

    gridLayout->setContentsMargins(18, 18, 18, 18);
    gridLayout->setSpacing(18);
    gridLayout->setAlignment(Qt::AlignHCenter);
}

void BookWindow::createModel()
{
    model = new QSqlRelationalTableModel(tableView);
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    model->setTable("books");

    authorIdx = model->fieldIndex("author");
    genreIdx = model->fieldIndex("genre");

    // Set the relations to the other database tables
    model->setRelation(authorIdx, QSqlRelation("authors", "id", "name"));
    model->setRelation(genreIdx, QSqlRelation("genres", "id", "name"));

    // Set the localised header captions
    model->setHeaderData(authorIdx, Qt::Horizontal, tr("Author Name"));
    model->setHeaderData(genreIdx, Qt::Horizontal, tr("Genre"));
    model->setHeaderData(model->fieldIndex("title"),
                         Qt::Horizontal, tr("Title"));
    model->setHeaderData(model->fieldIndex("year"), Qt::Horizontal, tr("Year"));
    model->setHeaderData(model->fieldIndex("rating"),
                         Qt::Horizontal, tr("Rating"));
}

void BookWindow::configureWidgets()
{
    tableView->setModel(model);
    tableView->setItemDelegate(new BookDelegate(model->fieldIndex("rating"), tableView));
    tableView->setColumnHidden(model->fieldIndex("id"), true);
    tableView->verticalHeader()->setVisible(false);
    tableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);

    // Lock and prohibit resizing of the width of the columns
    tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    tableView->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    tableView->horizontalHeader()->setFixedHeight(tableView->rowHeight(0));

    // increment by two to consider the frame
    tableView->setFixedWidth(tableView->horizontalHeader()->length() +
                             tableView->verticalScrollBar()->sizeHint().width() + 2);
    tableView->setMaximumHeight(tableView->verticalHeader()->length() +
                                tableView->horizontalHeader()->height() + 2);

    authorComboBox->setModel(model->relationModel(authorIdx));
    authorComboBox->setModelColumn(model->relationModel(authorIdx)->fieldIndex("name"));

    genreComboBox->setModel(model->relationModel(genreIdx));
    genreComboBox->setModelColumn(model->relationModel(genreIdx)->fieldIndex("name"));

    yearSpinBox->setMaximum(9999);

    ratingComboBox->setItemDelegate(new BookDelegate(0, this));
    ratingComboBox->setLabelDrawingMode(QComboBox::LabelDrawingMode::UseDelegate);
    ratingComboBox->addItems({"0", "1", "2", "3", "4", "5"});

    ratingComboBox->setIconSize(iconSize());
}

void BookWindow::createMappings()
{
    QDataWidgetMapper *mapper = new QDataWidgetMapper(this);
    mapper->setModel(model);
    mapper->setItemDelegate(new BookDelegate(model->fieldIndex("rating"), this));
    mapper->addMapping(titleLineEdit, model->fieldIndex("title"));
    mapper->addMapping(yearSpinBox, model->fieldIndex("year"));
    mapper->addMapping(authorComboBox, authorIdx);
    mapper->addMapping(genreComboBox, genreIdx);
    mapper->addMapping(ratingComboBox, model->fieldIndex("rating"), "currentIndex");
    connect(tableView->selectionModel(),
            &QItemSelectionModel::currentRowChanged,
            mapper,
            &QDataWidgetMapper::setCurrentModelIndex
            );
}

void BookWindow::createMenuBar()
{
    QAction *quitAction = new QAction(tr("&Quit"), this);
    QAction *aboutAction = new QAction(tr("&About"), this);
    QAction *aboutQtAction = new QAction(tr("&About Qt"), this);

    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(quitAction);

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAction);
    helpMenu->addAction(aboutQtAction);

    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
    connect(aboutAction, &QAction::triggered, this, &BookWindow::about);
    connect(aboutQtAction, &QAction::triggered, qApp, &QApplication::aboutQt);
}

void BookWindow::about()
{
    QMessageBox::about(this, tr("About Books"),
            tr("<p>The <b>Books</b> example shows how to use Qt SQL classes "
               "with a model/view framework."));
}
