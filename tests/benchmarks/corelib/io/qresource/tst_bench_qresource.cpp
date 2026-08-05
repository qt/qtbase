// Copyright (C) 2026 Aurélien Brooke <aurelien@bahiasoft.fr>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QResource>
#include <QTest>

class tst_QResource : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void lookup_data();
    void lookup();

private:
    QString resourceFile;
};

void tst_QResource::initTestCase()
{
    resourceFile = QFINDTESTDATA("resource.rcc");
    QVERIFY(!resourceFile.isEmpty());
    QVERIFY(QResource::registerResource(resourceFile, "/mappedroot"));
}

void tst_QResource::cleanupTestCase()
{
    QVERIFY(QResource::unregisterResource(resourceFile, "/mappedroot"));
}

void tst_QResource::lookup_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<bool>("expectedValid");

    QTest::newRow("valid") << QStringLiteral(":/mappedroot/resource.txt") << true;
    QTest::newRow("invalid") << QStringLiteral(":/mappedroot/missing.txt") << false;
}

void tst_QResource::lookup()
{
    QFETCH(QString, path);
    QFETCH(bool, expectedValid);

    bool isValid = false;
    QBENCHMARK {
        isValid = QResource(path).isValid();
    }
    QCOMPARE(isValid, expectedValid);
}

QTEST_MAIN(tst_QResource)

#include "tst_bench_qresource.moc"
