// Copyright (C) 2013 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Stephen Kelly <stephen.kelly@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef MYWRAPOBJECT_H
#define MYWRAPOBJECT_H

#include <QObject>

#include "myinterface.h"

class MyWrapObject : public QObject, MyInterface
{
  Q_OBJECT
  Q_INTERFACES(MyInterface)
public:
  explicit MyWrapObject(QObject *parent = nullptr) : QObject(parent) { }
};

#endif
