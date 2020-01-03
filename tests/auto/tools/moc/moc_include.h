/****************************************************************************
**
** Copyright (C) 2020 Olivier Goffart <ogoffart@woboq.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
