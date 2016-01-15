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


#include <QtCore/QCoreApplication>
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

    void fiveTablePasses() const;
    void fiveTablePasses_data() const;
};

void tst_DataTable::fiveTablePasses() const
{
    QFETCH(bool, test);

    QVERIFY(test);
}

void tst_DataTable::fiveTablePasses_data() const
{
    QTest::addColumn<bool>("test");

    QTest::newRow("fiveTablePasses_data1") << true;
    QTest::newRow("fiveTablePasses_data2") << true;
    QTest::newRow("fiveTablePasses_data3") << true;
    QTest::newRow("fiveTablePasses_data4") << true;
    QTest::newRow("fiveTablePasses_data5") << true;
}

QTEST_MAIN(tst_DataTable)

#include "tst_commandlinedata.moc"
