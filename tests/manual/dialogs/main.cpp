/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "filedialogpanel.h"
#include "colordialogpanel.h"
#include "fontdialogpanel.h"
#include "printdialogpanel.h"
#include "wizardpanel.h"
#include "messageboxpanel.h"

#include <QMainWindow>
#include <QApplication>
#include <QMenuBar>
#include <QTabWidget>
#include <QMenu>
#include <QAction>
#include <QKeySequence>

// Test for dialogs, allowing to play with all dialog options for implementing native dialogs.
// Compiles with Qt 4.8 and Qt 5.

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
};

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle(tr("Dialogs Qt %1").arg(QLatin1String(QT_VERSION_STR)));
    QMenu *fileMenu = menuBar()->addMenu(tr("File"));
    QAction *quitAction = fileMenu->addAction(tr("Quit"));
    quitAction->setShortcut(QKeySequence(QKeySequence::Quit));
    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
    QAction *action = editMenu->addAction(tr("Cut"));
    action->setShortcut(QKeySequence(QKeySequence::Quit));
    action = editMenu->addAction(tr("Copy"));
    action->setShortcut(QKeySequence(QKeySequence::Copy));
    action = editMenu->addAction(tr("Paste"));
    action->setShortcut(QKeySequence(QKeySequence::Paste));
    action = editMenu->addAction(tr("Select All"));
    action->setShortcut(QKeySequence(QKeySequence::SelectAll));
    QTabWidget *tabWidget = new QTabWidget;
    tabWidget->addTab(new FileDialogPanel, tr("QFileDialog"));
    tabWidget->addTab(new ColorDialogPanel, tr("QColorDialog"));
    tabWidget->addTab(new FontDialogPanel, tr("QFontDialog"));
    tabWidget->addTab(new WizardPanel, tr("QWizard"));
    tabWidget->addTab(new MessageBoxPanel, tr("QMessageBox"));
#ifndef QT_NO_PRINTER
    tabWidget->addTab(new PrintDialogPanel, tr("QPrintDialog"));
#endif
    setCentralWidget(tabWidget);
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.move(500, 200);
    w.show();
    return a.exec();
}

#include "main.moc"
