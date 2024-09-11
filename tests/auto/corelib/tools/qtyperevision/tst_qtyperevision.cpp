// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2014 Keith Gardner <kreios4004@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QtCore/qtyperevision.h>
#include <QtTest/private/qcomparisontesthelper_p.h>

using namespace Qt::StringLiterals;

class tst_QTypeRevision : public QObject
{
    Q_OBJECT

private slots:
    void qTypeRevision_data();
    void qTypeRevision();
    void qTypeRevisionTypes();
    void qTypeRevisionComparisonCompiles();
    void qTypeRevisionComparison_data();
    void qTypeRevisionComparison();
};

template<typename Integer>
void compileTestRevision()
{
    if constexpr (std::numeric_limits<Integer>::max() >= 0xffff) {
        const Integer value = 0x0510;
        const QTypeRevision r = QTypeRevision::fromEncodedVersion(value);

        QCOMPARE(r.majorVersion(), 5);
        QCOMPARE(r.minorVersion(), 16);
        QCOMPARE(r.toEncodedVersion<Integer>(), value);
    }

    const Integer major = 8;
    const Integer minor = 4;

    const QTypeRevision r2 = QTypeRevision::fromVersion(major, minor);
    QCOMPARE(r2.majorVersion(), 8);
    QCOMPARE(r2.minorVersion(), 4);

    const QTypeRevision r3 = QTypeRevision::fromMajorVersion(major);
    QCOMPARE(r3.majorVersion(), 8);
    QVERIFY(!r3.hasMinorVersion());

    const QTypeRevision r4 = QTypeRevision::fromMinorVersion(minor);
    QVERIFY(!r4.hasMajorVersion());
    QCOMPARE(r4.minorVersion(), 4);
}

void tst_QTypeRevision::qTypeRevision_data()
{
    QTest::addColumn<QTypeRevision>("revision");
    QTest::addColumn<bool>("valid");
    QTest::addColumn<int>("major");
    QTest::addColumn<int>("minor");

    QTest::addRow("Qt revision") << QTypeRevision::fromVersion(QT_VERSION_MAJOR, QT_VERSION_MINOR)
                                 << true << QT_VERSION_MAJOR << QT_VERSION_MINOR;
    QTest::addRow("invalid")     << QTypeRevision() << false << 0xff << 0xff;
    QTest::addRow("major")       << QTypeRevision::fromMajorVersion(6) << true << 6 << 0xff;
    QTest::addRow("minor")       << QTypeRevision::fromMinorVersion(15) << true << 0xff << 15;
    QTest::addRow("zero")        << QTypeRevision::fromVersion(0, 0) << true << 0 << 0;

    // We're intentionally not testing negative numbers.
    // There are asserts against negative numbers in QTypeRevision.
    // You must not pass them as major or minor versions, or values.
}

void tst_QTypeRevision::qTypeRevision()
{
    const QTypeRevision other = QTypeRevision::fromVersion(127, 128);

    QFETCH(QTypeRevision, revision);

    QFETCH(bool, valid);
    QFETCH(int, major);
    QFETCH(int, minor);

    QCOMPARE(revision.isValid(), valid);
    QCOMPARE(revision.majorVersion(), major);
    QCOMPARE(revision.minorVersion(), minor);

    QCOMPARE(revision.hasMajorVersion(), QTypeRevision::isValidSegment(major));
    QCOMPARE(revision.hasMinorVersion(), QTypeRevision::isValidSegment(minor));

    const QTypeRevision copy = QTypeRevision::fromEncodedVersion(revision.toEncodedVersion<int>());
    QCOMPARE(copy, revision);

    QVERIFY(revision != other);
    QVERIFY(copy != other);
}

void tst_QTypeRevision::qTypeRevisionTypes()
{
    compileTestRevision<quint8>();
    compileTestRevision<qint8>();
    compileTestRevision<quint16>();
    compileTestRevision<qint16>();
    compileTestRevision<quint32>();
    compileTestRevision<qint32>();
    compileTestRevision<quint64>();
    compileTestRevision<qint64>();
    compileTestRevision<long>();
    compileTestRevision<ulong>();
    compileTestRevision<char>();

    QVERIFY(!QTypeRevision::isValidSegment(0xff));
    QVERIFY(!QTypeRevision::isValidSegment(-1));

    const QTypeRevision maxRevision = QTypeRevision::fromVersion(254, 254);
    QVERIFY(maxRevision.hasMajorVersion());
    QVERIFY(maxRevision.hasMinorVersion());
}

void tst_QTypeRevision::qTypeRevisionComparisonCompiles()
{
    QTestPrivate::testAllComparisonOperatorsCompile<QTypeRevision>();
}

void tst_QTypeRevision::qTypeRevisionComparison_data()
{
    QTest::addColumn<QTypeRevision>("lhs");
    QTest::addColumn<QTypeRevision>("rhs");
    QTest::addColumn<Qt::strong_ordering>("expectedResult");

    static auto versionStr = [](QTypeRevision r) {
        QByteArray res = r.hasMajorVersion() ? QByteArray::number(r.majorVersion())
                                             : "x"_ba;
        res.append('.');
        res.append(r.hasMinorVersion() ? QByteArray::number(r.minorVersion())
                                       : "x"_ba);
        return res;
    };

    const QTypeRevision revisions[] = {
        QTypeRevision::zero(),
        QTypeRevision::fromMajorVersion(0),
        QTypeRevision::fromVersion(0, 1),
        QTypeRevision::fromVersion(0, 20),
        QTypeRevision::fromMinorVersion(0),
        QTypeRevision(),
        QTypeRevision::fromMinorVersion(1),
        QTypeRevision::fromMinorVersion(20),
        QTypeRevision::fromVersion(1, 0),
        QTypeRevision::fromMajorVersion(1),
        QTypeRevision::fromVersion(1, 1),
        QTypeRevision::fromVersion(1, 20),
        QTypeRevision::fromVersion(20, 0),
        QTypeRevision::fromMajorVersion(20),
        QTypeRevision::fromVersion(20, 1),
        QTypeRevision::fromVersion(20, 20),
    };

    const int length = sizeof(revisions) / sizeof(QTypeRevision);
    for (int i = 0; i < length; ++i) {
        for (int j = i; j < length; ++j) {
            const Qt::strong_ordering expectedRes = (i == j)
                    ? Qt::strong_ordering::equal
                    : (i < j) ? Qt::strong_ordering::less
                              : Qt::strong_ordering::greater;

            const auto lhs = revisions[i];
            const auto rhs = revisions[j];
            QTest::addRow("%s_vs_%s", versionStr(lhs).constData(), versionStr(rhs).constData())
                    << lhs << rhs << expectedRes;
        }
    }
}

void tst_QTypeRevision::qTypeRevisionComparison()
{
    QFETCH(const QTypeRevision, lhs);
    QFETCH(const QTypeRevision, rhs);
    QFETCH(const Qt::strong_ordering, expectedResult);

    QT_TEST_ALL_COMPARISON_OPS(lhs, rhs, expectedResult);
}

QTEST_APPLESS_MAIN(tst_QTypeRevision)

#include "tst_qtyperevision.moc"
