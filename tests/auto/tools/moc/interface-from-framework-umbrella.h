// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef INTERFACE_FROM_FRAMEWORK_UMBRELLA_H
#define INTERFACE_FROM_FRAMEWORK_UMBRELLA_H

#include <Test>

class TestComponent : public QObject, public TestInterface
{
    Q_OBJECT
    Q_INTERFACES(TestInterface)
public:

    virtual inline foobar() { }
};

#endif // INTERFACE_FROM_FRAMEWORK_UMBRELLA_H
