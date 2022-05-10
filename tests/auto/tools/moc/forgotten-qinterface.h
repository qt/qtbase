// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef FORGOTTEN_QINTERFACE_H
#define FORGOTTEN_QINTERFACE_H

#include <QObject>

struct MyInterface
{
    virtual ~MyInterface() {}
    virtual void foo() = 0;
};

Q_DECLARE_INTERFACE(MyInterface, "foo.bar.blah")

class Test : public QObject, public MyInterface
{
    Q_OBJECT
};
#endif // FORGOTTEN_QINTERFACE_H
