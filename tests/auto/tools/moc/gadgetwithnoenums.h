// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#ifndef TASK175491
#define TASK175491

#include <QObject>

class GadgetWithNoEnums
{
    Q_GADGET

public:
    GadgetWithNoEnums() {}
    virtual ~GadgetWithNoEnums() {}
};

class DerivedGadgetWithEnums : public GadgetWithNoEnums
{
    Q_GADGET

public:
    enum FooEnum { FooValue };
    Q_ENUM( FooEnum )
    DerivedGadgetWithEnums() {}
    ~DerivedGadgetWithEnums() {}
};

#endif
