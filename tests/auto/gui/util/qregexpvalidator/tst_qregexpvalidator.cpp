/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
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

public:
    tst_QRegExpValidator();
    virtual ~tst_QRegExpValidator();


    // I can think of no other way to do this for the moment
    enum State { Invalid=0, Intermediate=1, Acceptable=2 };
public slots:
    void init();
    void cleanup();
private slots:
    void validate_data();
    void validate();
};

tst_QRegExpValidator::tst_QRegExpValidator()
{
}

tst_QRegExpValidator::~tst_QRegExpValidator()
{

}

void tst_QRegExpValidator::init()
{
}

void tst_QRegExpValidator::cleanup()
{
}

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
