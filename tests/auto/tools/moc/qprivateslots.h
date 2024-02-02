// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QPRIVATESLOTS_H
#define QPRIVATESLOTS_H

#include <QObject>

struct TestQPrivateSlots_Private
{
    void _q_privateslot() {}
};

class TestQPrivateSlots: public QObject
{
    Q_OBJECT
public:
    TestQPrivateSlots() : d(new TestQPrivateSlots_Private()) {}
    ~TestQPrivateSlots() { delete d; }
private:
    Q_PRIVATE_SLOT(d, void _q_privateslot())
    Q_INVOKABLE void method1() {}; //task 204730
    TestQPrivateSlots_Private *d;
};

#endif // QPRIVATESLOTS_H
