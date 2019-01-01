/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
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
#include "mimetypemodel.h"

#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QInputDialog>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QSplitter>
#include <QStatusBar>
#include <QTextEdit>
#include <QTreeView>

#include <QFileInfo>
#include <QItemSelectionModel>
#include <QMimeDatabase>
#include <QMimeType>

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

    QAction *exitAction = fileMenu->addAction(tr("E&xit"), qApp, &QApplication::closeAllWindows);
    exitAction->setShortcuts(QKeySequence::Quit);

    QMenu *findMenu = menuBar()->addMenu(tr("&Edit"));
    QAction *findAction =
        findMenu->addAction(tr("&Find..."), this, &MainWindow::find);
    findAction->setShortcuts(QKeySequence::Find);
    m_findNextAction = findMenu->addAction(tr("Find &Next"), this, &MainWindow::findNext);
    m_findNextAction->setShortcuts(QKeySequence::FindNext);
    m_findPreviousAction = findMenu->addAction(tr("Find &Previous"), this, &MainWindow::findPrevious);
    m_findPreviousAction->setShortcuts(QKeySequence::FindPrevious);

    menuBar()->addMenu(tr("&About"))->addAction(tr("&About Qt"), qApp, &QApplication::aboutQt);

    QSplitter *centralSplitter = new QSplitter(this);
    setCentralWidget(centralSplitter);
    m_treeView->setUniformRowHeights(true);
    m_treeView->setModel(m_model);

    const auto items = m_model->findItems("application/octet-stream", Qt::MatchContains | Qt::MatchFixedString | Qt::MatchRecursive);
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
        statusBar()->showMessage(tr("\"%1\" is of type \"%2\"").arg(fi.fileName(), mimeType.name()));
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
    const QList<QStandardItem *> items =
            m_model->findItems(value, Qt::MatchContains | Qt::MatchFixedString | Qt::MatchRecursive);
    for (const QStandardItem *item : items)
        m_findMatches.append(m_model->indexFromItem(item));
    statusBar()->showMessage(tr("%n mime types match \"%1\".", 0, m_findMatches.size()).arg(value));
    updateFindActions();
    if (!m_findMatches.isEmpty())
        selectAndGoTo(m_findMatches.constFirst());
}
