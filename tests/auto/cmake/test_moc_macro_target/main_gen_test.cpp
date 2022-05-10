// Copyright (C) 2013 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Stephen Kelly <stephen.kelly@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QObject>

#include "myinterface.h"

class MyObject : public QObject, MyInterface
{
  Q_OBJECT
  Q_INTERFACES(MyInterface)
public:
  explicit MyObject(QObject *parent = nullptr) : QObject(parent) { }
};

int main(int argc, char **argv)
{
  MyObject mo;
  mo.objectName();
  return 0;
}

#include "main_gen_test.moc"
