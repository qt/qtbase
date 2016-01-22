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

#include <QtWidgets>

#include "locationdialog.h"
#include "mainwindow.h"
#include "settingstree.h"

MainWindow::MainWindow()
    : settingsTree(new SettingsTree)
    , locationDialog(Q_NULLPTR)
{
    setCentralWidget(settingsTree);

    createActions();

    autoRefreshAct->setChecked(true);
    fallbacksAct->setChecked(true);

    setWindowTitle(QCoreApplication::applicationName());
    const QRect availableGeometry = QApplication::desktop()->availableGeometry(this);
    adjustSize();
    move((availableGeometry.width() - width()) / 2, (availableGeometry.height() - height()) / 2);
}

void MainWindow::openSettings()
{
    if (!locationDialog)
        locationDialog = new LocationDialog(this);

    if (locationDialog->exec() != QDialog::Accepted)
        return;

    SettingsPtr settings(new QSettings(locationDialog->format(),
                                       locationDialog->scope(),
                                       locationDialog->organization(),
                                       locationDialog->application()));

    setSettingsObject(settings);
    fallbacksAct->setEnabled(true);
}

void MainWindow::openIniFile()
{
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    const QString fileName =
        QFileDialog::getOpenFileName(this, tr("Open INI File"),
                                     directory, tr("INI Files (*.ini *.conf)"));
    if (fileName.isEmpty())
        return;

    SettingsPtr settings(new QSettings(fileName, QSettings::IniFormat));

    setSettingsObject(settings);
    fallbacksAct->setEnabled(false);
}

void MainWindow::openPropertyList()
{
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    const QString fileName =
        QFileDialog::getOpenFileName(this, tr("Open Property List"),
                                     directory, tr("Property List Files (*.plist)"));
    if (fileName.isEmpty())
        return;

    SettingsPtr settings(new QSettings(fileName, QSettings::NativeFormat));
    setSettingsObject(settings);
    fallbacksAct->setEnabled(false);
}

void MainWindow::openRegistryPath()
{
    const QString path =
        QInputDialog::getText(this, tr("Open Registry Path"),
                              tr("Enter the path in the Windows registry:"),
                              QLineEdit::Normal, "HKEY_CURRENT_USER\\");
    if (path.isEmpty())
        return;

    SettingsPtr settings(new QSettings(path, QSettings::NativeFormat));

    setSettingsObject(settings);
    fallbacksAct->setEnabled(false);
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About Settings Editor"),
            tr("The <b>Settings Editor</b> example shows how to access "
               "application settings using Qt."));
}

void MainWindow::createActions()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));

    QAction *openSettingsAct = fileMenu->addAction(tr("&Open Application Settings..."), this, &MainWindow::openSettings);
    openSettingsAct->setShortcuts(QKeySequence::Open);

    QAction *openIniFileAct = fileMenu->addAction(tr("Open I&NI File..."), this, &MainWindow::openIniFile);
    openIniFileAct->setShortcut(tr("Ctrl+N"));

#ifdef Q_OS_OSX
    QAction *openPropertyListAct = fileMenu->addAction(tr("Open Apple &Property List..."), this, &MainWindow::openPropertyList);
    openPropertyListAct->setShortcut(tr("Ctrl+P"));
#endif // Q_OS_OSX

#ifdef Q_OS_WIN
    QAction *openRegistryPathAct = fileMenu->addAction(tr("Open Windows &Registry Path..."), this, &MainWindow::openRegistryPath);
    openRegistryPathAct->setShortcut(tr("Ctrl+G"));
#endif // Q_OS_WIN

    fileMenu->addSeparator();

    refreshAct = fileMenu->addAction(tr("&Refresh"), settingsTree, &SettingsTree::refresh);
    refreshAct->setShortcut(tr("Ctrl+R"));
    refreshAct->setEnabled(false);

    fileMenu->addSeparator();

    QAction *exitAct = fileMenu->addAction(tr("E&xit"), this, &QWidget::close);
    exitAct->setShortcuts(QKeySequence::Quit);

    QMenu *optionsMenu = menuBar()->addMenu(tr("&Options"));

    autoRefreshAct = optionsMenu->addAction(tr("&Auto-Refresh"));
    autoRefreshAct->setShortcut(tr("Ctrl+A"));
    autoRefreshAct->setCheckable(true);
    autoRefreshAct->setEnabled(false);
    connect(autoRefreshAct, &QAction::triggered,
            settingsTree, &SettingsTree::setAutoRefresh);
    connect(autoRefreshAct, &QAction::triggered,
            refreshAct, &QAction::setDisabled);

    fallbacksAct = optionsMenu->addAction(tr("&Fallbacks"));
    fallbacksAct->setShortcut(tr("Ctrl+F"));
    fallbacksAct->setCheckable(true);
    fallbacksAct->setEnabled(false);
    connect(fallbacksAct, &QAction::triggered,
            settingsTree, &SettingsTree::setFallbacksEnabled);

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(tr("&About"), this, &MainWindow::about);
    helpMenu->addAction(tr("About &Qt"), qApp, &QCoreApplication::quit);
}

void MainWindow::setSettingsObject(const SettingsPtr &settings)
{
    settings->setFallbacksEnabled(fallbacksAct->isChecked());
    settingsTree->setSettingsObject(settings);

    refreshAct->setEnabled(true);
    autoRefreshAct->setEnabled(true);

    QString niceName = QDir::cleanPath(settings->fileName());
    int pos = niceName.lastIndexOf(QLatin1Char('/'));
    if (pos != -1)
        niceName.remove(0, pos + 1);

    if (!settings->isWritable())
        niceName = tr("%1 (read only)").arg(niceName);

    setWindowTitle(tr("%1 - %2").arg(niceName, QCoreApplication::applicationName()));
    statusBar()->showMessage(tr("Opened \"%1\"").arg(QDir::toNativeSeparators(settings->fileName())));
}
