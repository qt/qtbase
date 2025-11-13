// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
#if QT_DEPRECATED_SINCE(6, 12)
    QT_WARNING_PUSH
    QT_WARNING_DISABLE_DEPRECATED
    Q_ENUMS(Baz2)
    QT_WARNING_POP
#else
    Q_ENUM(Baz2)
#endif
};

#endif // CSTYLE_ENUMS_H
