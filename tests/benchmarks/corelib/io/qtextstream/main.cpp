/****************************************************************************
**
** Copyright (C) 2014 David Faure <david.faure@kdab.com>
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

#include <QDebug>
#include <QIODevice>
#include <QString>
#include <QBuffer>
#include <qtest.h>

class tst_qtextstream : public QObject
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

void tst_qtextstream::writeSingleChar_data()
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

void tst_qtextstream::writeSingleChar()
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

QTEST_MAIN(tst_qtextstream)

#include "main.moc"
