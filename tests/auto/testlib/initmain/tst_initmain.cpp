// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QtCore/QCoreApplication>
#include <QTest>

class tst_InitMain : public QObject
{
    Q_OBJECT

public:
    static void initMain() { m_initMainCalled = true; }

private slots:
    void testcase();

private:
    static bool m_initMainCalled;
};

bool tst_InitMain::m_initMainCalled = false;

void tst_InitMain::testcase()
{
    QVERIFY(m_initMainCalled);
}

QTEST_MAIN(tst_InitMain)

#include "tst_initmain.moc"
