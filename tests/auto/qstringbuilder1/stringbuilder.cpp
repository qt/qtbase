/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#define LITERAL "some literal"
#define LITERAL_LEN (sizeof(LITERAL)-1)
#define LITERAL_EXTRA "some literal" "EXTRA"

// "some literal", but replacing all vocals by their umlauted UTF-8 string :)
#define UTF8_LITERAL "s\xc3\xb6m\xc3\xab l\xc3\xaft\xc3\xabr\xc3\xa4l"
#define UTF8_LITERAL_LEN (sizeof(UTF8_LITERAL)-1)
#define UTF8_LITERAL_EXTRA "s\xc3\xb6m\xc3\xab l\xc3\xaft\xc3\xabr\xc3\xa4l" "EXTRA"


//fix for gcc4.0: if the operator+ does not exist without QT_USE_FAST_OPERATOR_PLUS
#ifndef QT_USE_FAST_CONCATENATION
#define Q %
#else
#define Q P
#endif

void runScenario()
{
    // set codec for C strings to 0, enforcing Latin1
    QTextCodec::setCodecForCStrings(0);
    QVERIFY(!QTextCodec::codecForCStrings());

    QLatin1Literal l1literal(LITERAL);
    QLatin1String l1string(LITERAL);
    QString string(l1string);
    QStringRef stringref(&string, 2, 10);
    QLatin1Char achar('c');
    QString r2(QLatin1String(LITERAL LITERAL));
    QString r;

    r = l1literal Q l1literal;
    QCOMPARE(r, r2);
    r = string P string;
    QCOMPARE(r, r2);
    r = stringref Q stringref;
    QCOMPARE(r, QString(stringref.toString() + stringref.toString()));
    r = string Q l1literal;
    QCOMPARE(r, r2);
    r = string P l1string;
    QCOMPARE(r, r2);
    r = string + achar;
    QCOMPARE(r, QString(string P achar));
    r = achar + string;
    QCOMPARE(r, QString(achar P string));
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

    static const char badata[] = LITERAL_EXTRA;
    ba = QByteArray::fromRawData(badata, LITERAL_LEN);
    r = ba P string;
    QCOMPARE(r, r2);
    r = string P ba;
    QCOMPARE(r, r2);

    // now test with codec for C strings set
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
    QVERIFY(QTextCodec::codecForCStrings());
    QCOMPARE(QTextCodec::codecForCStrings()->name(), QByteArray("UTF-8"));

    string = QString::fromUtf8(UTF8_LITERAL);
    r2 = QString::fromUtf8(UTF8_LITERAL UTF8_LITERAL);
    ba = UTF8_LITERAL;

    r = string P UTF8_LITERAL;
    QCOMPARE(r.size(), r2.size());
    QCOMPARE(r, r2);
    r = UTF8_LITERAL P string;
    QCOMPARE(r, r2);
    r = ba P string;
    QCOMPARE(r, r2);
    r = string P ba;
    QCOMPARE(r, r2);

    ba = QByteArray::fromRawData(UTF8_LITERAL_EXTRA, UTF8_LITERAL_LEN);
    r = ba P string;
    QCOMPARE(r, r2);
    r = string P ba;
    QCOMPARE(r, r2);

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
