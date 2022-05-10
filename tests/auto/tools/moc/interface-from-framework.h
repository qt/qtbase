// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef INTERFACE_FROM_FRAMEWORK_H
#define INTERFACE_FROM_FRAMEWORK_H

#include <Test/testinterface.h>

class TestComponent : public QObject, public TestInterface
{
    Q_OBJECT
    Q_INTERFACES(TestInterface)
public:

    virtual inline foobar() { }
};

#endif // INTERFACE_FROM_FRAMEWORK_H
