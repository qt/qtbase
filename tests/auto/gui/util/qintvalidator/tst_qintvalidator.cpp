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


#include <QtTest/QtTest>
#include <qvalidator.h>

class tst_QIntValidator : public QObject
{
    Q_OBJECT
private slots:
    void validate_data();
    void validate();
    void validateArabic();
    void validateFrench();
    void notifySignals();
};

Q_DECLARE_METATYPE(QValidator::State);
#define INV QValidator::Invalid
#define INT QValidator::Intermediate
#define ACC QValidator::Acceptable

void tst_QIntValidator::validate_data()
{
    QTest::addColumn<int>("minimum");
    QTest::addColumn<int>("maximum");
    QTest::addColumn<QString>("value");
    QTest::addColumn<QValidator::State>("state");

    QTest::newRow("data0") << 0 << 100 << QString("50") << ACC;
    QTest::newRow("data1") << 0 << 100 << QString("500") << INV;
    QTest::newRow("data1a") << 0 << 100 << QString("5000") << INV;
    QTest::newRow("data1b") << -100 << 0 << QString("50") << INT;
    QTest::newRow("data1c") << -100 << 0 << QString("500") << INV;
    QTest::newRow("data1d") << -100 << 0 << QString("5000") << INV;
    QTest::newRow("data2") << 0 << 100 << QString("-35") << INV;
    QTest::newRow("data3") << 0 << 100 << QString("a") << INV;
    QTest::newRow("data4") << 0 << 100 << QString("-") << INV;
    QTest::newRow("data5") << 0 << 100 << QString("100") << ACC;
    QTest::newRow("data6") << -100 << 100 << QString("-") << INT;
    QTest::newRow("data7") << -100 << 100 << QString("-500") << INV;
    QTest::newRow("data8") << -100 << 100 << QString("-100") << ACC;
    QTest::newRow("data9") << -100 << -10 << QString("10") << INT;

    QTest::newRow("data10") << 100 << 999 << QString("") << INT;
    QTest::newRow("data11") << 100 << 999 << QString("5") << INT;
    QTest::newRow("data12") << 100 << 999 << QString("50") << INT;
    QTest::newRow("data13") << 100 << 999 << QString("99") << INT;
    QTest::newRow("data14") << 100 << 999 << QString("100") << ACC;
    QTest::newRow("data15") << 100 << 999 << QString("101") << ACC;
    QTest::newRow("data16") << 100 << 999 << QString("998") << ACC;
    QTest::newRow("data17") << 100 << 999 << QString("999") << ACC;
    QTest::newRow("data18") << 100 << 999 << QString("1000") << INV;
    QTest::newRow("data19") << 100 << 999 << QString("-10") << INV;

    QTest::newRow("data20") << -999 << -100 << QString("50") << INT;
    QTest::newRow("data21") << -999 << -100 << QString("-") << INT;
    QTest::newRow("data22") << -999 << -100 << QString("-1") << INT;
    QTest::newRow("data23") << -999 << -100 << QString("-10") << INT;
    QTest::newRow("data24") << -999 << -100 << QString("-100") << ACC;
    QTest::newRow("data25") << -999 << -100 << QString("-500") << ACC;
    QTest::newRow("data26") << -999 << -100 << QString("-998") << ACC;
    QTest::newRow("data27") << -999 << -100 << QString("-999") << ACC;
    QTest::newRow("data28") << -999 << -100 << QString("-1000") << INV;
    QTest::newRow("data29") << -999 << -100 << QString("-2000") << INV;

    QTest::newRow("1.1") << 0 << 10 << QString("") << INT;
    QTest::newRow("1.2") << 10 << 0 << QString("") << INT;

    QTest::newRow("2.1") << 0 << 10 << QString("-") << INV;
    QTest::newRow("2.2") << 0 << 10 << QString("-0") << INV;
    QTest::newRow("2.3") << -10 << -1 << QString("+") << INV;
    QTest::newRow("2.4") << -10 << 10 << QString("-") << INT;
    QTest::newRow("2.5") << -10 << 10 << QString("+") << INT;
    QTest::newRow("2.6") << -10 << 10 << QString("+0") << ACC;
    QTest::newRow("2.7") << -10 << 10 << QString("+1") << ACC;
    QTest::newRow("2.8") << -10 << 10 << QString("+-") << INV;
    QTest::newRow("2.9") << -10 << 10 << QString("-+") << INV;

    QTest::newRow("3.1") << 0 << 10 << QString("12345678901234567890") << INV;
    QTest::newRow("3.2") << 0 << 10 << QString("-12345678901234567890") << INV;
    QTest::newRow("3.3") << 0 << 10 << QString("000000000000000000000") << ACC;
    QTest::newRow("3.4") << 1 << 10 << QString("000000000000000000000") << INT;
    QTest::newRow("3.5") << 0 << 10 << QString("-000000000000000000000") << INV;
    QTest::newRow("3.6") << -10 << -1 << QString("-000000000000000000000") << INT;
    QTest::newRow("3.7") << -10 << -1 << QString("-0000000000000000000001") << ACC;

    QTest::newRow("4.1") << 0 << 10 << QString(" ") << INV;
    QTest::newRow("4.2") << 0 << 10 << QString(" 1") << INV;
    QTest::newRow("4.3") << 0 << 10 << QString("1 ") << INV;
    QTest::newRow("4.4") << 0 << 10 << QString("1.0") << INV;
    QTest::newRow("4.5") << 0 << 10 << QString("0.1") << INV;
    QTest::newRow("4.6") << 0 << 10 << QString(".1") << INV;
    QTest::newRow("4.7") << 0 << 10 << QString("-1.0") << INV;

    QTest::newRow("5.1") << 6 << 8 << QString("5") << INT;
    QTest::newRow("5.2") << 6 << 8 << QString("7") << ACC;
    QTest::newRow("5.3") << 6 << 8 << QString("9") << INV;
    QTest::newRow("5.3a") << 6 << 8 << QString("19") << INV;
    QTest::newRow("5.4") << -8 << -6 << QString("-5") << INT;
    QTest::newRow("5.5") << -8 << -6 << QString("-7") << ACC;
    QTest::newRow("5.6") << -8 << -6 << QString("-9") << INV;
    QTest::newRow("5.6a") << -8 << -6 << QString("-19") << INV;
    QTest::newRow("5.7") << -8 << -6 << QString("5") << INT;
    QTest::newRow("5.8") << -8 << -6 << QString("7") << INT;
    QTest::newRow("5.9") << -8 << -6 << QString("9") << INV;
    QTest::newRow("5.10") << -6 << 8 << QString("-5") << ACC;
    QTest::newRow("5.11") << -6 << 8 << QString("5") << ACC;
    QTest::newRow("5.12") << -6 << 8 << QString("-7") << INV;
    QTest::newRow("5.13") << -6 << 8 << QString("7") << ACC;
    QTest::newRow("5.14") << -6 << 8 << QString("-9") << INV;
    QTest::newRow("5.15") << -6 << 8 << QString("9") << INV;

    QTest::newRow("6.1") << 100 << 102 << QString("11") << INT;
    QTest::newRow("6.2") << 100 << 102 << QString("-11") << INV;

    QTest::newRow("7.1") << 0 << 10 << QString("100") << INV;
    QTest::newRow("7.2") << 0 << -10 << QString("100") << INV;
    QTest::newRow("7.3") << 0 << -10 << QString("-100") << INV;
    QTest::newRow("7.4") << -100 << 10 << QString("100") << INT;

    QTest::newRow("8.1") << -100 << -10 << QString("+") << INV;
    QTest::newRow("8.2") << -100 << -10 << QString("+50") << INV;
    QTest::newRow("8.3") << -100 << -10 << QString("50") << INT;
    QTest::newRow("8.4") << 10 << 100 << QString("-") << INV;
    QTest::newRow("8.5") << 10 << 100 << QString("-50") << INV;
    QTest::newRow("8.6") << 10 << 100 << QString("5") << INT;
    QTest::newRow("8.7") << -1 << 100 << QString("-") << INT;
    QTest::newRow("8.8") << -1 << 100 << QString("-50") << INV;
    QTest::newRow("8.9") << -1 << 100 << QString("5") << ACC;
    QTest::newRow("8.10") << -1 << 100 << QString("+") << INT;
    QTest::newRow("8.11") << -1 << 100 << QString("+50") << ACC;

    QTest::newRow("9.0") << -10 << 10 << QString("000") << ACC;
    QTest::newRow("9.1") << -10 << 10 << QString("008") << ACC;
    QTest::newRow("9.2") << -10 << 10 << QString("-008") << ACC;
    QTest::newRow("9.3") << -10 << 10 << QString("00010") << ACC;
    QTest::newRow("9.4") << -10 << 10 << QString("-00010") << ACC;
    QTest::newRow("9.5") << -10 << 10 << QString("00020") << INV;
    QTest::newRow("9.6") << -10 << 10 << QString("-00020") << INV;

}

