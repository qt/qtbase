// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef RELATED_METAOBJECTS_IN_GADGET_H
#define RELATED_METAOBJECTS_IN_GADGET_H

#include <QObject>
#include "qtbug-35657-gadget.h"

namespace QTBUG_35657 {
    class B : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(A::SomeEnum blah READ blah)
    public:

        A::SomeEnum blah() const { return A::SomeEnumValue; }
    };
}

#endif // RELATED_METAOBJECTS_IN_GADGET_H
