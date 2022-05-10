// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QLocale egyptian(QLocale::Arabic, QLocale::Egypt);
QString s1 = egyptian.toString(1.571429E+07, 'e');
QString s2 = egyptian.toString(10);

double d = egyptian.toDouble(s1);
int i = egyptian.toInt(s2);
//! [0]


//! [1]
bool ok;
double d;

QLocale::setDefault(QLocale::C);      // uses '.' as a decimal point
QLocale cLocale;                      // default-constructed C locale
d = cLocale.toDouble("1234,56", &ok); // ok == false, d == 0
d = cLocale.toDouble("1234.56", &ok); // ok == true,  d == 1234.56

QLocale::setDefault(QLocale::German); // uses ',' as a decimal point
QLocale german;                       // default-constructed German locale
d = german.toDouble("1234,56", &ok);  // ok == true,  d == 1234.56
d = german.toDouble("1234.56", &ok);  // ok == false, d == 0

QLocale::setDefault(QLocale::English);
// Default locale now uses ',' as a group separator.
QString str = QString("%1 %L2 %L3").arg(12345).arg(12345).arg(12345, 0, 16);
// str == "12345 12,345 3039"
//! [1]


//! [2]
QLocale korean("ko");
QLocale swiss("de_CH");
//! [2]


//! [3]
bool ok;
double d;

QLocale c(QLocale::C);
d = c.toDouble("1234.56", &ok);  // ok == true,  d == 1234.56
d = c.toDouble("1,234.56", &ok); // ok == true,  d == 1234.56
d = c.toDouble("1234,56", &ok);  // ok == false, d == 0

QLocale german(QLocale::German);
d = german.toDouble("1234,56", &ok);  // ok == true,  d == 1234.56
d = german.toDouble("1.234,56", &ok); // ok == true,  d == 1234.56
d = german.toDouble("1234.56", &ok);  // ok == false, d == 0

d = german.toDouble("1.234", &ok);    // ok == true,  d == 1234.0
//! [3]

//! [3-qstringview]
bool ok;
double d;

QLocale c(QLocale::C);
d = c.toDouble(u"1234.56", &ok);  // ok == true,  d == 1234.56
d = c.toDouble(u"1,234.56", &ok); // ok == true,  d == 1234.56
d = c.toDouble(u"1234,56", &ok);  // ok == false, d == 0

QLocale german(QLocale::German);
d = german.toDouble(u"1234,56", &ok);  // ok == true,  d == 1234.56
d = german.toDouble(u"1.234,56", &ok); // ok == true,  d == 1234.56
d = german.toDouble(u"1234.56", &ok);  // ok == false, d == 0

d = german.toDouble(u"1.234", &ok);    // ok == true,  d == 1234.0
//! [3-qstringview]