void tst_QIntValidator::validateArabic()
{
    QString arabicNum;
    arabicNum += QChar(1633); // "18" in arabic
    arabicNum += QChar(1640);

    QIntValidator validator(-20, 20, 0);
    validator.setLocale(QLocale(QLocale::Arabic, QLocale::SaudiArabia));
    int i;
    QCOMPARE(validator.validate(arabicNum, i), QValidator::Acceptable);

}


void tst_QIntValidator::validateFrench()
{
    QIntValidator validator(-2000, 2000, 0);
    validator.setLocale(QLocale::French);
    int i;
    QString s = QLatin1String("1 ");
    QCOMPARE(validator.validate(s, i), QValidator::Acceptable);
    validator.fixup(s);
    QCOMPARE(s, s);

    s = QLatin1String("1 000");
    QCOMPARE(validator.validate(s, i), QValidator::Acceptable);
    validator.fixup(s);
    QCOMPARE(s, s);


    s = QLatin1String("1 0 00");
    QCOMPARE(validator.validate(s, i), QValidator::Intermediate);
    validator.fixup(s);
    QCOMPARE(s, validator.locale().toString(1000));

    // Confim no fallback to C locale
    s = QLatin1String("1,000");
    QCOMPARE(validator.validate(s, i), QValidator::Invalid);
    validator.setLocale(QLocale::C);
    QCOMPARE(validator.validate(s, i), QValidator::Acceptable);
}

