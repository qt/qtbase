// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "server.h"

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow();

private slots:
    void addErrorMessage(const QString &message);
    void addWarningMessage(const QString &message);
    void addInfoMessage(const QString &message);
    void addClientMessage(const QString &peerInfo, const QByteArray &datagram,
                          const QByteArray &plainText);

    void on_startButton_clicked();
    void on_quitButton_clicked();

private:
    void updateUi();

    Ui::MainWindow *ui = nullptr;
    DtlsServer server;
};

#endif // MAINWINDOW_H
