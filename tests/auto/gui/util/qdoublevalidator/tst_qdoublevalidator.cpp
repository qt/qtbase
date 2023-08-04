// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QTest>
#include <QSignalSpy>

#include <qvalidator.h>

class tst_QDoubleValidator : public QObject
{
    Q_OBJECT
private slots:
    void validate_data();
    void validate();
    void zeroPaddedExponent_data();
    void zeroPaddedExponent();
    void validateThouSep_data();
    void validateThouSep();
    void validateIntEquiv_data();
    void validateIntEquiv();
    void notifySignals();
    void fixup();
    void fixup_data();
    void setRangeOverloads();
    void setRangeOverloads_data();
};

Q_DECLARE_METATYPE(QValidator::State);
#define INV QValidator::Invalid
#define ITM QValidator::Intermediate
#define ACC QValidator::Acceptable

void tst_QDoubleValidator::validateThouSep_data()
{
    QTest::addColumn<QString>("localeName");
    QTest::addColumn<QString>("value");
    QTest::addColumn<bool>("rejectGroupSeparator");
    QTest::addColumn<QValidator::State>("result");

    QTest::newRow("1,000C") << "C" << QString("1,000") << false << ACC;
    QTest::newRow("1,000.1C") << "C" << QString("1,000.1") << false << ACC;
    QTest::newRow("1,000.1C_reject") << "C" << QString("1,000.1") << true << INV;
    QTest::newRow("1.000C") << "C" << QString("1.000") << false << ACC;

    QTest::newRow("1,000de") << "de" << QString("1,000") << false << ACC;
    QTest::newRow("1.000de") << "de" << QString("1.000") << false << ACC;

    QTest::newRow(".C") << "C" << QString(".") << false << ITM;
    QTest::newRow(".de") << "de" << QString(".") << false << INV;
    QTest::newRow("1.000,1de") << "de" << QString("1.000,1") << false << ACC;
    QTest::newRow("1.000,1de_reject") << "de" << QString("1.000,1") << true << INV;
    QTest::newRow(",C") << "C" << QString(",") << false << INV;
    QTest::newRow(",de") << "de" << QString(",") << false << ITM;
    QTest::newRow("1,23") << "en_AU" << QString("1,00") << false << ITM;
}

void tst_QDoubleValidator::validateThouSep()
{
    QFETCH(QString, localeName);
    QFETCH(QString, value);
    QFETCH(bool, rejectGroupSeparator);
    QFETCH(QValidator::State, result);
    int dummy = 0;

    QDoubleValidator iv(-10000, 10000, 3, 0);
    iv.setNotation(QDoubleValidator::ScientificNotation);
    QLocale locale(localeName);
    if (rejectGroupSeparator)
        locale.setNumberOptions(QLocale::RejectGroupSeparator);
    iv.setLocale(locale);

    QCOMPARE(iv.validate(value, dummy), result);
}

