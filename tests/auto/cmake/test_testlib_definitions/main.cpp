// Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Stephen Kelly <stephen.kelly@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QObject>
#include <QTest>

class TestObject : public QObject
{
    Q_OBJECT
public:
    TestObject(QObject *parent = nullptr)
      : QObject(parent)
    {

    }
};

QTEST_MAIN(TestObject)

#include "main.moc"
