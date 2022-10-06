// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//! [0]
QVERIFY(spy.isValid());
//! [0]

//! [13]
QTest::keyClick(myWidget, 'a');
//! [13]

//! [14]
QTest::keyClick(myWidget, Qt::Key_Escape);

QTest::keyClick(myWidget, Qt::Key_Escape, Qt::ShiftModifier, 200);
//! [14]

//! [15]
QTest::keyClicks(myWidget, "hello world");
//! [15]

//! [16]
namespace QTest {
    template<>
    char *toString(const MyPoint &point)
    {
        const QByteArray ba("MyPoint("
                            + QByteArray::number(point.x()) + ", "
                            + QByteArray::number(point.y()) + ')');
        return qstrdup(ba.data());
    }
}
//! [16]

//! [toString-overload]
namespace {
    char *toString(const MyPoint &point)
    {
        return QTest::toString("MyPoint(" +
                               QByteArray::number(point.x()) + ", " +
                               QByteArray::number(point.y()) + ')');
    }
}
//! [toString-overload]

void processTouchEvent()
{
//! [25]
QTouchDevice *dev = QTest::createTouchDevice();
QWidget widget;

QTest::touchEvent(&widget, dev)
    .press(0, QPoint(10, 10));
QTest::touchEvent(&widget, dev)
    .stationary(0)
    .press(1, QPoint(40, 10));
QTest::touchEvent(&widget, dev)
    .move(0, QPoint(12, 12))
    .move(1, QPoint(45, 5));
QTest::touchEvent(&widget, dev)
    .release(0, QPoint(12, 12))
    .release(1, QPoint(45, 5));
//! [25]
}

//! [26]
bool tst_MyXmlParser::parse()
{
    MyXmlParser parser;
    QString input = QFINDTESTDATA("testxml/simple1.xml");
    QVERIFY(parser.parse(input));
}
//! [26]

//! [28]
QWidget myWindow;
QTest::keyClick(&myWindow, Qt::Key_Tab);
//! [28]

//! [29]
QTest::keyClick(&myWindow, Qt::Key_Escape);
QTest::keyClick(&myWindow, Qt::Key_Escape, Qt::ShiftModifier, 200);
//! [29]

//! [30]
void TestQLocale::initTestCase_data()
{
    QTest::addColumn<QLocale>("locale");
    QTest::newRow("C") << QLocale::c();
    QTest::newRow("UKish") << QLocale("en_GB");
    QTest::newRow("USAish") << QLocale(QLocale::English, QLocale::UnitedStates);
}

void TestQLocale::roundTripInt_data()
{
    QTest::addColumn<int>("number");
    QTest::newRow("zero") << 0;
    QTest::newRow("one") << 1;
    QTest::newRow("two") << 2;
    QTest::newRow("ten") << 10;
}
//! [30]

//! [31]
void TestQLocale::roundTripInt()
{
    QFETCH_GLOBAL(QLocale, locale);
    QFETCH(int, number);
    bool ok;
    QCOMPARE(locale.toInt(locale.toString(number), &ok), number);
    QVERIFY(ok);
}
//! [31]

//! [34]
char *toString(const MyType &t)
{
    char *repr = new char[t.reprSize()];
    t.writeRepr(repr);
    return repr;
}
//! [34]

//! [35]
QSignalSpy doubleClickSpy(target, &TargetClass::doubleClicked);
const QPoint p(1, 2);
QTest::mousePress(&myWindow, Qt::LeftButton, Qt::NoModifier, p);
QVERIFY(target.isPressed());
QTest::mouseRelease(&myWindow, Qt::LeftButton, Qt::NoModifier, p, 10);
QCOMPARE(target.isPressed(), false);
QTest::mousePress(&myWindow, Qt::LeftButton, Qt::NoModifier, p, 10);
QCOMPARE(target.pressCount(), 2);
QTest::mouseRelease(&myWindow, Qt::LeftButton, Qt::NoModifier, p, 10);
QCOMPARE(doubleClickSpy.count(), 1);
//! [35]