void tst_QDoubleValidator::validate_data()
{
    QTest::addColumn<QString>("localeName");
    QTest::addColumn<double>("minimum");
    QTest::addColumn<double>("maximum");
    QTest::addColumn<int>("decimals");
    QTest::addColumn<QString>("value");
    QTest::addColumn<QValidator::State>("scientific_state");
    QTest::addColumn<QValidator::State>("standard_state");

    QTest::newRow("data0")  << "C" << 0.0 << 100.0 << 1 << QString("50.0") << ACC << ACC;
    QTest::newRow("data1")  << "C" << 00.0 << 100.0 << 1 << QString("500.0") << ITM << ITM;
    QTest::newRow("data1a")  << "C" << 00.0 << 100.0 << 1 << QString("5001.0") << ITM << INV;
    QTest::newRow("data2")  << "C" << 00.0 << 100.0 << 1 << QString("-35.0") << INV << INV;
    QTest::newRow("data3")  << "C" << 00.0 << 100.0 << 1 << QString("a") << INV << INV;
    QTest::newRow("data4")  << "C" << 0.0 << 100.0 << 1 << QString("-") << INV << INV;
    QTest::newRow("data5")  << "C" << 0.0 << 100.0 << 1 << QString("100.0") << ACC << ACC;
    QTest::newRow("data6")  << "C" << -100.0 << 100.0 << 1 << QString("-") << ITM << ITM;
    QTest::newRow("data7")  << "C" << -100.0 << 100.0 << 1 << QString("-500.0") << ITM << ITM;
    QTest::newRow("data8")  << "C" << -100.0 << 100.0 << 1 << QString("-100") << ACC << ACC;
    QTest::newRow("data9")  << "C" << -100.0 << -10.0 << 1 << QString("10") << ITM << ITM;
    QTest::newRow("data10") << "C" << 0.3 << 0.5 << 5 << QString("0.34567") << ACC << ACC;
    QTest::newRow("data11") << "C" << -0.3 << -0.5 << 5 << QString("-0.345678") << INV << INV;
    QTest::newRow("data12") << "C" << -0.32 << 0.32 << 1 << QString("0") << ACC << ACC;
    QTest::newRow("data13") << "C" << 0.0 << 100.0 << 1 << QString("3456a") << INV << INV;
    QTest::newRow("data14") << "C" << -100.0 << 100.0 << 1 << QString("-3456a") << INV << INV;
    QTest::newRow("data15") << "C" << -100.0 << 100.0 << 1 << QString("a-3456") << INV << INV;
    QTest::newRow("data16") << "C" << -100.0 << 100.0 << 1 << QString("a-3456a") << INV << INV;
    QTest::newRow("data17") << "C" << 1229.0 << 1231.0 << 0 << QString("123e") << ITM << INV;
    QTest::newRow("data18") << "C" << 1229.0 << 1231.0 << 0 << QString("123e+") << ITM << INV;
    QTest::newRow("data19") << "C" << 1229.0 << 1231.0 << 0 << QString("123e+1") << ACC << INV;
    QTest::newRow("data20") << "C" << 12290.0 << 12310.0 << 0 << QString("123e+2") << ACC << INV;
    QTest::newRow("data21") << "C" << 12.290 << 12.310 << 2 << QString("123e-") << ITM << INV;
    QTest::newRow("data22") << "C" << 12.290 << 12.310 << 2 << QString("123e-1") << ACC << INV;
    QTest::newRow("data23") << "C" << 1.2290 << 1.2310 << 3 << QString("123e-2") << ACC << INV;
    QTest::newRow("data24") << "C" << 1229.0 << 1231.0 << 0 << QString("123E") << ITM << INV;
    QTest::newRow("data25") << "C" << 1229.0 << 1231.0 << 0 << QString("123E+") << ITM << INV;
    QTest::newRow("data26") << "C" << 1229.0 << 1231.0 << 0 << QString("123E+1") << ACC << INV;
    QTest::newRow("data27") << "C" << 12290.0 << 12310.0 << 0 << QString("123E+2") << ACC << INV;
    QTest::newRow("data28") << "C" << 12.290 << 12.310 << 2 << QString("123E-") << ITM << INV;
    QTest::newRow("data29") << "C" << 12.290 << 12.310 << 2 << QString("123E-1") << ACC << INV;
    QTest::newRow("data30") << "C" << 1.2290 << 1.2310 << 3 << QString("123E-2") << ACC << INV;
    QTest::newRow("data31") << "C" << 1.2290 << 1.2310 << 3 << QString("e") << ITM << INV;
    QTest::newRow("data32") << "C" << 1.2290 << 1.2310 << 3 << QString("e+") << ITM << INV;
    QTest::newRow("data33") << "C" << 1.2290 << 1.2310 << 3 << QString("e+1") << ITM << INV;
    QTest::newRow("data34") << "C" << 1.2290 << 1.2310 << 3 << QString("e-") << ITM << INV;
    QTest::newRow("data35") << "C" << 1.2290 << 1.2310 << 3 << QString("e-1") << ITM << INV;
    QTest::newRow("data36") << "C" << 1.2290 << 1.2310 << 3 << QString("E") << ITM << INV;
    QTest::newRow("data37") << "C" << 1.2290 << 1.2310 << 3 << QString("E+") << ITM << INV;
    QTest::newRow("data38") << "C" << 1.2290 << 1.2310 << 3 << QString("E+1") << ITM << INV;
    QTest::newRow("data39") << "C" << 1.2290 << 1.2310 << 3 << QString("E-") << ITM << INV;
    QTest::newRow("data40") << "C" << 1.2290 << 1.2310 << 3 << QString("E-1") << ITM << INV;
    QTest::newRow("data41") << "C" << -100.0 << 100.0 << 0 << QString("10e") << ITM << INV;
    QTest::newRow("data42") << "C" << -100.0 << 100.0 << 0 << QString("10e+") << ITM << INV;
    QTest::newRow("data43") << "C" << 0.01 << 0.09 << 2 << QString("0") << ITM << ITM;
    QTest::newRow("data44") << "C" << 0.0 << 10.0 << 1 << QString("11") << ITM << ITM;
    QTest::newRow("data45") << "C" << 0.0 << 10.0 << 2 << QString("11") << ITM << ITM;
    QTest::newRow("data46")  << "C" << 0.0 << 100.0 << 0 << QString("0.") << ACC << ACC;
    QTest::newRow("data47")  << "C" << 0.0 << 100.0 << 0 << QString("0.0") << INV << INV;
    QTest::newRow("data48")  << "C" << 0.0 << 100.0 << 1 << QString("0.0") << ACC << ACC;
    QTest::newRow("data49")  << "C" << 0.0 << 100.0 << 0 << QString(".") << ITM << ITM;
    QTest::newRow("data50")  << "C" << 0.0 << 100.0 << 1 << QString(".") << ITM << ITM;
    QTest::newRow("data51")  << "C" << 0.0 << 2.0 << 2 << QString("9.99") << ITM << ITM;
    QTest::newRow("data52")  << "C" << 100.0 << 200.0 << 4 << QString("999.9999") << ITM << ITM;
    QTest::newRow("data53")  << "C" << 0.0 << 2.0 << 2 << QString("9.9999") << INV << INV;
    QTest::newRow("data54")  << "C" << 100.0 << 200.0 << 4 << QString("9999.9999") << ITM << INV;
    QTest::newRow("data55") << "C" << 1229.0 << 1231.0 << -1 << QString("123E") << ITM << INV;
    QTest::newRow("data56") << "C" << 1229.0 << 1231.0 << -1 << QString("123E+") << ITM << INV;
    QTest::newRow("data57") << "C" << 1229.0 << 1231.0 << -1 << QString("123E+1") << ACC << INV;
    QTest::newRow("data58") << "C" << 0.0 << 100.0 << -1 << QString("0.0") << ACC << ACC;
    QTest::newRow("overlong") << "C" << 0.0 << 99.9 << 2 << QString("1234.0") << ITM << INV;

    QTest::newRow("data_de0")  << "de" << 0.0 << 100.0 << 1 << QString("50,0") << ACC << ACC;
    QTest::newRow("data_de1")  << "de" << 00.0 << 100.0 << 1 << QString("500,0") << ITM << ITM;
    QTest::newRow("data_de1a")  << "de" << 00.0 << 100.0 << 1 << QString("5001,0") << ITM << INV;
    QTest::newRow("data_de0C")  << "de" << 0.0 << 100.0 << 1 << QString("50,0") << ACC << ACC;
    QTest::newRow("data_de1C")  << "de" << 00.0 << 100.0 << 1 << QString("500,0") << ITM << ITM;
    QTest::newRow("data_de1aC")  << "de" << 00.0 << 100.0 << 1 << QString("5001,0") << ITM << INV;
    QTest::newRow("data_de2")  << "de" << 00.0 << 100.0 << 1 << QString("-35,0") << INV << INV;
    QTest::newRow("data_de3")  << "de" << 00.0 << 100.0 << 1 << QString("a") << INV << INV;
    QTest::newRow("data_de4")  << "de" << 0.0 << 100.0 << 1 << QString("-") << INV << INV;
    QTest::newRow("data_de5")  << "de" << 0.0 << 100.0 << 1 << QString("100,0") << ACC << ACC;
    QTest::newRow("data_de6")  << "de" << -100.0 << 100.0 << 1 << QString("-") << ITM << ITM;
    QTest::newRow("data_de7")  << "de" << -100.0 << 100.0 << 1 << QString("-500,0") << ITM << ITM;
    QTest::newRow("data_de8")  << "de" << -100.0 << 100.0 << 1 << QString("-100") << ACC << ACC;
    QTest::newRow("data_de9")  << "de" << -100.0 << -10.0 << 1 << QString("10") << ITM << ITM;
    QTest::newRow("data_de10") << "de" << 0.3 << 0.5 << 5 << QString("0,34567") << ACC << ACC;
    QTest::newRow("data_de11") << "de" << -0.3 << -0.5 << 5 << QString("-0,345678") << INV << INV;
    QTest::newRow("data_de12") << "de" << -0.32 << 0.32 << 1 << QString("0") << ACC << ACC;
    QTest::newRow("data_de13") << "de" << 0.0 << 100.0 << 1 << QString("3456a") << INV << INV;
    QTest::newRow("data_de14") << "de" << -100.0 << 100.0 << 1 << QString("-3456a") << INV << INV;
    QTest::newRow("data_de15") << "de" << -100.0 << 100.0 << 1 << QString("a-3456") << INV << INV;
    QTest::newRow("data_de16") << "de" << -100.0 << 100.0 << 1 << QString("a-3456a") << INV << INV;
    QTest::newRow("data_de17") << "de" << 1229.0 << 1231.0 << 0 << QString("123e") << ITM << INV;
    QTest::newRow("data_de18") << "de" << 1229.0 << 1231.0 << 0 << QString("123e+") << ITM << INV;
    QTest::newRow("data_de19") << "de" << 1229.0 << 1231.0 << 0 << QString("123e+1") << ACC << INV;
    QTest::newRow("data_de20") << "de" << 12290.0 << 12310.0 << 0 << QString("123e+2") << ACC << INV;
    QTest::newRow("data_de21") << "de" << 12.290 << 12.310 << 2 << QString("123e-") << ITM << INV;
    QTest::newRow("data_de22") << "de" << 12.290 << 12.310 << 2 << QString("123e-1") << ACC << INV;
    QTest::newRow("data_de23") << "de" << 1.2290 << 1.2310 << 3 << QString("123e-2") << ACC << INV;
    QTest::newRow("data_de24") << "de" << 1229.0 << 1231.0 << 0 << QString("123E") << ITM << INV;
    QTest::newRow("data_de25") << "de" << 1229.0 << 1231.0 << 0 << QString("123E+") << ITM << INV;
    QTest::newRow("data_de26") << "de" << 1229.0 << 1231.0 << 0 << QString("123E+1") << ACC << INV;
    QTest::newRow("data_de27") << "de" << 12290.0 << 12310.0 << 0 << QString("123E+2") << ACC << INV;
    QTest::newRow("data_de28") << "de" << 12.290 << 12.310 << 2 << QString("123E-") << ITM << INV;
    QTest::newRow("data_de29") << "de" << 12.290 << 12.310 << 2 << QString("123E-1") << ACC << INV;
    QTest::newRow("data_de30") << "de" << 1.2290 << 1.2310 << 3 << QString("123E-2") << ACC << INV;
    QTest::newRow("data_de31") << "de" << 1.2290 << 1.2310 << 3 << QString("e") << ITM << INV;
    QTest::newRow("data_de32") << "de" << 1.2290 << 1.2310 << 3 << QString("e+") << ITM << INV;
    QTest::newRow("data_de33") << "de" << 1.2290 << 1.2310 << 3 << QString("e+1") << ITM << INV;
    QTest::newRow("data_de34") << "de" << 1.2290 << 1.2310 << 3 << QString("e-") << ITM << INV;
    QTest::newRow("data_de35") << "de" << 1.2290 << 1.2310 << 3 << QString("e-1") << ITM << INV;
    QTest::newRow("data_de36") << "de" << 1.2290 << 1.2310 << 3 << QString("E") << ITM << INV;
    QTest::newRow("data_de37") << "de" << 1.2290 << 1.2310 << 3 << QString("E+") << ITM << INV;
    QTest::newRow("data_de38") << "de" << 1.2290 << 1.2310 << 3 << QString("E+1") << ITM << INV;
    QTest::newRow("data_de39") << "de" << 1.2290 << 1.2310 << 3 << QString("E-") << ITM << INV;
    QTest::newRow("data_de40") << "de" << 1.2290 << 1.2310 << 3 << QString("E-1") << ITM << INV;
    QTest::newRow("data_de41") << "de" << -100.0 << 100.0 << 0 << QString("10e") << ITM << INV;
    QTest::newRow("data_de42") << "de" << -100.0 << 100.0 << 0 << QString("10e+") << ITM << INV;
    QTest::newRow("data_de43") << "de" << 0.01 << 0.09 << 2 << QString("0") << ITM << ITM;
    QTest::newRow("data_de44") << "de" << 0.0 << 10.0 << 1 << QString("11") << ITM << ITM;
    QTest::newRow("data_de45") << "de" << 0.0 << 10.0 << 2 << QString("11") << ITM << ITM;
    QTest::newRow("data_de46") << "de" << 0.0 << 2.0 << 2 << QString("9,99") << ITM << ITM;
    QTest::newRow("data_de47") << "de" << 100.0 << 200.0 << 4 << QString("999,9999") << ITM << ITM;
    QTest::newRow("data_de48") << "de" << 0.0 << 2.0 << 2 << QString("9,9999") << INV << INV;
    QTest::newRow("data_de49") << "de" << 100.0 << 200.0 << 4 << QString("9999,9999") << ITM << INV;

    // using default QDoubleValidator parameters for initialization
    QTest::newRow("inf") << "C" << -HUGE_VAL << HUGE_VAL << 1000 << QString("inf") << INV << INV;
    QTest::newRow("+inf") << "C" << -HUGE_VAL << HUGE_VAL << 1000 << QString("+inf") << INV << INV;
    QTest::newRow("-inf") << "C" << -HUGE_VAL << HUGE_VAL << 1000 << QString("-inf") << INV << INV;
    QTest::newRow("nan") << "C" << -HUGE_VAL << HUGE_VAL << 1000 << QString("nan") << INV << INV;
    QTest::newRow("+nan") << "C" << -HUGE_VAL << HUGE_VAL << 1000 << QString("+nan") << INV << INV;
    QTest::newRow("-nan") << "C" << -HUGE_VAL << HUGE_VAL << 1000 << QString("-nan") << INV << INV;

    QString arabicNum;
    arabicNum += QChar(1633); // "18.4" in arabic
    arabicNum += QChar(1640);
    arabicNum += QChar(1643);
    arabicNum += QChar(1636);
    QTest::newRow("arabic") << "ar" << 0.0 << 20.0 << 2 << arabicNum << ACC << ACC;

    // Confim no fallback to C locale
    QTest::newRow("data_C1") << "de" << 0.0 << 1000.0 << 2 << QString("1.000,00") << ACC << ACC;
    QTest::newRow("data_C2") << "de" << 0.0 << 1000.0 << 2 << QString("1,000.00") << INV << INV;
}

