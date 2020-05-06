/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <QTest>
#include <QSqlDatabase>

// dummy
class TestBenchmark : public QObject
{
    Q_OBJECT
private slots:
    void simple();
};

// dummy
class MyTestClass : public QObject
{
    public:
        void cleanup();
        void addSingleStringRows();
        void addMultStringRows();
        void addDataRow();
};
// dummy
void closeAllDatabases()
{
};

class TestQString : public QObject
{
    public:
        void toInt_data();
        void toInt();
        void toUpper();
        void Compare();
};

void wrapInFunction()
{
//! [1]
QVERIFY2(QFileInfo("file.txt").exists(), "file.txt does not exist.");
//! [1]

//! [2]
QCOMPARE(QString("hello").toUpper(), QString("HELLO"));
//! [2]
}

//! [3]
void TestQString::toInt_data()
{
    QTest::addColumn<QString>("aString");
    QTest::addColumn<int>("expected");

    QTest::newRow("positive value") << "42" << 42;
    QTest::newRow("negative value") << "-42" << -42;
    QTest::newRow("zero") << "0" << 0;
}
//! [3]

//! [4]
void TestQString::toInt()
{
     QFETCH(QString, aString);
     QFETCH(int, expected);

     QCOMPARE(aString.toInt(), expected);
}
//! [4]

void testInt()
{
// dummy
int i = 0, j = 0;
//! [5]
if (sizeof(int) != 4)
    QFAIL("This test has not been ported to this platform yet.");
//! [5]

//! [6]
QFETCH(QString, myString);
QCOMPARE(QString("hello").toUpper(), myString);
//! [6]

//! [7]
QTEST(QString("hello").toUpper(), "myString");
//! [7]

//! [8]
if (!QSqlDatabase::drivers().contains("SQLITE"))
    QSKIP("This test requires the SQLITE database driver");
//! [8]

//! [9]
QEXPECT_FAIL("", "Will fix in the next release", Continue);
QCOMPARE(i, 42);
QCOMPARE(j, 43);
//! [9]

//! [10]
QEXPECT_FAIL("data27", "Oh my, this is soooo broken", Abort);
QCOMPARE(i, 42);
//! [10]
}

//! [11]
QTEST_MAIN(TestQString)
//! [11]

void testObject()
{
class MyTestObject: public QObject
{
    public:
        void toString() {}
};
//! [18]
MyTestObject test1;
QTest::qExec(&test1);
//! [18]
}

void tstQDir()
{
//! [19]
QDir dir;
QTest::ignoreMessage(QtWarningMsg, "QDir::mkdir: Empty or null file name(s)");
dir.mkdir("");
//! [19]
}

//! [20]
void MyTestClass::addSingleStringRows()
{
    QTest::addColumn<QString>("aString");
    QTest::newRow("just hello") << QString("hello");
    QTest::newRow("a null string") << QString();
}
//! [20]

void MyTestClass::addMultStringRows()
{
//! [addRow]
    QTest::addColumn<int>("input");
    QTest::addColumn<QString>("output");
    QTest::addRow("%d", 0) << 0 << QString("0");
    QTest::addRow("%d", 1) << 1 << QString("1");
//! [addRow]
}

void MyTestClass::addDataRow()
{
//! [21]
    QTest::addColumn<int>("intval");
    QTest::addColumn<QString>("str");
    QTest::addColumn<double>("dbl");
    QTest::newRow("row1") << 1 << "hello" << 1.5;
//! [21]
}

//! [22]
void MyTestClass::cleanup()
{
    if (qstrcmp(QTest::currentTestFunction(), "myDatabaseTest") == 0) {
        // clean up all database connections
        closeAllDatabases();
    }
}
//! [22]

void mySleep()
{
//! [23]
QTest::qSleep(250);
//! [23]
}

//! [27]
void TestBenchmark::simple()
{
    QString str1 = QLatin1String("This is a test string");
    QString str2 = QLatin1String("This is a test string");
    QCOMPARE(str1.localeAwareCompare(str2), 0);
    QBENCHMARK {
        str1.localeAwareCompare(str2);
    }
}
//! [27]

void verifyString()
{
QFile file;
//! [32]
bool opened = file.open(QIODevice::WriteOnly);
QVERIFY(opened);
//! [32]
//! [33]
QVERIFY2(file.open(QIODevice::WriteOnly),
         qPrintable(QString("open %1: %2")
                    .arg(file.fileName()).arg(file.errorString())));
//! [33]
}
