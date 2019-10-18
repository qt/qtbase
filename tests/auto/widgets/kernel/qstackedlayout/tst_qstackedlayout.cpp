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


#include <QtTest/QtTest>
#include <QLineEdit>
#include <QLabel>
#include <QStackedLayout>
#include <qapplication.h>
#include <qwidget.h>
#include <QPushButton>

class tst_QStackedLayout : public QObject
{
    Q_OBJECT

public:
    tst_QStackedLayout();

private slots:
    void init();
    void cleanup();

    void getSetCheck();
    void testCase();
    void deleteCurrent();
    void removeWidget();
    void keepFocusAfterSetCurrent();
    void heigthForWidth();
    void replaceWidget();

private:
    QWidget *testWidget;
};

// Testing get/set functions
void tst_QStackedLayout::getSetCheck()
{
    QStackedLayout obj1;
    // int QStackedLayout::currentIndex()
    // void QStackedLayout::setCurrentIndex(int)
    obj1.setCurrentIndex(0);
    QCOMPARE(-1, obj1.currentIndex());
    obj1.setCurrentIndex(INT_MIN);
    QCOMPARE(-1, obj1.currentIndex());
    obj1.setCurrentIndex(INT_MAX);
    QCOMPARE(-1, obj1.currentIndex());

    // QWidget * QStackedLayout::currentWidget()
    // void QStackedLayout::setCurrentWidget(QWidget *)
    QWidget *var2 = new QWidget();
    obj1.addWidget(var2);
    obj1.setCurrentWidget(var2);
    QCOMPARE(var2, obj1.currentWidget());

    obj1.setCurrentWidget((QWidget *)0);
    QCOMPARE(obj1.currentWidget(), var2);

    delete var2;
}


tst_QStackedLayout::tst_QStackedLayout()
    : testWidget(0)
{
}

void tst_QStackedLayout::init()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    if (testWidget) {
        delete testWidget;
        testWidget = 0;
    }
    testWidget = new QWidget(0);
    testWidget->resize( 200, 200 );
    testWidget->show();

    // make sure the tests work with focus follows mouse
    QCursor::setPos(testWidget->geometry().center());
    testWidget->activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(testWidget));
}

void tst_QStackedLayout::cleanup()
{
    delete testWidget;
    testWidget = 0;
}