void tst_QDoubleValidator::validate()
{
    QFETCH(QString, localeName);
    QFETCH(double, minimum);
    QFETCH(double, maximum);
    QFETCH(int, decimals);
    QFETCH(QString, value);
    QFETCH(QValidator::State, scientific_state);
    QFETCH(QValidator::State, standard_state);

    QLocale::setDefault(QLocale(localeName));

    QDoubleValidator dv(minimum, maximum, decimals, 0);
    int dummy;
    QCOMPARE(dv.validate(value, dummy), scientific_state);
    dv.setNotation(QDoubleValidator::StandardNotation);
    QCOMPARE(dv.validate(value, dummy), standard_state);
}

void tst_QDoubleValidator::zeroPaddedExponent_data()
{
    QTest::addColumn<double>("minimum");
    QTest::addColumn<double>("maximum");
    QTest::addColumn<int>("decimals");
    QTest::addColumn<QString>("value");
    QTest::addColumn<bool>("rejectZeroPaddedExponent");
    QTest::addColumn<QValidator::State>("state");

    QTest::newRow("data01") << 1229.0  << 1231.0  << 0 << QString("123e+1") << false << ACC;
    QTest::newRow("data02") << 12290.0 << 12310.0 << 0 << QString("123e2")  << false << ACC;
    QTest::newRow("data03") << 12.290  << 12.310  << 2 << QString("123e-")  << false << ITM;
    QTest::newRow("data04") << 12.290  << 12.310  << 2 << QString("123e-1") << false << ACC;
    QTest::newRow("data05") << 1.2290  << 1.2310  << 3 << QString("123e-2") << false << ACC;

    QTest::newRow("data11") << 1229.0  << 1231.0  << 0 << QString("123e+1") << true << ACC;
    QTest::newRow("data12") << 12290.0 << 12310.0 << 0 << QString("123e2")  << true << ACC;
    QTest::newRow("data13") << 12.290  << 12.310  << 2 << QString("123e-")  << true << ITM;
    QTest::newRow("data14") << 12.290  << 12.310  << 2 << QString("123e-1") << true << ACC;
    QTest::newRow("data15") << 1.2290  << 1.2310  << 3 << QString("123e-2") << true << ACC;

    QTest::newRow("data21") << 1229.0  << 1231.0  << 0 << QString("123e+01") << false << ACC;
    QTest::newRow("data22") << 12290.0 << 12310.0 << 0 << QString("123e02")  << false << ACC;
    QTest::newRow("data23") << 12.290  << 12.310  << 2 << QString("123e-0")  << false << ITM;
    QTest::newRow("data24") << 12.290  << 12.310  << 2 << QString("123e-01") << false << ACC;
    QTest::newRow("data25") << 1.2290  << 1.2310  << 3 << QString("123e-02") << false << ACC;

    QTest::newRow("data31") << 1229.0  << 1231.0  << 0 << QString("123e+01") << true << INV;
    QTest::newRow("data32") << 12290.0 << 12310.0 << 0 << QString("123e02")  << true << INV;
    QTest::newRow("data33") << 12.290  << 12.310  << 2 << QString("123e-0")  << true << INV;
    QTest::newRow("data34") << 12.290  << 12.310  << 2 << QString("123e-01") << true << INV;
    QTest::newRow("data35") << 1.2290  << 1.2310  << 3 << QString("123e-02") << true << INV;

}

