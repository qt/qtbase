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


#include <QtCore/QCoreApplication>
#include <QtTest/QtTest>
#include <QtGui/QImage>
#include <QtGui/QPixmap>

/* XPM test data for QPixmap, QImage tests (use drag cursors as example) */

static const char * const xpmPixmapData1[] = {
"11 20 3 1",
".        c None",
"a        c #FFFFFF",
"X        c #000000", // X11 cursor is traditionally black
"aa.........",
"aXa........",
"aXXa.......",
"aXXXa......",
"aXXXXa.....",
"aXXXXXa....",
"aXXXXXXa...",
"aXXXXXXXa..",
"aXXXXXXXXa.",
"aXXXXXXXXXa",
"aXXXXXXaaaa",
"aXXXaXXa...",
"aXXaaXXa...",
"aXa..aXXa..",
"aa...aXXa..",
"a.....aXXa.",
"......aXXa.",
".......aXXa",
".......aXXa",
"........aa."};

static const char * const xpmPixmapData2[] = {
"11 20 4 1",
".        c None",
"a        c #FFFFFF",
"b        c #0000FF",
"X        c #000000",
"aab........",
"aXab.......",
"aXXab......",
"aXXXab.....",
"aXXXXab....",
"aXXXXXab...",
"aXXXXXXab..",
"aXXXXXXXa..",
"aXXXXXXXXa.",
"aXXXXXXXXXa",
"aXXXXXXaaaa",
"aXXXaXXa...",
"aXXaaXXa...",
"aXa..aXXa..",
"aa...aXXa..",
"a.....aXXa.",
"......aXXa.",
".......aXXa",
".......aXXa",
"........aa."};

static const char * const xpmPixmapData3[] = {
"20 20 2 1",
"       c #000000",
".      c #C32D2D",
"          ..........",
"            ........",
"             .......",
"              ......",
"                ....",
"                  ..",
"                   .",
"                    ",
"                    ",
".                   ",
"...                 ",
".....               ",
"......              ",
".......             ",
".........           ",
"...........         ",
"...........         ",
"............        ",
"............        ",
".............       "};

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
    void compareQListInt();
    void compareQListDouble();
    void compareQPixmaps();
    void compareQPixmaps_data();
    void compareQImages();
    void compareQImages_data();
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
        QStringList opB(opA);

        QTest::newRow("empty lists") << opA << opB;
    }

    {
        QStringList opA;
        opA.append(QLatin1String("string1"));
        opA.append(QLatin1String("string2"));
        opA.append(QLatin1String("string3"));
        opA.append(QLatin1String("string4"));

        QStringList opB(opA);

        QTest::newRow("equal lists") << opA << opB;
    }

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

void tst_Cmptest::compareQListInt()
{
    QList<int> int1; int1 << 1 << 2 << 3;
    QList<int> int2; int2 << 1 << 2 << 4;
    QCOMPARE(int1, int2);
}

void tst_Cmptest::compareQListDouble()
{
    QList<double> double1; double1 << 1.5 << 2 << 3;
    QList<double> double2; double2 << 1 << 2 << 4;
    QCOMPARE(double1, double2);
}

void tst_Cmptest::compareQPixmaps_data()
{
    QTest::addColumn<QPixmap>("opA");
    QTest::addColumn<QPixmap>("opB");

    const QPixmap pixmap1(xpmPixmapData1);
    const QPixmap pixmap2(xpmPixmapData2);
    const QPixmap pixmap3(xpmPixmapData3);

    QTest::newRow("both null") << QPixmap() << QPixmap();
    QTest::newRow("one null") << QPixmap() << pixmap1;
    QTest::newRow("other null") << pixmap1 << QPixmap();
    QTest::newRow("equal") << pixmap1 << pixmap1;
    QTest::newRow("different size") << pixmap1 << pixmap3;
    QTest::newRow("different pixels") << pixmap1 << pixmap2;
}

void tst_Cmptest::compareQPixmaps()
{
    QFETCH(QPixmap, opA);
    QFETCH(QPixmap, opB);

    QCOMPARE(opA, opB);
}

void tst_Cmptest::compareQImages_data()
{
    QTest::addColumn<QImage>("opA");
    QTest::addColumn<QImage>("opB");

    const QImage image1(QPixmap(xpmPixmapData1).toImage());
    const QImage image2(QPixmap(xpmPixmapData2).toImage());
    const QImage image1Indexed = image1.convertToFormat(QImage::Format_Indexed8);
    const QImage image3(QPixmap(xpmPixmapData3).toImage());

    QTest::newRow("both null") << QImage() << QImage();
    QTest::newRow("one null") << QImage() << image1;
    QTest::newRow("other null") << image1 << QImage();
    QTest::newRow("equal") << image1 << image1;
    QTest::newRow("different size") << image1 << image3;
    QTest::newRow("different format") << image1 << image1Indexed;
    QTest::newRow("different pixels") << image1 << image2;
}

void tst_Cmptest::compareQImages()
{
    QFETCH(QImage, opA);
    QFETCH(QImage, opB);

    QCOMPARE(opA, opB);
}

QTEST_MAIN(tst_Cmptest)
#include "tst_cmptest.moc"
