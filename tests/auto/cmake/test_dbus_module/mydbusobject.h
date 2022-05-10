// Copyright (C) 2011 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Stephen Kelly <stephen.kelly@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef MYDBUSOBJECT_H
#define MYDBUSOBJECT_H

#include <QObject>

class MyDBusObject : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.qtProject.Tests.MyDBusObject")
public:
    MyDBusObject(QObject *parent = nullptr);

signals:
    void someSignal();
};

#endif