void tst_QDoubleValidator::zeroPaddedExponent()
{
    QFETCH(double, minimum);
    QFETCH(double, maximum);
    QFETCH(int, decimals);
    QFETCH(QString, value);
    QFETCH(bool, rejectZeroPaddedExponent);
    QFETCH(QValidator::State, state);

    QLocale locale(QLocale::C);
    if (rejectZeroPaddedExponent)
        locale.setNumberOptions(QLocale::RejectLeadingZeroInExponent);

    QDoubleValidator dv(minimum, maximum, decimals, 0);
    dv.setLocale(locale);
    int dummy;
    QCOMPARE(dv.validate(value, dummy), state);
}

void tst_QDoubleValidator::notifySignals()
{
    QLocale::setDefault(QLocale("C"));

    QDoubleValidator dv(0.1, 0.9, 10, 0);
    QSignalSpy topSpy(&dv, SIGNAL(topChanged(double)));
    QSignalSpy bottomSpy(&dv, SIGNAL(bottomChanged(double)));
    QSignalSpy decSpy(&dv, SIGNAL(decimalsChanged(int)));
    QSignalSpy changedSpy(&dv, SIGNAL(changed()));

    qRegisterMetaType<QDoubleValidator::Notation>("QDoubleValidator::Notation");
    QSignalSpy notSpy(&dv, SIGNAL(notationChanged(QDoubleValidator::Notation)));

    QCOMPARE(dv.bottom(), 0.1);
    QCOMPARE(dv.top(), 0.9);
    QCOMPARE(dv.decimals(), 10);

    dv.setTop(0.8);
    QCOMPARE(topSpy.size(), 1);
    QCOMPARE(changedSpy.size(), 1);
    QCOMPARE(dv.top(), 0.8);
    dv.setBottom(0.2);
    QCOMPARE(bottomSpy.size(), 1);
    QCOMPARE(changedSpy.size(), 2);
    QCOMPARE(dv.bottom(), 0.2);

    dv.setRange(0.2, 0.7);
    QCOMPARE(topSpy.size(), 2);
    QCOMPARE(bottomSpy.size(), 1);
    QCOMPARE(decSpy.size(), 0);
    QCOMPARE(changedSpy.size(), 3);
    QCOMPARE(dv.bottom(), 0.2);
    QCOMPARE(dv.top(), 0.7);
    QCOMPARE(dv.decimals(), 10);

    dv.setRange(0.3, 0.7);
    QCOMPARE(topSpy.size(), 2);
    QCOMPARE(bottomSpy.size(), 2);
    QCOMPARE(changedSpy.size(), 4);
    QCOMPARE(dv.bottom(), 0.3);
    QCOMPARE(dv.top(), 0.7);
    QCOMPARE(dv.decimals(), 10);

    dv.setRange(0.4, 0.6);
    QCOMPARE(topSpy.size(), 3);
    QCOMPARE(bottomSpy.size(), 3);
    QCOMPARE(changedSpy.size(), 5);
    QCOMPARE(dv.bottom(), 0.4);
    QCOMPARE(dv.top(), 0.6);
    QCOMPARE(dv.decimals(), 10);

    dv.setDecimals(5);
    QCOMPARE(decSpy.size(), 1);
    QCOMPARE(changedSpy.size(), 6);
    QCOMPARE(dv.decimals(), 5);


    dv.setRange(0.4, 0.6, 100);
    QCOMPARE(topSpy.size(), 3);
    QCOMPARE(bottomSpy.size(), 3);
    QCOMPARE(decSpy.size(), 2);
    QCOMPARE(changedSpy.size(), 7);
    QCOMPARE(dv.bottom(), 0.4);
    QCOMPARE(dv.top(), 0.6);
    QCOMPARE(dv.decimals(), 100);

    dv.setNotation(QDoubleValidator::StandardNotation);
    QCOMPARE(notSpy.size(), 1);
    QCOMPARE(changedSpy.size(), 8);
    QCOMPARE(dv.notation(), QDoubleValidator::StandardNotation);

    dv.setRange(dv.bottom(), dv.top(), dv.decimals());
    QCOMPARE(topSpy.size(), 3);
    QCOMPARE(bottomSpy.size(), 3);
    QCOMPARE(decSpy.size(), 2);
    QCOMPARE(changedSpy.size(), 8);

    dv.setNotation(dv.notation());
    QCOMPARE(notSpy.size(), 1);
    QCOMPARE(changedSpy.size(), 8);

    dv.setLocale(QLocale("C"));
    QCOMPARE(changedSpy.size(), 8);

    dv.setLocale(QLocale("en"));
    QCOMPARE(changedSpy.size(), 9);
}

