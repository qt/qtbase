/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
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
