/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include <QtTest/QtTest>
#include <private/qdatetimeparser_p.h>

class tst_QDateTimeParser : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void intermediateYear_data();
    void intermediateYear();
};

void tst_QDateTimeParser::intermediateYear_data()
{
    QTest::addColumn<QString>("format");
    QTest::addColumn<QString>("input");
    QTest::addColumn<QDate>("expected");

    QTest::newRow("short-year-begin")
        << "yyyy_MM_dd" << "200_12_15" << QDate(200, 12, 15);
    QTest::newRow("short-year-mid")
        << "MM_yyyy_dd" << "12_200_15" << QDate(200, 12, 15);
    QTest::newRow("short-year-end")
        << "MM_dd_yyyy" << "12_15_200" << QDate(200, 12, 15);
}

void tst_QDateTimeParser::intermediateYear()
{
    QFETCH(QString, format);
    QFETCH(QString, input);
    QFETCH(QDate, expected);

    QDateTimeParser testParser(QMetaType::QDateTime, QDateTimeParser::DateTimeEdit);

    QVERIFY(testParser.parseFormat(format));

    QDateTime val(QDate(1900, 1, 1).startOfDay());
    const QDateTimeParser::StateNode tmp = testParser.parse(input, -1, val, false);
    QCOMPARE(tmp.state, QDateTimeParser::Intermediate);
    QCOMPARE(tmp.value, expected.startOfDay());
}

QTEST_APPLESS_MAIN(tst_QDateTimeParser)

#include "tst_qdatetimeparser.moc"
