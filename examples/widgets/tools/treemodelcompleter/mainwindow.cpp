// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "mainwindow.h"
#include "treemodelcompleter.h"

#include <QAbstractProxyModel>
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QFile>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QStringListModel>
#include <QTreeView>

//! [0]
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    createMenu();

    completer = new TreeModelCompleter(this);
    completer->setModel(modelFromFile(":/resources/treemodel.txt"));
    completer->setSeparator(QLatin1String("."));
    QObject::connect(completer, QOverload<const QModelIndex &>::of(&TreeModelCompleter::highlighted),
                     this, &MainWindow::highlight);

    QWidget *centralWidget = new QWidget;

    QLabel *modelLabel = new QLabel;
    modelLabel->setText(tr("Tree Model<br>(Double click items to edit)"));

    QLabel *modeLabel = new QLabel;
    modeLabel->setText(tr("Completion Mode"));
    modeCombo = new QComboBox;
    modeCombo->addItem(tr("Inline"));
    modeCombo->addItem(tr("Filtered Popup"));
    modeCombo->addItem(tr("Unfiltered Popup"));
    modeCombo->setCurrentIndex(1);

    QLabel *caseLabel = new QLabel;
    caseLabel->setText(tr("Case Sensitivity"));
    caseCombo = new QComboBox;
    caseCombo->addItem(tr("Case Insensitive"));
    caseCombo->addItem(tr("Case Sensitive"));
    caseCombo->setCurrentIndex(0);
//! [0]

//! [1]
    QLabel *separatorLabel = new QLabel;
    separatorLabel->setText(tr("Tree Separator"));

    QLineEdit *separatorLineEdit = new QLineEdit;
    separatorLineEdit->setText(completer->separator());
    connect(separatorLineEdit, &QLineEdit::textChanged,
            completer, &TreeModelCompleter::setSeparator);

    QCheckBox *wrapCheckBox = new QCheckBox;
    wrapCheckBox->setText(tr("Wrap around completions"));
    wrapCheckBox->setChecked(completer->wrapAround());
    connect(wrapCheckBox, &QAbstractButton::clicked, completer, &QCompleter::setWrapAround);

    contentsLabel = new QLabel;
    contentsLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(separatorLineEdit, &QLineEdit::textChanged,
            this, &MainWindow::updateContentsLabel);

    treeView = new QTreeView;
    treeView->setModel(completer->model());
    treeView->header()->hide();
    treeView->expandAll();
//! [1]

//! [2]
    connect(modeCombo, &QComboBox::activated,
            this, &MainWindow::changeMode);
    connect(caseCombo, &QComboBox::activated,
            this, &MainWindow::changeMode);

    lineEdit = new QLineEdit;
    lineEdit->setCompleter(completer);
//! [2]

//! [3]
    QGridLayout *layout = new QGridLayout;
    layout->addWidget(modelLabel, 0, 0); layout->addWidget(treeView, 0, 1);
    layout->addWidget(modeLabel, 1, 0);  layout->addWidget(modeCombo, 1, 1);
    layout->addWidget(caseLabel, 2, 0);  layout->addWidget(caseCombo, 2, 1);
    layout->addWidget(separatorLabel, 3, 0); layout->addWidget(separatorLineEdit, 3, 1);
    layout->addWidget(wrapCheckBox, 4, 0);
    layout->addWidget(contentsLabel, 5, 0, 1, 2);
    layout->addWidget(lineEdit, 6, 0, 1, 2);
    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);

    changeCase(caseCombo->currentIndex());
    changeMode(modeCombo->currentIndex());

    setWindowTitle(tr("Tree Model Completer"));
    lineEdit->setFocus();
}
//! [3]

//! [4]
void MainWindow::createMenu()
{
    QAction *exitAction = new QAction(tr("Exit"), this);
    QAction *aboutAct = new QAction(tr("About"), this);
    QAction *aboutQtAct = new QAction(tr("About Qt"), this);

    connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);
    connect(aboutAct, &QAction::triggered, this, &MainWindow::about);
    connect(aboutQtAct, &QAction::triggered, qApp, &QApplication::aboutQt);

    QMenu *fileMenu = menuBar()->addMenu(tr("File"));
    fileMenu->addAction(exitAction);

    QMenu *helpMenu = menuBar()->addMenu(tr("About"));
    helpMenu->addAction(aboutAct);
    helpMenu->addAction(aboutQtAct);
}
//! [4]

//! [5]
void MainWindow::changeMode(int index)
{
    QCompleter::CompletionMode mode;
    if (index == 0)
        mode = QCompleter::InlineCompletion;
    else if (index == 1)
        mode = QCompleter::PopupCompletion;
    else
        mode = QCompleter::UnfilteredPopupCompletion;

    completer->setCompletionMode(mode);
}
//! [5]

QAbstractItemModel *MainWindow::modelFromFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly))
        return new QStringListModel(completer);

#ifndef QT_NO_CURSOR
    QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
#endif

    QStandardItemModel *model = new QStandardItemModel(completer);
    QList<QStandardItem *> parents(10);
    parents[0] = model->invisibleRootItem();

    QRegularExpression re("^\\s+");
    while (!file.atEnd()) {
        const QString line = QString::fromUtf8(file.readLine());
        const QString trimmedLine = line.trimmed();
        if (trimmedLine.isEmpty())
            continue;

        const QRegularExpressionMatch match = re.match(line);
        int nonws = match.capturedStart();
        int level = 0;
        if (nonws == -1) {
            level = 0;
        } else {
            const int capLen = match.capturedLength();
            level = capLen / 4;
        }

        if (level + 1 >= parents.size())
            parents.resize(parents.size() * 2);

        QStandardItem *item = new QStandardItem;
        item->setText(trimmedLine);
        parents[level]->appendRow(item);
        parents[level + 1] = item;
    }

#ifndef QT_NO_CURSOR
    QGuiApplication::restoreOverrideCursor();
#endif

    return model;
}

void MainWindow::highlight(const QModelIndex &index)
{
    QAbstractItemModel *completionModel = completer->completionModel();
    QAbstractProxyModel *proxy = qobject_cast<QAbstractProxyModel *>(completionModel);
    if (!proxy)
        return;
    QModelIndex sourceIndex = proxy->mapToSource(index);
    treeView->selectionModel()->select(sourceIndex, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    treeView->scrollTo(sourceIndex);
}

//! [6]
void MainWindow::about()
{
    QMessageBox::about(this, tr("About"), tr("This example demonstrates how "
        "to use a QCompleter with a custom tree model."));
}
//! [6]

//! [7]
void MainWindow::changeCase(int cs)
{
    completer->setCaseSensitivity(cs ? Qt::CaseSensitive : Qt::CaseInsensitive);
}
//! [7]

void MainWindow::updateContentsLabel(const QString &sep)
{
    contentsLabel->setText(tr("Type path from model above with items at each level separated by a '%1'").arg(sep));
}
