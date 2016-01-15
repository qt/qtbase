/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qsurfaceformat.h>

#include <QtTest/QtTest>

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
