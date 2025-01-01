// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "browser.h"

#include <QApplication>
#include <QDebug>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QSqlError>
#include <QStatusBar>
#include <QUrl>

void addConnectionsFromCommandline(const QStringList &args, Browser *browser)
{
    for (qsizetype i = 1; i < args.count(); ++i) {
        const auto &arg = args.at(i);
        const QUrl url(arg, QUrl::TolerantMode);
        if (!url.isValid()) {
            qWarning("Invalid URL: %s", qPrintable(arg));
            continue;
        }
        QSqlError err = browser->addConnection(url.scheme(), url.path().mid(1), url.host(),
                                               url.userName(), url.password(), url.port(-1));
        if (err.type() != QSqlError::NoError)
            qDebug() << "Unable to open connection:" << err;
    }
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QMainWindow mainWin;
    mainWin.setWindowTitle(QApplication::translate("MainWindow", "Qt SQL Browser"));

    Browser browser(&mainWin);
    mainWin.setCentralWidget(&browser);

    QMenu *fileMenu = mainWin.menuBar()->addMenu(QObject::tr("&File"));
    fileMenu->addAction(QApplication::translate("MainWindow", "Add &Connection..."),
                        &browser, &Browser::openNewConnectionDialog);
    fileMenu->addSeparator();
    fileMenu->addAction(QApplication::translate("MainWindow", "&Quit"),
                        qApp, &QApplication::quit);

    QMenu *helpMenu = mainWin.menuBar()->addMenu(QObject::tr("&Help"));
    helpMenu->addAction(QApplication::translate("MainWindow", "About"),
                        &browser, &Browser::about);
    helpMenu->addAction(QApplication::translate("MainWindow", "About Qt"),
                        qApp, &QApplication::aboutQt);

    QObject::connect(&browser, &Browser::statusMessage,
                     &mainWin, [&mainWin](const QString &text) { mainWin.statusBar()->showMessage(text); });

    addConnectionsFromCommandline(app.arguments(), &browser);
    mainWin.show();
    if (QSqlDatabase::connectionNames().isEmpty())
        QMetaObject::invokeMethod(&browser, &Browser::openNewConnectionDialog,
                                  Qt::QueuedConnection);

    return app.exec();
}
