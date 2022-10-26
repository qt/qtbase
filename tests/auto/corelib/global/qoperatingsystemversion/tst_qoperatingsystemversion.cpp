// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <qoperatingsystemversion.h>

class tst_QOperatingSystemVersion : public QObject
{
    Q_OBJECT
private slots:
    void construction_data();
    void construction();
    void globals_data();
    void globals();

    void anyOf();

    void comparison_data();
    void comparison();
    void comparison2_data();
    void comparison2();

    void mixedComparison();
};

void tst_QOperatingSystemVersion::construction_data()
{
    QTest::addColumn<QOperatingSystemVersion::OSType>("osType");
    QTest::addColumn<int>("majorVersion");
    QTest::addColumn<int>("minorVersion");
    QTest::addColumn<int>("microVersion");
    QTest::addColumn<int>("segmentCount");

    QTest::newRow("Major only") << QOperatingSystemVersion::OSType::Windows << 1 << -1 << -1 << 1;
    QTest::newRow("Major and minor") << QOperatingSystemVersion::OSType::MacOS
                                     << 1 << 2 << -1 << 2;
    QTest::newRow("Major, minor and micro") << QOperatingSystemVersion::OSType::Android
                                            << 1 << 2 << 3 << 3;
}

void tst_QOperatingSystemVersion::construction()
{
    QFETCH(QOperatingSystemVersion::OSType, osType);
    QFETCH(int, majorVersion);
    QFETCH(int, minorVersion);
    QFETCH(int, microVersion);
    QFETCH(int, segmentCount);

    const QOperatingSystemVersion systemVersion(osType, majorVersion, minorVersion, microVersion);
    QCOMPARE(systemVersion.type(), osType);
    QCOMPARE(systemVersion.segmentCount(), segmentCount);
    QCOMPARE(systemVersion.majorVersion(), majorVersion);
    QCOMPARE(systemVersion.minorVersion(), minorVersion);
    QCOMPARE(systemVersion.microVersion(), microVersion);
    if (osType != QOperatingSystemVersion::OSType::Unknown)
        QVERIFY(!systemVersion.name().isEmpty());
}

void tst_QOperatingSystemVersion::globals_data()
{
    QTest::addColumn<QOperatingSystemVersion>("osver");
    QTest::addColumn<QOperatingSystemVersion::OSType>("osType");

#define ADDROW(os)  QTest::newRow(#os) << QOperatingSystemVersion(QOperatingSystemVersion::os)
    // legacy ones (global variables)
    ADDROW(Windows7) << QOperatingSystemVersion::Windows;
    ADDROW(Windows10) << QOperatingSystemVersion::Windows;
    ADDROW(OSXMavericks) << QOperatingSystemVersion::MacOS;
    ADDROW(MacOSMonterey) << QOperatingSystemVersion::MacOS;
    ADDROW(AndroidJellyBean) << QOperatingSystemVersion::Android;
    ADDROW(Android11) << QOperatingSystemVersion::Android;

    // new ones (static constexpr)
    ADDROW(Windows11) << QOperatingSystemVersion::Windows;
    ADDROW(Android12) << QOperatingSystemVersion::Android;
#undef ADDROW
}

void tst_QOperatingSystemVersion::globals()
{
    QFETCH(QOperatingSystemVersion, osver);
    QFETCH(QOperatingSystemVersion::OSType, osType);
    QCOMPARE(osver.type(), osType);
    QCOMPARE_NE(osver.majorVersion(), 0);
}

void tst_QOperatingSystemVersion::anyOf()
{
    std::initializer_list<QOperatingSystemVersion::OSType> typesToCheck = {
        QOperatingSystemVersion::OSType::Windows, QOperatingSystemVersion::OSType::Android,
        QOperatingSystemVersion::OSType::MacOS, QOperatingSystemVersion::OSType::Unknown
    };
    {
        // type found case
        const QOperatingSystemVersion systemVersion(QOperatingSystemVersion::OSType::MacOS, 1);
        QCOMPARE(systemVersion.isAnyOfType(typesToCheck), true);
    }
    {
        // type NOT found case
        const QOperatingSystemVersion systemVersion(QOperatingSystemVersion::OSType::WatchOS, 1);
        QCOMPARE(systemVersion.isAnyOfType(typesToCheck), false);
    }
}

