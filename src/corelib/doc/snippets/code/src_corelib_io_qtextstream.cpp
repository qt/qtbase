// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QFile data("output.txt");
if (data.open(QFile::WriteOnly | QFile::Truncate)) {
    QTextStream out(&data);
    out << "Result: " << qSetFieldWidth(10) << left << 3.14 << 2.7;
    // writes "Result: 3.14      2.7       "
}
//! [0]


//! [1]
QTextStream stream(stdin);
QString line;
while (stream.readLineInto(&line)) {
    ...
}
//! [1]


//! [2]
QTextStream in("0x50 0x20");
int firstNumber, secondNumber;

in >> firstNumber;             // firstNumber == 80
in >> dec >> secondNumber;     // secondNumber == 0

char ch;
in >> ch;                      // ch == 'x'
//! [2]


//! [3]
int main(int argc, char *argv[])
{
    // read numeric arguments (123, 0x20, 4.5...)
    for (int i = 1; i < argc; ++i) {
          int number;
          QTextStream in(argv[i]);
          in >> number;
          ...
    }
}
//! [3]


//! [4]
QString str;
QTextStream in(stdin);
in >> str;
//! [4]


//! [5]
QString s;
QTextStream out(&s);
out.setFieldWidth(10);
out.setFieldAlignment(QTextStream::AlignCenter);
out.setPadChar('-');
out << "Qt" << "rocks!";
//! [5]


//! [6]
----Qt------rocks!--
//! [6]


//! [7]
QTextStream in(file);
QChar ch1, ch2, ch3;
in >> ch1 >> ch2 >> ch3;
//! [7]


//! [8]
QTextStream out(stdout);
out << "Qt rocks!" << Qt::endl;
//! [8]


//! [9]
stream << '\n' << Qt::flush;
//! [9]