void tst_QDoubleValidator::fixup()
{
    QFETCH(QString, localeName);
    QFETCH(QDoubleValidator::Notation, notation);
    QFETCH(int, decimals);
    QFETCH(QString, input);
    QFETCH(QString, output);

    QDoubleValidator val;
    val.setLocale(QLocale(localeName));
    val.setNotation(notation);
    val.setDecimals(decimals);

    val.fixup(input);
    QCOMPARE(input, output);
}

void tst_QDoubleValidator::fixup_data()
{
    QTest::addColumn<QString>("localeName");
    QTest::addColumn<QDoubleValidator::Notation>("notation");
    QTest::addColumn<int>("decimals");
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("output");

    // C locale uses '.' as decimal point and ',' as grouping separator.
    // C locale does not group digits by default.
    QTest::newRow("C standard no digit grouping")
            << "C" << QDoubleValidator::StandardNotation << -1 << "12.345"
            << "12.345";
    QTest::newRow("C standard with digit grouping")
            << "C" << QDoubleValidator::StandardNotation << -1 << "-12,345.678"
            << "-12345.678";
    QTest::newRow("C standard with invalid digit grouping")
            << "C" << QDoubleValidator::StandardNotation << -1 << "1,234,5.678"
            << "12345.678";
    QTest::newRow("C standard with invalid group size")
            << "C" << QDoubleValidator::StandardNotation << 2 << "-12,34.678"
            << "-1234.68";
    QTest::newRow("C standard truncate decimals")
            << "C" << QDoubleValidator::StandardNotation << -1
            << "1.23456789012345678901234567890"
            << "1.2345678901234567";
    QTest::newRow("C standard skip trailing zeroes")
            << "C" << QDoubleValidator::StandardNotation << -1 << "1,234.5670000"
            << "1234.567";
    QTest::newRow("C standard zero value")
            << "C" << QDoubleValidator::StandardNotation << -1 << "0.0"
            << "0";
    QTest::newRow("C standard scientific value")
            << "C" << QDoubleValidator::StandardNotation << -1 << "1.23e-2"
            << "1.23e-2";
    QTest::newRow("C standard no fractional part")
            << "C" << QDoubleValidator::StandardNotation << -1 << "-1,234"
            << "-1234";

    QTest::newRow("C scientific no digit grouping")
            << "C" << QDoubleValidator::ScientificNotation << -1 << "0.98765e2"
            << "9.8765e+01";
    QTest::newRow("C scientific with digit grouping")
            << "C" << QDoubleValidator::ScientificNotation << -1 << "-1,234.98765E-4"
            << "-1.23498765E-01";
    QTest::newRow("C scientific with invalid digit grouping")
            << "C" << QDoubleValidator::ScientificNotation << -1 << "12,34.98765e2"
            << "1.23498765e+05";
    QTest::newRow("C scientific with invalid group size")
            << "C" << QDoubleValidator::ScientificNotation << 2 << "-12,34.98765e2"
            << "-1.23e+05";
    QTest::newRow("C scientific truncate decimals")
            << "C" << QDoubleValidator::ScientificNotation << -1
            << "1.23456789012345678901234567890E5"
            << "1.2345678901234567E+05";
    QTest::newRow("C scientific skip trailing zeroes")
            << "C" << QDoubleValidator::ScientificNotation << -1 << "1,234.5670000e3"
            << "1.234567e+06";
    QTest::newRow("C scientific zero value")
            << "C" << QDoubleValidator::ScientificNotation << -1 << "0.0"
            << "0e+00";
    QTest::newRow("C scientific standard value")
            << "C" << QDoubleValidator::ScientificNotation << -1 << "12.345"
            << "1.2345e+01";
    QTest::newRow("C scientific no fractional part")
            << "C" << QDoubleValidator::ScientificNotation << -1 << "1,234e2"
            << "1.234e+05";
    QTest::newRow("C scientific negative no fractional part")
            << "C" << QDoubleValidator::ScientificNotation << -1 << "-1,234e2"
            << "-1.234e+05";
    QTest::newRow("C scientific no fractional and exponent")
            << "C" << QDoubleValidator::ScientificNotation << -1 << "1,234"
            << "1.234e+03";
    QTest::newRow("C scientific negative no fractional and exponent")
            << "C" << QDoubleValidator::ScientificNotation << -1 << "-1,234"
            << "-1.234e+03";

    // en locale uses '.' as decimal point and ',' as grouping separator.
    // en locale groups digits by default. 'E' is used in scientific notation.
    QTest::newRow("en standard no digit grouping")
            << "en" << QDoubleValidator::StandardNotation << -1 << "-12.345"
            << "-12.345";
    QTest::newRow("en standard with digit grouping")
            << "en" << QDoubleValidator::StandardNotation << -1 << "12,345.678"
            << "12,345.678";
    QTest::newRow("en standard with invalid digit grouping")
            << "en" << QDoubleValidator::StandardNotation << -1 << "-1,234,5.678"
            << "-12,345.678";
    QTest::newRow("en standard with invalid group size")
            << "en" << QDoubleValidator::StandardNotation << 2 << "12,34.678"
            << "1,234.68";
    QTest::newRow("en standard no fractional part")
            << "en" << QDoubleValidator::StandardNotation << -1 << "-12,34"
            << "-1,234";

    QTest::newRow("en scientific no digit grouping")
            << "en" << QDoubleValidator::ScientificNotation << -1 << "-0.98765e2"
            << "-9.8765E+01";
    QTest::newRow("en scientific with digit grouping")
            << "en" << QDoubleValidator::ScientificNotation << -1 << "1,234.98765E-4"
            << "1.23498765E-01";
    QTest::newRow("en scientific with invalid digit grouping")
            << "en" << QDoubleValidator::ScientificNotation << -1 << "-12,34.98765e2"
            << "-1.23498765E+05";
    QTest::newRow("en scientific with invalid group size")
            << "en" << QDoubleValidator::ScientificNotation << 2 << "12,34.98765e2"
            << "1.23E+05";
    QTest::newRow("en scientific no fractional part")
            << "en" << QDoubleValidator::ScientificNotation << -1 << "12,34e2"
            << "1.234E+05";
    QTest::newRow("en scientific negative no fractional part")
            << "en" << QDoubleValidator::ScientificNotation << -1 << "-12,34e2"
            << "-1.234E+05";
    QTest::newRow("en scientific no fractional and exponent")
            << "en" << QDoubleValidator::ScientificNotation << -1 << "1,234"
            << "1.234E+03";
    QTest::newRow("en scientific negative no fractional and exponent")
            << "en" << QDoubleValidator::ScientificNotation << -1 << "-1,234"
            << "-1.234E+03";

    // de locale uses ',' as decimal point and '.' as grouping separator.
    // de locale groups digits by default. 'E' is used in scientific notation.
    QTest::newRow("de standard no digit grouping")
            << "de" << QDoubleValidator::StandardNotation << -1 << "12,345"
            << "12,345";
    QTest::newRow("de standard with digit grouping")
            << "de" << QDoubleValidator::StandardNotation << -1 << "-12.345,678"
            << "-12.345,678";
    QTest::newRow("de standard with invalid digit grouping")
            << "de" << QDoubleValidator::StandardNotation << -1 << "1.234.5,678"
            << "12.345,678";
    QTest::newRow("de standard with invalid group size")
            << "de" << QDoubleValidator::StandardNotation << 2 << "-12.34,678"
            << "-1.234,68";
    QTest::newRow("de standard no fractional part")
            << "de" << QDoubleValidator::StandardNotation << -1 << "12.34" << "1.234";

    QTest::newRow("de scientific no digit grouping")
            << "de" << QDoubleValidator::ScientificNotation << -1 << "0,98765e2"
            << "9,8765E+01";
    QTest::newRow("de scientific with digit grouping")
            << "de" << QDoubleValidator::ScientificNotation << -1 << "-1.234,98765E-4"
            << "-1,23498765E-01";
    QTest::newRow("de scientific with invalid digit grouping")
            << "de" << QDoubleValidator::ScientificNotation << -1 << "12.34,98765e2"
            << "1,23498765E+05";
    QTest::newRow("de scientific with invalid group size")
            << "de" << QDoubleValidator::ScientificNotation << 2 << "-12.34,98765e2"
            << "-1,23E+05";
    QTest::newRow("de scientific no fractional part")
            << "de" << QDoubleValidator::ScientificNotation << -1 << "1.234e2"
            << "1,234E+05";
    QTest::newRow("de scientific negative no fractional part")
            << "de" << QDoubleValidator::ScientificNotation << -1 << "-1.234e2"
            << "-1,234E+05";
    QTest::newRow("de scientific no fractional and exponent")
            << "de" << QDoubleValidator::ScientificNotation << -1 << "12.34"
            << "1,234E+03";
    QTest::newRow("de scientific negative no fractional and exponent")
            << "de" << QDoubleValidator::ScientificNotation << -1 << "-12.34"
            << "-1,234E+03";

    // es locale uses ',' as decimal point and '.' as grouping separator.
    // It doesn't apply grouping unless the the next-to-least significant group
    // has more than one digit in it.
    QTest::newRow("es standard no digit grouping")
            << "es" << QDoubleValidator::StandardNotation << -1 << "1234,567" << "1234,567";
    QTest::newRow("es standard with digit grouping")
            << "es" << QDoubleValidator::StandardNotation << -1 << "-12.345,678" << "-12.345,678";
    QTest::newRow("es standard with invalid group size")
            << "es" << QDoubleValidator::StandardNotation << -1 << "1.234.5,678" << "12.345,678";
    QTest::newRow("es standard with invalid digit grouping")
            << "es" << QDoubleValidator::StandardNotation << 2 << "-1.234,678" << "-1234,68";
    QTest::newRow("es standard big with invalid digit grouping")
            << "es" << QDoubleValidator::StandardNotation << 2 << "-1234.678,9" << "-1.234.678,9";
    QTest::newRow("es standard no fractional part")
            << "es" << QDoubleValidator::StandardNotation << -1 << "12.34" << "1234";

    // hi locale uses '.' as decimal point and ',' as grouping separator.
    // The rightmost group is of three digits, all the others contain two
    // digits.
    QTest::newRow("hi standard no digit grouping")
            << "hi" << QDoubleValidator::StandardNotation << -1 << "123456.78"
            << "1,23,456.78";
    QTest::newRow("hi standard with digit grouping")
            << "hi" << QDoubleValidator::StandardNotation << -1 << "-12,345.678"
            << "-12,345.678";
    QTest::newRow("hi standard with invalid digit grouping")
            << "hi" << QDoubleValidator::StandardNotation << -1 << "12,34,56.78"
            << "1,23,456.78";
    QTest::newRow("hi standard no fractional part")
            << "hi" << QDoubleValidator::StandardNotation << -1 << "-12,345,6"
            << "-1,23,456";

    QTest::newRow("hi scientific no digit grouping")
            << "hi" << QDoubleValidator::ScientificNotation << -1 << "-0.123e-2"
            << "-1.23E-03";
    QTest::newRow("hi scientific with digit grouping")
            << "hi" << QDoubleValidator::ScientificNotation << -1 << "12,345.678e-2"
            << "1.2345678E+02";
    QTest::newRow("hi scientific with invalid digit grouping")
            << "hi" << QDoubleValidator::ScientificNotation << -1 << "-1,23,45.678e-2"
            << "-1.2345678E+02";
    QTest::newRow("hi scientific no fractional part")
            << "hi" << QDoubleValidator::ScientificNotation << -1 << "1,23,456e2"
            << "1.23456E+07";
    QTest::newRow("hi scientific negative no fractional part")
            << "hi" << QDoubleValidator::ScientificNotation << -1 << "-1,23,456e2"
            << "-1.23456E+07";
    QTest::newRow("hi scientific no fractional and exponent")
            << "hi" << QDoubleValidator::ScientificNotation << -1 << "1,234,56"
            << "1.23456E+05";
    QTest::newRow("hi scientific negative no fractional and exponent")
            << "hi" << QDoubleValidator::ScientificNotation << -1 << "-1,234,56"
            << "-1.23456E+05";
}