void tst_QIntValidator::validate()
{
    QFETCH(int, minimum);
    QFETCH(int, maximum);
    QFETCH(QString, value);
    QFETCH(QValidator::State, state);

    QIntValidator iv(minimum, maximum, 0);
    iv.setLocale(QLocale::C);
    int dummy;
    QCOMPARE((int)iv.validate(value, dummy), (int)state);
}

void tst_QIntValidator::notifySignals()
{
    QLocale::setDefault(QLocale("C"));

    QIntValidator iv(0, 10, 0);
    QSignalSpy topSpy(&iv, SIGNAL(topChanged(int)));
    QSignalSpy bottomSpy(&iv, SIGNAL(bottomChanged(int)));
    QSignalSpy changedSpy(&iv, SIGNAL(changed()));

    iv.setTop(9);
    QCOMPARE(topSpy.count(), 1);
    QCOMPARE(changedSpy.count(), 1);
    QCOMPARE(iv.top(), 9);
    iv.setBottom(1);
    QCOMPARE(bottomSpy.count(), 1);
    QCOMPARE(changedSpy.count(), 2);
    QCOMPARE(iv.bottom(), 1);

    iv.setRange(1, 8);
    QCOMPARE(topSpy.count(), 2);
    QCOMPARE(bottomSpy.count(), 1);
    QCOMPARE(changedSpy.count(), 3);
    QCOMPARE(iv.top(), 8);
    QCOMPARE(iv.bottom(), 1);

    iv.setRange(2, 8);
    QCOMPARE(topSpy.count(), 2);
    QCOMPARE(bottomSpy.count(), 2);
    QCOMPARE(changedSpy.count(), 4);
    QCOMPARE(iv.top(), 8);
    QCOMPARE(iv.bottom(), 2);

    iv.setRange(3, 7);
    QCOMPARE(topSpy.count(), 3);
    QCOMPARE(bottomSpy.count(), 3);
    QCOMPARE(changedSpy.count(), 5);
    QCOMPARE(iv.top(), 7);
    QCOMPARE(iv.bottom(), 3);

    iv.setRange(3, 7);
    QCOMPARE(topSpy.count(), 3);
    QCOMPARE(bottomSpy.count(), 3);
    QCOMPARE(changedSpy.count(), 5);

    iv.setLocale(QLocale("C"));
    QCOMPARE(changedSpy.count(), 5);

    iv.setLocale(QLocale("en"));
    QCOMPARE(changedSpy.count(), 6);
}

QTEST_APPLESS_MAIN(tst_QIntValidator)
#include "tst_qintvalidator.moc"
