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
#include <QAbstractSlider>
#include <QScrollBar>
#include <QSlider>
#include <QStyle>
#include <QStyleOption>
#include <QTime>
#include <QDebug>

// defined to be 120 by the wheel mouse vendors according to the docs
#define WHEEL_DELTA 120

static inline void setFrameless(QWidget *w)
{
    Qt::WindowFlags flags = w->windowFlags();
    flags |= Qt::FramelessWindowHint;
    flags &= ~(Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    w->setWindowFlags(flags);
}

class Slider : public QAbstractSlider
{
    public:
        Slider(QWidget *parent)
            : QAbstractSlider(parent) {}
        using QAbstractSlider::setRepeatAction;
        using QAbstractSlider::repeatAction;
};

class tst_QAbstractSlider: public QObject
{
    Q_OBJECT
public slots:
    void initTestCase();
    void cleanupTestCase();
    void actionTriggered(int action);
    void rangeChanged(int min, int max);
    void valueChanged(int value);
    void sliderMoved(int value);

private slots:
    void triggerAction_data();
    void triggerAction();
    void minimum_maximum_data();
    void minimum_maximum();
    void keyPressed_data();
    void keyPressed();
#ifndef QT_NO_WHEELEVENT
    void wheelEvent_data();
    void wheelEvent();
#endif
    void sliderPressedReleased_data();
    void sliderPressedReleased();
    void setOrientation();
    void sliderMoved_data();
    void sliderMoved();
    void rangeChanged_data();
    void rangeChanged();
    void setSliderPosition_data();
    void setSliderPosition();
    void setValue_data();
    void setValue();
    void setRepeatAction();
    void connectedSliders();

private:
    void waitUntilTimeElapsed(const QTime& t, int ms);

    QWidget *topLevel;
    Slider *slider;
    int previousAction;
    int reportedMinimum;
    int reportedMaximum;
    int reportedValue;
    int reportedSliderPosition;
    qint64 timeStamp;
    qint64 actionTriggeredTimeStamp;
    qint64 valueChangedTimeStamp;
    qint64 rangeChangedTimeStamp;
    qint64 sliderMovedTimeStamp;
};

Q_DECLARE_METATYPE(QList<Qt::Key>)

void tst_QAbstractSlider::initTestCase()
{
    topLevel = new QWidget;
    slider = new Slider(topLevel);
    slider->setObjectName("testWidget");
    slider->resize(100,100);
    slider->show();

    previousAction = QAbstractSlider::SliderNoAction;
    timeStamp = 0;

    connect(slider,SIGNAL(actionTriggered(int)),this,SLOT(actionTriggered(int)));
    connect(slider,SIGNAL(rangeChanged(int,int)),this,SLOT(rangeChanged(int,int)));
    connect(slider,SIGNAL(valueChanged(int)),this,SLOT(valueChanged(int)));
    connect(slider,SIGNAL(sliderMoved(int)),this,SLOT(sliderMoved(int)));
}

void tst_QAbstractSlider::cleanupTestCase()
{
    delete topLevel;
}

void tst_QAbstractSlider::actionTriggered(int action)
{
    previousAction = action;
    actionTriggeredTimeStamp = timeStamp++;
}

void tst_QAbstractSlider::rangeChanged(int min,int max)
{
    reportedMinimum = min;
    reportedMaximum = max;
    rangeChangedTimeStamp = timeStamp++;
}

void tst_QAbstractSlider::valueChanged(int value)
{
    reportedValue = value;
    valueChangedTimeStamp = timeStamp++;
}

void tst_QAbstractSlider::sliderMoved(int value)
{
    reportedSliderPosition = value;
    sliderMovedTimeStamp = timeStamp++;
}

void tst_QAbstractSlider::triggerAction_data()
{
    QTest::addColumn<int>("sliderAction");
    QTest::addColumn<int>("maximum");
    QTest::addColumn<int>("minimum");
    QTest::addColumn<int>("initialSliderPosition");
    QTest::addColumn<int>("singleStep");
    QTest::addColumn<int>("pageStep");
    QTest::addColumn<int>("expectedSliderPosition");

    QTest::newRow("No action") << int(QAbstractSlider::SliderNoAction) // sliderAction
                            << 1000                                 // max
                            << 900                                  // min
                            << 987                                  // initial position
                            << 237                                  // single step size
                            << 234                                  // page step size
                            << 987;                                 // expected position after

    QTest::newRow("Move action") << int(QAbstractSlider::SliderMove) // sliderAction
                              << 1000                                 // max
                              << 900                                  // min
                              << 988                                  // initial position
                              << 237                                  // single step size
                              << 234                                  // page step size
                              << 988;                                 // expected position after

    QTest::newRow("Empty step add") << int(QAbstractSlider::SliderSingleStepAdd) // sliderAction
                                 << 1000                                      // max
                                 << 900                                       // min
                                 << 988                                       // initial position
                                 << 0                                         // single step size
                                 << 234                                       // page step size
                                 << 988;                                      // expected position after

     QTest::newRow("Empty step sub") << int(QAbstractSlider::SliderSingleStepSub) // sliderAction
                                 << 1000                                       // max
                                 << 900                                        // min
                                 << 987                                        // initial position
                                 << 0                                          // single step size
                                 << 234                                        // page step size
                                 << 987;                                       // expected position after

    QTest::newRow("Empty page add") << int(QAbstractSlider::SliderPageStepAdd) // sliderAction
                                 << 1000                                    // max
                                 << 900                                     // min
                                 << 988                                     // initial position
                                 << 234                                     // single step size
                                 << 0                                       // page step size
                                 << 988;                                    // expected position after

     QTest::newRow("Empty page sub") << int(QAbstractSlider::SliderPageStepSub) // sliderAction
                                 << 1000                                     // max
                                 << 900                                      // min
                                 << 987                                      // initial position
                                 << 234                                      // single step size
                                 << 0                                        // page step size
                                 << 987;                                     // expected position after

    QTest::newRow("Legal step add") << int(QAbstractSlider::SliderSingleStepAdd) // sliderAction
                                 << 1000                                      // max
                                 << 900                                       // min
                                 << 988                                       // initial position
                                 << 5                                         // single step size
                                 << 234                                       // page step size
                                 << 993;                                      // expected position after

     QTest::newRow("Legal step sub") << int(QAbstractSlider::SliderSingleStepSub) // sliderAction
                                 << 1000                                       // max
                                 << 900                                        // min
                                 << 987                                        // initial position
                                 << 5                                          // single step size
                                 << 234                                        // page step size
                                 << 982;                                       // expected position after

    QTest::newRow("Legal page add") << int(QAbstractSlider::SliderPageStepAdd) // sliderAction
                                 << 1000                                    // max
                                 << 900                                     // min
                                 << 988                                     // initial position
                                 << 234                                     // single step size
                                 << 5                                       // page step size
                                 << 993;                                    // expected position after

     QTest::newRow("Legal page sub") << int(QAbstractSlider::SliderPageStepSub) // sliderAction
                                 << 1000                                     // max
                                 << 900                                      // min
                                 << 987                                      // initial position
                                 << 234                                      // single step size
                                 << 5                                        // page step size
                                 << 982;                                     // expected position after

    QTest::newRow("Illegal step add") << int(QAbstractSlider::SliderSingleStepAdd) // sliderAction
                                   << 1000                                      // max
                                   << 900                                       // min
                                   << 988                                       // initial position
                                   << 500                                       // single step size
                                   << 234                                       // page step size
                                   << 1000;                                     // expected position after

     QTest::newRow("Illegal step sub") << int(QAbstractSlider::SliderSingleStepSub) // sliderAction
                                    << 1000                                      // max
                                    << 900                                       // min
                                    << 987                                       // initial position
                                    << 500                                       // single step size
                                    << 234                                       // page step size
                                    << 900;                                      // expected position after

    QTest::newRow("Illegal page add") << int(QAbstractSlider::SliderPageStepAdd) // sliderAction
                                   << 1000                                    // max
                                   << 900                                     // min
                                   << 988                                     // initial position
                                   << 234                                     // single step size
                                   << 500                                     // page step size
                                   << 1000;                                   // expected position after

     QTest::newRow("Illegal page sub") << int(QAbstractSlider::SliderPageStepSub) // sliderAction
                                    << 1000                                    // max
                                    << 900                                     // min
                                    << 987                                     // initial position
                                    << 234                                     // single step size
                                    << 500                                     // page step size
                                    << 900;                                    // expected position after

    // Negative steps will also be abs()'d so, check that case.
    QTest::newRow("Negative step add") << int(QAbstractSlider::SliderSingleStepAdd) // sliderAction
                                    << 1000                                      // max
                                    << 900                                       // min
                                    << 988                                       // initial position
                                    << -1                                        // single step size
                                    << 234                                       // page step size
                                    << 989;                                      // expected position after

     QTest::newRow("Negative step sub") << int(QAbstractSlider::SliderSingleStepSub) // sliderAction
                                     << 1000                                      // max
                                     << 900                                       // min
                                     << 987                                       // initial position
                                     << -1                                        // single step size
                                     << 234                                       // page step size
                                     << 986;                                      // expected position after

    QTest::newRow("Negative page add") << int(QAbstractSlider::SliderPageStepAdd) // sliderAction
                                    << 1000                                    // max
                                    << 900                                     // min
                                    << 988                                     // initial position
                                    << 234                                     // single step size
                                    << -1                                      // page step size
                                    << 989;                                    // expected position after

     QTest::newRow("Negative page sub") << int(QAbstractSlider::SliderPageStepSub) // sliderAction
                                     << 1000                                    // max
                                     << 900                                     // min
                                     << 987                                     // initial position
                                     << 234                                     // single step size
                                     << -1                                      // page step size
                                     << 986;                                    // expected position after

    QTest::newRow("Illegal negative step add") << int(QAbstractSlider::SliderSingleStepAdd) // sliderAction
                                            << 1000                                      // max
                                            << 900                                       // min
                                            << 988                                       // initial position
                                            << -500                                      // single step size
                                            << 234                                       // page step size
                                            << 1000;                                      // expected position after


    QTest::newRow("Illegal negative step sub") << int(QAbstractSlider::SliderSingleStepSub) // sliderAction
                                            << 1000                                      // max
                                            << 900                                       // min
                                            << 988                                       // initial position
                                            << -500                                      // single step size
                                            << 234                                       // page step size
                                            << 900;                                     // expected position after

    QTest::newRow("Illegal negative page add") << int(QAbstractSlider::SliderPageStepAdd) // sliderAction
                                            << 1000                                    // max
                                            << 900                                     // min
                                            << 988                                     // initial position
                                            << 234                                     // single step size
                                            << -500                                    // page step size
                                            << 1000;                                    // expected position after

    QTest::newRow("Illegal negative page sub") << int(QAbstractSlider::SliderPageStepSub) // sliderAction
                                            << 1000                                    // max
                                            << 900                                     // min
                                            << 988                                     // initial position
                                            << 245                                     // single step size
                                            << -500                                    // page step size
                                            << 900;                                   // expected position after

    QTest::newRow("Slider to minimum") << int(QAbstractSlider::SliderToMinimum)   // sliderAction
                                    << 1000                                    // max
                                    << 900                                     // min
                                    << 988                                     // initial position
                                    << 245                                     // single step size
                                    << 1                                       // page step size
                                    << 900;                                    // expected position after

    QTest::newRow("Slider to maximum") << int(QAbstractSlider::SliderToMaximum)   // sliderAction
                                    << 1000                                    // max
                                    << 900                                     // min
                                    << 988                                     // initial position
                                    << 245                                     // single step size
                                    << 1                                       // page step size
                                    << 1000;                                   // expected position after

}

void tst_QAbstractSlider::triggerAction()
{
    QFETCH(int,sliderAction);
    QFETCH(int,maximum);
    QFETCH(int,minimum);
    QFETCH(int,initialSliderPosition);
    QFETCH(int,singleStep);
    QFETCH(int,pageStep);
    QFETCH(int,expectedSliderPosition);

    slider->setTracking(true);
    slider->setRange(minimum,maximum);
    slider->setSingleStep(singleStep);
    slider->setPageStep(pageStep);
    QCOMPARE(slider->singleStep(), qAbs(singleStep));
    QCOMPARE(slider->pageStep(), qAbs(pageStep));

    int oldPosition = slider->sliderPosition();
    slider->setSliderPosition(initialSliderPosition);

    QVERIFY( (oldPosition == initialSliderPosition && previousAction == int(QAbstractSlider::SliderNoAction)) ||
            (oldPosition != initialSliderPosition && previousAction == int(QAbstractSlider::SliderMove)));
    previousAction = int(QAbstractSlider::SliderNoAction);

    QAbstractSlider::SliderAction *action = reinterpret_cast<QAbstractSlider::SliderAction*>(&sliderAction);
    QVERIFY(action != 0);

    slider->triggerAction(*action);
    QCOMPARE(previousAction,sliderAction); // previousAction set in the actionTriggered() slot
    QCOMPARE(slider->sliderPosition(),expectedSliderPosition);
    QCOMPARE(slider->value(),expectedSliderPosition);
    QCOMPARE(reportedValue,expectedSliderPosition);
    previousAction = int(QAbstractSlider::SliderNoAction);
    if (initialSliderPosition != expectedSliderPosition)
        QVERIFY(actionTriggeredTimeStamp < valueChangedTimeStamp);
}

void tst_QAbstractSlider::minimum_maximum_data()
{
    QTest::addColumn<int>("minimum");
    QTest::addColumn<int>("maximum");
    QTest::addColumn<int>("expectedMinimum");
    QTest::addColumn<int>("expectedMaximum");

    QTest::newRow("Normal range") << 100 << 200 << 100 << 200;
    QTest::newRow("Minimum higher") << 100 << 0 << 100 << 100;
    QTest::newRow("Negative minimum") << -100 << 100 << -100 << 100;
    QTest::newRow("Negative range") << -100 << -50 << -100 << -50;
}

void tst_QAbstractSlider::minimum_maximum()
{
    QFETCH(int, minimum);
    QFETCH(int, maximum);
    QFETCH(int, expectedMinimum);
    QFETCH(int, expectedMaximum);

    slider->setRange(minimum,maximum);
    QCOMPARE(slider->minimum(),expectedMinimum);
    QCOMPARE(slider->maximum(),expectedMaximum);
    QCOMPARE(reportedMinimum,expectedMinimum);
    QCOMPARE(reportedMaximum,expectedMaximum);

    slider->setRange(minimum,maximum);
    slider->setMaximum(slider->minimum() - 1);
    QCOMPARE(slider->maximum(),slider->minimum());
    QCOMPARE(reportedMinimum,slider->minimum());
    QCOMPARE(reportedMaximum,slider->maximum());

    slider->setRange(minimum,maximum);
    slider->setMinimum(slider->maximum() + 1);
    QCOMPARE(slider->minimum(),slider->maximum());
    QCOMPARE(reportedMinimum,slider->minimum());
    QCOMPARE(reportedMaximum,slider->maximum());

    slider->setRange(minimum,maximum);
    slider->setSliderPosition(slider->maximum() + 1);
    QCOMPARE(slider->sliderPosition(), slider->maximum());
    QCOMPARE(slider->value(), slider->maximum());
    QCOMPARE(reportedValue, slider->maximum());

    slider->setRange(minimum,maximum);
    slider->setSliderPosition(slider->minimum() - 1);
    QCOMPARE(slider->sliderPosition(), slider->minimum());
    QCOMPARE(slider->value(), slider->minimum());
    QCOMPARE(reportedValue, slider->minimum());

    slider->setRange(minimum,maximum);
    int oldPosition = slider->sliderPosition();
    slider->setMaximum(oldPosition - 1);
    QCOMPARE(slider->sliderPosition(),oldPosition - 1);

    slider->setRange(minimum,maximum);
    oldPosition = slider->sliderPosition();
    slider->setMinimum(oldPosition + 1);
    QCOMPARE(slider->sliderPosition(), oldPosition + 1);
}

void tst_QAbstractSlider::keyPressed_data()
{
    QTest::addColumn<int>("initialSliderPosition");
    QTest::addColumn<int>("minimum");
    QTest::addColumn<int>("maximum");
    QTest::addColumn<int>("stepSize");
    QTest::addColumn<int>("pageSize");
    QTest::addColumn<QList<Qt::Key> >("keySequence");
    QTest::addColumn<int>("expectedSliderPositionHorizontal");
    QTest::addColumn<int>("expectedSliderPositionVertical");
    QTest::addColumn<int>("expectedSliderPositionHorizontalInverted"); // :)
    QTest::addColumn<int>("expectedSliderPositionVerticalInverted");


    QList<Qt::Key> list;

    list << Qt::Key_Down;
    QTest::newRow("Step down once 1") << 10   // initial position
                                 << 0    // minimum
                                 << 100  // maximum
                                 << 3    // single step size
                                 << 0    // page step size
                                 << list // key sequence
                                 << 7   // result in case of horizontal slider
                                 << 7    // result in case of vertical slider
                                 << 13   // result in case of inverted horiz. slider
                                 << 13;   // result in case of inverted vertical slider

    list = QList<Qt::Key>();
    list << Qt::Key_Up;
    QTest::newRow("Step down once 2") << 10   // initial position
                                 << 0    // minimum
                                 << 100  // maximum
                                 << 3    // single step size
                                 << 0    // page step size
                                 << list // key sequence
                                 << 13   // result in case of horizontal slider
                                 << 13    // result in case of vertical slider
                                 << 7   // result in case of inverted horiz. slider
                                 << 7;   // result in case of inverted vertical slider


    list = QList<Qt::Key>();
    list << Qt::Key_Left;
    QTest::newRow("Step left once") << 10   // initial position
                                 << 0    // minimum
                                 << 100  // maximum
                                 << 3    // single step size
                                 << 0    // page step size
                                 << list // key sequence
                                 << 7    // result in case of horizontal slider
                                 << 7   // result in case of vertical slider
                                 << 13   // result in case of inverted horiz. slider
                                 << 13;   // result in case of inverted vertical slider

    list = QList<Qt::Key>();
    list << Qt::Key_Right;
    QTest::newRow("Step right once") << 10   // initial position
                                  << 0    // minimum
                                  << 100  // maximum
                                  << 3    // single step size
                                  << 0    // page step size
                                  << list // key sequence
                                  << 13   // result in case of horizontal slider
                                  << 13   // result in case of vertical slider
                                  << 7    // result in case of inverted horiz. slider
                                  << 7;  // result in case of inverted vertical slider

    list = QList<Qt::Key>();
    list << Qt::Key_PageDown;
    QTest::newRow("Page down once")  << 10   // initial position
                                  << 0    // minimum
                                  << 100  // maximum
                                  << 0    // single step size
                                  << 3    // page step size
                                  << list // key sequence
                                  << 7   // result in case of horizontal slider
                                  << 7   // result in case of vertical slider
                                  << 13    // result in case of inverted horiz. slider
                                  << 13;  // result in case of inverted vertical slider

    list = QList<Qt::Key>();
    list << Qt::Key_PageUp;
    QTest::newRow("Page up once")    << 10   // initial position
                                  << 0    // minimum
                                  << 100  // maximum
                                  << 0    // single step size
                                  << 3    // page step size
                                  << list // key sequence
                                  << 13   // result in case of horizontal slider
                                  << 13   // result in case of vertical slider
                                  << 7    // result in case of inverted horiz. slider
                                  << 7;  // result in case of inverted vertical slider

    list = QList<Qt::Key>();
    list << Qt::Key_Up << Qt::Key_Up << Qt::Key_PageDown << Qt::Key_PageDown  << Qt::Key_Left << Qt::Key_Left
         << Qt::Key_Right << Qt::Key_Down << Qt::Key_PageUp << Qt::Key_PageUp << Qt::Key_Down << Qt::Key_Right;
    QTest::newRow("Symmetric seq")  << 50   // initial position
                                 << 0    // minimum
                                 << 100  // maximum
                                 << 3    // single step size
                                 << 3    // page step size
                                 << list // key sequence
                                 << 50   // result in case of horizontal slider
                                 << 50    // result in case of vertical slider
                                 << 50   // result in case of inverted horiz. slider
                                 << 50;  // result in case of inverted vertical slider


    list = QList<Qt::Key>();
    list << Qt::Key_Home;
    QTest::newRow("Home")    << 10   // initial position
                          << 0    // minimum
                          << 100  // maximum
                          << 0    // single step size
                          << 3    // page step size
                          << list // key sequence
                          << 0   // result in case of horizontal slider
                          << 0   // result in case of vertical slider
                          << 0    // result in case of inverted horiz. slider
                          << 0;  // result in case of inverted vertical slider

    list = QList<Qt::Key>();
    list << Qt::Key_End;
    QTest::newRow("End")    << 10   // initial position
                          << 0    // minimum
                          << 100  // maximum
                          << 0    // single step size
                          << 3    // page step size
                          << list // key sequence
                          << 100   // result in case of horizontal slider
                          << 100   // result in case of vertical slider
                          << 100    // result in case of inverted horiz. slider
                          << 100;  // result in case of inverted vertical slider

    list = QList<Qt::Key>();
    list << Qt::Key_End << Qt::Key_Up;
    QTest::newRow("Past end")<< 10   // initial position
                          << 0    // minimum
                          << 100  // maximum
                          << 3    // single step size
                          << 3    // page step size
                          << list // key sequence
                          << 100   // result in case of horizontal slider
                          << 100   // result in case of vertical slider
                          << 97    // result in case of inverted horiz. slider
                          << 97;  // result in case of inverted vertical slider

    list = QList<Qt::Key>();
    list << Qt::Key_Home << Qt::Key_Down;
    QTest::newRow("Past home")<< 10   // initial position
                           << 0    // minimum
                           << 100  // maximum
                           << 3    // single step size
                           << 3    // page step size
                           << list // key sequence
                           << 0    // result in case of horizontal slider
                           << 0    // result in case of vertical slider
                           << 3    // result in case of inverted horiz. slider
                           << 3;  // result in case of inverted vertical slider

}

void tst_QAbstractSlider::keyPressed()
{
    QFETCH(int, initialSliderPosition);
    QFETCH(int, minimum);
    QFETCH(int, maximum);
    QFETCH(int, stepSize);
    QFETCH(int, pageSize);
    QFETCH(QList<Qt::Key>, keySequence);
    QFETCH(int, expectedSliderPositionHorizontal);
    QFETCH(int, expectedSliderPositionVertical);
    QFETCH(int, expectedSliderPositionHorizontalInverted);
    QFETCH(int, expectedSliderPositionVerticalInverted);

    // Horizontal non-inverted
    slider->setRange(minimum,maximum);
    slider->setSliderPosition(initialSliderPosition);
    slider->setSingleStep(stepSize);
    slider->setPageStep(pageSize);
    slider->setOrientation(Qt::Horizontal);
    slider->setInvertedAppearance(false);
    slider->setInvertedControls(false);
    for (int i=0;i<keySequence.count();i++) {
        QTest::keyClick(slider, keySequence.at(i));
    }
    QCOMPARE(slider->sliderPosition(), expectedSliderPositionHorizontal);

    // Horizontal inverted
    slider->setRange(minimum,maximum);
    slider->setSliderPosition(initialSliderPosition);
    slider->setSingleStep(stepSize);
    slider->setPageStep(pageSize);
    slider->setOrientation(Qt::Horizontal);
    slider->setInvertedAppearance(true);
    slider->setInvertedControls(true);
    for (int i=0;i<keySequence.count();i++)
        QTest::keyPress(slider, keySequence.at(i));
    QCOMPARE(slider->sliderPosition(), expectedSliderPositionHorizontalInverted);

    // Vertical non-inverted
    slider->setRange(minimum,maximum);
    slider->setSliderPosition(initialSliderPosition);
    slider->setSingleStep(stepSize);
    slider->setPageStep(pageSize);
    slider->setOrientation(Qt::Vertical);
    slider->setInvertedAppearance(false);
    slider->setInvertedControls(false);
    for (int i=0;i<keySequence.count();i++)
        QTest::keyPress(slider, keySequence.at(i));
    QCOMPARE(slider->sliderPosition(), expectedSliderPositionVertical);

    // Vertical inverted
    slider->setRange(minimum,maximum);
    slider->setSliderPosition(initialSliderPosition);
    slider->setSingleStep(stepSize);
    slider->setPageStep(pageSize);
    slider->setOrientation(Qt::Vertical);
    slider->setInvertedAppearance(true);
    slider->setInvertedControls(true);
    for (int i=0;i<keySequence.count();i++)
        QTest::keyPress(slider, keySequence.at(i));
    QCOMPARE(slider->sliderPosition(), expectedSliderPositionVerticalInverted);
}

#ifndef QT_NO_WHEELEVENT
void tst_QAbstractSlider::wheelEvent_data()
{
    QTest::addColumn<int>("initialSliderPosition");
    QTest::addColumn<int>("minimum");
    QTest::addColumn<int>("maximum");
    QTest::addColumn<int>("singleStep");
    QTest::addColumn<int>("pageStep");
    QTest::addColumn<bool>("invertedControls");
    QTest::addColumn<int>("wheelScrollLines");
    QTest::addColumn<bool>("withModifiers"); // use keyboard modifiers while scrolling? (CTRL and SHIFT)
    QTest::addColumn<int>("deltaMultiple"); // multiples of WHEEL_DELTA
    QTest::addColumn<int>("sliderOrientation");
    QTest::addColumn<int>("wheelOrientation");
    QTest::addColumn<int>("expectedSliderPosition");
    QTest::addColumn<QPoint>("distanceFromBottomRight"); // mpointer's distance from bottom-right corner of widget

    QTest::newRow("Normal data step") << 0                                  // initial position
                                   << 0                                  // minimum
                                   << 100                                // maximum
                                   << 1                                  // single step
                                   << 100                                // page step
                                   << false                              // inverted controls
                                   << 20                                 // wheel scroll lines
                                   << false                              // with modifiers
                                   << 1                                  // delta
                                   << int(Qt::Vertical)                  // orientation of slider
                                   << int(Qt::Vertical)                  // orientation of wheel
                                   << 20                                 // expected position after
                                   << QPoint(0,0);

    QTest::newRow("Normal data page") << 0                                  // initial position
                                   << 0                                  // minimum
                                   << 100                                // maximum
                                   << 100                                // single step
                                   << 1                                  // page step
                                   << false                              // inverted controls
                                   << 20                                 // wheel scroll lines
                                   << false                              // with modifiers
                                   << 1                                  // delta
                                   << int(Qt::Vertical)                  // orientation of slider
                                   << int(Qt::Vertical)                  // orientation of wheel
#ifndef Q_OS_MAC
                                   << 1                                  // expected position after
#else
                                   // We don't restrict scrolling to pageStep on Mac
                                   << 100                                // expected position after
#endif
                                   << QPoint(1,1);
    QTest::newRow("Different orientation") << 0                             // initial position
                                        << 0                             // minimum
                                        << 100                           // maximum
                                        << 100                           // single step
                                        << 1                             // page step
                                        << false                         // inverted controls
                                        << 20                            // wheel scroll lines
                                        << false                         // with modifiers
                                        << 1                             // delta
                                        << int(Qt::Horizontal)           // orientation of slider
                                        << int(Qt::Vertical)             // orientation of wheel
#ifndef Q_OS_MAC
                                        << 1                             // expected position after
#else
                                        // We don't restrict scrolling to pageStep on Mac
                                        << 100                           // expected position after
#endif
                                        << QPoint(1,1);

    QTest::newRow("Different orientation2")<< 0                             // initial position
                                        << 0                             // minimum
                                        << 100                           // maximum
                                        << 100                           // single step
                                        << 1                             // page step
                                        << false                         // inverted controls
                                        << 20                            // wheel scroll lines
                                        << false                         // with modifiers
                                        << 1                             // delta
                                        << int(Qt::Horizontal)           // orientation of slider
                                        << int(Qt::Vertical)             // orientation of wheel
#ifndef Q_OS_MAC
                                        << 1                             // expected position after
#else
                                        // We don't restrict scrolling to pageStep on Mac
                                        << 100                           // expected position after
#endif
                                        << QPoint(0,0);

    QTest::newRow("Inverted controls")     << 50                            // initial position
                                        << 0                             // minimum
                                        << 100                           // maximum
                                        << 1                             // single step
                                        << 100                           // page step
                                        << true                          // inverted controls
                                        << 20                            // wheel scroll lines
                                        << false                         // with modifiers
                                        << -1                            // delta
                                        << int(Qt::Horizontal)           // orientation of slider
                                        << int(Qt::Horizontal)           // orientation of wheel
                                        << 30                            // expected position after
                                        << QPoint(1,1);

    QTest::newRow("Past end")              << 50                            // initial position
                                        << 0                             // minimum
                                        << 100                           // maximum
                                        << 26                            // single step
                                        << 100                           // page step
                                        << false                         // inverted controls
                                        << 1                             // wheel scroll lines
                                        << false                         // with modifiers
                                        << -2                            // delta
                                        << int(Qt::Horizontal)           // orientation of slider
                                        << int(Qt::Horizontal)           // orientation of wheel
                                        << 100                           // expected position after
                                        << QPoint(0,0);

    QTest::newRow("Past start")            << 50                            // initial position
                                        << 0                             // minimum
                                        << 100                           // maximum
                                        << 26                            // single step
                                        << 100                           // page step
                                        << false                         // inverted controls
                                        << 1                             // wheel scroll lines
                                        << false                         // with modifiers
                                        << 2                             // delta
                                        << int(Qt::Horizontal)           // orientation of slider
                                        << int(Qt::Horizontal)           // orientation of wheel
                                        << 0                             // expected position after
                                        << QPoint(0,0);

    QTest::newRow("With modifiers")        << 50                            // initial position
                                        << 0                             // minimum
                                        << 100                           // maximum
                                        << 1                             // single step
                                        << 40                            // page step
                                        << false                         // inverted controls
                                        << 20                            // wheel scroll lines
                                        << true                          // with modifiers
                                        << -1                            // delta
                                        << int(Qt::Horizontal)           // orientation of slider
                                        << int(Qt::Horizontal)           // orientation of wheel
                                        << 90                            // expected position after
                                        << QPoint(0,0);

}

void tst_QAbstractSlider::wheelEvent()
{
    QFETCH(int,initialSliderPosition);
    QFETCH(int,minimum);
    QFETCH(int,maximum);
    QFETCH(int,singleStep);
    QFETCH(int,pageStep);
    QFETCH(bool,invertedControls);
    QFETCH(int,wheelScrollLines);
    QFETCH(bool,withModifiers);
    QFETCH(int,deltaMultiple);
    QFETCH(int,sliderOrientation);
    QFETCH(int,wheelOrientation);
    QFETCH(int,expectedSliderPosition);
    QFETCH(QPoint,distanceFromBottomRight);

    QCoreApplication *applicationInstance = QCoreApplication::instance();
    QVERIFY(applicationInstance != 0);
    QApplication::setWheelScrollLines(wheelScrollLines);

    Qt::Orientation orientation = *reinterpret_cast<Qt::Orientation*>(&sliderOrientation);
    slider->setRange(minimum,maximum);
    slider->setSliderPosition(initialSliderPosition);
    slider->setSingleStep(singleStep);
    slider->setPageStep(pageStep);
    slider->setInvertedControls(invertedControls);
    slider->setOrientation(orientation);

    Qt::KeyboardModifier k = withModifiers ? Qt::ControlModifier : Qt::NoModifier;
    orientation = *reinterpret_cast<Qt::Orientation*>(&wheelOrientation);
    QWheelEvent event(slider->rect().bottomRight() + distanceFromBottomRight, WHEEL_DELTA * deltaMultiple,
                      Qt::NoButton, k, orientation);
    QVERIFY(applicationInstance->sendEvent(slider,&event));
#ifdef Q_OS_MAC
    QEXPECT_FAIL("Normal data page", "QTBUG-23679", Continue);
    QEXPECT_FAIL("Different orientation", "QTBUG-23679", Continue);
    QEXPECT_FAIL("Different orientation2", "QTBUG-23679", Continue);
#endif
    QCOMPARE(slider->sliderPosition(),expectedSliderPosition);

    slider->setSliderPosition(initialSliderPosition);
    k = withModifiers ? Qt::ShiftModifier : Qt::NoModifier;
    event = QWheelEvent(slider->rect().bottomRight() + distanceFromBottomRight, WHEEL_DELTA * deltaMultiple,
                      Qt::NoButton, k, orientation);
    QSignalSpy spy1(slider, SIGNAL(actionTriggered(int)));
    QSignalSpy spy2(slider, SIGNAL(valueChanged(int)));
    QVERIFY(applicationInstance->sendEvent(slider,&event));
#ifdef Q_OS_MAC
    QEXPECT_FAIL("Normal data page", "QTBUG-23679", Continue);
    QEXPECT_FAIL("Different orientation", "QTBUG-23679", Continue);
    QEXPECT_FAIL("Different orientation2", "QTBUG-23679", Continue);
#endif
    QCOMPARE(slider->sliderPosition(),expectedSliderPosition);
    int expectedSignalCount = (initialSliderPosition == expectedSliderPosition) ? 0 : 1;
    QCOMPARE(spy1.count(), expectedSignalCount);
    QCOMPARE(spy2.count(), expectedSignalCount);
    if (expectedSignalCount)
        QVERIFY(actionTriggeredTimeStamp < valueChangedTimeStamp);
}
#endif // !QT_NO_WHEELEVENT

void tst_QAbstractSlider::sliderPressedReleased_data()
{
    QTest::addColumn<int>("control");
    QTest::addColumn<int>("minimum");
    QTest::addColumn<int>("maximum");
    QTest::addColumn<uint>("subControl");
    QTest::addColumn<int>("expectedCount");

    QTest::newRow("slider on the handle") << int(QStyle::CC_Slider)
                               << 0
                               << 20
                               << uint(QStyle::SC_SliderHandle)
                               << 1;

    QTest::newRow("slider on the groove") << int(QStyle::CC_Slider)
                               << 0
                               << 20
                               << uint(QStyle::SC_SliderGroove)
                               << ((qApp->style()->styleHint(QStyle::SH_Slider_AbsoluteSetButtons) & Qt::LeftButton) ? 1 : 0);

    QTest::newRow("scrollbar on the handle") << int(QStyle::CC_ScrollBar)
                               << 0
                               << 20
                               << uint(QStyle::SC_ScrollBarSlider)
                               << 1;

    QTest::newRow("scrollbar on the groove") << int(QStyle::CC_ScrollBar)
                               << 0
                               << 20
                               << uint(QStyle::SC_ScrollBarGroove)
                               << 0;
}

void tst_QAbstractSlider::sliderPressedReleased()
{
    QFETCH(int, control);
    QFETCH(int, minimum);
    QFETCH(int, maximum);
    QFETCH(uint, subControl);
    QFETCH(int, expectedCount);

    QWidget topLevel;
    setFrameless(&topLevel);
    QAbstractSlider *slider;
    switch (control) {
    default:
        qWarning("Bad input into test, leaving");
        return;
        break;
    case QStyle::CC_Slider:
        slider = new QSlider(&topLevel);
        slider->setLayoutDirection(Qt::LeftToRight);   // Makes "upside down" much easier to compute
        break;
    case QStyle::CC_ScrollBar:
        slider = new QScrollBar(&topLevel);
        break;
    }


    slider->setMinimum(minimum);
    slider->setMaximum(maximum);
    slider->setValue(0);
    slider->setOrientation(Qt::Vertical);
    slider->resize(slider->sizeHint().width(), slider->sizeHint().height() + 100);
    QSignalSpy spy1(slider, SIGNAL(sliderPressed()));
    QSignalSpy spy2(slider, SIGNAL(sliderReleased()));

    // Mac Style requires the control to be active to get the correct values...
    topLevel.show();
    slider->activateWindow();

    QStyleOptionSlider option;
    option.init(slider);
    option.upsideDown = control == QStyle::CC_Slider ? !slider->invertedAppearance()
                                                     : slider->invertedAppearance();
    option.subControls = QStyle::SC_None;
    option.activeSubControls = QStyle::SC_None;
    option.orientation = slider->orientation();
    option.maximum = maximum;
    option.minimum = minimum;
    option.sliderPosition = slider->value();
    option.sliderValue = slider->value();
    option.singleStep = slider->singleStep();
    option.pageStep = slider->pageStep();
    QRect rect = slider->style()->subControlRect(QStyle::ComplexControl(control), &option,
                                                 QStyle::SubControl(subControl), slider);

    if (qApp->style()->styleHint(QStyle::SH_ScrollBar_LeftClickAbsolutePosition))
        QSKIP("The result depends on system setting on mac");

    QTest::mousePress(slider, Qt::LeftButton, 0, QPoint(rect.center().x() + 2, rect.center().y() + 2));
    QCOMPARE(spy1.count(), expectedCount);
    QTest::mouseRelease(slider, Qt::LeftButton, 0, rect.center());
    QCOMPARE(spy2.count(), expectedCount);

    delete slider;
}

void tst_QAbstractSlider::sliderMoved_data()
{
    QTest::addColumn<int>("control");
    QTest::addColumn<int>("minimum");
    QTest::addColumn<int>("maximum");
    QTest::addColumn<int>("position");
    QTest::addColumn<bool>("sliderDown");
    QTest::addColumn<int>("expectedCount");

    QTest::newRow("slider pressed") << int(QStyle::CC_Slider)
                               << 0
                               << 20
                               << 10
                               << true
                               << 1;

    QTest::newRow("slider not pressed") << int(QStyle::CC_Slider)
                               << 0
                               << 20
                               << 10
                               << false
                               << 0;

    QTest::newRow("scrollbar pressed") << int(QStyle::CC_ScrollBar)
                               << 0
                               << 20
                               << 10
                               << true
                               << 1;

    QTest::newRow("scrollbar not pressed") << int(QStyle::CC_ScrollBar)
                               << 0
                               << 20
                               << 10
                               << false
                               << 0;
}

void tst_QAbstractSlider::sliderMoved()
{
    QFETCH(int, control);
    QFETCH(int, minimum);
    QFETCH(int, maximum);
    QFETCH(int, position);
    QFETCH(bool, sliderDown);
    QFETCH(int, expectedCount);
    QAbstractSlider *slider;
    switch (control) {
    default:
        slider = 0;
        break;
    case QStyle::CC_Slider:
        slider = new QSlider;
        break;
    case QStyle::CC_ScrollBar:
        slider = new QScrollBar;
        break;
    }
    QSignalSpy spy(slider, SIGNAL(sliderMoved(int)));

    slider->setMinimum(minimum);
    slider->setMaximum(maximum);
    slider->setSliderDown(sliderDown);
    slider->setSliderPosition(position);
    QCOMPARE(spy.count(), expectedCount);

    delete slider;
}

void tst_QAbstractSlider::setOrientation()
{
    QSlider slider(0);

    QSizePolicy sp = slider.sizePolicy();
    slider.setOrientation(slider.orientation());
    QSizePolicy sp2 = slider.sizePolicy();
    QCOMPARE(sp, sp2);

    slider.setOrientation(Qt::Horizontal);
    sp = slider.sizePolicy();
    slider.setOrientation(Qt::Vertical);
    sp2 = slider.sizePolicy();

    QVERIFY(sp != sp2);
    sp2.transpose();
    QCOMPARE(sp, sp2);
}


void tst_QAbstractSlider::rangeChanged_data()
{
    QTest::addColumn<int>("minimum");
    QTest::addColumn<int>("maximum");
    QTest::addColumn<int>("newMin");
    QTest::addColumn<int>("newMax");
    QTest::addColumn<int>("expectedCount");

    QTest::newRow("no change")
                               << 0
                               << 20
                               << 0
                               << 20
                               << 0;

    QTest::newRow("min change")
                               << 0
                               << 20
                               << 10
                               << 20
                               << 1;
    QTest::newRow("max change")
                               << 0
                               << 20
                               << 0
                               << 30
                               << 1;

    QTest::newRow("both change")
                               << 0
                               << 20
                               << 10
                               << 30
                               << 1;
}

void tst_QAbstractSlider::rangeChanged()
{
    QFETCH(int, minimum);
    QFETCH(int, maximum);
    QFETCH(int, newMin);
    QFETCH(int, newMax);
    QFETCH(int, expectedCount);
    QSlider slider;
    slider.setRange(minimum, maximum);
    QSignalSpy spy(&slider, SIGNAL(rangeChanged(int,int)));
    slider.setRange(newMin, newMax);
    QCOMPARE(spy.count(), expectedCount);
}

void tst_QAbstractSlider::setSliderPosition_data()
{
    QTest::addColumn<bool>("tracking");
    QTest::addColumn<bool>("down");

    QTest::newRow("tracking, slider down")
                               << true
                               << true;
    QTest::newRow("tracking, slider not down")
                               << true
                               << false;
    QTest::newRow("no tracking, slider down")
                               << false
                               << true;
    QTest::newRow("no tracking, slider not down")
                               << false
                               << false;
}

void tst_QAbstractSlider::setSliderPosition()
{
    QFETCH(bool, tracking);
    QFETCH(bool, down);
    const int minimum = 0;
    const int maximum = 100;
    const int initialValue = 50;
    const int targetPosition = 75;
    slider->setRange(minimum, maximum);
    slider->setTracking(tracking);
    slider->setSliderDown(down);
    slider->setValue(initialValue);
    QCOMPARE(slider->sliderPosition(), initialValue);
    QSignalSpy spy1(slider, SIGNAL(sliderMoved(int)));
    QSignalSpy spy2(slider, SIGNAL(valueChanged(int)));
    slider->setSliderPosition(targetPosition);
    QCOMPARE(slider->sliderPosition(), targetPosition);
    QCOMPARE(spy1.count(), down ? 1 : 0);
    QCOMPARE(spy2.count(), tracking ? 1 : 0);
    QCOMPARE(slider->value(), tracking ? targetPosition : initialValue);
    if (tracking && down)
        QVERIFY(sliderMovedTimeStamp < valueChangedTimeStamp);
}

void tst_QAbstractSlider::setValue_data()
{
    QTest::addColumn<bool>("down");

    QTest::newRow("slider down")
                               << true;
    QTest::newRow("slider not down")
                               << false;
}

void tst_QAbstractSlider::setValue()
{
    QFETCH(bool, down);
    const int minimum = 0;
    const int maximum = 100;
    slider->setRange(minimum, maximum);
    slider->setSliderDown(down);
    slider->setValue(49); // to force a valueChanged() below
    QSignalSpy spy1(slider, SIGNAL(sliderMoved(int)));
    QSignalSpy spy2(slider, SIGNAL(valueChanged(int)));
    QSignalSpy spy3(slider, SIGNAL(actionTriggered(int)));
    slider->setValue(50);
    QCOMPARE(spy1.count(), down ? 1 : 0);
    QCOMPARE(spy2.count(), 1);
    QCOMPARE(spy3.count(), 0);
    QCOMPARE(slider->value(), reportedValue);
    QCOMPARE(slider->sliderPosition(), reportedSliderPosition);
    if (down)
        QVERIFY(sliderMovedTimeStamp < valueChangedTimeStamp);
}

void tst_QAbstractSlider::waitUntilTimeElapsed(const QTime& t, int ms)
{
    const int eps = 80;
    while (t.elapsed() < ms + eps)
        QTest::qWait(qMax(ms - t.elapsed() + eps, 25));
}

void tst_QAbstractSlider::setRepeatAction()
{
    slider->setRange(0, 1000);
    slider->setValue(55);
    slider->setPageStep(10);
    QSignalSpy spy(slider, SIGNAL(actionTriggered(int)));

    // Start repeat action with initial delay of 500 ms, and then repeating
    // every 250 ms.
    slider->setRepeatAction(QAbstractSlider::SliderPageStepAdd, 500, 250);
    QCOMPARE(spy.count(), 0);
    QCOMPARE(slider->value(), 55);

    QTime t;
    t.start();
    QTest::qWait(300);
    QCOMPARE(spy.count(), 0);
    QCOMPARE(slider->value(), 55);

    waitUntilTimeElapsed(t, 550);
    QTRY_COMPARE(spy.count(), 1);
    QCOMPARE(slider->value(), 65);
    QCOMPARE(spy.at(0).at(0).toUInt(), (uint)QAbstractSlider::SliderPageStepAdd);

    waitUntilTimeElapsed(t, 790);
    QTRY_COMPARE(spy.count(), 2);
    QCOMPARE(slider->value(), 75);
    QCOMPARE(spy.at(1).at(0).toUInt(), (uint)QAbstractSlider::SliderPageStepAdd);

    waitUntilTimeElapsed(t, 1790);
    QTRY_COMPARE(spy.count(), 6);
    QCOMPARE(slider->value(), 115);
    QCOMPARE(spy.at(4).at(0).toUInt(), (uint)QAbstractSlider::SliderPageStepAdd);
    QCOMPARE(spy.at(5).at(0).toUInt(), (uint)QAbstractSlider::SliderPageStepAdd);

    slider->setRepeatAction(QAbstractSlider::SliderNoAction);
    QCOMPARE(spy.count(), 6);
    QCOMPARE(slider->value(), 115);

    QTest::qWait(300);
    QCOMPARE(spy.count(), 6);
    QCOMPARE(slider->value(), 115);
}

void tst_QAbstractSlider::connectedSliders()
{
    Slider *slider2 = new Slider(topLevel);
    connect(slider, SIGNAL(rangeChanged(int,int)), slider2, SLOT(setRange(int,int)));
    const int sliderlow = 13;
    const int sliderhigh = 1017;
    slider->setRange(sliderlow, sliderhigh);
    QCOMPARE(slider2->minimum(), sliderlow);
    QCOMPARE(slider2->maximum(), sliderhigh);
    delete slider2;
}

QTEST_MAIN(tst_QAbstractSlider)
#include "tst_qabstractslider.moc"
