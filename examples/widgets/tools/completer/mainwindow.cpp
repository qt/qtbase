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

#include "mainwindow.h"
#include "fsmodel.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QCompleter>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QSpinBox>
#include <QStandardItemModel>
#include <QStringListModel>
#include <QTreeView>

//! [0]
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    createMenu();

    QWidget *centralWidget = new QWidget;

    QLabel *modelLabel = new QLabel;
    modelLabel->setText(tr("Model"));

    modelCombo = new QComboBox;
    modelCombo->addItem(tr("QFileSystemModel"));
    modelCombo->addItem(tr("QFileSystemModel that shows full path"));
    modelCombo->addItem(tr("Country list"));
    modelCombo->addItem(tr("Word list"));
    modelCombo->setCurrentIndex(0);

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
    QLabel *maxVisibleLabel = new QLabel;
    maxVisibleLabel->setText(tr("Max Visible Items"));
    maxVisibleSpinBox = new QSpinBox;
    maxVisibleSpinBox->setRange(3,25);
    maxVisibleSpinBox->setValue(10);

    wrapCheckBox = new QCheckBox;
    wrapCheckBox->setText(tr("Wrap around completions"));
    wrapCheckBox->setChecked(true);
//! [1]

//! [2]
    contentsLabel = new QLabel;
    contentsLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    connect(modelCombo, QOverload<int>::of(&QComboBox::activated),
            this, &MainWindow::changeModel);
    connect(modeCombo, QOverload<int>::of(&QComboBox::activated),
            this, &MainWindow::changeMode);
    connect(caseCombo, QOverload<int>::of(&QComboBox::activated),
            this, &MainWindow::changeCase);
    connect(maxVisibleSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::changeMaxVisible);
//! [2]

//! [3]
    lineEdit = new QLineEdit;

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(modelLabel, 0, 0); layout->addWidget(modelCombo, 0, 1);
    layout->addWidget(modeLabel, 1, 0);  layout->addWidget(modeCombo, 1, 1);
    layout->addWidget(caseLabel, 2, 0);  layout->addWidget(caseCombo, 2, 1);
    layout->addWidget(maxVisibleLabel, 3, 0); layout->addWidget(maxVisibleSpinBox, 3, 1);
    layout->addWidget(wrapCheckBox, 4, 0);
    layout->addWidget(contentsLabel, 5, 0, 1, 2);
    layout->addWidget(lineEdit, 6, 0, 1, 2);
    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);

    changeModel();

    setWindowTitle(tr("Completer"));
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
QAbstractItemModel *MainWindow::modelFromFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly))
        return new QStringListModel(completer);
//! [5]

//! [6]
#ifndef QT_NO_CURSOR
    QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
#endif
    QStringList words;

    while (!file.atEnd()) {
        QByteArray line = file.readLine();
        if (!line.isEmpty())
            words << QString::fromUtf8(line.trimmed());
    }

#ifndef QT_NO_CURSOR
    QGuiApplication::restoreOverrideCursor();
#endif
//! [6]

//! [7]
    if (!fileName.contains(QLatin1String("countries.txt")))
        return new QStringListModel(words, completer);
//! [7]

    // The last two chars of the countries.txt file indicate the country
    // symbol. We put that in column 2 of a standard item model
//! [8]
    QStandardItemModel *m = new QStandardItemModel(words.count(), 2, completer);
//! [8] //! [9]
    for (int i = 0; i < words.count(); ++i) {
        QModelIndex countryIdx = m->index(i, 0);
        QModelIndex symbolIdx = m->index(i, 1);
        QString country = words.at(i).mid(0, words[i].length() - 2).trimmed();
        QString symbol = words.at(i).right(2);
        m->setData(countryIdx, country);
        m->setData(symbolIdx, symbol);
    }

    return m;
}
//! [9]

//! [10]
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
//! [10]

void MainWindow::changeCase(int cs)
{
    completer->setCaseSensitivity(cs ? Qt::CaseSensitive : Qt::CaseInsensitive);
}

//! [11]
void MainWindow::changeModel()
{
    delete completer;
    completer = new QCompleter(this);
    completer->setMaxVisibleItems(maxVisibleSpinBox->value());

    switch (modelCombo->currentIndex()) {
    default:
    case 0:
        { // Unsorted QFileSystemModel
            QFileSystemModel *fsModel = new QFileSystemModel(completer);
            fsModel->setRootPath(QString());
            completer->setModel(fsModel);
            contentsLabel->setText(tr("Enter file path"));
        }
        break;
//! [11] //! [12]
    case 1:
        {   // FileSystemModel that shows full paths
            FileSystemModel *fsModel = new FileSystemModel(completer);
            completer->setModel(fsModel);
            fsModel->setRootPath(QString());
            contentsLabel->setText(tr("Enter file path"));
        }
        break;
//! [12] //! [13]
    case 2:
        { // Country List
            completer->setModel(modelFromFile(":/resources/countries.txt"));
            QTreeView *treeView = new QTreeView;
            completer->setPopup(treeView);
            treeView->setRootIsDecorated(false);
            treeView->header()->hide();
            treeView->header()->setStretchLastSection(false);
            treeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
            treeView->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
            contentsLabel->setText(tr("Enter name of your country"));
        }
        break;
//! [13] //! [14]
    case 3:
        { // Word list
            completer->setModel(modelFromFile(":/resources/wordlist.txt"));
            completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
            contentsLabel->setText(tr("Enter a word"));
        }
        break;
    }

    changeMode(modeCombo->currentIndex());
    changeCase(caseCombo->currentIndex());
    completer->setWrapAround(wrapCheckBox->isChecked());
    lineEdit->setCompleter(completer);
    connect(wrapCheckBox, &QAbstractButton::clicked, completer, &QCompleter::setWrapAround);
}
//! [14]

//! [15]
void MainWindow::changeMaxVisible(int max)
{
    completer->setMaxVisibleItems(max);
}
//! [15]

//! [16]
void MainWindow::about()
{
    QMessageBox::about(this, tr("About"), tr("This example demonstrates the "
        "different features of the QCompleter class."));
}
//! [16]
