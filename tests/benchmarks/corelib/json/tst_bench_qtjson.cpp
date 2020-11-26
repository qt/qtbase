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

#include <QTest>
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

