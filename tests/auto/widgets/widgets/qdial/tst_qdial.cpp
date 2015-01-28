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


#include <QtTest/QtTest>

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
    QCOMPARE(spy.count(), 1);
    spy.clear();
    dial.setValue(25);
    QCOMPARE(spy.count(), 1);
    spy.clear();
    // repeat!
    dial.setValue(25);
    QCOMPARE(spy.count(), 0);
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

    QMouseEvent pressevent(QEvent::MouseButtonPress, init,
        Qt::LeftButton, Qt::LeftButton, 0);
    qApp->sendEvent(&dial, &pressevent);

    QSignalSpy sliderspy(&dial, SIGNAL(sliderMoved(int)));
    QSignalSpy valuespy(&dial, SIGNAL(valueChanged(int)));


    { //move on top of the slider
        init = QPoint(dial.width()/2, dial.height()/4);
        QMouseEvent moveevent(QEvent::MouseMove, init,
            Qt::LeftButton, Qt::LeftButton, 0);
        qApp->sendEvent(&dial, &moveevent);
        QCOMPARE( sliderspy.count(), 1);
        QCOMPARE( valuespy.count(), 0);
    }


    { //move on the right of the slider
        init = QPoint(dial.width()*3/4, dial.height()/2);
        QMouseEvent moveevent(QEvent::MouseMove, init,
            Qt::LeftButton, Qt::LeftButton, 0);
        qApp->sendEvent(&dial, &moveevent);
        QCOMPARE( sliderspy.count(), 2);
        QCOMPARE( valuespy.count(), 0);
    }

    QMouseEvent releaseevent(QEvent::MouseButtonRelease, init,
        Qt::LeftButton, Qt::LeftButton, 0);
    qApp->sendEvent(&dial, &releaseevent);
    QCOMPARE( valuespy.count(), 1); // valuechanged signal should be called at this point

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

QTEST_MAIN(tst_QDial)
#include "tst_qdial.moc"
