// Copyright (C) 2013 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Stephen Kelly <stephen.kelly@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "somelib.h"

SomeObject::SomeObject(QObject *parent)
  : QTextDocument(parent)
{
}

int SomeObject::value()
{
    return 0;
}
