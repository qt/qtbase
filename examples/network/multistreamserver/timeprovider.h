// Copyright (C) 2016 Alex Trotsenko <alex1973tr@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef TIMEPROVIDER_H
#define TIMEPROVIDER_H

#include "provider.h"

class TimeProvider : public Provider
{
    Q_OBJECT
public:
    explicit TimeProvider(QObject *parent = nullptr);

    void readDatagram(QSctpSocket &from, const QByteArray &ba) override;
};

#endif
