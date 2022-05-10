// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QTBUG_35657_GADGET_H
#define QTBUG_35657_GADGET_H

#include <QObject>

namespace QTBUG_35657 {
    class A {
        Q_GADGET
        Q_ENUMS(SomeEnum)
    public:
        enum SomeEnum { SomeEnumValue = 0 };
    };
}

#endif // QTBUG_35657_GADGET_H
