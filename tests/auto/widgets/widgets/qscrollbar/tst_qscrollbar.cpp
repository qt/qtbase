/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>
#include <QScrollBar>
#include <QStyleOptionSlider>
#include <QScrollArea>

class tst_QScrollBar : public QObject
{
    Q_OBJECT
public slots:
    void initTestCase();
    void cleanupTestCase();
    void hideAndShow(int action);

private slots:
    void scrollSingleStep();
    void task_209492();

private:
    QScrollBar *testWidget;
};

void tst_QScrollBar::initTestCase()
{
    testWidget = new QScrollBar(Qt::Horizontal);
    testWidget->resize(100, testWidget->height());
    testWidget->show();
}

void tst_QScrollBar::cleanupTestCase()
{
    delete testWidget;
    testWidget = 0;
}

void tst_QScrollBar::hideAndShow(int)
{
    testWidget->hide();
    testWidget->show();
}

// Check that the scrollbar doesn't scroll after calling hide and show
// from a slot connected to the scrollbar's actionTriggered signal.
void tst_QScrollBar::scrollSingleStep()
{
    testWidget->setValue(testWidget->minimum());
    QCOMPARE(testWidget->value(), testWidget->minimum());
    connect(testWidget, SIGNAL(actionTriggered(int)), this, SLOT(hideAndShow(int)));

    // Get rect for the area to click on
    const QStyleOptionSlider opt = qt_qscrollbarStyleOption(testWidget);
    QRect sr = testWidget->style()->subControlRect(QStyle::CC_ScrollBar, &opt,
                                                   QStyle::SC_ScrollBarAddLine, testWidget);

    if (!sr.isValid())
        QSKIP("SC_ScrollBarAddLine not valid");

    QTest::mouseClick(testWidget, Qt::LeftButton, Qt::NoModifier, QPoint(sr.x(), sr.y()));
    QTest::qWait(510); // initial delay is 500 for setRepeatAction
    disconnect(testWidget, SIGNAL(actionTriggered(int)), 0, 0);
    QCOMPARE(testWidget->value(), testWidget->singleStep());
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
    scrollArea.show();
    QTest::qWait(300);

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
    QCOMPARE(scrollArea.scrollCount, 1);
    QCOMPARE(spy.count(), 1);
}

QTEST_MAIN(tst_QScrollBar)
#include "tst_qscrollbar.moc"
