// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtTest/private/qtestregistry_p.h>

QT_REQUIRE_CONFIG(batch_test_support);

QT_BEGIN_NAMESPACE

namespace QTest {
Q_GLOBAL_STATIC(TestRegistry, g_registry);

TestRegistry *TestRegistry::instance()
{
    return g_registry;
}

void TestRegistry::registerTest(const QString& name, TestEntryFunction entry)
{
    m_tests.emplace(name, std::move(entry));
}

TestRegistry::TestEntryFunction
TestRegistry::getTestEntryFunction(const QString& name) const
{
    const auto it = m_tests.find(name);
    return it != m_tests.end() ? it.value() : nullptr;
}

QStringList TestRegistry::getAllTestNames() const
{
    return m_tests.keys();
}
}

QT_END_NAMESPACE
