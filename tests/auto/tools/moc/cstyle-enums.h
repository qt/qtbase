// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef CSTYLE_ENUMS_H
#define CSTYLE_ENUMS_H
#include <QObject>

class CStyleEnums
{
    Q_GADGET
public:
    typedef enum { Foo, Bar } Baz;
    typedef enum { Foo2, Bar2 } Baz2;
    Q_ENUM(Baz)
    Q_ENUMS(Baz2)
};

#endif // CSTYLE_ENUMS_H
