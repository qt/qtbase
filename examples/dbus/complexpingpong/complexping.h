// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef COMPLEXPING_H
#define COMPLEXPING_H

#include <QtCore/QObject>
#include <QtCore/QFile>
#include <QtDBus/QDBusInterface>

class Ping: public QObject
{
    Q_OBJECT
public slots:
    void start(const QString &);
public:
    QFile qstdin;
    QDBusInterface *iface;
};

#endif
