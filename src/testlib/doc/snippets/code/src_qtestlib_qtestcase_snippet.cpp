/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/
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
