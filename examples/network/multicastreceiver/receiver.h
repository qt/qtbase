// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef RECEIVER_H
#define RECEIVER_H

#include <QDialog>
#include <QHostAddress>
#include <QUdpSocket>

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE

class Receiver : public QDialog
{
    Q_OBJECT

public:
    explicit Receiver(QWidget *parent = nullptr);

private slots:
    void processPendingDatagrams();

private:
    QLabel *statusLabel = nullptr;
    QUdpSocket udpSocket4;
    QUdpSocket udpSocket6;
    QHostAddress groupAddress4;
    QHostAddress groupAddress6;
};

#endif
