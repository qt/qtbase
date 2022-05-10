// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef RECEIVER_H
#define RECEIVER_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QLabel;
class QUdpSocket;
QT_END_NAMESPACE

class Receiver : public QWidget
{
    Q_OBJECT

public:
    explicit Receiver(QWidget *parent = nullptr);

private slots:
    void processPendingDatagrams();

private:
    QLabel *statusLabel = nullptr;
    QUdpSocket *udpSocket = nullptr;
};

#endif
