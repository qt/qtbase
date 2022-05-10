// Copyright (C) 2011 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Stephen Kelly <stephen.kelly@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mydbusobject.h"
#include "mydbusobjectadaptor.h"

MyDBusObject::MyDBusObject(QObject *parent)
    : QObject(parent)
{
    new MyDBusObjectAdaptor(this);
    emit someSignal();
}

int main(int argc, char **argv)
{
    MyDBusObject myDBusObject;
    return 0;
}
