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

#include <qtoolbox.h>

QT_FORWARD_DECLARE_CLASS(QToolBox)

class tst_QToolBoxPrivate;

class tst_QToolBox : public QObject
{
    Q_OBJECT

public:
    tst_QToolBox();
    virtual ~tst_QToolBox();

protected slots:
    void currentChanged(int);

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
private slots:
    void getSetCheck();
    void populate();
    void change();
    void clear();

private:
    QToolBox *testWidget;
    int currentIndex;

    tst_QToolBoxPrivate *d;
};

// Testing get/set functions
void tst_QToolBox::getSetCheck()
{
    QToolBox obj1;
    QWidget *w1 = new QWidget;
    QWidget *w2 = new QWidget;
    QWidget *w3 = new QWidget;
    QWidget *w4 = new QWidget;
    QWidget *w5 = new QWidget;
    obj1.addItem(w1, "Page1");
    obj1.addItem(w2, "Page2");
    obj1.addItem(w3, "Page3");
    obj1.addItem(w4, "Page4");
    obj1.addItem(w5, "Page5");

    // int QToolBox::currentIndex()
    // void QToolBox::setCurrentIndex(int)
    obj1.setCurrentIndex(3);
    QCOMPARE(3, obj1.currentIndex());
    obj1.setCurrentIndex(INT_MIN);
    QCOMPARE(3, obj1.currentIndex());
    obj1.setCurrentIndex(INT_MAX);
    QCOMPARE(3, obj1.currentIndex());
    obj1.setCurrentIndex(4);
    QCOMPARE(4, obj1.currentIndex());

    // QWidget * QToolBox::currentWidget()
    // void QToolBox::setCurrentWidget(QWidget *)
    obj1.setCurrentWidget(w1);
    QCOMPARE(w1, obj1.currentWidget());
    obj1.setCurrentWidget(w3);
    QCOMPARE(w3, obj1.currentWidget());

    obj1.setCurrentWidget((QWidget *)0);
    QCOMPARE(w3, obj1.currentWidget());
}

tst_QToolBox::tst_QToolBox()
{
}

tst_QToolBox::~tst_QToolBox()
{
}

class tst_QToolBoxPrivate
{
public:
    int count0, count1, count2, count3, count4;
    int idx1, idx2, idx3, idx32;
    int i0, i1, i2, i3, i4;
    int ci0, ci1, ci2, ci3, ci4;
    bool ci_correct;

    int manual_count;
};

void tst_QToolBox::init()
{
    currentIndex = -1;
    testWidget = new QToolBox;
    connect(testWidget, SIGNAL(currentChanged(int)), this, SLOT(currentChanged(int)));

    d = new tst_QToolBoxPrivate;


    d->count0 =  testWidget->count();
    d->ci0 =  currentIndex;

    QWidget *item1 = new QWidget( testWidget );
    testWidget->addItem( item1, "Item1" );

    d->count1 = testWidget->count();
    d->idx1 =  testWidget->indexOf(item1);
    d->ci1 = currentIndex;
    d->ci_correct = testWidget->widget(testWidget->currentIndex()) == item1;

    currentIndex = -1; // reset to make sure signal doesn't fire

    QWidget *item3 = new QWidget( testWidget );
    testWidget->addItem( item3, "Item3" );

    d->count2 = testWidget->count();
    d->idx3 =  testWidget->indexOf(item3);
    d->ci2 = currentIndex;


    QWidget *item2 = new QWidget( testWidget );
    testWidget->insertItem( 1, item2, "Item2");

    d->count3 = testWidget->count();
    d->idx2 =  testWidget->indexOf(item2);
    d->idx32 = testWidget->indexOf(item3);
    d->ci3 = currentIndex;

    QWidget *item0 = new QWidget( testWidget );
    testWidget->insertItem( 0, item0, "Item0");

    d->count4 =  testWidget->count();
    d->i0 =  testWidget->indexOf(item0);
    d->i1 = testWidget->indexOf(item1);
    d->i2 = testWidget->indexOf(item2);
    d->i3 =  testWidget->indexOf(item3);
    d->ci4 = currentIndex;

    d->manual_count = 4;
}

void tst_QToolBox::cleanup()
{
    delete testWidget;
    delete d;
}

void tst_QToolBox::initTestCase()
{
}

void tst_QToolBox::cleanupTestCase()
{
}

void tst_QToolBox::currentChanged(int index)
{
    currentIndex = index;
}

