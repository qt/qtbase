// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qsurfaceformat.h>

#include <QTest>

class tst_QSurfaceFormat: public QObject
{
    Q_OBJECT

private slots:
    void versionCheck_data();
    void versionCheck();
};

void tst_QSurfaceFormat::versionCheck_data()
{
    QTest::addColumn<int>("formatMajor");
    QTest::addColumn<int>("formatMinor");
    QTest::addColumn<int>("compareMajor");
    QTest::addColumn<int>("compareMinor");
    QTest::addColumn<bool>("expected");

    QTest::newRow("lower major, lower minor")
        << 3 << 2 << 2 << 1 << true;
    QTest::newRow("lower major, same minor")
        << 3 << 2 << 2 << 2 << true;
    QTest::newRow("lower major, greater minor")
        << 3 << 2 << 2 << 3 << true;
    QTest::newRow("same major, lower minor")
        << 3 << 2 << 3 << 1 << true;
    QTest::newRow("same major, same minor")
        << 3 << 2 << 3 << 2 << true;
    QTest::newRow("same major, greater minor")
        << 3 << 2 << 3 << 3 << false;
    QTest::newRow("greater major, lower minor")
        << 3 << 2 << 4 << 1 << false;
    QTest::newRow("greater major, same minor")
        << 3 << 2 << 4 << 2 << false;
    QTest::newRow("greater major, greater minor")
        << 3 << 2 << 4 << 3 << false;
}

void tst_QSurfaceFormat::versionCheck()
{
    QFETCH( int, formatMajor );
    QFETCH( int, formatMinor );
    QFETCH( int, compareMajor );
    QFETCH( int, compareMinor );
    QFETCH( bool, expected );

    QSurfaceFormat format;
    format.setMinorVersion(formatMinor);
    format.setMajorVersion(formatMajor);

    QCOMPARE(format.version() >= qMakePair(compareMajor, compareMinor), expected);

    format.setVersion(formatMajor, formatMinor);
    QCOMPARE(format.version() >= qMakePair(compareMajor, compareMinor), expected);
}

#include <tst_qsurfaceformat.moc>
QTEST_MAIN(tst_QSurfaceFormat);
