// Copyright (C) 2016 Alex Trotsenko <alex1973tr@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef PROVIDER_H
#define PROVIDER_H

#include <QObject>

QT_BEGIN_NAMESPACE
class QSctpSocket;
class QByteArray;
QT_END_NAMESPACE

class Provider : public QObject
{
    Q_OBJECT

public:
    explicit inline Provider(QObject *parent = nullptr) : QObject(parent) { }

    virtual void readDatagram(QSctpSocket &, const QByteArray &) { }
    virtual void newConnection(QSctpSocket &) { }
    virtual void clientDisconnected(QSctpSocket &) { }

signals:
    void writeDatagram(QSctpSocket *to, const QByteArray &ba);
};

#endif
