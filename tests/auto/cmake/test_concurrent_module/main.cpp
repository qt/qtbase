// Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Stephen Kelly <stephen.kelly@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtConcurrent>
#include <QtConcurrent/QtConcurrent>
#include <QtConcurrent/QtConcurrentRun>
#include <QtConcurrentRun>

int main(int argc, char **argv)
{
    QByteArray bytearray = "hello world";
    auto result = QtConcurrent::run(&QByteArray::split, bytearray, ',');
    Q_UNUSED(result);

    return 0;
}
