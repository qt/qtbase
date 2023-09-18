// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "mainwindow.h"
#include "mimetypemodel.h"

#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QItemSelectionModel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeDatabase>
#include <QMimeType>
#include <QSplitter>
#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_model(new MimetypeModel(this))
    , m_treeView(new QTreeView(this))
    , m_detailsText(new QTextEdit(this))
    , m_findIndex(0)
{
    setWindowTitle(tr("Qt Mime Database Browser"));

    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    QAction *detectFileAction =
        fileMenu->addAction(tr("&Detect File Type..."), this, &MainWindow::detectFile);
    detectFileAction->setShortcuts(QKeySequence::Open);

    QAction *exitAction = fileMenu->addAction(tr("E&xit"), qApp, &QApplication::quit);
    exitAction->setShortcuts(QKeySequence::Quit);

    QMenu *findMenu = menuBar()->addMenu(tr("&Edit"));
    QAction *findAction =
        findMenu->addAction(tr("&Find..."), this, &MainWindow::find);
    findAction->setShortcuts(QKeySequence::Find);
    m_findNextAction = findMenu->addAction(tr("Find &Next"), this, &MainWindow::findNext);
    m_findNextAction->setShortcuts(QKeySequence::FindNext);
    m_findPreviousAction = findMenu->addAction(tr("Find &Previous"), this,
                                               &MainWindow::findPrevious);
    m_findPreviousAction->setShortcuts(QKeySequence::FindPrevious);

    menuBar()->addMenu(tr("&About"))->addAction(tr("&About Qt"), qApp, &QApplication::aboutQt);

    QSplitter *centralSplitter = new QSplitter(this);
    setCentralWidget(centralSplitter);
    m_treeView->setUniformRowHeights(true);
    m_treeView->setModel(m_model);
    const auto flags = Qt::MatchContains | Qt::MatchFixedString | Qt::MatchRecursive;
    const auto items = m_model->findItems("application/octet-stream", flags);
    if (!items.isEmpty())
        m_treeView->expand(m_model->indexFromItem(items.constFirst()));

    connect(m_treeView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &MainWindow::currentChanged);
    centralSplitter->addWidget(m_treeView);
    m_detailsText->setReadOnly(true);
    centralSplitter->addWidget(m_detailsText);

    updateFindActions();
}

void MainWindow::currentChanged(const QModelIndex &index)
{
    if (index.isValid())
        m_detailsText->setText(MimetypeModel::formatMimeTypeInfo(m_model->mimeType(index)));
    else
        m_detailsText->clear();
}

void MainWindow::selectAndGoTo(const QModelIndex &index)
{
    m_treeView->scrollTo(index, QAbstractItemView::PositionAtCenter);
    m_treeView->setCurrentIndex(index);
}

void MainWindow::detectFile()
{
    const QString fileName = QFileDialog::getOpenFileName(this, tr("Choose File"));
    if (fileName.isEmpty())
        return;
    QMimeDatabase mimeDatabase;
    const QFileInfo fi(fileName);
    const QMimeType mimeType = mimeDatabase.mimeTypeForFile(fi);
    const QModelIndex index = mimeType.isValid()
        ? m_model->indexForMimeType(mimeType.name()) : QModelIndex();
    if (index.isValid()) {
        statusBar()->showMessage(tr("\"%1\" is of type \"%2\"").arg(fi.fileName(),
                                 mimeType.name()));
        selectAndGoTo(index);
    } else {
        QMessageBox::information(this, tr("Unknown File Type"),
                                 tr("The type of %1 could not be determined.")
                                 .arg(QDir::toNativeSeparators(fileName)));
    }
}

void MainWindow::updateFindActions()
{
    const bool findNextPreviousEnabled = m_findMatches.size() > 1;
    m_findNextAction->setEnabled(findNextPreviousEnabled);
    m_findPreviousAction->setEnabled(findNextPreviousEnabled);
}

void MainWindow::findPrevious()
{
    if (--m_findIndex < 0)
        m_findIndex = m_findMatches.size() - 1;
    if (m_findIndex >= 0)
        selectAndGoTo(m_findMatches.at(m_findIndex));
}

void MainWindow::findNext()
{
    if (++m_findIndex >= m_findMatches.size())
        m_findIndex = 0;
    if (m_findIndex < m_findMatches.size())
        selectAndGoTo(m_findMatches.at(m_findIndex));
}

void MainWindow::find()
{
    QInputDialog inputDialog(this);
    inputDialog.setWindowTitle(tr("Find"));
    inputDialog.setLabelText(tr("Text:"));
    if (inputDialog.exec() != QDialog::Accepted)
        return;
    const QString value = inputDialog.textValue().trimmed();
    if (value.isEmpty())
        return;

    m_findMatches.clear();
    m_findIndex = 0;
    const auto flags = Qt::MatchContains | Qt::MatchFixedString | Qt::MatchRecursive;
    const QList<QStandardItem *> items = m_model->findItems(value, flags);
    for (const QStandardItem *item : items)
        m_findMatches.append(m_model->indexFromItem(item));
    statusBar()->showMessage(tr("%n mime types match \"%1\".", 0, m_findMatches.size()).arg(value));
    updateFindActions();
    if (!m_findMatches.isEmpty())
        selectAndGoTo(m_findMatches.constFirst());
}
