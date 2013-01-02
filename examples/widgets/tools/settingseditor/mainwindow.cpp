/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
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
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
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

#include <QtWidgets>

#include "locationdialog.h"
#include "mainwindow.h"
#include "settingstree.h"

MainWindow::MainWindow()
{
    settingsTree = new SettingsTree;
    setCentralWidget(settingsTree);

    locationDialog = 0;

    createActions();
    createMenus();

    autoRefreshAct->setChecked(true);
    fallbacksAct->setChecked(true);

    setWindowTitle(tr("Settings Editor"));
    resize(500, 600);
}

void MainWindow::openSettings()
{
    if (!locationDialog)
        locationDialog = new LocationDialog(this);

    if (locationDialog->exec()) {
        QSettings *settings = new QSettings(locationDialog->format(),
                                            locationDialog->scope(),
                                            locationDialog->organization(),
                                            locationDialog->application());
        setSettingsObject(settings);
        fallbacksAct->setEnabled(true);
    }
}

void MainWindow::openIniFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open INI File"),
                               "", tr("INI Files (*.ini *.conf)"));
    if (!fileName.isEmpty()) {
        QSettings *settings = new QSettings(fileName, QSettings::IniFormat);
        setSettingsObject(settings);
        fallbacksAct->setEnabled(false);
    }
}

void MainWindow::openPropertyList()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                               tr("Open Property List"),
                               "", tr("Property List Files (*.plist)"));
    if (!fileName.isEmpty()) {
        QSettings *settings = new QSettings(fileName, QSettings::NativeFormat);
        setSettingsObject(settings);
        fallbacksAct->setEnabled(false);
    }
}

void MainWindow::openRegistryPath()
{
    QString path = QInputDialog::getText(this, tr("Open Registry Path"),
                           tr("Enter the path in the Windows registry:"),
                           QLineEdit::Normal, "HKEY_CURRENT_USER\\");
    if (!path.isEmpty()) {
        QSettings *settings = new QSettings(path, QSettings::NativeFormat);
        setSettingsObject(settings);
        fallbacksAct->setEnabled(false);
    }
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About Settings Editor"),
            tr("The <b>Settings Editor</b> example shows how to access "
               "application settings using Qt."));
}

void MainWindow::createActions()
{
    openSettingsAct = new QAction(tr("&Open Application Settings..."), this);
    openSettingsAct->setShortcuts(QKeySequence::Open);
    connect(openSettingsAct, SIGNAL(triggered()), this, SLOT(openSettings()));

    openIniFileAct = new QAction(tr("Open I&NI File..."), this);
    openIniFileAct->setShortcut(tr("Ctrl+N"));
    connect(openIniFileAct, SIGNAL(triggered()), this, SLOT(openIniFile()));

    openPropertyListAct = new QAction(tr("Open Mac &Property List..."), this);
    openPropertyListAct->setShortcut(tr("Ctrl+P"));
    connect(openPropertyListAct, SIGNAL(triggered()),
            this, SLOT(openPropertyList()));

    openRegistryPathAct = new QAction(tr("Open Windows &Registry Path..."),
                                      this);
    openRegistryPathAct->setShortcut(tr("Ctrl+G"));
    connect(openRegistryPathAct, SIGNAL(triggered()),
            this, SLOT(openRegistryPath()));

    refreshAct = new QAction(tr("&Refresh"), this);
    refreshAct->setShortcut(tr("Ctrl+R"));
    refreshAct->setEnabled(false);
    connect(refreshAct, SIGNAL(triggered()), settingsTree, SLOT(refresh()));

    exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcuts(QKeySequence::Quit);
    connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));

    autoRefreshAct = new QAction(tr("&Auto-Refresh"), this);
    autoRefreshAct->setShortcut(tr("Ctrl+A"));
    autoRefreshAct->setCheckable(true);
    autoRefreshAct->setEnabled(false);
    connect(autoRefreshAct, SIGNAL(triggered(bool)),
            settingsTree, SLOT(setAutoRefresh(bool)));
    connect(autoRefreshAct, SIGNAL(triggered(bool)),
            refreshAct, SLOT(setDisabled(bool)));

    fallbacksAct = new QAction(tr("&Fallbacks"), this);
    fallbacksAct->setShortcut(tr("Ctrl+F"));
    fallbacksAct->setCheckable(true);
    fallbacksAct->setEnabled(false);
    connect(fallbacksAct, SIGNAL(triggered(bool)),
            settingsTree, SLOT(setFallbacksEnabled(bool)));

    aboutAct = new QAction(tr("&About"), this);
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));

    aboutQtAct = new QAction(tr("About &Qt"), this);
    connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

#ifndef Q_OS_MAC
    openPropertyListAct->setEnabled(false);
#endif
#ifndef Q_OS_WIN
    openRegistryPathAct->setEnabled(false);
#endif
}

void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(openSettingsAct);
    fileMenu->addAction(openIniFileAct);
    fileMenu->addAction(openPropertyListAct);
    fileMenu->addAction(openRegistryPathAct);
    fileMenu->addSeparator();
    fileMenu->addAction(refreshAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    optionsMenu = menuBar()->addMenu(tr("&Options"));
    optionsMenu->addAction(autoRefreshAct);
    optionsMenu->addAction(fallbacksAct);

    menuBar()->addSeparator();

    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAct);
    helpMenu->addAction(aboutQtAct);
}

void MainWindow::setSettingsObject(QSettings *settings)
{
    settings->setFallbacksEnabled(fallbacksAct->isChecked());
    settingsTree->setSettingsObject(settings);

    refreshAct->setEnabled(true);
    autoRefreshAct->setEnabled(true);

    QString niceName = settings->fileName();
    niceName.replace("\\", "/");
    int pos = niceName.lastIndexOf("/");
    if (pos != -1)
        niceName.remove(0, pos + 1);

    if (!settings->isWritable())
        niceName = tr("%1 (read only)").arg(niceName);

    setWindowTitle(tr("%1 - %2").arg(niceName).arg(tr("Settings Editor")));
}
