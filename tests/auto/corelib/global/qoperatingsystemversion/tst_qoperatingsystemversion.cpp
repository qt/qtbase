// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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

    void constantsSemantics_data();
    void constantsSemantics();
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

    QTest::addColumn<Qt::partial_ordering>("expectedResult");

    QTest::addRow("mismatching types") << QOperatingSystemVersion::OSType::Windows << 1 << 2 << 3
                                       << QOperatingSystemVersion::OSType::MacOS << 1 << 2 << 3
                                       << Qt::partial_ordering::unordered;

    QTest::addRow("equal versions") << QOperatingSystemVersion::OSType::Windows << 1 << 2 << 3
                                    << QOperatingSystemVersion::OSType::Windows << 1 << 2 << 3
                                    << Qt::partial_ordering::equivalent;

    QTest::addRow("lhs micro less") << QOperatingSystemVersion::OSType::Windows << 1 << 2 << 2
                                    << QOperatingSystemVersion::OSType::Windows << 1 << 2 << 3
                                    << Qt::partial_ordering::less;

    QTest::addRow("rhs micro less") << QOperatingSystemVersion::OSType::Windows << 1 << 2 << 2
                                    << QOperatingSystemVersion::OSType::Windows << 1 << 2 << 1
                                    << Qt::partial_ordering::greater;

    QTest::addRow("lhs minor less") << QOperatingSystemVersion::OSType::Windows << 1 << 2 << 3
                                    << QOperatingSystemVersion::OSType::Windows << 1 << 3 << 3
                                    << Qt::partial_ordering::less;

    QTest::addRow("rhs minor less") << QOperatingSystemVersion::OSType::Windows << 1 << 2 << 2
                                    << QOperatingSystemVersion::OSType::Windows << 1 << 1 << 3
                                    << Qt::partial_ordering::greater;

    QTest::addRow("lhs major less") << QOperatingSystemVersion::OSType::Windows << 0 << 5 << 6
                                    << QOperatingSystemVersion::OSType::Windows << 1 << 2 << 3
                                    << Qt::partial_ordering::less;

    QTest::addRow("rhs major less") << QOperatingSystemVersion::OSType::Windows << 1 << 2 << 3
                                    << QOperatingSystemVersion::OSType::Windows << 0 << 2 << 3
                                    << Qt::partial_ordering::greater;

    QTest::addRow("different segmentCount")
            << QOperatingSystemVersion::OSType::Windows << 1 << 2 << 3
            << QOperatingSystemVersion::OSType::Windows << 1 << 2 << -1
            << Qt::partial_ordering::equivalent;
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

    QFETCH(const Qt::partial_ordering, expectedResult);

    QCOMPARE_EQ(lhsSystemInfo < rhsSystemInfo, is_lt(expectedResult));
    QCOMPARE_EQ(lhsSystemInfo <= rhsSystemInfo, is_lteq(expectedResult));
    QCOMPARE_EQ(lhsSystemInfo > rhsSystemInfo, is_gt(expectedResult));
    QCOMPARE_EQ(lhsSystemInfo >= rhsSystemInfo, is_gteq(expectedResult));
#ifdef __cpp_lib_three_way_comparison
    QCOMPARE_EQ(lhsSystemInfo <=> rhsSystemInfo, expectedResult);
#endif
}