void tst_QStackedLayout::testCase()
{
    QStackedLayout onStack(testWidget);
    QStackedLayout *testLayout = &onStack;
    testWidget->setLayout(testLayout);

    QSignalSpy spy(testLayout,SIGNAL(currentChanged(int)));

    // Nothing in layout
    QCOMPARE(testLayout->currentIndex(), -1);
    QCOMPARE(testLayout->currentWidget(), nullptr);
    QCOMPARE(testLayout->count(), 0);

    // One widget added to layout
    QWidget *w1 = new QWidget(testWidget);
    testLayout->addWidget(w1);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toInt(), 0);
    spy.clear();
    QCOMPARE(testLayout->currentIndex(), 0);
    QCOMPARE(testLayout->currentWidget(), w1);
    QCOMPARE(testLayout->count(), 1);

    // Another widget added to layout
    QWidget *w2 = new QWidget(testWidget);
    testLayout->addWidget(w2);
    QCOMPARE(testLayout->currentIndex(), 0);
    QCOMPARE(testLayout->currentWidget(), w1);
    QCOMPARE(testLayout->indexOf(w2), 1);
    QCOMPARE(testLayout->count(), 2);

    // Change the current index
    testLayout->setCurrentIndex(1);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toInt(), 1);
    spy.clear();
    QCOMPARE(testLayout->currentIndex(), 1);
    QCOMPARE(testLayout->currentWidget(), w2);

    // First widget removed from layout
    testLayout->removeWidget(w1);
    QCOMPARE(testLayout->currentIndex(), 0);
    QCOMPARE(testLayout->currentWidget(), w2);
    QCOMPARE(testLayout->count(), 1);

    // Second widget removed from layout; back to nothing
    testLayout->removeWidget(w2);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toInt(), -1);
    spy.clear();
    QCOMPARE(testLayout->currentIndex(), -1);
    QCOMPARE(testLayout->currentWidget(), nullptr);
    QCOMPARE(testLayout->count(), 0);

    // Another widget inserted at current index.
    // Current index should become current index + 1, but the
    // current widget should stay the same
    testLayout->addWidget(w1);
    QCOMPARE(testLayout->currentIndex(), 0);
    QCOMPARE(testLayout->currentWidget(), w1);
    testLayout->insertWidget(0, w2);
    QCOMPARE(testLayout->currentIndex(), 1);
    QCOMPARE(testLayout->currentWidget(), w1);
    QVERIFY(w1->isVisible());
    QVERIFY(!w2->isVisible());

    testLayout->setCurrentWidget(w2);
    // Another widget added, so we have: w2, w1, w3 with w2 current
    QWidget *w3 = new QWidget(testWidget);
    testLayout->addWidget(w3);
    QCOMPARE(testLayout->indexOf(w2), 0);
    QCOMPARE(testLayout->indexOf(w1), 1);
    QCOMPARE(testLayout->indexOf(w3), 2);

    // Set index to 1 and remove that widget (w1).
    // Then, current index should still be 1, but w3
    // should be the new current widget.
    testLayout->setCurrentIndex(1);
    testLayout->removeWidget(w1);
    QCOMPARE(testLayout->currentIndex(), 1);
    QCOMPARE(testLayout->currentWidget(), w3);
    QVERIFY(w3->isVisible());

    // Remove the current widget (w3).
    // Since it's the last one in the list, current index should now
    // become 0 and w2 becomes the current widget.
    testLayout->removeWidget(w3);
    QCOMPARE(testLayout->currentIndex(), 0);
    QCOMPARE(testLayout->currentWidget(), w2);
    QVERIFY(w2->isVisible());

    // Make sure index is decremented when we remove a widget at index < current index
    testLayout->addWidget(w1);
    testLayout->addWidget(w3);
    testLayout->setCurrentIndex(2);
    testLayout->removeWidget(w2); // At index 0
    QCOMPARE(testLayout->currentIndex(), 1);
    QCOMPARE(testLayout->currentWidget(), w3);
    QVERIFY(w3->isVisible());
    testLayout->removeWidget(w1); // At index 0
    QCOMPARE(testLayout->currentIndex(), 0);
    QCOMPARE(testLayout->currentWidget(), w3);
    QVERIFY(w3->isVisible());
    testLayout->removeWidget(w3);
    QCOMPARE(testLayout->currentIndex(), -1);
    QCOMPARE(testLayout->currentWidget(), nullptr);
}

void tst_QStackedLayout::deleteCurrent()
{
    QStackedLayout *testLayout = new QStackedLayout(testWidget);

    QWidget *w1 = new QWidget;
    testLayout->addWidget(w1);
    QWidget *w2 = new QWidget;
    testLayout->addWidget(w2);
    QCOMPARE(testLayout->currentWidget(), w1);
    delete testLayout->currentWidget();
    QCOMPARE(testLayout->currentWidget(), w2);
}

void tst_QStackedLayout::removeWidget()
{
    if (testWidget->layout()) delete testWidget->layout();
    QVBoxLayout *vbox = new QVBoxLayout(testWidget);

    QPushButton *top = new QPushButton("top", testWidget);  //add another widget that can receive focus
    top->setObjectName("top");
    vbox->addWidget(top);

    QStackedLayout *testLayout = new QStackedLayout();
    QPushButton *w1 = new QPushButton("1st", testWidget);
    w1->setObjectName("1st");
    testLayout->addWidget(w1);
    QPushButton *w2 = new QPushButton("2nd", testWidget);
    w2->setObjectName("2nd");
    testLayout->addWidget(w2);
    vbox->addLayout(testLayout);
    top->setFocus();
    top->activateWindow();
    QTRY_COMPARE(QApplication::focusWidget(), top);

    // focus should stay at the 'top' widget
    testLayout->removeWidget(w1);

    QCOMPARE(QApplication::focusWidget(), top);
}

