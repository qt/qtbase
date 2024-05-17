// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QTest>

#include "../../network-settings.h"

class tst_NetworkSelftest : public QObject
{
    Q_OBJECT

private slots:
    void testServerIsAvailableInCI();
};

void tst_NetworkSelftest::testServerIsAvailableInCI()
{
    if (!qEnvironmentVariable("QTEST_ENVIRONMENT").split(' ').contains("ci"))
        QSKIP("Not running in the CI");

    if (qEnvironmentVariable("QT_QPA_PLATFORM").contains("offscreen")
          && !qEnvironmentVariableIsEmpty("QEMU_LD_PREFIX"))
        QSKIP("Not support yet for B2Qt");

#if !defined(QT_TEST_SERVER)
    QVERIFY2(QtNetworkSettings::verifyTestNetworkSettings(),
        "Test server must be available when running in the CI");
#endif
}

QTEST_MAIN(tst_NetworkSelftest)

#include "tst_networkselftest.moc"