void tst_QOperatingSystemVersion::comparison2_data()
{
    QTest::addColumn<QOperatingSystemVersion>("lhs");
    QTest::addColumn<QOperatingSystemVersion>("rhs");
    QTest::addColumn<Qt::partial_ordering>("result");

#define ADDROW(os1, os2)    \
    QTest::newRow(#os1 "-vs-" #os2) << QOperatingSystemVersion(QOperatingSystemVersion::os1) \
                                    << QOperatingSystemVersion(QOperatingSystemVersion::os2)

    // Cross-OS testing: not comparables.
    ADDROW(Windows10, MacOSMonterey) << Qt::partial_ordering::unordered;
    ADDROW(Windows11, MacOSMonterey) << Qt::partial_ordering::unordered;
    ADDROW(MacOSMonterey, Windows10) << Qt::partial_ordering::unordered;
    ADDROW(MacOSMonterey, Windows11) << Qt::partial_ordering::unordered;
    ADDROW(Windows10, MacOSVentura) << Qt::partial_ordering::unordered;
    ADDROW(Windows11, MacOSVentura) << Qt::partial_ordering::unordered;
    ADDROW(MacOSVentura, Windows10) << Qt::partial_ordering::unordered;
    ADDROW(MacOSVentura, Windows11) << Qt::partial_ordering::unordered;
    ADDROW(Windows10, Android10) << Qt::partial_ordering::unordered;
    ADDROW(Windows11, Android11) << Qt::partial_ordering::unordered;

    // Same-OS tests. This list does not have to be exhaustive.
    ADDROW(Windows7, Windows7) << Qt::partial_ordering::equivalent;
    ADDROW(Windows7, Windows8) << Qt::partial_ordering::less;
    ADDROW(Windows8, Windows7) << Qt::partial_ordering::greater;
    ADDROW(Windows8, Windows10) << Qt::partial_ordering::less;
    ADDROW(Windows10, Windows8) << Qt::partial_ordering::greater;
    ADDROW(Windows10, Windows10_21H1) << Qt::partial_ordering::less;
    ADDROW(Windows10_21H1, Windows10) << Qt::partial_ordering::greater;
    ADDROW(Windows10, Windows11) << Qt::partial_ordering::less;
    ADDROW(MacOSCatalina, MacOSCatalina) << Qt::partial_ordering::equivalent;
    ADDROW(MacOSCatalina, MacOSBigSur) << Qt::partial_ordering::less;
    ADDROW(MacOSBigSur, MacOSCatalina) << Qt::partial_ordering::greater;
    ADDROW(MacOSMonterey, MacOSVentura) << Qt::partial_ordering::less;
    ADDROW(MacOSVentura, MacOSVentura) << Qt::partial_ordering::equivalent;
    ADDROW(MacOSVentura, MacOSMonterey) << Qt::partial_ordering::greater;
#undef ADDROW
}

void tst_QOperatingSystemVersion::comparison2()
{
    QFETCH(QOperatingSystemVersion, lhs);
    QFETCH(QOperatingSystemVersion, rhs);
    QFETCH(const Qt::partial_ordering, result);

    QEXPECT_FAIL("Windows10-vs-Windows10_21H1", "QTBUG-107907: Unexpected behavior", Abort);
    QEXPECT_FAIL("Windows10-vs-Windows11", "QTBUG-107907: Unexpected behavior", Abort);

    const bool comparable = (result != Qt::partial_ordering::unordered);
    QCOMPARE_EQ(lhs < rhs, is_lt(result) && comparable);
    QEXPECT_FAIL("Windows10_21H1-vs-Windows10", "QTBUG-107907: Unexpected behavior", Abort);
    QCOMPARE_EQ(lhs <= rhs, is_lteq(result) && comparable);
    QCOMPARE_EQ(lhs > rhs, is_gt(result) && comparable);
    QCOMPARE_EQ(lhs >= rhs, is_gteq(result) && comparable);
#ifdef __cpp_lib_three_way_comparison
    QCOMPARE_EQ(lhs <=> rhs, result);
#endif
}

void tst_QOperatingSystemVersion::mixedComparison()
{
    // ==
    QVERIFY(QOperatingSystemVersion::Windows10
            >= QOperatingSystemVersionBase(QOperatingSystemVersionBase::Windows, 10, 0));
    QVERIFY(QOperatingSystemVersion::Windows10
            <= QOperatingSystemVersionBase(QOperatingSystemVersionBase::Windows, 10, 0));
}

void tst_QOperatingSystemVersion::constantsSemantics_data()
{
    QTest::addColumn<QOperatingSystemVersionBase>("referenceVersion");
    QTest::addColumn<int>("segmentCount");

    // OSX/macOS 10.x
    QTest::newRow("Mavericks") << QOperatingSystemVersionBase(QOperatingSystemVersion::OSXMavericks) << 2;
    QTest::newRow("Yosemite") << QOperatingSystemVersionBase(QOperatingSystemVersion::OSXYosemite) << 2;
    QTest::newRow("El Capitan") << QOperatingSystemVersionBase(QOperatingSystemVersion::OSXElCapitan) << 2;
    QTest::newRow("Sierra") << QOperatingSystemVersionBase(QOperatingSystemVersion::MacOSSierra) << 2;
    QTest::newRow("High Sierra") << QOperatingSystemVersionBase(QOperatingSystemVersion::MacOSHighSierra) << 2;
    QTest::newRow("Mojave") << QOperatingSystemVersionBase(QOperatingSystemVersion::MacOSMojave) << 2;
    QTest::newRow("Catalina") << QOperatingSystemVersionBase(QOperatingSystemVersion::MacOSCatalina) << 2;

    // macOS 11+
    QTest::newRow("Big Sur") << QOperatingSystemVersionBase(QOperatingSystemVersion::MacOSBigSur) << 1;
    QTest::newRow("Monterey") << QOperatingSystemVersionBase(QOperatingSystemVersion::MacOSMonterey) << 1;
    QTest::newRow("Ventura") << QOperatingSystemVersion::MacOSVentura << 1;
    QTest::newRow("Sonoma") << QOperatingSystemVersion::MacOSSonoma << 1;
    QTest::newRow("Sequoia") << QOperatingSystemVersion::MacOSSequoia << 1;
    QTest::newRow("Tahoe") << QOperatingSystemVersion::MacOSTahoe << 1;
}

void tst_QOperatingSystemVersion::constantsSemantics()
{
    QFETCH(QOperatingSystemVersionBase, referenceVersion);
    QFETCH(int, segmentCount);

    QCOMPARE(referenceVersion.segmentCount(), segmentCount);

    auto adjustedVersion = [&](QOperatingSystemVersionBase referenceVersion, int segment, int adjustment) {
        auto adjustedSegments = referenceVersion.version().segments();
        adjustedSegments[segment] += adjustment;
        return QOperatingSystemVersionBase(referenceVersion.type(), adjustedSegments.at(0),
            qMax(adjustedSegments.at(1), 0), qMax(adjustedSegments.at(2), 0));

    };

    // Increases or decreases of the significant version part should contribute to
    // a smaller or larger version
    for (int segment = 0; segment < referenceVersion.segmentCount(); ++segment) {
        auto smallerVersion = adjustedVersion(referenceVersion, segment, -1);
        QCOMPARE_LT(smallerVersion, referenceVersion);
        auto largerVersion = adjustedVersion(referenceVersion, segment, 1);
        QCOMPARE_GT(largerVersion, referenceVersion);
    }

    // The non-significant parts of a version should never contribute to being
    // larger or smaller than a reference version's significant segments.
    for (int segment = referenceVersion.segmentCount(); segment < 3; ++segment) {
        auto largerInsignificantVersion = adjustedVersion(referenceVersion, segment, 1);
        QVERIFY(!(largerInsignificantVersion > referenceVersion));
        QVERIFY(!(largerInsignificantVersion < referenceVersion));
    }
}

QTEST_MAIN(tst_QOperatingSystemVersion)
#include "tst_qoperatingsystemversion.moc"