void tst_QDoubleValidator::setRangeOverloads()
{
    QFETCH(QDoubleValidator::Notation, notation);
    QFETCH(int, initialDecimals);
    QFETCH(double, minimum);
    QFETCH(double, maximum);
    QFETCH(int, updatedDecimals);
    QFETCH(QString, input);
    QFETCH(QValidator::State, initDecimalsState);
    QFETCH(QValidator::State, updDecimalsState);

    QDoubleValidator dv;
    dv.setLocale(QLocale::C);
    dv.setNotation(notation);
    dv.setDecimals(initialDecimals);
    dv.setRange(minimum, maximum);
    QCOMPARE(dv.decimals(), initialDecimals);

    int dummy;
    QCOMPARE(dv.validate(input, dummy), initDecimalsState);

    dv.setRange(minimum, maximum, updatedDecimals);
    QCOMPARE(dv.decimals(), updatedDecimals);
    QCOMPARE(dv.validate(input, dummy), updDecimalsState);
}

void tst_QDoubleValidator::setRangeOverloads_data()
{
    QTest::addColumn<QDoubleValidator::Notation>("notation");
    QTest::addColumn<int>("initialDecimals");
    QTest::addColumn<double>("minimum");
    QTest::addColumn<double>("maximum");
    QTest::addColumn<int>("updatedDecimals");
    QTest::addColumn<QString>("input");
    QTest::addColumn<QValidator::State>("initDecimalsState");
    QTest::addColumn<QValidator::State>("updDecimalsState");

    QTest::newRow("scientific, 0 digits after point")
            << QDoubleValidator::ScientificNotation << -1 << -100.0 << 100.0 << 0
            << QString("1e1") << ACC << ACC;
    QTest::newRow("scientific, 1 digits after point")
            << QDoubleValidator::ScientificNotation << -1 << -100.0 << 100.0 << 0
            << QString("1.2e1") << ACC << INV;
    QTest::newRow("scientific, 3 digits after point, demand fewer")
            << QDoubleValidator::ScientificNotation << 3 << -100.0 << 100.0 << 1
            << QString("10.234e-1") << ACC << INV;
    QTest::newRow("scientific, 3 digits after point, not in range")
            << QDoubleValidator::ScientificNotation << 3 << -100.0 << 100.0 << 5
            << QString("1.234e3") << ITM << ITM;
    QTest::newRow("standard, 0 digits after point")
            << QDoubleValidator::StandardNotation << -1 << -100.0 << 100.0 << 0
            << QString("12.") << ACC << ACC;
    QTest::newRow("standard, 2 digits after point")
            << QDoubleValidator::StandardNotation << -1 << -100.0 << 100.0 << 1
            << QString("12.34") << ACC << INV;
    QTest::newRow("standard, 2 digits after point, not in range")
            << QDoubleValidator::StandardNotation << -1 << -100.0 << 100.0 << 1
            << QString("123.45") << ITM << INV;
    QTest::newRow("standard, 5 digits after point")
            << QDoubleValidator::StandardNotation << 5 << -100.0 << 100.0 << 3
            << QString("12.34567") << ACC << INV;
}

