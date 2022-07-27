// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QTest>
#include <QSqlDatabase>
#include <QFontDatabase>

#include <initializer_list>

using namespace Qt::StringLiterals;

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

    QTest::newRow("positive+value") << "42" << 42;
    QTest::newRow("negative-value") << "-42" << -42;
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
    class MyTestObject : public QObject
    {
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
    QTest::newRow("just.hello") << QString("hello");
    QTest::newRow("a.null.string") << QString();
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
    QString str1 = u"This is a test string"_s;
    QString str2 = u"This is a test string"_s;
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

void compareListToArray()
{
//! [34]
    const int expected[] = {8, 10, 12, 16, 20, 24};
    QCOMPARE(QFontDatabase::standardSizes(), expected);
//! [34]
}

void compareListToInitializerList()
{
//! [35]
 #define ARG(...) __VA_ARGS__
     QCOMPARE(QFontDatabase::standardSizes(), ARG({8, 10, 12, 16, 20, 24}));
 #undef ARG
//! [35]
}
