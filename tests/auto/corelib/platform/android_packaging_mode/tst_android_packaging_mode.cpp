// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#include <private/qjnihelpers_p.h>

class tst_AndroidPackagingMode : public QObject
{
Q_OBJECT
private slots:
    void runtimeMatchesPackagingMode();
};

void tst_AndroidPackagingMode::runtimeMatchesPackagingMode()
{
#ifdef QT_TEST_EXPECT_LEGACY_PACKAGING
    QVERIFY(!QtAndroidPrivate::isUncompressedNativeLibs());
#else
    QVERIFY(QtAndroidPrivate::isUncompressedNativeLibs());
#endif
}

QTEST_MAIN(tst_AndroidPackagingMode)
#include "tst_android_packaging_mode.moc"
