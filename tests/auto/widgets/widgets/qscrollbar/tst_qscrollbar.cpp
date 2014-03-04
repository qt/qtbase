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


#include <QtTest/QtTest>
#include <QScrollBar>
#include <QStyleOptionSlider>
#include <QScrollArea>
#include <QScreen>

static inline void centerOnScreen(QWidget *w, const QSize &size)
{
    const QPoint offset = QPoint(size.width() / 2, size.height() / 2);
    w->move(QGuiApplication::primaryScreen()->availableGeometry().center() - offset);
}

static inline void centerOnScreen(QWidget *w)
{
    centerOnScreen(w, w->geometry().size());
}

class tst_QScrollBar : public QObject
{
    Q_OBJECT
private slots:
    void scrollSingleStep();
    void task_209492();
#ifndef QT_NO_WHEELEVENT
    void QTBUG_27308();
#endif
};

class SingleStepTestScrollBar : public QScrollBar {
    Q_OBJECT
public:
    explicit SingleStepTestScrollBar(Qt::Orientation o, QWidget *parent = 0) : QScrollBar(o, parent) {}

public slots:
    void hideAndShow()
    {
        hide();
        show();
    }
};

// Check that the scrollbar doesn't scroll after calling hide and show
// from a slot connected to the scrollbar's actionTriggered signal.
void tst_QScrollBar::scrollSingleStep()
{
    SingleStepTestScrollBar testWidget(Qt::Horizontal);
    connect(&testWidget, &QAbstractSlider::actionTriggered, &testWidget, &SingleStepTestScrollBar::hideAndShow);
    testWidget.resize(100, testWidget.height());
    centerOnScreen(&testWidget);
    testWidget.show();
    QTest::qWaitForWindowExposed(&testWidget);

    testWidget.setValue(testWidget.minimum());
    QCOMPARE(testWidget.value(), testWidget.minimum());

    // Get rect for the area to click on
    const QStyleOptionSlider opt = qt_qscrollbarStyleOption(&testWidget);
    QRect sr = testWidget.style()->subControlRect(QStyle::CC_ScrollBar, &opt,
                                                  QStyle::SC_ScrollBarAddLine, &testWidget);

    if (!sr.isValid())
        QSKIP("SC_ScrollBarAddLine not valid");

    QTest::mouseClick(&testWidget, Qt::LeftButton, Qt::NoModifier, QPoint(sr.x(), sr.y()));
    QTest::qWait(510); // initial delay is 500 for setRepeatAction
    disconnect(&testWidget, &QAbstractSlider::actionTriggered, &testWidget, &SingleStepTestScrollBar::hideAndShow);
#ifdef Q_OS_MAC
    QEXPECT_FAIL("", "This test fails on Mac OS X, see QTBUG-25272", Abort);
#endif
    QCOMPARE(testWidget.value(), testWidget.singleStep());
}

void tst_QScrollBar::task_209492()
{
    class MyScrollArea : public QScrollArea
    {
    public:
        int scrollCount;
        MyScrollArea(QWidget *parent = 0) : QScrollArea(parent), scrollCount(0) {}
    protected:
        void paintEvent(QPaintEvent *) { QTest::qSleep(600); }
        void scrollContentsBy(int, int) { ++scrollCount; viewport()->update(); }
    };

    MyScrollArea scrollArea;
    QScrollBar *verticalScrollBar = scrollArea.verticalScrollBar();
    verticalScrollBar->setRange(0, 1000);
    centerOnScreen(&scrollArea);
    scrollArea.show();
    QTest::qWaitForWindowExposed(&scrollArea);

    QSignalSpy spy(verticalScrollBar, SIGNAL(actionTriggered(int)));
    QCOMPARE(scrollArea.scrollCount, 0);
    QCOMPARE(spy.count(), 0);

    // Simulate a mouse click on the "scroll down button".
    const QPoint pressPoint(verticalScrollBar->width() / 2, verticalScrollBar->height() - 10);
    const QPoint globalPressPoint = verticalScrollBar->mapToGlobal(globalPressPoint);
    QMouseEvent mousePressEvent(QEvent::MouseButtonPress, pressPoint, globalPressPoint,
                                Qt::LeftButton, Qt::LeftButton, 0);
    QApplication::sendEvent(verticalScrollBar, &mousePressEvent);
    QTest::qWait(1);
    QMouseEvent mouseReleaseEvent(QEvent::MouseButtonRelease, pressPoint, globalPressPoint,
                                  Qt::LeftButton, Qt::LeftButton, 0);
    QApplication::sendEvent(verticalScrollBar, &mouseReleaseEvent);

    // Check that the action was triggered once.

#ifdef Q_OS_MAC
    QSKIP("The result depends on system setting and is not relevant on Mac");
#endif
    QCOMPARE(scrollArea.scrollCount, 1);
    QCOMPARE(spy.count(), 1);
}

#ifndef QT_NO_WHEELEVENT
#define WHEEL_DELTA 120 // copied from tst_QAbstractSlider / tst_QComboBox
void tst_QScrollBar::QTBUG_27308()
{
    // https://bugreports.qt-project.org/browse/QTBUG-27308
    // Check that a disabled scrollbar doesn't react on wheel events anymore

    QScrollBar testWidget(Qt::Horizontal);
    testWidget.resize(100, testWidget.height());
    centerOnScreen(&testWidget);
    testWidget.show();
    QTest::qWaitForWindowExposed(&testWidget);

    testWidget.setValue(testWidget.minimum());
    testWidget.setEnabled(false);
    QWheelEvent event(testWidget.rect().center(),
                      -WHEEL_DELTA, Qt::NoButton, Qt::NoModifier, testWidget.orientation());
    qApp->sendEvent(&testWidget, &event);
    QCOMPARE(testWidget.value(), testWidget.minimum());
}
#endif

QTEST_MAIN(tst_QScrollBar)
#include "tst_qscrollbar.moc"