void tst_QOperatingSystemVersion::comparison_data()
{
    QTest::addColumn<QOperatingSystemVersion::OSType>("lhsType");
    QTest::addColumn<int>("lhsMajor");
    QTest::addColumn<int>("lhsMinor");
    QTest::addColumn<int>("lhsMicro");

    QTest::addColumn<QOperatingSystemVersion::OSType>("rhsType");
    QTest::addColumn<int>("rhsMajor");
    QTest::addColumn<int>("rhsMinor");
    QTest::addColumn<int>("rhsMicro");

    QTest::addColumn<bool>("lessResult");
    QTest::addColumn<bool>("lessEqualResult");
    QTest::addColumn<bool>("moreResult");
    QTest::addColumn<bool>("moreEqualResult");

    QTest::addRow("mismatching types") << QOperatingSystemVersion::OSType::Windows << 1 << 2 << 3
                                       << QOperatingSystemVersion::OSType::MacOS << 1 << 2 << 3
                                       << false << false << false << false;

    QTest::addRow("equal versions") << QOperatingSystemVersion::OSType::Windows << 1 << 2 << 3
                                    << QOperatingSystemVersion::OSType::Windows << 1 << 2 << 3
                                    << false << true << false << true;

    QTest::addRow("lhs micro less") << QOperatingSystemVersion::OSType::Windows << 1 << 2 << 2
                                    << QOperatingSystemVersion::OSType::Windows << 1 << 2 << 3
                                    << true << true << false << false;

    QTest::addRow("rhs micro less") << QOperatingSystemVersion::OSType::Windows << 1 << 2 << 2
                                    << QOperatingSystemVersion::OSType::Windows << 1 << 2 << 1
                                    << false << false << true << true;

    QTest::addRow("lhs minor less") << QOperatingSystemVersion::OSType::Windows << 1 << 2 << 3
                                    << QOperatingSystemVersion::OSType::Windows << 1 << 3 << 3
                                    << true << true << false << false;

    QTest::addRow("rhs minor less") << QOperatingSystemVersion::OSType::Windows << 1 << 2 << 2
                                    << QOperatingSystemVersion::OSType::Windows << 1 << 1 << 3
                                    << false << false << true << true;

    QTest::addRow("lhs major less") << QOperatingSystemVersion::OSType::Windows << 0 << 5 << 6
                                    << QOperatingSystemVersion::OSType::Windows << 1 << 2 << 3
                                    << true << true << false << false;

    QTest::addRow("rhs major less") << QOperatingSystemVersion::OSType::Windows << 1 << 2 << 3
                                    << QOperatingSystemVersion::OSType::Windows << 0 << 2 << 3
                                    << false << false << true << true;

    QTest::addRow("different segmentCount")
            << QOperatingSystemVersion::OSType::Windows << 1 << 2 << 3
            << QOperatingSystemVersion::OSType::Windows << 1 << 2 << -1
            << false << true << false << true;
}

void tst_QOperatingSystemVersion::comparison()
{
    QFETCH(QOperatingSystemVersion::OSType, lhsType);
    QFETCH(int, lhsMajor);
    QFETCH(int, lhsMinor);
    QFETCH(int, lhsMicro);

    const QOperatingSystemVersion lhsSystemInfo(lhsType, lhsMajor, lhsMinor, lhsMicro);

    QFETCH(QOperatingSystemVersion::OSType, rhsType);
    QFETCH(int, rhsMajor);
    QFETCH(int, rhsMinor);
    QFETCH(int, rhsMicro);

    const QOperatingSystemVersion rhsSystemInfo(rhsType, rhsMajor, rhsMinor, rhsMicro);

    QFETCH(bool, lessResult);
    QCOMPARE(lhsSystemInfo < rhsSystemInfo, lessResult);

    QFETCH(bool, lessEqualResult);
    QCOMPARE(lhsSystemInfo <= rhsSystemInfo, lessEqualResult);

    QFETCH(bool, moreResult);
    QCOMPARE(lhsSystemInfo > rhsSystemInfo, moreResult);

    QFETCH(bool, moreEqualResult);
    QCOMPARE(lhsSystemInfo >= rhsSystemInfo, moreEqualResult);
}

