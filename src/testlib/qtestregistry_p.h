// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTESTREGISTRY_P_H
#define QTESTREGISTRY_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qstring.h>
#include <QtCore/qhash.h>
#include <QtTest/qttestglobal.h>

QT_REQUIRE_CONFIG(batch_test_support);

QT_BEGIN_NAMESPACE

namespace QTest {
class TestRegistry {
public:
    using TestEntryFunction = int(*)(int argv, char** argc);

    static TestRegistry* instance();

    void registerTest(const QString& name, TestEntryFunction data);
    size_t total() const {
        return m_tests.size();
    }
    TestEntryFunction getTestEntryFunction(const QString& name) const;
    QStringList getAllTestNames() const;

private:
    QHash<QString, TestEntryFunction> m_tests;
};
}  // namespace QTest

QT_END_NAMESPACE

#endif  // QTESTREGISTRY_P_H
