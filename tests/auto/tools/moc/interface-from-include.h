// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#ifndef INTERFACE_FROM_INCLUDE_H
#define INTERFACE_FROM_INCLUDE_H

#include <testinterface.h>

class TestComponent : public QObject, public TestInterface
{
    Q_OBJECT
    Q_INTERFACES(TestInterface)
public:

    virtual void foobar() { }
};

#endif