class LineEdit : public QLineEdit
{
public:
    LineEdit() : hasFakeEditFocus(false)
    { }

    bool hasFakeEditFocus;

protected:
    bool isSingleFocusWidget() const
    {
        const QWidget *w = this;
        while ((w = w->nextInFocusChain()) != this) {
            if (w->isVisible() && static_cast<const QWidget*>(w->focusProxy()) != this
                && w->focusPolicy() & Qt::TabFocus) {
                return false;
            }
        }
        return true;
    }

    void focusInEvent(QFocusEvent *event)
    {
        QLineEdit::focusInEvent(event);
        hasFakeEditFocus = isSingleFocusWidget();
    }

    void focusOutEvent(QFocusEvent *event)
    {
        hasFakeEditFocus = false;
        QLineEdit::focusOutEvent(event);
    }
};

void tst_QStackedLayout::keepFocusAfterSetCurrent()
{
    if (testWidget->layout()) delete testWidget->layout();
    QStackedLayout *stackLayout = new QStackedLayout(testWidget);
    testWidget->setFocusPolicy(Qt::NoFocus);

    LineEdit *edit1 = new LineEdit;
    LineEdit *edit2 = new LineEdit;
    stackLayout->addWidget(edit1);
    stackLayout->addWidget(edit2);

    stackLayout->setCurrentIndex(0);

    testWidget->show();
    QApplication::setActiveWindow(testWidget);
    QVERIFY(QTest::qWaitForWindowActive(testWidget));

    edit1->setFocus();
    edit1->activateWindow();

    QTRY_VERIFY(edit1->hasFocus());

    stackLayout->setCurrentIndex(1);
    QVERIFY(!edit1->hasFocus());
    QVERIFY(edit2->hasFocus());
    QVERIFY(edit2->hasFakeEditFocus);
}

void tst_QStackedLayout::heigthForWidth()
{
    if (testWidget->layout()) delete testWidget->layout();
    QStackedLayout *stackLayout = new QStackedLayout(testWidget);

    QLabel *shortLabel = new QLabel("This is a short text.");
    shortLabel->setWordWrap(true);
    stackLayout->addWidget(shortLabel);

    QLabel *longLabel = new QLabel("Write code once to target multiple platforms\n"
                         "Qt allows you to write advanced applications and UIs once, "
                         "and deploy them across desktop and embedded operating systems "
                         "without rewriting the source code saving time and development cost.\n\n"
                         "Create amazing user experiences\n"
                         "Whether you prefer C++ or JavaScript, Qt provides the building blocks - "
                         "a broad set of customizable widgets, graphics canvas, style engine "
                         "and more that you need to build modern user interfaces. "
                         "Incorporate 3D graphics, multimedia audio or video, visual effects, "
                         "and animations to set your application apart from the competition.");

    longLabel->setWordWrap(true);
    stackLayout->addWidget(longLabel);
    stackLayout->setCurrentIndex(0);
    int hfw_index0 = stackLayout->heightForWidth(200);

    stackLayout->setCurrentIndex(1);
    QCOMPARE(stackLayout->heightForWidth(200), hfw_index0);

}

void tst_QStackedLayout::replaceWidget()
{
    QWidget w;
    QStackedLayout *stackLayout = new QStackedLayout(&w);

    QLineEdit *replaceFrom = new QLineEdit;
    QLineEdit *replaceTo = new QLineEdit;
    stackLayout->addWidget(new QLineEdit());
    stackLayout->addWidget(replaceFrom);
    stackLayout->addWidget(new QLineEdit());
    stackLayout->setCurrentWidget(replaceFrom);

    QCOMPARE(stackLayout->indexOf(replaceFrom), 1);
    QCOMPARE(stackLayout->indexOf(replaceTo), -1);
    delete stackLayout->replaceWidget(replaceFrom, replaceTo);

    QCOMPARE(stackLayout->indexOf(replaceFrom), -1);
    QCOMPARE(stackLayout->indexOf(replaceTo), 1);
    QCOMPARE(stackLayout->currentWidget(), replaceTo);
}

QTEST_MAIN(tst_QStackedLayout)
#include "tst_qstackedlayout.moc"

