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
#include <QtTest/QtTest>
#include <QChar>

class tst_QChar: public QObject
{
    Q_OBJECT
private slots:
    void isUpper_data();
    void isUpper();
    void isLower_data();
    void isLower();
    void isLetter_data();
    void isLetter();
    void isDigit_data();
    void isDigit();
    void isLetterOrNumber_data();
    void isLetterOrNumber();
    void isSpace_data();
    void isSpace();
};

void tst_QChar::isUpper_data()
{
    QTest::addColumn<QChar>("c");

    QTest::newRow("k") << QChar('k');
    QTest::newRow("K") << QChar('K');
    QTest::newRow("5") << QChar('5');
    QTest::newRow("\\0") << QChar();
    QTest::newRow("space") << QChar(' ');
    QTest::newRow("\\u3C20") << QChar(0x3C20);
}

void tst_QChar::isUpper()
{
    QFETCH(QChar, c);
    QBENCHMARK {
        c.isUpper();
    }
}

void tst_QChar::isLower_data()
{
    isUpper_data();
}

void tst_QChar::isLower()
{
    QFETCH(QChar, c);
    QBENCHMARK {
        c.isLower();
    }
}

void tst_QChar::isLetter_data()
{
    isUpper_data();
}

void tst_QChar::isLetter()
{
    QFETCH(QChar, c);
    QBENCHMARK {
        c.isLetter();
    }
}

void tst_QChar::isDigit_data()
{
    isUpper_data();
}

void tst_QChar::isDigit()
{
    QFETCH(QChar, c);
    QBENCHMARK {
        c.isDigit();
    }
}

void tst_QChar::isLetterOrNumber_data()
{
    isUpper_data();
}

void tst_QChar::isLetterOrNumber()
{
    QFETCH(QChar, c);
    QBENCHMARK {
        c.isLetterOrNumber();
    }
}

void tst_QChar::isSpace_data()
{
    isUpper_data();
}

void tst_QChar::isSpace()
{
    QFETCH(QChar, c);
    QBENCHMARK {
        c.isSpace();
    }
}

QTEST_MAIN(tst_QChar)

#include "main.moc"
