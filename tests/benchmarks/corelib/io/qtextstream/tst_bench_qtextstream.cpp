// Copyright (C) 2014 David Faure <david.faure@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QDebug>
#include <QIODevice>
#include <QString>
#include <QBuffer>
#include <qtest.h>

class tst_QTextStream : public QObject
{
    Q_OBJECT
private slots:
    void writeSingleChar_data();
    void writeSingleChar();

private:
};

enum Output { StringOutput, DeviceOutput };
Q_DECLARE_METATYPE(Output);

enum Input { CharStarInput, QStringInput, CharInput, QCharInput };
Q_DECLARE_METATYPE(Input);

void tst_QTextStream::writeSingleChar_data()
{
    QTest::addColumn<Output>("output");
    QTest::addColumn<Input>("input");

    QTest::newRow("string_charstar") << StringOutput << CharStarInput;
    QTest::newRow("string_string") << StringOutput << QStringInput;
    QTest::newRow("string_char") << StringOutput << CharInput;
    QTest::newRow("string_qchar") << StringOutput << QCharInput;
    QTest::newRow("device_charstar") << DeviceOutput << CharStarInput;
    QTest::newRow("device_string") << DeviceOutput << QStringInput;
    QTest::newRow("device_char") << DeviceOutput << CharInput;
    QTest::newRow("device_qchar") << DeviceOutput << QCharInput;
}

void tst_QTextStream::writeSingleChar()
{
    QFETCH(Output, output);
    QFETCH(Input, input);

    QString str;
    QBuffer buffer;
    QTextStream stream;
    if (output == StringOutput) {
        stream.setString(&str, QIODevice::WriteOnly);
    } else {
        QVERIFY(buffer.open(QIODevice::WriteOnly));
        stream.setDevice(&buffer);
    }
    // Test many different ways to write a single char into a QTextStream
    QString inputString = "h";
    const int amount = 100000;
    switch (input) {
    case CharStarInput:
        QBENCHMARK {
            for (qint64 i = 0; i < amount; ++i)
                stream << "h";
        }
        break;
    case QStringInput:
        QBENCHMARK {
            for (qint64 i = 0; i < amount; ++i)
                stream << inputString;
        }
        break;
    case CharInput:
        QBENCHMARK {
            for (qint64 i = 0; i < amount; ++i)
                stream << 'h';
        }
        break;
    case QCharInput:
        QBENCHMARK {
            for (qint64 i = 0; i < amount; ++i)
                stream << QChar('h');
        }
        break;
    }
    QString result;
    if (output == StringOutput)
        result = str;
    else
        result = QString(buffer.data());

    QCOMPARE(result.left(10), QString("hhhhhhhhhh"));
}

QTEST_MAIN(tst_QTextStream)

#include "tst_bench_qtextstream.moc"
