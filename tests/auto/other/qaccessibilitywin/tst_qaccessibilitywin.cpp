// Copyright (C) 2026 Reza Bakhshi Laktasaraei
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qglobal.h>
#include <QtCore/private/qcomvariant_p.h>
#include <QtCore/private/qfunctions_win_p.h>
#include <QtGui/QAccessible>
#include <QtTest/QTest>
#include <QtWidgets/QMenu>

#include <uiautomation.h>

using namespace Qt::StringLiterals;

class tst_QAccessibilityWin : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void menuItemAttributes();
};

void tst_QAccessibilityWin::initTestCase()
{
    if (QGuiApplication::platformName() != "windows"_L1)
        QSKIP("This test requires the Windows platform plugin");
    QAccessible::setActive(true);
    QVERIFY(QAccessible::isActive());
}

void tst_QAccessibilityWin::menuItemAttributes()
{
    QMenu menu;
    auto *action = menu.addAction("First"_L1);
    menu.addSeparator();
    menu.addAction("Second"_L1);
    menu.show();
    QVERIFY(QTest::qWaitForWindowExposed(&menu));

    QComHelper comHelper;
    QVERIFY(comHelper.isValid());
    const GUID automationClsid{0xff48dba4, 0x60ef, 0x4201,
                               {0xaa, 0x87, 0x54, 0x10, 0x3e, 0xef, 0x59, 0x4e}};
    ComPtr<IUIAutomation> automation;
    QVERIFY(SUCCEEDED(CoCreateInstance(automationClsid, nullptr, CLSCTX_INPROC_SERVER,
                                       IID_PPV_ARGS(&automation))));
    ComPtr<IUIAutomationElement> menuElement;
    QVERIFY(SUCCEEDED(automation->ElementFromHandle(reinterpret_cast<HWND>(menu.winId()),
                                                  &menuElement)));
    QVERIFY(menuElement);
    ComPtr<IUIAutomationTreeWalker> walker;
    QVERIFY(SUCCEEDED(automation->get_RawViewWalker(&walker)));
    QVERIFY(walker);
    ComPtr<IUIAutomationElement> itemElement;
    QVERIFY(SUCCEEDED(walker->GetFirstChildElement(menuElement.Get(), &itemElement)));
    QVERIFY(itemElement);

    QComVariant name;
    QVERIFY(SUCCEEDED(itemElement->GetCurrentPropertyValue(UIA_NamePropertyId, &name)));
    QCOMPARE(name.get().vt, VT_BSTR);
    QCOMPARE(QString::fromWCharArray(name.get().bstrVal), "First"_L1);

    for (const bool hidden : { false, true, false }) {
        action->setVisible(!hidden);
        QComVariant attributes;
        QVERIFY(SUCCEEDED(itemElement->GetCurrentPropertyValue(UIA_AriaPropertiesPropertyId,
                                                               &attributes)));
        QCOMPARE(attributes.get().vt, VT_BSTR);
        const auto entries = QString::fromWCharArray(attributes.get().bstrVal).split(u';');
        QStringList positions;
        QStringList sizes;
        for (const auto &entry : entries) {
            if (entry.startsWith("posinset="_L1))
                positions.append(entry);
            if (entry.startsWith("setsize="_L1))
                sizes.append(entry);
        }
        QCOMPARE(positions, hidden ? QStringList{} : QStringList{"posinset=1"_L1});
        QCOMPARE(sizes, hidden ? QStringList{} : QStringList{"setsize=2"_L1});
    }
}

QTEST_MAIN(tst_QAccessibilityWin)
#include "tst_qaccessibilitywin.moc"
