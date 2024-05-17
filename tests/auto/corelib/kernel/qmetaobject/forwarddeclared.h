// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef FORWARDDECLARED_H
#define FORWARDDECLARED_H

struct MyForwardDeclaredType;      // and ONLY forward-declared

const MyForwardDeclaredType &getForwardDeclaredType() noexcept;
MyForwardDeclaredType *getForwardDeclaredPointer() noexcept;

#endif // FORWARDDECLARED_H
