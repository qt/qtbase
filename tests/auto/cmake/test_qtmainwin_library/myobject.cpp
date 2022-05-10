// Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Stephen Kelly <stephen.kelly@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "myobject.h"

MyObject::MyObject(QObject *parent)
    : QObject(parent)
{
    emit someSignal();
}

int main(int argc, char **argv)
{
    MyObject myObject;
    return 0;
}
