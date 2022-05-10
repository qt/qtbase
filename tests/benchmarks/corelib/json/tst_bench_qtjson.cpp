// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <QVariantMap>
#include <qjsondocument.h>
#include <qjsonobject.h>

class BenchmarkQtJson: public QObject
{
    Q_OBJECT
public:
    BenchmarkQtJson(QObject *parent = nullptr);

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void parseNumbers();
    void parseJson();
    void parseJsonToVariant();

    void jsonObjectInsert();
    void variantMapInsert();
};

BenchmarkQtJson::BenchmarkQtJson(QObject *parent) : QObject(parent)
{

}

void BenchmarkQtJson::initTestCase()
{

}

void BenchmarkQtJson::cleanupTestCase()
{

}

void BenchmarkQtJson::init()
{

}

void BenchmarkQtJson::cleanup()
{

}

void BenchmarkQtJson::parseNumbers()
{
    QString testFile = QFINDTESTDATA("numbers.json");
    QVERIFY2(!testFile.isEmpty(), "cannot find test file numbers.json!");
    QFile file(testFile);
    file.open(QFile::ReadOnly);
    QByteArray testJson = file.readAll();

    QBENCHMARK {
        QJsonDocument doc = QJsonDocument::fromJson(testJson);
        QJsonObject object = doc.object();
    }
}

void BenchmarkQtJson::parseJson()
{
    QString testFile = QFINDTESTDATA("test.json");
    QVERIFY2(!testFile.isEmpty(), "cannot find test file test.json!");
    QFile file(testFile);
    file.open(QFile::ReadOnly);
    QByteArray testJson = file.readAll();

    QBENCHMARK {
        QJsonDocument doc = QJsonDocument::fromJson(testJson);
        QJsonObject object = doc.object();
    }
}

void BenchmarkQtJson::parseJsonToVariant()
{
    QString testFile = QFINDTESTDATA("test.json");
    QVERIFY2(!testFile.isEmpty(), "cannot find test file test.json!");
    QFile file(testFile);
    file.open(QFile::ReadOnly);
    QByteArray testJson = file.readAll();

    QBENCHMARK {
        QJsonDocument doc = QJsonDocument::fromJson(testJson);
        QVariant v = doc.toVariant();
    }
}

void BenchmarkQtJson::jsonObjectInsert()
{
    QJsonObject object;
    QString test(QStringLiteral("testString"));
    QJsonValue value(1.5);

    QBENCHMARK {
        for (int i = 0; i < 1000; i++)
            object.insert("testkey_" + QString::number(i), value);
    }
}

void BenchmarkQtJson::variantMapInsert()
{
    QVariantMap object;
    QString test(QStringLiteral("testString"));
    QVariant variantValue(1.5);

    QBENCHMARK {
        for (int i = 0; i < 1000; i++)
            object.insert("testkey_" + QString::number(i), variantValue);
    }
}

QTEST_MAIN(BenchmarkQtJson)
#include "tst_bench_qtjson.moc"

