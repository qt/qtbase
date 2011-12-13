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


#include <QtCore>
#include <QtTest/QtTest>

class tst_Cmptest: public QObject
{
    Q_OBJECT

private slots:
    void compare_boolfuncs();
    void compare_pointerfuncs();
    void compare_tostring();
    void compare_tostring_data();
    void compareQStringLists();
    void compareQStringLists_data();
};

static bool boolfunc() { return true; }
static bool boolfunc2() { return true; }

void tst_Cmptest::compare_boolfuncs()
{
    QCOMPARE(boolfunc(), boolfunc());
    QCOMPARE(boolfunc(), boolfunc2());
    QCOMPARE(!boolfunc(), !boolfunc2());
    QCOMPARE(boolfunc(), true);
    QCOMPARE(!boolfunc(), false);
}

static int i = 0;

static int *intptr() { return &i; }

void tst_Cmptest::compare_pointerfuncs()
{
    QCOMPARE(intptr(), intptr());
    QCOMPARE(&i, &i);
    QCOMPARE(intptr(), &i);
    QCOMPARE(&i, intptr());
}

Q_DECLARE_METATYPE(QVariant)

struct PhonyClass
{
    int i;
};

void tst_Cmptest::compare_tostring_data()
{
    QTest::addColumn<QVariant>("actual");
    QTest::addColumn<QVariant>("expected");

    QTest::newRow("int, string")
        << QVariant::fromValue(123)
        << QVariant::fromValue(QString("hi"))
    ;

    QTest::newRow("both invalid")
        << QVariant()
        << QVariant()
    ;

    QTest::newRow("null hash, invalid")
        << QVariant(QVariant::Hash)
        << QVariant()
    ;

    QTest::newRow("string, null user type")
        << QVariant::fromValue(QString::fromLatin1("A simple string"))
        << QVariant(QVariant::Type(qRegisterMetaType<PhonyClass>("PhonyClass")))
    ;

    PhonyClass fake1 = {1};
    PhonyClass fake2 = {2};
    QTest::newRow("both non-null user type")
        << QVariant(qRegisterMetaType<PhonyClass>("PhonyClass"), (const void*)&fake1)
        << QVariant(qRegisterMetaType<PhonyClass>("PhonyClass"), (const void*)&fake2)
    ;
}

void tst_Cmptest::compare_tostring()
{
    QFETCH(QVariant, actual);
    QFETCH(QVariant, expected);

    QCOMPARE(actual, expected);
}

void tst_Cmptest::compareQStringLists_data()
{
    QTest::addColumn<QStringList>("opA");
    QTest::addColumn<QStringList>("opB");

    {
        QStringList opA;
        opA.append(QLatin1String("string1"));
        opA.append(QLatin1String("string2"));

        QStringList opB(opA);
        opA.append(QLatin1String("string3"));
        opB.append(QLatin1String("DIFFERS"));

        QTest::newRow("last item different") << opA << opB;
    }

    {
        QStringList opA;
        opA.append(QLatin1String("string1"));
        opA.append(QLatin1String("string2"));

        QStringList opB(opA);
        opA.append(QLatin1String("string3"));
        opA.append(QLatin1String("string4"));

        opB.append(QLatin1String("DIFFERS"));
        opB.append(QLatin1String("string4"));

        QTest::newRow("second-last item different") << opA << opB;
    }

    {
        QStringList opA;
        opA.append(QLatin1String("string1"));
        opA.append(QLatin1String("string2"));

        QStringList opB;
        opB.append(QLatin1String("string1"));

        QTest::newRow("prefix") << opA << opB;
    }

    {
        QStringList opA;
        opA.append(QLatin1String("openInNewWindow"));
        opA.append(QLatin1String("openInNewTab"));
        opA.append(QLatin1String("separator"));
        opA.append(QLatin1String("bookmark_add"));
        opA.append(QLatin1String("savelinkas"));
        opA.append(QLatin1String("copylinklocation"));
        opA.append(QLatin1String("separator"));
        opA.append(QLatin1String("openWith_submenu"));
        opA.append(QLatin1String("preview1"));
        opA.append(QLatin1String("actions_submenu"));
        opA.append(QLatin1String("separator"));
        opA.append(QLatin1String("viewDocumentSource"));

        QStringList opB;
        opB.append(QLatin1String("viewDocumentSource"));

        QTest::newRow("short list second") << opA << opB;

        QTest::newRow("short list first") << opB << opA;
    }
}

void tst_Cmptest::compareQStringLists()
{
    QFETCH(QStringList, opA);
    QFETCH(QStringList, opB);

    QCOMPARE(opA, opB);
}

QTEST_MAIN(tst_Cmptest)
#include "tst_cmptest.moc"
