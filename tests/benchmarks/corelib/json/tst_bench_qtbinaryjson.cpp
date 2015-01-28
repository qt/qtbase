/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest>
#include <qjsondocument.h>
#include <qjsonobject.h>

class BenchmarkQtBinaryJson: public QObject
{
    Q_OBJECT
public:
    BenchmarkQtBinaryJson(QObject *parent = 0);

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void parseNumbers();
    void parseJson();
    void parseJsonToVariant();

    void toByteArray();
    void fromByteArray();

    void jsonObjectInsert();
    void variantMapInsert();
};

BenchmarkQtBinaryJson::BenchmarkQtBinaryJson(QObject *parent) : QObject(parent)
{

}

void BenchmarkQtBinaryJson::initTestCase()
{

}

void BenchmarkQtBinaryJson::cleanupTestCase()
{

}

void BenchmarkQtBinaryJson::init()
{

}

void BenchmarkQtBinaryJson::cleanup()
{

}

void BenchmarkQtBinaryJson::parseNumbers()
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

void BenchmarkQtBinaryJson::parseJson()
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

void BenchmarkQtBinaryJson::parseJsonToVariant()
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

void BenchmarkQtBinaryJson::toByteArray()
{
    // Example: send information over a datastream to another process
    // Measure performance of creating and processing data into bytearray
    QBENCHMARK {
        QVariantMap message;
        message.insert("command", 1);
        message.insert("key", "some information");
        message.insert("env", "some environment variables");
        QByteArray msg = QJsonDocument(QJsonObject::fromVariantMap(message)).toBinaryData();
    }
}

void BenchmarkQtBinaryJson::fromByteArray()
{
    // Example: receive information over a datastream from another process
    // Measure performance of converting content back to QVariantMap
    // We need to recreate the bytearray but here we only want to measure the latter
    QVariantMap message;
    message.insert("command", 1);
    message.insert("key", "some information");
    message.insert("env", "some environment variables");
    QByteArray msg = QJsonDocument(QJsonObject::fromVariantMap(message)).toBinaryData();

    QBENCHMARK {
        QVariantMap message;
        message = QJsonDocument::fromBinaryData(msg, QJsonDocument::Validate).object().toVariantMap();
    }
}

void BenchmarkQtBinaryJson::jsonObjectInsert()
{
    QJsonObject object;
    QString test(QStringLiteral("testString"));
    QJsonValue value(1.5);

    QBENCHMARK {
        for (int i = 0; i < 1000; i++)
            object.insert("testkey_" + QString::number(i), value);
    }
}

void BenchmarkQtBinaryJson::variantMapInsert()
{
    QVariantMap object;
    QString test(QStringLiteral("testString"));
    QVariant variantValue(1.5);

    QBENCHMARK {
        for (int i = 0; i < 1000; i++)
            object.insert("testkey_" + QString::number(i), variantValue);
    }
}

QTEST_MAIN(BenchmarkQtBinaryJson)
#include "tst_bench_qtbinaryjson.moc"

