/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