void tst_QDoubleValidator::validateIntEquiv_data()
{
    QTest::addColumn<double>("minimum");
    QTest::addColumn<double>("maximum");
    QTest::addColumn<QString>("input");
    QTest::addColumn<QValidator::State>("state");

    QTest::newRow("1.1") << 0.0 << 10.0 << QString("") << ITM;
    QTest::newRow("1.2") << 10.0 << 0.0 << QString("") << ITM;

    QTest::newRow("2.1") << 0.0 << 10.0 << QString("-") << INV;
    QTest::newRow("2.2") << 0.0 << 10.0 << QString("-0") << INV;
    QTest::newRow("2.3") << -10.0 << -1.0 << QString("+") << INV;
    QTest::newRow("2.4") << -10.0 << 10.0 << QString("-") << ITM;
    QTest::newRow("2.5") << -10.0 << 10.0 << QString("+") << ITM;
    QTest::newRow("2.5a") << -10.0 << -9.0 << QString("+") << INV;
    QTest::newRow("2.6") << -10.0 << 10.0 << QString("+0") << ACC;
    QTest::newRow("2.7") << -10.0 << 10.0 << QString("+1") << ACC;
    QTest::newRow("2.8") << -10.0 << 10.0 << QString("+-") << INV;
    QTest::newRow("2.9") << -10.0 << 10.0 << QString("-+") << INV;

    QTest::newRow("3.1") << 0.0 << 10.0 << QString("12345678901234567890") << INV;
    QTest::newRow("3.2") << 0.0 << 10.0 << QString("-12345678901234567890") << INV;
    QTest::newRow("3.3") << 0.0 << 10.0 << QString("000000000000000000000") << ACC;
    QTest::newRow("3.4") << 1.0 << 10.0 << QString("000000000000000000000") << ITM;
    QTest::newRow("3.5") << 0.0 << 10.0 << QString("-000000000000000000000") << INV;
    QTest::newRow("3.6") << -10.0 << -1.0 << QString("-000000000000000000000") << ITM;
    QTest::newRow("3.7") << -10.0 << -1.0 << QString("-0000000000000000000001") << ACC;

    QTest::newRow("4.1") << 0.0 << 10.0 << QString(" ") << INV;
    QTest::newRow("4.2") << 0.0 << 10.0 << QString(" 1") << INV;
    QTest::newRow("4.3") << 0.0 << 10.0 << QString("1 ") << INV;
    QTest::newRow("4.4") << 0.0 << 10.0 << QString("1.0") << INV;
    QTest::newRow("4.5") << 0.0 << 10.0 << QString("0.1") << INV;
    QTest::newRow("4.6") << 0.0 << 10.0 << QString(".1") << INV;
    QTest::newRow("4.7") << 0.0 << 10.0 << QString("-1.0") << INV;

    QTest::newRow("5.1a") << 6.0 << 8.0 << QString("5") << ITM;
    QTest::newRow("5.1b") << 6.0 << 8.0 << QString("56") << INV;
    QTest::newRow("5.2") << 6.0 << 8.0 << QString("7") << ACC;
    QTest::newRow("5.3a") << 6.0 << 8.0 << QString("9") << ITM;
    QTest::newRow("5.3b") << 6.0 << 8.0 << QString("-") << INV;
    QTest::newRow("5.4a") << -8.0 << -6.0 << QString("+") << INV;
    QTest::newRow("5.4b") << -8.0 << -6.0 << QString("+5") << INV;
    QTest::newRow("5.4c") << -8.0 << -6.0 << QString("-5") << ITM;
    QTest::newRow("5.5") << -8.0 << -6.0 << QString("-7") << ACC;
    QTest::newRow("5.6") << -8.0 << -6.0 << QString("-9") << ITM;
    QTest::newRow("5.7") << -8.0 << -6.0 << QString("5") << ITM;
    QTest::newRow("5.8") << -8.0 << -6.0 << QString("7") << ITM;
    QTest::newRow("5.9") << -8.0 << -6.0 << QString("9") << ITM;
    QTest::newRow("5.10") << -6.0 << 8.0 << QString("-5") << ACC;
    QTest::newRow("5.11") << -6.0 << 8.0 << QString("5") << ACC;
    QTest::newRow("5.12") << -6.0 << 8.0 << QString("-7") << ITM;
    QTest::newRow("5.13") << -6.0 << 8.0 << QString("7") << ACC;
    QTest::newRow("5.14") << -6.0 << 8.0 << QString("-9") << ITM;
    QTest::newRow("5.15") << -6.0 << 8.0 << QString("9") << ITM;

    QTest::newRow("6.1") << 100.0 << 102.0 << QString("11") << ITM;
    QTest::newRow("6.2") << 100.0 << 102.0 << QString("-11") << INV;

    QTest::newRow("7.1") << 0.0 << 10.0 << QString("100") << INV;
    QTest::newRow("7.2") << 0.0 << -10.0 << QString("100") << INV;
    QTest::newRow("7.3") << 0.0 << -10.0 << QString("-100") << INV;
    QTest::newRow("7.4") << -100.0 << 10.0 << QString("100") << ITM;
}

void tst_QDoubleValidator::validateIntEquiv()
{
    QFETCH(double, minimum);
    QFETCH(double, maximum);
    QFETCH(QString, input);
    QFETCH(QValidator::State, state);

    QLocale::setDefault(QLocale("C"));

    QDoubleValidator dv(minimum, maximum, 0, 0);
    dv.setNotation(QDoubleValidator::StandardNotation);
    int dummy;
    QCOMPARE(dv.validate(input, dummy), state);
}

QTEST_APPLESS_MAIN(tst_QDoubleValidator)
#include "tst_qdoublevalidator.moc"
