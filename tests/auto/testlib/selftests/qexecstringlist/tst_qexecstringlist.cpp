// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QtCore/QCoreApplication>
#include <QTest>

class tst_QExecStringList: public QObject
{
    Q_OBJECT

private slots:
    void testA() const;
    void testB() const;
    void testB_data() const;
    void testC() const;
};

void tst_QExecStringList::testA() const
{
}

void tst_QExecStringList::testB() const
{
    QFETCH(bool, dummy);
    Q_UNUSED(dummy);
}

void tst_QExecStringList::testB_data() const
{
    QTest::addColumn<bool>("dummy");

    QTest::newRow("Data1") << false;
    QTest::newRow("Data2") << false;
    QTest::newRow("Data3") << false;
}

void tst_QExecStringList::testC() const
{
}

int main(int argc,char *argv[])
{
    QCoreApplication app(argc, argv);

    tst_QExecStringList test;

    QTest::qExec(&test, app.arguments());
    QTest::qExec(&test, QStringList("appName"));
    QTest::qExec(&test, QStringList("appName") << "testA");
    QTest::qExec(&test, QStringList("appName") << "testB");
    QTest::qExec(&test, QStringList("appName") << "testB:Data2");
    QTest::qExec(&test, QStringList("appName") << "testC");

    return 0;
}

#include "tst_qexecstringlist.moc"
