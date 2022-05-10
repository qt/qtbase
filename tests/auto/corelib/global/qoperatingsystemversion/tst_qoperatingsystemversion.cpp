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

    void anyOf();

    void comparison_data();
    void comparison();

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
