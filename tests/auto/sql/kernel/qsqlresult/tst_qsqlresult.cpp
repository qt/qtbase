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
#include <QtSql/QtSql>

#include "testsqldriver.h"

class tst_QSqlResult : public QObject
{
    Q_OBJECT

public:
    tst_QSqlResult();

private slots:
    void positionalToNamedBinding();
    void parseOfBoundValues();

};

tst_QSqlResult::tst_QSqlResult()
{
}

void tst_QSqlResult::positionalToNamedBinding()
{
    TestSqlDriver testDriver;
    TestSqlDriverResult result(&testDriver);
    QString query("INSERT INTO MYTABLE (ID, NAME, BIRTH) VALUES(?, ?, ?)");
    QVERIFY(result.savePrepare(query));
    QCOMPARE(result.boundValues().count(), 3);
}

void tst_QSqlResult::parseOfBoundValues()
{
    TestSqlDriver testDriver;
    TestSqlDriverResult result(&testDriver);
    QVERIFY(result.savePrepare("SELECT :1 AS \":2\""));
    QCOMPARE(result.boundValues().count(), 1);
    QVERIFY(result.savePrepare("SELECT :1 AS ':2'"));
    QCOMPARE(result.boundValues().count(), 1);
    QVERIFY(result.savePrepare("SELECT :1 AS [:2]"));
    QCOMPARE(result.boundValues().count(), 1);
    QVERIFY(result.savePrepare("SELECT :1 AS [:2]]]"));
    QCOMPARE(result.boundValues().count(), 1);
    QVERIFY(result.savePrepare("SELECT :1 AS [:2]]]]]"));
    QCOMPARE(result.boundValues().count(), 1);

    QVERIFY(result.savePrepare("SELECT ? AS \"?\""));
    QCOMPARE(result.boundValues().count(), 1);
    QVERIFY(result.savePrepare("SELECT ? AS '?'"));
    QCOMPARE(result.boundValues().count(), 1);
    QVERIFY(result.savePrepare("SELECT ? AS [?]"));
    QCOMPARE(result.boundValues().count(), 1);

    QVERIFY(result.savePrepare("SELECT ? AS \"'?\""));
    QCOMPARE(result.boundValues().count(), 1);
    QVERIFY(result.savePrepare("SELECT ? AS '?\"'"));
    QCOMPARE(result.boundValues().count(), 1);
    QVERIFY(result.savePrepare("SELECT ? AS '?''?'"));
    QCOMPARE(result.boundValues().count(), 1);
    QVERIFY(result.savePrepare("SELECT ? AS [\"?']"));
    QCOMPARE(result.boundValues().count(), 1);
}

QTEST_MAIN( tst_QSqlResult )
#include "tst_qsqlresult.moc"
