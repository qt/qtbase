// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef BLOCKINGCLIENT_H
#define BLOCKINGCLIENT_H

#include <QWidget>

#include "fortunethread.h"

QT_BEGIN_NAMESPACE
class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QAction;
QT_END_NAMESPACE

//! [0]
class BlockingClient : public QWidget
{
    Q_OBJECT

public:
    BlockingClient(QWidget *parent = nullptr);

private slots:
    void requestNewFortune();
    void showFortune(const QString &fortune);
    void displayError(int socketError, const QString &message);
    void enableGetFortuneButton();

private:
    QLabel *hostLabel;
    QLabel *portLabel;
    QLineEdit *hostLineEdit;
    QLineEdit *portLineEdit;
    QLabel *statusLabel;
    QPushButton *getFortuneButton;
    QPushButton *quitButton;
    QDialogButtonBox *buttonBox;

    FortuneThread thread;
    QString currentFortune;
};
//! [0]

#endif
