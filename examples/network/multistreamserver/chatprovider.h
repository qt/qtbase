// Copyright (C) 2016 Alex Trotsenko <alex1973tr@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef CHATPROVIDER_H
#define CHATPROVIDER_H

#include "provider.h"

class ChatProvider : public Provider
{
    Q_OBJECT
public:
    explicit ChatProvider(QObject *parent = nullptr);

    void readDatagram(QSctpSocket &from, const QByteArray &ba) override;
    void newConnection(QSctpSocket &client) override;
    void clientDisconnected(QSctpSocket &client) override;
};

#endif
