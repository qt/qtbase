// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qcoreapplication.h>
#include <QtCore/qharmonyappjscontext.h>
#include <QtCore/qharmonyjscontext.h>
#include <QtCore/qharmonyjsobjectref.h>
#include <QtGui/qwindow.h>
#include <QtTest/qtest.h>

using QHarmonyApplication = QNativeInterface::QHarmonyApplication;
using QHarmonyWindow = QNativeInterface::QHarmonyWindow;

namespace {

QHarmonyJsObjectRef fetchDefaultAbility()
{
    return QHarmonyApplication::runOnJsThreadAndWait(
        [](QHarmonyJsContext &jsContext) {
            return QHarmonyJsObjectRef::fromValue(jsContext, QHarmonyAppJsContext::fromJsContext(jsContext).defaultAbility());
        });
}

}

class tst_HarmonyWindowDefaultAbility : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void appJsContext_defaultAbility_switchesWhenFirstAbilityDestroyed();

private:
    QWindow m_firstWindow;
    QWindow m_secondWindow;
};

void tst_HarmonyWindowDefaultAbility::initTestCase()
{
    m_firstWindow.show();
    QVERIFY(QTest::qWaitForWindowExposed(&m_firstWindow));
    auto *firstHarmonyWindow = m_firstWindow.nativeInterface<QHarmonyWindow>();
    QVERIFY(firstHarmonyWindow);

    const auto firstAbilityRef = firstHarmonyWindow->ability();
    QVERIFY(!firstAbilityRef.isNull());
    QCOMPARE(fetchDefaultAbility(), firstAbilityRef);
}

void tst_HarmonyWindowDefaultAbility::appJsContext_defaultAbility_switchesWhenFirstAbilityDestroyed()
{
    m_secondWindow.show();
    QVERIFY(QTest::qWaitForWindowExposed(&m_secondWindow));
    auto *secondHarmonyWindow = m_secondWindow.nativeInterface<QHarmonyWindow>();
    QVERIFY(secondHarmonyWindow);
    const auto secondAbilityRef = secondHarmonyWindow->ability();
    QVERIFY(!secondAbilityRef.isNull());
    QCOMPARE_NE(fetchDefaultAbility(), secondAbilityRef);

    m_firstWindow.destroy();

    QTRY_COMPARE(fetchDefaultAbility(), secondAbilityRef);
}

QTEST_MAIN(tst_HarmonyWindowDefaultAbility)
#include "tst_harmonywindow_defaultability.moc"
