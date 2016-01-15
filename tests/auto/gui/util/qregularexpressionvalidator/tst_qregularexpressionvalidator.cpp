/****************************************************************************
**
** Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
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

#include <QtGui/QRegularExpressionValidator>
#include <QtTest/QtTest>

class tst_QRegularExpressionValidator : public QObject
{
    Q_OBJECT

private slots:
    void validate_data();
    void validate();
};

Q_DECLARE_METATYPE(QValidator::State)

void tst_QRegularExpressionValidator::validate_data()
{
    QTest::addColumn<QRegularExpression>("re");
    QTest::addColumn<QString>("value");
    QTest::addColumn<QValidator::State>("state");

    QTest::newRow("data0") << QRegularExpression("[1-9]\\d{0,3}") << QString("0") << QValidator::Invalid;
    QTest::newRow("data1") << QRegularExpression("[1-9]\\d{0,3}") << QString("12345") << QValidator::Invalid;
    QTest::newRow("data2") << QRegularExpression("[1-9]\\d{0,3}") << QString("1") << QValidator::Acceptable;

    QTest::newRow("data3") << QRegularExpression("\\S+") << QString("myfile.txt") << QValidator::Acceptable;
    QTest::newRow("data4") << QRegularExpression("\\S+") << QString("my file.txt") << QValidator::Invalid;

    QTest::newRow("data5") << QRegularExpression("[A-C]\\d{5}[W-Z]") << QString("a12345Z") << QValidator::Invalid;
    QTest::newRow("data6") << QRegularExpression("[A-C]\\d{5}[W-Z]") << QString("A12345Z") << QValidator::Acceptable;
    QTest::newRow("data7") << QRegularExpression("[A-C]\\d{5}[W-Z]") << QString("B12") << QValidator::Intermediate;

    QTest::newRow("data8") << QRegularExpression("read\\S?me(\\.(txt|asc|1st))?") << QString("readme") << QValidator::Acceptable;
    QTest::newRow("data9") << QRegularExpression("read\\S?me(\\.(txt|asc|1st))?") << QString("read me.txt") << QValidator::Invalid;
    QTest::newRow("data10") << QRegularExpression("read\\S?me(\\.(txt|asc|1st))?") << QString("readm") << QValidator::Intermediate;

    QTest::newRow("data11") << QRegularExpression("read\\S?me(\\.(txt|asc|1st))?") << QString("read me.txt") << QValidator::Invalid;
    QTest::newRow("data12") << QRegularExpression("read\\S?me(\\.(txt|asc|1st))?") << QString("readm") << QValidator::Intermediate;

    QTest::newRow("data13") << QRegularExpression("\\w\\d\\d") << QString("A57") << QValidator::Acceptable;
    QTest::newRow("data14") << QRegularExpression("\\w\\d\\d") << QString("E5") << QValidator::Intermediate;
    QTest::newRow("data15") << QRegularExpression("\\w\\d\\d") << QString("+9") << QValidator::Invalid;

    QTest::newRow("emptystr1") << QRegularExpression("[T][e][s][t]") << QString("") << QValidator::Intermediate;
    QTest::newRow("emptystr2") << QRegularExpression("[T][e][s][t]") << QString() << QValidator::Intermediate;

    QTest::newRow("empty01") << QRegularExpression() << QString() << QValidator::Acceptable;
    QTest::newRow("empty02") << QRegularExpression() << QString("test") << QValidator::Acceptable;
}

void tst_QRegularExpressionValidator::validate()
{
    QFETCH(QRegularExpression, re);
    QFETCH(QString, value);

    QRegularExpressionValidator rv;

    // setting the same regexp won't emit signals
    const int signalCount = (rv.regularExpression() == re) ? 0 : 1;

    QSignalSpy spy(&rv, SIGNAL(regularExpressionChanged(QRegularExpression)));
    QSignalSpy changedSpy(&rv, SIGNAL(changed()));

    rv.setRegularExpression(re);
    QCOMPARE(rv.regularExpression(), re);

    int pos = -1;
    QValidator::State result = rv.validate(value, pos);

    QTEST(result, "state");
    if (result == QValidator::Invalid)
        QCOMPARE(pos, value.length());
    else
        QCOMPARE(pos, -1); // ensure pos is not modified if validate returned Acceptable or Intermediate

    QCOMPARE(spy.count(), signalCount);
    QCOMPARE(changedSpy.count(), signalCount);
}

QTEST_GUILESS_MAIN(tst_QRegularExpressionValidator)

#include "tst_qregularexpressionvalidator.moc"
