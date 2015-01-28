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


#include <QtCore/QCoreApplication>
#include <QtTest/QtTest>

class tst_StrCmp: public QObject
{
    Q_OBJECT

private slots:
    void compareCharStars() const;
    void compareByteArray() const;
    void failByteArray() const;
    void failByteArrayNull() const;
    void failByteArrayEmpty() const;
    void failByteArraySingleChars() const;
};

void tst_StrCmp::compareCharStars() const
{
    QCOMPARE((const char *)"foo", (const char *)"foo");

    const char *str1 = "foo";
    QCOMPARE("foo", str1);
    QCOMPARE(str1, "foo");
    QCOMPARE(str1, str1);

    char *str2 = const_cast<char *>("foo");
    QCOMPARE("foo", str2);
    QCOMPARE(str2, "foo");
    QCOMPARE(str2, str2);
    QCOMPARE(str1, str2);
    QCOMPARE(str2, str1);

    const char str3[] = "foo";
    QCOMPARE((const char *)str3, "foo");
    QCOMPARE("foo", (const char *)str3);
    QCOMPARE((const char *)str3, str1);
    QCOMPARE((const char *)str3, str2);
    QCOMPARE(str1, (const char *)str3);
    QCOMPARE(str2, (const char *)str3);
}

void tst_StrCmp::compareByteArray() const
{
    QByteArray ba = "foo";
    QEXPECT_FAIL("", "Next test should fail", Continue);
    QCOMPARE(ba.constData(), "bar");
    QCOMPARE(ba.constData(), "foo");

    char *bar = const_cast<char *>("bar");
    char *foo = const_cast<char *>("foo");

    QEXPECT_FAIL("", "Next test should fail", Continue);
    QCOMPARE(ba.data(), bar);
    QCOMPARE(ba.data(), foo);

    const char *cbar = "bar";
    const char *cfoo = "foo";

    QEXPECT_FAIL("", "Next test should fail", Continue);
    QCOMPARE(ba.constData(), cbar);
    QCOMPARE(ba.constData(), cfoo);

    /* Create QByteArrays of the size that makes the corresponding toString() crop output. */
    const QByteArray b(500, 'A');
    const QByteArray a(500, 'B');

    QCOMPARE(a, b);
}

void tst_StrCmp::failByteArray() const
{
    /* Compare small, different byte arrays. */
    QCOMPARE(QByteArray("abc"), QByteArray("cba"));
}

void tst_StrCmp::failByteArrayNull() const
{
    /* Compare null byte array against with content. */
    QCOMPARE(QByteArray("foo"), QByteArray());
}

void tst_StrCmp::failByteArrayEmpty() const
{
    QCOMPARE(QByteArray(""), QByteArray("foo"));
}

void tst_StrCmp::failByteArraySingleChars() const
{
    /* Compare null byte array against with content. */
    //QCOMPARE(QString(250, 'a'), QString(250, 'b'));
    QCOMPARE(QByteArray("6"), QByteArray("7"));
}

QTEST_MAIN(tst_StrCmp)

#include "tst_strcmp.moc"
