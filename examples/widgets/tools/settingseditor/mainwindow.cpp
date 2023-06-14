// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "locationdialog.h"
#include "mainwindow.h"
#include "settingstree.h"

#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QScreen>
#include <QStandardPaths>
#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
    , settingsTree(new SettingsTree)
{
    setCentralWidget(settingsTree);

    createActions();

    autoRefreshAct->setChecked(true);
    fallbacksAct->setChecked(true);

    setWindowTitle(QCoreApplication::applicationName());
    const QRect availableGeometry = screen()->availableGeometry();
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

#ifdef Q_OS_MACOS
    QAction *openPropertyListAct = fileMenu->addAction(tr("Open Apple &Property List..."), this, &MainWindow::openPropertyList);
    openPropertyListAct->setShortcut(tr("Ctrl+P"));
#endif // Q_OS_MACOS

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
    helpMenu->addAction(tr("About &Qt"), qApp, &QApplication::aboutQt);
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
