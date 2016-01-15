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


#include <QtTest/QtTest>
#include <qregexp.h>


#include <qvalidator.h>

class tst_QRegExpValidator : public QObject
{
    Q_OBJECT

private slots:
    void validate_data();
    void validate();
};

void tst_QRegExpValidator::validate_data()
{

    QTest::addColumn<QString>("rx");
    QTest::addColumn<QString>("value");
    QTest::addColumn<int>("state");

    QTest::newRow( "data0" ) << QString("[1-9]\\d{0,3}") << QString("0") << 0;
    QTest::newRow( "data1" ) << QString("[1-9]\\d{0,3}") << QString("12345") << 0;
    QTest::newRow( "data2" ) << QString("[1-9]\\d{0,3}") << QString("1") << 2;

    QTest::newRow( "data3" ) << QString("\\S+") << QString("myfile.txt") << 2;
    QTest::newRow( "data4" ) << QString("\\S+") << QString("my file.txt") << 0;

    QTest::newRow( "data5" ) << QString("[A-C]\\d{5}[W-Z]") << QString("a12345Z") << 0;
    QTest::newRow( "data6" ) << QString("[A-C]\\d{5}[W-Z]") << QString("A12345Z") << 2;
    QTest::newRow( "data7" ) << QString("[A-C]\\d{5}[W-Z]") << QString("B12") << 1;

    QTest::newRow( "data8" ) << QString("read\\S?me(\\.(txt|asc|1st))?") << QString("readme") << 2;
    QTest::newRow( "data9" ) << QString("read\\S?me(\\.(txt|asc|1st))?") << QString("read me.txt") << 0;
    QTest::newRow( "data10" ) << QString("read\\S?me(\\.(txt|asc|1st))?") << QString("readm") << 1;
}

void tst_QRegExpValidator::validate()
{
    QFETCH( QString, rx );
    QFETCH( QString, value );
    QFETCH( int, state );

    QRegExpValidator rv( 0 );
    QSignalSpy spy(&rv, SIGNAL(regExpChanged(QRegExp)));
    QSignalSpy changedSpy(&rv, SIGNAL(changed()));

    rv.setRegExp( QRegExp( rx ) );
    int pos = -1;

    QCOMPARE( (int)rv.validate( value, pos ), state );

    if (state == QValidator::Invalid)
        QCOMPARE(pos, value.length());
    else
        QCOMPARE(pos, -1); // untouched on Acceptable or Intermediate

    QCOMPARE(spy.count(), 1);
    QCOMPARE(changedSpy.count(), 1);
}

QTEST_APPLESS_MAIN(tst_QRegExpValidator)
#include "tst_qregexpvalidator.moc"
