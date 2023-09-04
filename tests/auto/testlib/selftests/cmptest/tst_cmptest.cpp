// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <QtCore/QCoreApplication>
#include <QtCore/QTimer>
#ifdef QT_GUI_LIB
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtGui/QPalette>
#include <QtGui/QPixmap>
#include <QtGui/QVector2D>
#include <QtGui/QVector3D>
#include <QtGui/QVector4D>
#endif
#include <QSet>
#include <vector>
using namespace Qt::StringLiterals;

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
    void test_windowflags_data();
    void test_windowflags();
    void test_unregistered_flags_data();
    void test_unregistered_flags();
    void compare_boolfuncs();
    void compare_to_nullptr();
    void compare_pointerfuncs();
    void compare_tostring();
    void compare_tostring_data();
    void compare_unknown();
    void compare_textFromDebug();
    void compareQObjects();
    void compareQStringLists();
    void compareQStringLists_data();
    void compareQListInt_data();
    void compareQListInt();
    void compareQListIntToArray_data();
    void compareQListIntToArray();
    void compareQListIntToInitializerList_data();
    void compareQListIntToInitializerList();
    void compareQListDouble();
    void compareContainerToInitializerList();
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
    void compareQPalettes_data();
    void compareQPalettes();
#endif
    void tryCompare();
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

void tst_Cmptest::test_windowflags_data()
{
    QTest::addColumn<Qt::WindowFlags>("actualWindowFlags");
    QTest::addColumn<Qt::WindowFlags>("expectedWindowFlags");

    const Qt::WindowFlags windowFlags = Qt::Window
        | Qt::WindowSystemMenuHint | Qt::WindowStaysOnBottomHint;
    QTest::newRow("pass")
        << windowFlags
        << windowFlags;
    QTest::newRow("fail1")
        << windowFlags
        << (windowFlags | Qt::FramelessWindowHint);
    QTest::newRow("fail2")
        << Qt::WindowFlags(Qt::Window)
        << Qt::WindowFlags(Qt::Window | Qt::FramelessWindowHint);
}

void tst_Cmptest::test_windowflags()
{
    QFETCH(Qt::WindowFlags, actualWindowFlags);
    QFETCH(Qt::WindowFlags, expectedWindowFlags);
    QCOMPARE(actualWindowFlags, expectedWindowFlags);
}

enum UnregisteredEnum {
    UnregisteredEnumValue1 = 0x1,
    UnregisteredEnumValue2 = 0x2,
    UnregisteredEnumValue3 = 0x4
};

typedef QFlags<UnregisteredEnum> UnregisteredFlags;

Q_DECLARE_METATYPE(UnregisteredFlags);

void tst_Cmptest::test_unregistered_flags_data()
{
    QTest::addColumn<UnregisteredFlags>("actualFlags");
    QTest::addColumn<UnregisteredFlags>("expectedFlags");

    QTest::newRow("pass")
        << UnregisteredFlags(UnregisteredEnumValue1)
        << UnregisteredFlags(UnregisteredEnumValue1);
    QTest::newRow("fail1")
        << UnregisteredFlags(UnregisteredEnumValue1 | UnregisteredEnumValue2)
        << UnregisteredFlags(UnregisteredEnumValue1 | UnregisteredEnumValue3);
    QTest::newRow("fail2")
        << UnregisteredFlags(UnregisteredEnumValue1)
        << UnregisteredFlags(UnregisteredEnumValue1 | UnregisteredEnumValue3);
}

