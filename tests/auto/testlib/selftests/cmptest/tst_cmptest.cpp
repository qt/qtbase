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


#include <QtCore/QCoreApplication>
#include <QtTest/QtTest>
#ifdef QT_GUI_LIB
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtGui/QPixmap>
#include <QtGui/QVector2D>
#include <QtGui/QVector3D>
#include <QtGui/QVector4D>
#endif

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

public:
    enum class MyClassEnum { MyClassEnumValue1, MyClassEnumValue2 };
    Q_ENUM(MyClassEnum)

private slots:
    void compare_unregistered_enums();
    void compare_registered_enums();
    void compare_class_enums();
    void compare_boolfuncs();
    void compare_to_nullptr();
    void compare_pointerfuncs();
    void compare_tostring();
    void compare_tostring_data();
    void compareQStringLists();
    void compareQStringLists_data();
    void compareQListInt();
    void compareQListDouble();
#ifdef QT_GUI_LIB
    void compareQColor_data();
    void compareQColor();
    void compareQPixmaps();
    void compareQPixmaps_data();
    void compareQImages();
    void compareQImages_data();
    void compareQRegion_data();
    void compareQRegion();
    void compareQVector2D();
    void compareQVector3D();
    void compareQVector4D();
#endif
    void verify();
    void verify2();
    void tryVerify();
    void tryVerify2();
    void verifyExplicitOperatorBool();
};

enum MyUnregisteredEnum { MyUnregisteredEnumValue1, MyUnregisteredEnumValue2 };

void tst_Cmptest::compare_unregistered_enums()
{
    QCOMPARE(MyUnregisteredEnumValue1, MyUnregisteredEnumValue1);
    QCOMPARE(MyUnregisteredEnumValue1, MyUnregisteredEnumValue2);
}

void tst_Cmptest::compare_registered_enums()
{
    // use an enum that doesn't start at 0
    QCOMPARE(Qt::Monday, Qt::Monday);
    QCOMPARE(Qt::Monday, Qt::Sunday);
}

void tst_Cmptest::compare_class_enums()
{
    QCOMPARE(MyClassEnum::MyClassEnumValue1, MyClassEnum::MyClassEnumValue1);
    QCOMPARE(MyClassEnum::MyClassEnumValue1, MyClassEnum::MyClassEnumValue2);
}

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

namespace {
template <typename T>
T *null() Q_DECL_NOTHROW { return nullptr; }
}

void tst_Cmptest::compare_to_nullptr()
{
    QCOMPARE(null<int>(), nullptr);
    QCOMPARE(null<const int>(), nullptr);
    QCOMPARE(null<volatile int>(), nullptr);
    QCOMPARE(null<const volatile int>(), nullptr);

    QCOMPARE(nullptr, null<int>());
    QCOMPARE(nullptr, null<const int>());
    QCOMPARE(nullptr, null<volatile int>());
    QCOMPARE(nullptr, null<const volatile int>());
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

#ifdef QT_GUI_LIB
void tst_Cmptest::compareQColor_data()
{
    QTest::addColumn<QColor>("colorA");
    QTest::addColumn<QColor>("colorB");

    QTest::newRow("Qt::yellow vs \"yellow\"") << QColor(Qt::yellow) << QColor(QStringLiteral("yellow"));
    QTest::newRow("Qt::yellow vs Qt::green") << QColor(Qt::yellow) << QColor(Qt::green);
    QTest::newRow("0x88ff0000 vs 0xffff0000") << QColor::fromRgba(0x88ff0000) << QColor::fromRgba(0xffff0000);
}

void tst_Cmptest::compareQColor()
{
    QFETCH(QColor, colorA);
    QFETCH(QColor, colorB);

    QCOMPARE(colorA, colorB);
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

void tst_Cmptest::compareQRegion_data()
{
    QTest::addColumn<QRegion>("rA");
    QTest::addColumn<QRegion>("rB");
    const QRect rect1(QPoint(10, 10), QSize(200, 50));
    const QRegion region1(rect1);
    QRegion listRegion2;
    const QVector<QRect> list2 = QVector<QRect>() << QRect(QPoint(100, 200), QSize(50, 200)) << rect1;
    listRegion2.setRects(list2.constData(), list2.size());
    QTest::newRow("equal-empty") << QRegion() << QRegion();
    QTest::newRow("1-empty") << region1 << QRegion();
    QTest::newRow("equal") << region1 << region1;
    QTest::newRow("different lists") << region1 << listRegion2;
}

void tst_Cmptest::compareQRegion()
{
    QFETCH(QRegion, rA);
    QFETCH(QRegion, rB);

    QCOMPARE(rA, rB);
}

void tst_Cmptest::compareQVector2D()
{
    QVector2D v2a{1, 2};
    QVector2D v2b = v2a;
    QCOMPARE(v2a, v2b);
    v2b.setY(3);
    QCOMPARE(v2a, v2b);
}

void tst_Cmptest::compareQVector3D()
{
    QVector3D v3a{1, 2, 3};
    QVector3D v3b = v3a;
    QCOMPARE(v3a, v3b);
    v3b.setY(3);
    QCOMPARE(v3a, v3b);
}

void tst_Cmptest::compareQVector4D()
{
    QVector4D v4a{1, 2, 3, 4};
    QVector4D v4b = v4a;
    QCOMPARE(v4a, v4b);
    v4b.setY(3);
    QCOMPARE(v4a, v4b);
}
#endif // QT_GUI_LIB

static int opaqueFunc()
{
    return 42;
}

void tst_Cmptest::verify()
{
    QVERIFY(opaqueFunc() > 2);
    QVERIFY(opaqueFunc() < 2);
}

void tst_Cmptest::verify2()
{
    QVERIFY2(opaqueFunc() > 2, QByteArray::number(opaqueFunc()).constData());
    QVERIFY2(opaqueFunc() < 2, QByteArray::number(opaqueFunc()).constData());
}

void tst_Cmptest::tryVerify()
{
    QTRY_VERIFY(opaqueFunc() > 2);
    QTRY_VERIFY_WITH_TIMEOUT(opaqueFunc() < 2, 1);
}

void tst_Cmptest::tryVerify2()
{
    QTRY_VERIFY2(opaqueFunc() > 2, QByteArray::number(opaqueFunc()).constData());
    QTRY_VERIFY2_WITH_TIMEOUT(opaqueFunc() < 2, QByteArray::number(opaqueFunc()).constData(), 1);
}

void tst_Cmptest::verifyExplicitOperatorBool()
{
    struct ExplicitOperatorBool {
        int m_i;
        explicit ExplicitOperatorBool(int i) : m_i(i) {}
        explicit operator bool() const { return m_i > 0; }
        bool operator !() const { return !bool(*this); }
    };

    ExplicitOperatorBool val1(42);
    QVERIFY(val1);

    ExplicitOperatorBool val2(-273);
    QVERIFY(!val2);
}

QTEST_MAIN(tst_Cmptest)
#include "tst_cmptest.moc"
