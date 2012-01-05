/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtCore>
#include <QtTest/QtTest>

/*!
 \internal
 \since 4.4
 \brief Tests that reporting of tables are done in a certain way.
 */
class tst_DataTable: public QObject
{
    Q_OBJECT

private slots:
    void singleTestFunction1() const;
    void singleTestFunction2() const;

    void fiveTablePasses() const;
    void fiveTablePasses_data() const;
    void fiveTableFailures() const;
    void fiveTableFailures_data() const;

    void startsWithFailure() const;
    void startsWithFailure_data() const;

    void endsWithFailure() const;
    void endsWithFailure_data() const;

    void failureInMiddle() const;
    void failureInMiddle_data() const;

    void fiveIsolatedFailures() const;
    void fiveIsolatedFailures_data() const;
};

void tst_DataTable::singleTestFunction1() const
{
    /* Do nothing, just pass. */
}

void tst_DataTable::singleTestFunction2() const
{
    /* Do nothing, just pass. */
}

void tst_DataTable::fiveTableFailures() const
{
    QFETCH(bool, test);

    QVERIFY(test);
}

void tst_DataTable::fiveTableFailures_data() const
{
    QTest::addColumn<bool>("test");

    /* Unconditionally fail. */
    QTest::newRow("fiveTableFailures_data 1") << false;
    QTest::newRow("fiveTableFailures_data 2") << false;
    QTest::newRow("fiveTableFailures_data 3") << false;
    QTest::newRow("fiveTableFailures_data 4") << false;
    QTest::newRow("fiveTableFailures_data 5") << false;
}

void tst_DataTable::startsWithFailure() const
{
    fiveTableFailures();
}

void tst_DataTable::fiveTablePasses() const
{
    fiveTableFailures();
}

void tst_DataTable::fiveTablePasses_data() const
{
    QTest::addColumn<bool>("test");

    QTest::newRow("fiveTablePasses_data 1") << true;
    QTest::newRow("fiveTablePasses_data 2") << true;
    QTest::newRow("fiveTablePasses_data 3") << true;
    QTest::newRow("fiveTablePasses_data 4") << true;
    QTest::newRow("fiveTablePasses_data 5") << true;
}

void tst_DataTable::startsWithFailure_data() const
{
    QTest::addColumn<bool>("test");

    QTest::newRow("startsWithFailure_data 1") << false;
    QTest::newRow("startsWithFailure_data 2") << true;
    QTest::newRow("startsWithFailure_data 3") << true;
    QTest::newRow("startsWithFailure_data 4") << true;
    QTest::newRow("startsWithFailure_data 5") << true;
}

void tst_DataTable::endsWithFailure() const
{
    fiveTableFailures();
}

void tst_DataTable::endsWithFailure_data() const
{
    QTest::addColumn<bool>("test");

    QTest::newRow("endsWithFailure 1") << true;
    QTest::newRow("endsWithFailure 2") << true;
    QTest::newRow("endsWithFailure 3") << true;
    QTest::newRow("endsWithFailure 4") << true;
    QTest::newRow("endsWithFailure 5") << false;
}

void tst_DataTable::failureInMiddle() const
{
    fiveTableFailures();
}

void tst_DataTable::failureInMiddle_data() const
{
    QTest::addColumn<bool>("test");

    QTest::newRow("failureInMiddle_data 1") << true;
    QTest::newRow("failureInMiddle_data 2") << true;
    QTest::newRow("failureInMiddle_data 3") << false;
    QTest::newRow("failureInMiddle_data 4") << true;
    QTest::newRow("failureInMiddle_data 5") << true;
}

void tst_DataTable::fiveIsolatedFailures() const
{
    QFETCH(bool, test);
    QVERIFY(!test);
}

void tst_DataTable::fiveIsolatedFailures_data() const
{
    QTest::addColumn<bool>("test");

    QTest::newRow("fiveIsolatedFailures_data 1") << true;
    QTest::newRow("fiveIsolatedFailures_data 2") << true;
    QTest::newRow("fiveIsolatedFailures_data 3") << true;
    QTest::newRow("fiveIsolatedFailures_data 4") << true;
    QTest::newRow("fiveIsolatedFailures_data 5") << true;
}

QTEST_MAIN(tst_DataTable)

#include "tst_datatable.moc"
