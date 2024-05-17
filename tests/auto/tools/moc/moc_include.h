// Copyright (C) 2020 Olivier Goffart <ogoffart@woboq.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef MOC_INCLUDE_H
#define MOC_INCLUDE_H

#include <QObject>

class FwdClass1;
class FwdClass2;
class FwdClass3;

Q_MOC_INCLUDE(fwdclass3.h)

namespace SomeRandomNamespace {
Q_MOC_INCLUDE("fwdclass1.h")
Q_NAMESPACE
}

class TestFwdProperties : public QObject
{
    Q_OBJECT
    Q_PROPERTY(FwdClass1 prop1 WRITE setProp1 READ getProp1)
    Q_PROPERTY(FwdClass2 prop2 WRITE setProp2 READ getProp2)
    Q_PROPERTY(FwdClass3 prop3 WRITE setProp3 READ getProp3)
public:
    ~TestFwdProperties();

    void setProp1(const FwdClass1 &val);
    void setProp2(const FwdClass2 &val);
    void setProp3(const FwdClass3 &val);
    const FwdClass1 &getProp1() { return *prop1; }
    const FwdClass2 &getProp2() { return *prop2; }
    const FwdClass3 &getProp3() { return *prop3; }

    QScopedPointer<FwdClass1> prop1;
    QScopedPointer<FwdClass2> prop2;
    QScopedPointer<FwdClass3> prop3;

    Q_MOC_INCLUDE(
        \
        "fwdclass2.h"
    )

};

Q_MOC_INCLUDE(<QString>)

#endif // MOC_INCLUDE_H