void tst_QToolBox::populate()
{
    // verify preconditions
    QCOMPARE( d->count0, 0 );
    QCOMPARE( d->ci0, -1 );
    QVERIFY( d->ci_correct );

    QCOMPARE( d->count1, 1 );
    QCOMPARE( d->idx1, 0 );
    QCOMPARE( d->ci1, 0 );

    QCOMPARE( d->count2, 2 );
    QCOMPARE( d->idx3, 1 );
    QCOMPARE( d->ci2, -1 );

    QCOMPARE( d->count3, 3 );
    QCOMPARE( d->idx2, 1 );
    QCOMPARE( d->idx32, 2 );
    QCOMPARE( d->ci3, -1 );


    QCOMPARE( d->count4, 4 );
    QCOMPARE( d->i0, 0 );
    QCOMPARE( d->i1, 1 );
    QCOMPARE( d->i2, 2 );
    QCOMPARE( d->i3, 3 );
    QCOMPARE( d->ci4, 1 );

    QCOMPARE (testWidget->count(), d->manual_count);
    int oldcount = testWidget->count();

    QWidget *item = new QWidget( testWidget );
    testWidget->addItem( item, "Item");
    d->manual_count++;

    QCOMPARE( testWidget->count(), oldcount+1 );
    QCOMPARE( testWidget->indexOf(item), oldcount );
    QCOMPARE( testWidget->widget(oldcount), item );
}

void tst_QToolBox::change()
{
    QWidget *lastItem = testWidget->widget(testWidget->count());
    QVERIFY( !lastItem );
    lastItem = testWidget->widget(testWidget->count() - 1);
    QVERIFY( lastItem );

    for ( int c = 0; c < testWidget->count(); ++c ) {
	QString label = "Item " + QString::number(c);
	testWidget->setItemText(c, label);
	QCOMPARE( testWidget->itemText(c), label );
    }

    testWidget->setCurrentIndex( 0 );
    QCOMPARE( currentIndex, 0 );

    currentIndex = -1; // reset to make sure signal doesn't fire
    testWidget->setCurrentIndex( 0 );
    QCOMPARE( currentIndex, -1 );
    QCOMPARE( testWidget->currentIndex(), 0 );

    testWidget->setCurrentIndex( testWidget->count() );
    QCOMPARE( currentIndex, -1 );
    QCOMPARE( testWidget->currentIndex(), 0 );

    testWidget->setCurrentIndex( 1 );
    QCOMPARE( currentIndex, 1 );
    QCOMPARE( testWidget->currentIndex(), 1 );

    testWidget->setItemEnabled( testWidget->currentIndex(), false );
    QCOMPARE( currentIndex, 2 );
    QCOMPARE( testWidget->currentIndex(), 2 );

    currentIndex = -1;
    testWidget->setItemEnabled( testWidget->indexOf(lastItem), false );
    QCOMPARE( currentIndex, -1 );
    QCOMPARE( testWidget->currentIndex(), 2 );

    testWidget->setItemEnabled( testWidget->currentIndex(), false );
    QCOMPARE( currentIndex, 0 );

    currentIndex = -1;
    testWidget->setItemEnabled( testWidget->currentIndex(), false );
    QCOMPARE( currentIndex, -1 );

    testWidget->setItemEnabled( 1, true );
}

void tst_QToolBox::clear()
{
    // precondition: only item(1) is enabled
    QCOMPARE( testWidget->count(), 4 );
    testWidget->setCurrentIndex(0);
    currentIndex = -1;

    // delete current item(0)
    QPointer<QWidget> item = testWidget->widget(testWidget->currentIndex());
    testWidget->removeItem(testWidget->indexOf(item));
    QVERIFY(item);
    QCOMPARE( testWidget->count(), 3 );
    QCOMPARE( testWidget->indexOf(item), -1 );
    QCOMPARE( testWidget->currentIndex(), 0 );
    QCOMPARE(currentIndex, 0 );

    currentIndex = -1;

    item = testWidget->widget(1);
    testWidget->removeItem(testWidget->indexOf(item));
    QVERIFY( item );
    QCOMPARE( currentIndex, -1 );
    QCOMPARE( testWidget->currentIndex(), 0 );
    QCOMPARE( testWidget->count(), 2 );
    QCOMPARE( testWidget->indexOf(item), -1 );

    item = testWidget->widget(1);
    delete item;
    QCOMPARE( testWidget->count(), 1 );
    QCOMPARE( currentIndex, -1 );
    currentIndex = testWidget->currentIndex();

    item = testWidget->widget(0);
    testWidget->removeItem(testWidget->indexOf(item));
    QCOMPARE( testWidget->count(), 0 );
    QCOMPARE( currentIndex, -1 );
}

QTEST_MAIN(tst_QToolBox)
#include "tst_qtoolbox.moc"