void tst_Cmptest::test_unregistered_flags()
{
    QFETCH(UnregisteredFlags, actualFlags);
    QFETCH(UnregisteredFlags, expectedFlags);
    QCOMPARE(actualFlags, expectedFlags);
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
T *null() noexcept { return nullptr; }
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

void tst_Cmptest::compareQObjects()
{
    QObject object1;
    object1.setObjectName(QStringLiteral("object1"));
    QObject object2;
    object2.setObjectName(QStringLiteral("object2"));
    QCOMPARE(&object1, &object1);
    QCOMPARE(&object1, &object2);
    QCOMPARE(&object1, nullptr);
    QCOMPARE(nullptr, &object2);
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
        << QVariant(QMetaType(QMetaType::QVariantHash))
        << QVariant()
    ;

    QTest::newRow("string, null user type")
        << QVariant::fromValue(QString::fromLatin1("A simple string"))
        << QVariant(QMetaType::fromType<PhonyClass>())
    ;

    PhonyClass fake1 = {1};
    PhonyClass fake2 = {2};
    QTest::newRow("both non-null user type")
        << QVariant(QMetaType::fromType<PhonyClass>(), (const void*)&fake1)
        << QVariant(QMetaType::fromType<PhonyClass>(), (const void*)&fake2)
    ;
}

void tst_Cmptest::compare_tostring()
{
    QFETCH(QVariant, actual);
    QFETCH(QVariant, expected);

    QCOMPARE(actual, expected);
}

struct UnknownType
{
    int value;
    bool operator==(const UnknownType &rhs) const { return value == rhs.value; }
};

void tst_Cmptest::compare_unknown()
{
    UnknownType a{1};
    UnknownType b{2};

    QCOMPARE(a, b);
}

struct CustomType
{
    int value;
    bool operator==(const CustomType &rhs) const { return value == rhs.value; }
};

QDebug operator<<(QDebug dbg, const CustomType &val)
{
    dbg << "QDebug stream: " << val.value;
    return dbg;
}

void tst_Cmptest::compare_textFromDebug()
{
    CustomType a{0};
    CustomType b{1};

    QCOMPARE(a, b);
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

using IntList = QList<int>;

void tst_Cmptest::compareQListInt_data()
{
      QTest::addColumn<IntList>("actual");

      QTest::newRow("match") << IntList{1, 2, 3};
      QTest::newRow("size mismatch") << IntList{1, 2};
      QTest::newRow("value mismatch") << IntList{1, 2, 4};
}

void tst_Cmptest::compareQListInt()
{
    QFETCH(IntList, actual);
    const QList<int> expected{1, 2, 3};
    QCOMPARE(actual, expected);
}

void tst_Cmptest::compareQListIntToArray_data()
{
    compareQListInt_data();
}

void tst_Cmptest::compareQListIntToArray()
{
    QFETCH(IntList, actual);
    const int expected[] = {1, 2, 3};
    QCOMPARE(actual, expected);
}

void tst_Cmptest::compareQListIntToInitializerList_data()
{
    compareQListInt_data();
}

void tst_Cmptest::compareQListIntToInitializerList()
{
    QFETCH(IntList, actual);
    // Protect ',' in the list
#define ARG(...) __VA_ARGS__
    QCOMPARE(actual,  ARG({1, 2, 3}));
#undef ARG
}

void tst_Cmptest::compareQListDouble()
{
    QList<double> double1; double1 << 1.5 << 2 << 3;
    QList<double> double2; double2 << 1 << 2 << 4;
    QCOMPARE(double1, double2);
}

void tst_Cmptest::compareContainerToInitializerList()
{
    // Protect ',' in the list
#define ARG(...) __VA_ARGS__
    QSet<int> set{1, 2, 3};
    QCOMPARE(set, ARG({1, 2, 3}));

    std::vector<int> vec{1, 2, 3};
    QCOMPARE(vec, ARG({1, 2, 3}));

    vec.clear();
    QCOMPARE(vec, {});

    vec.push_back(42);
    QCOMPARE(vec, {42});
#undef ARG
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
    QPixmap pixmapWrongDpr = pixmap1.scaled(2, 2);
    pixmapWrongDpr.setDevicePixelRatio(2);

    QTest::newRow("both null") << QPixmap() << QPixmap();
    QTest::newRow("one null") << QPixmap() << pixmap1;
    QTest::newRow("other null") << pixmap1 << QPixmap();
    QTest::newRow("equal") << pixmap1 << pixmap1;
    QTest::newRow("different size") << pixmap1 << pixmap3;
    QTest::newRow("different pixels") << pixmap1 << pixmap2;
    QTest::newRow("different dpr") << pixmap1 << pixmapWrongDpr;
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
    QImage imageWrongDpr = image1.scaled(2, 2);
    imageWrongDpr.setDevicePixelRatio(2);

    QTest::newRow("both null") << QImage() << QImage();
    QTest::newRow("one null") << QImage() << image1;
    QTest::newRow("other null") << image1 << QImage();
    QTest::newRow("equal") << image1 << image1;
    QTest::newRow("different size") << image1 << image3;
    QTest::newRow("different format") << image1 << image1Indexed;
    QTest::newRow("different pixels") << image1 << image2;
    QTest::newRow("different dpr") << image1 << imageWrongDpr;
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
    const QList<QRect> list2 = QList<QRect>() << QRect(QPoint(100, 200), QSize(50, 200)) << rect1;
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

void tst_Cmptest::compareQPalettes_data()
{
    QTest::addColumn<QPalette>("actualPalette");
    QTest::addColumn<QPalette>("expectedPalette");

    // Initialize both to black, as the default palette values change
    // depending on whether the test is run directly from a shell
    // vs through generate_expected_output.py. We're not testing
    // the defaults, we're testing that the full output is printed
    // (QTBUG-5903 and QTBUG-87039).
    QPalette actualPalette;
    for (int i = 0; i < QPalette::NColorRoles; ++i) {
        const auto role = QPalette::ColorRole(i);
        actualPalette.setColor(QPalette::All, role, QColorConstants::Black);
    }
    QPalette expectedPalette;
    for (int i = 0; i < QPalette::NColorRoles; ++i) {
        const auto role = QPalette::ColorRole(i);
        expectedPalette.setColor(QPalette::All, role, QColorConstants::Black);
    }

    for (int i = 0; i < QPalette::NColorRoles; ++i) {
        const auto role = QPalette::ColorRole(i);
        const auto color = QColor::fromRgb(i);
        actualPalette.setColor(role, color);
    }
    QTest::newRow("all roles are different") << actualPalette << expectedPalette;

    for (int i = 0; i < QPalette::NColorRoles - 1; ++i) {
        const auto role = QPalette::ColorRole(i);
        const auto color = QColor::fromRgb(i);
        expectedPalette.setColor(role, color);
    }
    QTest::newRow("one role is different") << actualPalette << expectedPalette;

    const auto lastRole = QPalette::ColorRole(QPalette::NColorRoles - 1);
    expectedPalette.setColor(lastRole, QColor::fromRgb(lastRole));
    QTest::newRow("all roles are the same") << actualPalette << expectedPalette;
}

void tst_Cmptest::compareQPalettes()
{
    QFETCH(QPalette, actualPalette);
    QFETCH(QPalette, expectedPalette);

    QCOMPARE(actualPalette, expectedPalette);
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
    QVERIFY2(opaqueFunc() < 2,
             // Message with parenthetical part, to catch mis-parses of the
             // resulting message:
             u"%1 >= 2 (as expected, in fact)"_s.arg(opaqueFunc()).toUtf8().constData());
}

class DeferredFlag : public QObject // Can't be const.
{
    Q_OBJECT
    bool m_flag;
public:
    // A boolean that either starts out true or decays to true after 50 ms.
    // However, that decay will only happen when the event loop is run.
    explicit DeferredFlag(bool initial = false) : m_flag(initial)
    {
        if (!initial)
            QTimer::singleShot(50, this, &DeferredFlag::onTimeOut);
    }
    explicit operator bool() const { return m_flag; }
    bool operator!() const { return !m_flag; }
    friend bool operator==(const DeferredFlag &a, const DeferredFlag &b)
    {
        return bool(a) == bool(b);
    }
public slots:
    void onTimeOut() { m_flag = true; }
};

char *toString(const DeferredFlag &val)
{
    return qstrdup(bool(val) ? "DeferredFlag(true)" : "DeferredFlag(false)");
}

void tst_Cmptest::tryCompare()
{
    /* Note that expected values given as DeferredFlag() shall be re-evaluated
       each time the comparison is checked, hence supply a fresh false instance,
       that'll be discarded before it has a chance to decay, hence only compare
       equal to a false instance. Do not replace them with a local variable
       initialized to false, as it would (of course) decay.
    */
    DeferredFlag trueAlready(true);
    {
        DeferredFlag c;
        // QTRY should check before looping, so be equal to the fresh false immediately.
        QTRY_COMPARE(c, DeferredFlag());
        // Given time, it'll end up equal to a true one.
        QTRY_COMPARE(c, trueAlready);
    }
    {
        DeferredFlag c;
        QTRY_COMPARE_WITH_TIMEOUT(c, DeferredFlag(), 300);
        QVERIFY(!c); // Instantly equal, so succeeded without delay.
        QTRY_COMPARE_WITH_TIMEOUT(c, trueAlready, 200);
        qInfo("Should now time out and fail");
        QTRY_COMPARE_WITH_TIMEOUT(c, DeferredFlag(), 200);
    }
}

void tst_Cmptest::tryVerify()
{
    {
        DeferredFlag c;
        QTRY_VERIFY(!c);
        QTRY_VERIFY(c);
    }
    {
        DeferredFlag c;
        QTRY_VERIFY_WITH_TIMEOUT(!c, 300);
        QTRY_VERIFY_WITH_TIMEOUT(c, 200);
        qInfo("Should now time out and fail");
        QTRY_VERIFY_WITH_TIMEOUT(!c, 200);
    }
}

void tst_Cmptest::tryVerify2()
{
    {
        DeferredFlag c;
        QTRY_VERIFY2(!c, "Failed to check before looping");
        QTRY_VERIFY2(c, "Failed to trigger single-shot");
    }
    {
        DeferredFlag c;
        QTRY_VERIFY2_WITH_TIMEOUT(!c, "Failed to check before looping", 300);
        QTRY_VERIFY2_WITH_TIMEOUT(c, "Failed to trigger single-shot", 200);
        QTRY_VERIFY2_WITH_TIMEOUT(!c, "Should time out and fail", 200);
    }
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
