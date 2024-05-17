// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TST_MOC_NAMESPACED_BASE_CLASS_H
#define TST_MOC_NAMESPACED_BASE_CLASS_H

#include <QObject>

namespace QTBUG_101141 {
    class Base : public QObject {
        Q_OBJECT
    };

    class Derived : public Base
    {
        Q_OBJECT
    };
}

#endif // TST_MOC_NAMESPACED_BASE_CLASS_H
