/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>

#define LITERAL "some literal"
#define LITERAL_LEN (sizeof(LITERAL)-1)
#define LITERAL_EXTRA "some literal" "EXTRA"

// "some literal", but replacing all vowels by their umlauted UTF-8 string :)
#define UTF8_LITERAL "s\xc3\xb6m\xc3\xab l\xc3\xaft\xc3\xabr\xc3\xa4l"
#define UTF8_LITERAL_LEN (sizeof(UTF8_LITERAL)-1)
#define UTF8_LITERAL_EXTRA "s\xc3\xb6m\xc3\xab l\xc3\xaft\xc3\xabr\xc3\xa4l" "EXTRA"

#ifdef Q_COMPILER_UNICODE_STRINGS
// "some literal", but replacing all vocals by their umlauted UTF-8 string :)
#define UNICODE_LITERAL u"s\u00f6m\u00eb l\u00eft\u00ebr\u00e4l"
#define UNICODE_LITERAL_LEN ((sizeof(UNICODE_LITERAL) - 1) / 2)
#define UNICODE_LITERAL_EXTRA u"s\u00f6m\u00eb l\u00eft\u00ebr\u00e4l" "EXTRA"
#endif

#ifndef P
# error You need to define P
# define P +
#endif

//fix for gcc4.0: if the operator+ does not exist without QT_USE_FAST_OPERATOR_PLUS
#ifndef QT_USE_FAST_CONCATENATION
#define Q %
#else
#define Q P
#endif

