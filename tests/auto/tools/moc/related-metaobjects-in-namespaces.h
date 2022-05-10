// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef RELATED_METAOBJECTS_IN_NAMESPACES_H
#define RELATED_METAOBJECTS_IN_NAMESPACES_H

#include <QObject>

namespace QTBUG_2151 {
    class A : public QObject {
        Q_OBJECT
        Q_ENUMS(SomeEnum)
    public:
        enum SomeEnum { SomeEnumValue = 0 };
    };

    class B : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(A::SomeEnum blah READ blah)
    public:

        A::SomeEnum blah() const { return A::SomeEnumValue; }
    };
}

#endif // RELATED_METAOBJECTS_IN_NAMESPACES_H