void tst_QOperatingSystemVersion::comparison2_data()
{
    QTest::addColumn<QOperatingSystemVersion>("lhs");
    QTest::addColumn<QOperatingSystemVersion>("rhs");
    QTest::addColumn<int>("result");

#define ADDROW(os1, os2)    \
    QTest::newRow(#os1 "-vs-" #os2) << QOperatingSystemVersion(QOperatingSystemVersion::os1) \
                                    << QOperatingSystemVersion(QOperatingSystemVersion::os2)

    // Cross-OS testing: not comparables.
    ADDROW(Windows10, MacOSMonterey) << -128;
    ADDROW(Windows11, MacOSMonterey) << -128;
    ADDROW(MacOSMonterey, Windows10) << -128;
    ADDROW(MacOSMonterey, Windows11) << -128;
    ADDROW(Windows10, MacOSVentura) << -128;
    ADDROW(Windows11, MacOSVentura) << -128;
    ADDROW(MacOSVentura, Windows10) << -128;
    ADDROW(MacOSVentura, Windows11) << -128;
    ADDROW(Windows10, Android10) << -128;
    ADDROW(Windows11, Android11) << -128;

    // Same-OS tests. This list does not have to be exhaustive.
    ADDROW(Windows7, Windows7) << 0;
    ADDROW(Windows7, Windows8) << -1;
    ADDROW(Windows8, Windows7) << 1;
    ADDROW(Windows8, Windows10) << -1;
    ADDROW(Windows10, Windows8) << 1;
    ADDROW(Windows10, Windows10_21H1) << -1;
    ADDROW(Windows10_21H1, Windows10) << 1;
    ADDROW(Windows10, Windows11) << -1;
    ADDROW(MacOSCatalina, MacOSCatalina) << 0;
    ADDROW(MacOSCatalina, MacOSBigSur) << -1;
    ADDROW(MacOSBigSur, MacOSCatalina) << 1;
    ADDROW(MacOSMonterey, MacOSVentura) << -1;
    ADDROW(MacOSVentura, MacOSVentura) << 0;
    ADDROW(MacOSVentura, MacOSMonterey) << 1;
#undef ADDROW
}

void tst_QOperatingSystemVersion::comparison2()
{
    QFETCH(QOperatingSystemVersion, lhs);
    QFETCH(QOperatingSystemVersion, rhs);
    QFETCH(int, result);

    QEXPECT_FAIL("Windows10-vs-Windows10_21H1", "QTBUG-107907: Unexpected behavior", Abort);
    QEXPECT_FAIL("Windows10-vs-Windows11", "QTBUG-107907: Unexpected behavior", Abort);

    // value -128 indicates "not comparable"
    bool comparable = (result != -128);
    QCOMPARE(lhs < rhs, result < 0 && comparable);
    QEXPECT_FAIL("Windows10_21H1-vs-Windows10", "QTBUG-107907: Unexpected behavior", Abort);
    QCOMPARE(lhs <= rhs, result <= 0 && comparable);
    QCOMPARE(lhs > rhs, result > 0 && comparable);
    QCOMPARE(lhs >= rhs, result >= 0 && comparable);
}

void tst_QOperatingSystemVersion::mixedComparison()
{
    // ==
    QVERIFY(QOperatingSystemVersion::Windows10
            >= QOperatingSystemVersionBase(QOperatingSystemVersionBase::Windows, 10, 0));
    QVERIFY(QOperatingSystemVersion::Windows10
            <= QOperatingSystemVersionBase(QOperatingSystemVersionBase::Windows, 10, 0));
}

QTEST_MAIN(tst_QOperatingSystemVersion)
#include "tst_qoperatingsystemversion.moc"
