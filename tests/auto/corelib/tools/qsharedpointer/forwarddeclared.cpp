// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "forwarddeclared.h"
#include "qsharedpointer.h"

class ForwardDeclared
{
public:
    ~ForwardDeclared();
};

QSharedPointer<ForwardDeclared> *forwardPointer()
{
    return new QSharedPointer<ForwardDeclared>(new ForwardDeclared);
}

int forwardDeclaredDestructorRunCount;
ForwardDeclared::~ForwardDeclared()
{
    ++forwardDeclaredDestructorRunCount;
}
