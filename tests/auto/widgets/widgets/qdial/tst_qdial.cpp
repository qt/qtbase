// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QTest>
#include <QSignalSpy>
#include <QDial>

class tst_QDial : public QObject
{
    Q_OBJECT
public:
    tst_QDial();

private slots:
    void getSetCheck();
    void valueChanged();
    void sliderMoved();
    void wrappingCheck();
    void minEqualMaxValueOutsideRange();

    void notchSize_data();
    void notchSize();
};

// Testing get/set functions
void tst_QDial::getSetCheck()
{
    QDial obj1;
    // bool QDial::notchesVisible()
    // void QDial::setNotchesVisible(bool)
    obj1.setNotchesVisible(false);
    QCOMPARE(false, obj1.notchesVisible());
    obj1.setNotchesVisible(true);
    QCOMPARE(true, obj1.notchesVisible());

    // bool QDial::wrapping()
    // void QDial::setWrapping(bool)
    obj1.setWrapping(false);
    QCOMPARE(false, obj1.wrapping());
    obj1.setWrapping(true);
    QCOMPARE(true, obj1.wrapping());
}

tst_QDial::tst_QDial()
{
}

void tst_QDial::valueChanged()
{
    QDial dial;
    dial.setMinimum(0);
    dial.setMaximum(100);
    QSignalSpy spy(&dial, SIGNAL(valueChanged(int)));
    dial.setValue(50);
    QCOMPARE(spy.size(), 1);
    spy.clear();
    dial.setValue(25);
    QCOMPARE(spy.size(), 1);
    spy.clear();
    // repeat!
    dial.setValue(25);
    QCOMPARE(spy.size(), 0);
}

void tst_QDial::sliderMoved()
{
    //this tests that when dragging the arrow that the sliderMoved signal is emitted
    //even if tracking is set to false
    QDial dial;
    dial.setTracking(false);
    dial.setMinimum(0);
    dial.setMaximum(100);

    dial.show();

    QPoint init(dial.width()/4, dial.height()/2);

    QMouseEvent pressevent(QEvent::MouseButtonPress, init, dial.mapToGlobal(init),
                           Qt::LeftButton, Qt::LeftButton, {});
    qApp->sendEvent(&dial, &pressevent);

    QSignalSpy sliderspy(&dial, SIGNAL(sliderMoved(int)));
    QSignalSpy valuespy(&dial, SIGNAL(valueChanged(int)));


    { //move on top of the slider
        init = QPoint(dial.width()/2, dial.height()/4);
        QMouseEvent moveevent(QEvent::MouseMove, init, dial.mapToGlobal(init),
                              Qt::LeftButton, Qt::LeftButton, {});
        qApp->sendEvent(&dial, &moveevent);
        QCOMPARE( sliderspy.size(), 1);
        QCOMPARE( valuespy.size(), 0);
    }


    { //move on the right of the slider
        init = QPoint(dial.width()*3/4, dial.height()/2);
        QMouseEvent moveevent(QEvent::MouseMove, init, dial.mapToGlobal(init),
                              Qt::LeftButton, Qt::LeftButton, {});
        qApp->sendEvent(&dial, &moveevent);
        QCOMPARE( sliderspy.size(), 2);
        QCOMPARE( valuespy.size(), 0);
    }

    QMouseEvent releaseevent(QEvent::MouseButtonRelease, init, dial.mapToGlobal(init),
                             Qt::LeftButton, Qt::LeftButton, {});
    qApp->sendEvent(&dial, &releaseevent);
    QCOMPARE( valuespy.size(), 1); // valuechanged signal should be called at this point

}

void tst_QDial::wrappingCheck()
{
    //This tests if dial will wrap past the maximum value back to the minimum
    //and vice versa when changing the value with a keypress
    QDial dial;
    dial.setMinimum(0);
    dial.setMaximum(100);
    dial.setSingleStep(1);
    dial.setWrapping(true);
    dial.setValue(99);
    dial.show();

    { //set value to maximum but do not wrap
        QTest::keyPress(&dial, Qt::Key_Up);
        QCOMPARE( dial.value(), 100);
    }

    { //step up once more and wrap clockwise to minimum + 1
        QTest::keyPress(&dial, Qt::Key_Up);
        QCOMPARE( dial.value(), 1);
    }

    { //step down once, and wrap anti-clockwise to minimum, then again to maximum - 1
        QTest::keyPress(&dial, Qt::Key_Down);
        QCOMPARE( dial.value(), 0);

        QTest::keyPress(&dial, Qt::Key_Down);
        QCOMPARE( dial.value(), 99);
    }

    { //when wrapping property is false no wrapping will occur
        dial.setWrapping(false);
        dial.setValue(100);

        QTest::keyPress(&dial, Qt::Key_Up);
        QCOMPARE( dial.value(), 100);

        dial.setValue(0);
        QTest::keyPress(&dial, Qt::Key_Down);
        QCOMPARE( dial.value(), 0);
    }

    { //When the step is really big or small, wrapping should still behave
        dial.setWrapping(true);
        dial.setValue(dial.minimum());
        dial.setSingleStep(305);

        QTest::keyPress(&dial, Qt::Key_Up);
        QCOMPARE( dial.value(), 5);

        dial.setValue(dial.minimum());
        QTest::keyPress(&dial, Qt::Key_Down);
        QCOMPARE( dial.value(), 95);

        dial.setMinimum(-30);
        dial.setMaximum(-4);
        dial.setSingleStep(200);
        dial.setValue(dial.minimum());
        QTest::keyPress(&dial, Qt::Key_Down);
        QCOMPARE( dial.value(), -22);
    }
}

// QTBUG-104641
void tst_QDial::minEqualMaxValueOutsideRange()
{
    QDial dial;
    dial.setRange(30, 30);
    dial.setWrapping(true);
    dial.setValue(45);
}

/*
    Verify that the notchSizes calculated don't change compared
    to Qt 5.15 results for dial sizes at the edge values of the
    algorithm.
*/
void tst_QDial::notchSize_data()
{
    QTest::addColumn<int>("diameter");
    QTest::addColumn<int>("notchSize");

    QTest::newRow("data0") << 50 << 4;
    QTest::newRow("data1") << 80 << 4;
    QTest::newRow("data2") << 95 << 4;
    QTest::newRow("data3") << 110 << 4;
    QTest::newRow("data4") << 152 << 2;
    QTest::newRow("data5") << 210 << 2;
    QTest::newRow("data6") << 228 << 1;
}

void tst_QDial::notchSize()
{
    QFETCH(int, diameter);
    QFETCH(int, notchSize);
    QDial dial;
    dial.setFixedSize(QSize(diameter, diameter));
    QCOMPARE(dial.notchSize(), notchSize);
}

QTEST_MAIN(tst_QDial)
#include "tst_qdial.moc"