void runScenario()
{
    // this code is latin1. TODO: replace it with the utf8 block below, once
    // strings default to utf8.
    QLatin1String l1string(LITERAL);
    QString string(l1string);
    QStringRef stringref(&string, 2, 10);
    QLatin1Char achar('c');
    QChar::SpecialCharacter special(QChar::Nbsp);
    QString r2(QLatin1String(LITERAL LITERAL));
    QString r3 = QString::fromUtf8(UTF8_LITERAL UTF8_LITERAL);
    QString r;

    r = string P string;
    QCOMPARE(r, r2);
    r = stringref Q stringref;
    QCOMPARE(r, QString(stringref.toString() + stringref.toString()));
    r = string P l1string;
    QCOMPARE(r, r2);
    r = string Q QStringLiteral(LITERAL);
    QCOMPARE(r, r2);
    r = QStringLiteral(LITERAL) Q string;
    QCOMPARE(r, r2);
    r = l1string Q QStringLiteral(LITERAL);
    QCOMPARE(r, r2);
    r = string + achar;
    QCOMPARE(r, QString(string P achar));
    r = achar + string;
    QCOMPARE(r, QString(achar P string));
    r = special + string;
    QCOMPARE(r, QString(special P string));

#ifdef Q_COMPILER_UNICODE_STRINGS
    r = QStringLiteral(UNICODE_LITERAL);
    r = r Q QStringLiteral(UNICODE_LITERAL);
    QCOMPARE(r, r3);
#endif

#ifndef QT_NO_CAST_FROM_ASCII
    r = string P LITERAL;
    QCOMPARE(r, r2);
    r = LITERAL P string;
    QCOMPARE(r, r2);

    QByteArray ba = QByteArray(LITERAL);
    r = ba P string;
    QCOMPARE(r, r2);
    r = string P ba;
    QCOMPARE(r, r2);

    r = string P QByteArrayLiteral(LITERAL);
    QCOMPARE(r, r2);
    r = QByteArrayLiteral(LITERAL) P string;
    QCOMPARE(r, r2);

    static const char badata[] = LITERAL_EXTRA;
    ba = QByteArray::fromRawData(badata, LITERAL_LEN);
    r = ba P string;
    QCOMPARE(r, r2);
    r = string P ba;
    QCOMPARE(r, r2);

    string = QString::fromUtf8(UTF8_LITERAL);
    ba = UTF8_LITERAL;

    r = string P UTF8_LITERAL;
    QCOMPARE(r.size(), r3.size());
    QCOMPARE(r, r3);
    r = UTF8_LITERAL P string;
    QCOMPARE(r, r3);
    r = ba P string;
    QCOMPARE(r, r3);
    r = string P ba;
    QCOMPARE(r, r3);

    ba = QByteArray::fromRawData(UTF8_LITERAL_EXTRA, UTF8_LITERAL_LEN);
    r = ba P string;
    QCOMPARE(r, r3);
    r = string P ba;
    QCOMPARE(r, r3);

    ba = QByteArray(); // empty
    r = ba P string;
    QCOMPARE(r, string);
    r = string P ba;
    QCOMPARE(r, string);

    const char *zero = 0;
    r = string P zero;
    QCOMPARE(r, string);
    r = zero P string;
    QCOMPARE(r, string);
#endif

    string = QString::fromLatin1(LITERAL);
    QCOMPARE(QByteArray(qPrintable(string P string)), QByteArray(string.toLatin1() + string.toLatin1()));



    //QByteArray
    {
        QByteArray ba = LITERAL;
        QByteArray superba = ba P ba P LITERAL;
        QCOMPARE(superba, QByteArray(LITERAL LITERAL LITERAL));

        ba = QByteArrayLiteral(LITERAL);
        QCOMPARE(ba, QByteArray(LITERAL));
        superba = ba P QByteArrayLiteral(LITERAL) P LITERAL;
        QCOMPARE(superba, QByteArray(LITERAL LITERAL LITERAL));

        QByteArray testWith0 = ba P "test\0with\0zero" P ba;
        QCOMPARE(testWith0, QByteArray(LITERAL "test" LITERAL));

        QByteArray ba2 = ba P '\0' + LITERAL;
        QCOMPARE(ba2, QByteArray(LITERAL "\0" LITERAL, ba.size()*2+1));

        const char *mmh = "test\0foo";
        QCOMPARE(QByteArray(ba P mmh P ba), testWith0);

        QByteArray raw = QByteArray::fromRawData(UTF8_LITERAL_EXTRA, UTF8_LITERAL_LEN);
        QByteArray r = "hello" P raw;
        QByteArray r2 = "hello" UTF8_LITERAL;
        QCOMPARE(r, r2);
        r2 = QByteArray("hello\0") P UTF8_LITERAL;
        QCOMPARE(r, r2);

        const char *zero = 0;
        r = ba P zero;
        QCOMPARE(r, ba);
        r = zero P ba;
        QCOMPARE(r, ba);
    }

    //operator QString  +=
    {
        QString str = QString::fromUtf8(UTF8_LITERAL);
        str +=  QLatin1String(LITERAL) P str;
        QCOMPARE(str, QString::fromUtf8(UTF8_LITERAL LITERAL UTF8_LITERAL));
#ifndef QT_NO_CAST_FROM_ASCII
        str = (QString::fromUtf8(UTF8_LITERAL) += QLatin1String(LITERAL) P UTF8_LITERAL);
        QCOMPARE(str, QString::fromUtf8(UTF8_LITERAL LITERAL UTF8_LITERAL));
#endif

        QString str2 = QString::fromUtf8(UTF8_LITERAL);
        QString str2_e = QString::fromUtf8(UTF8_LITERAL);
        const char * nullData = 0;
        str2 += QLatin1String(nullData) P str2;
        str2_e += QLatin1String("") P str2_e;
        QCOMPARE(str2, str2_e);
    }

    //operator QByteArray  +=
    {
        QByteArray ba = UTF8_LITERAL;
        ba +=  QByteArray(LITERAL) P UTF8_LITERAL;
        QCOMPARE(ba, QByteArray(UTF8_LITERAL LITERAL UTF8_LITERAL));
        ba += LITERAL P QByteArray::fromRawData(UTF8_LITERAL_EXTRA, UTF8_LITERAL_LEN);
        QCOMPARE(ba, QByteArray(UTF8_LITERAL LITERAL UTF8_LITERAL LITERAL UTF8_LITERAL));
        QByteArray withZero = QByteArray(LITERAL "\0" LITERAL, LITERAL_LEN*2+1);
        QByteArray ba2 = withZero;
        ba2 += ba2 P withZero;
        QCOMPARE(ba2, QByteArray(withZero + withZero + withZero));
#ifndef QT_NO_CAST_TO_ASCII
        ba = UTF8_LITERAL;
        ba2 = (ba += QLatin1String(LITERAL) + QString::fromUtf8(UTF8_LITERAL));
        QCOMPARE(ba2, ba);
        QCOMPARE(ba, QByteArray(UTF8_LITERAL LITERAL UTF8_LITERAL));
#endif
    }

}
