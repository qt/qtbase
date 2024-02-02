// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QtCore/QCoreApplication>
#include <QTest>
#include <QDebug>

class tst_globaldata: public QObject
{
    Q_OBJECT
public slots:
    void init();
    void initTestCase();
    void initTestCase_data();

    void cleanup();
    void cleanupTestCase();

private slots:
    void testGlobal_data();
    void testGlobal();

    void skip_data();
    void skip();

    void skipLocal_data() { testGlobal_data(); }
    void skipLocal();

    void skipSingle_data() { testGlobal_data(); }
    void skipSingle();
};


void tst_globaldata::initTestCase()
{
    qDebug() << "initTestCase"
             << (QTest::currentTestFunction() ? QTest::currentTestFunction() : "(null)")
             << (QTest::currentDataTag() ? QTest::currentDataTag() : "(null)");
}

void tst_globaldata::initTestCase_data()
{
    // QFETCH_GLOBAL shall iterate these, for every test:
    QTest::addColumn<bool>("global");
    QTest::newRow("global=false") << false;
    QTest::newRow("global=true") << true;
}

void tst_globaldata::cleanupTestCase()
{
    qDebug() << "cleanupTestCase"
             << (QTest::currentTestFunction() ? QTest::currentTestFunction() : "(null)")
             << (QTest::currentDataTag() ? QTest::currentDataTag() : "(null)");
}

void tst_globaldata::init()
{
    qDebug() << "init"
             << (QTest::currentTestFunction() ? QTest::currentTestFunction() : "(null)")
             << (QTest::currentDataTag() ? QTest::currentDataTag() : "(null)");
}

void tst_globaldata::cleanup()
{
    qDebug() << "cleanup"
             << (QTest::currentTestFunction() ? QTest::currentTestFunction() : "(null)")
             << (QTest::currentDataTag() ? QTest::currentDataTag() : "(null)");
}

void tst_globaldata::testGlobal_data()
{
    QTest::addColumn<bool>("local");
    QTest::newRow("local=false") << false;
    QTest::newRow("local=true") << true;
}

void tst_globaldata::testGlobal()
{
    QFETCH_GLOBAL(bool, global);
    qDebug() << "global:" << global;
    QFETCH(bool, local);
    qDebug() << "local:" << local;
}

void tst_globaldata::skip_data()
{
    testGlobal_data();
    QSKIP("skipping");
}

void tst_globaldata::skip()
{
    // A skip in _data() causes the whole test to be skipped, for all global rows.
    QVERIFY(!"This line should never be reached.");
}

void tst_globaldata::skipSingle()
{
    QFETCH_GLOBAL(bool, global);
    QFETCH(bool, local);

    // A skip in the last run of one global row used to suppress the test in the
    // next global row (where a skip in an earlier run of the first row did not).
    if (global ^ local)
        QSKIP("Skipping");
    qDebug() << "global:" << global << "local:" << local;
    QCOMPARE(global, local);
}

void tst_globaldata::skipLocal()
{
    QSKIP("skipping");
}

QTEST_MAIN(tst_globaldata)

#include "tst_globaldata.moc"
