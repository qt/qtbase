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


#include "qbuttongroup.h"
#include <qaction.h>
#include <qapplication.h>
#include <qdialog.h>
#include <qlayout.h>
#include <qgroupbox.h>
#include <qradiobutton.h>
#include <qpushbutton.h>
#include <qlineedit.h>
#include <qtoolbutton.h>
#ifdef Q_OS_MAC
#include <qsettings.h>
#endif

class SpecialRadioButton: public QRadioButton
{
public:
    SpecialRadioButton(QWidget *parent) : QRadioButton(parent)
    { }

protected:
    void focusInEvent(QFocusEvent *)
    {
        QCoreApplication::postEvent(this, new QKeyEvent(QEvent::KeyPress,
                                                        Qt::Key_Down, Qt::NoModifier));
    }
};

#include <qbuttongroup.h>

class tst_QButtonGroup : public QObject
{
Q_OBJECT
public:
    tst_QButtonGroup();

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
private slots:
    void arrowKeyNavigation();
    void exclusive();
    void exclusiveWithActions();
    void testSignals();
    void checkedButton();

    void task106609();

    void autoIncrementId();

    void task209485_removeFromGroupInEventHandler_data();
    void task209485_removeFromGroupInEventHandler();
};

tst_QButtonGroup::tst_QButtonGroup()
{
}


void tst_QButtonGroup::initTestCase()
{
}

void tst_QButtonGroup::cleanupTestCase()
{
}

void tst_QButtonGroup::init()
{
}

void tst_QButtonGroup::cleanup()
{
}

QT_BEGIN_NAMESPACE
extern bool Q_GUI_EXPORT qt_tab_all_widgets();
QT_END_NAMESPACE


void tst_QButtonGroup::arrowKeyNavigation()
{
    if (!qt_tab_all_widgets())
        QSKIP("This test requires full keyboard control to be enabled.");

    QDialog dlg(0);
    QHBoxLayout layout(&dlg);
    QGroupBox g1("1", &dlg);
    QHBoxLayout g1layout(&g1);
    QRadioButton bt1("Radio1", &g1);
    QPushButton pb("PB", &g1);
    QLineEdit le(&g1);
    QRadioButton bt2("Radio2", &g1);
    g1layout.addWidget(&bt1);
    g1layout.addWidget(&pb);
    g1layout.addWidget(&le);
    g1layout.addWidget(&bt2);

    // create a mixed button group with radion buttons and push
    // buttons. Not very useful, but it tests borderline cases wrt
    // focus handling.
    QButtonGroup bgrp1(&g1);
    bgrp1.addButton(&bt1);
    bgrp1.addButton(&pb);
    bgrp1.addButton(&bt2);

    QGroupBox g2("2", &dlg);
    QVBoxLayout g2layout(&g2);
    // we don't need a button group here, because radio buttons are
    // auto exclusive, i.e. they group themselves in he same parent
    // widget.
    QRadioButton bt3("Radio3", &g2);
    QRadioButton bt4("Radio4", &g2);
    g2layout.addWidget(&bt3);
    g2layout.addWidget(&bt4);

    layout.addWidget(&g1);
    layout.addWidget(&g2);

    dlg.show();
    qApp->setActiveWindow(&dlg);
    QVERIFY(QTest::qWaitForWindowActive(&dlg));

    bt1.setFocus();

    QTRY_VERIFY(bt1.hasFocus());

    QTest::keyClick(&bt1, Qt::Key_Right);
    QVERIFY(pb.hasFocus());
    QTest::keyClick(&pb, Qt::Key_Right);
    QVERIFY(bt2.hasFocus());
    QTest::keyClick(&bt2, Qt::Key_Right);
    QVERIFY(bt2.hasFocus());
    QTest::keyClick(&bt2, Qt::Key_Left);
    QVERIFY(pb.hasFocus());
    QTest::keyClick(&pb, Qt::Key_Left);
    QVERIFY(bt1.hasFocus());

    QTest::keyClick(&bt1, Qt::Key_Tab);
    QVERIFY(pb.hasFocus());
    QTest::keyClick(&pb, Qt::Key_Tab);

    QVERIFY(le.hasFocus());
    QCOMPARE(le.selectedText(), le.text());
    QTest::keyClick(&le, Qt::Key_Tab);

    QVERIFY(bt2.hasFocus());
    QTest::keyClick(&bt2, Qt::Key_Tab);
    QVERIFY(bt3.hasFocus());

    QTest::keyClick(&bt3, Qt::Key_Down);
    QVERIFY(bt4.hasFocus());
    QTest::keyClick(&bt4, Qt::Key_Down);
    QVERIFY(bt4.hasFocus());

    QTest::keyClick(&bt4, Qt::Key_Up);
    QVERIFY(bt3.hasFocus());
    QTest::keyClick(&bt3, Qt::Key_Up);
    QVERIFY(bt3.hasFocus());
}

void tst_QButtonGroup::exclusiveWithActions()
{
    QDialog dlg(0);
    QHBoxLayout layout(&dlg);
    QAction *action1 = new QAction("Action 1", &dlg);
    action1->setCheckable(true);
    QAction *action2 = new QAction("Action 2", &dlg);
    action2->setCheckable(true);
    QAction *action3 = new QAction("Action 3", &dlg);
    action3->setCheckable(true);
    QToolButton *toolButton1 = new QToolButton(&dlg);
    QToolButton *toolButton2 = new QToolButton(&dlg);
    QToolButton *toolButton3 = new QToolButton(&dlg);
    toolButton1->setDefaultAction(action1);
    toolButton2->setDefaultAction(action2);
    toolButton3->setDefaultAction(action3);
    layout.addWidget(toolButton1);
    layout.addWidget(toolButton2);
    layout.addWidget(toolButton3);
    QButtonGroup *buttonGroup = new QButtonGroup( &dlg );
    buttonGroup->setExclusive(true);
    buttonGroup->addButton(toolButton1, 1);
    buttonGroup->addButton(toolButton2, 2);
    buttonGroup->addButton(toolButton3, 3);
    dlg.show();

    QTest::mouseClick(toolButton1, Qt::LeftButton);
    QVERIFY(toolButton1->isChecked());
    QVERIFY(action1->isChecked());
    QVERIFY(!toolButton2->isChecked());
    QVERIFY(!toolButton3->isChecked());
    QVERIFY(!action2->isChecked());
    QVERIFY(!action3->isChecked());

    QTest::mouseClick(toolButton2, Qt::LeftButton);
    QVERIFY(toolButton2->isChecked());
    QVERIFY(action2->isChecked());
    QVERIFY(!toolButton1->isChecked());
    QVERIFY(!toolButton3->isChecked());
    QVERIFY(!action1->isChecked());
    QVERIFY(!action3->isChecked());

    QTest::mouseClick(toolButton3, Qt::LeftButton);
    QVERIFY(toolButton3->isChecked());
    QVERIFY(action3->isChecked());
    QVERIFY(!toolButton1->isChecked());
    QVERIFY(!toolButton2->isChecked());
    QVERIFY(!action1->isChecked());
    QVERIFY(!action2->isChecked());

    QTest::mouseClick(toolButton2, Qt::LeftButton);
    QVERIFY(toolButton2->isChecked());
    QVERIFY(action2->isChecked());
    QVERIFY(!toolButton1->isChecked());
    QVERIFY(!toolButton3->isChecked());
    QVERIFY(!action1->isChecked());
    QVERIFY(!action3->isChecked());
}

void tst_QButtonGroup::exclusive()
{
    QDialog dlg(0);
    QHBoxLayout layout(&dlg);
    QPushButton *pushButton1 = new QPushButton(&dlg);
    QPushButton *pushButton2 = new QPushButton(&dlg);
    QPushButton *pushButton3 = new QPushButton(&dlg);
    pushButton1->setCheckable(true);
    pushButton2->setCheckable(true);
    pushButton3->setCheckable(true);
    layout.addWidget(pushButton1);
    layout.addWidget(pushButton2);
    layout.addWidget(pushButton3);
    QButtonGroup *buttonGroup = new QButtonGroup( &dlg );
    buttonGroup->setExclusive(true);
    buttonGroup->addButton(pushButton1, 1);
    buttonGroup->addButton(pushButton2, 2);
    buttonGroup->addButton(pushButton3, 3);
    dlg.show();

    QTest::mouseClick(pushButton1, Qt::LeftButton);
    QVERIFY(pushButton1->isChecked());
    QVERIFY(!pushButton2->isChecked());
    QVERIFY(!pushButton3->isChecked());

    QTest::mouseClick(pushButton2, Qt::LeftButton);
    QVERIFY(pushButton2->isChecked());
    QVERIFY(!pushButton1->isChecked());
    QVERIFY(!pushButton3->isChecked());

    QTest::mouseClick(pushButton3, Qt::LeftButton);
    QVERIFY(pushButton3->isChecked());
    QVERIFY(!pushButton1->isChecked());
    QVERIFY(!pushButton2->isChecked());

    QTest::mouseClick(pushButton2, Qt::LeftButton);
    QVERIFY(pushButton2->isChecked());
    QVERIFY(!pushButton1->isChecked());
    QVERIFY(!pushButton3->isChecked());
}

void tst_QButtonGroup::testSignals()
{
    QButtonGroup buttons;
    QPushButton pb1;
    QPushButton pb2;
    QPushButton pb3;
    buttons.addButton(&pb1);
    buttons.addButton(&pb2, 23);
    buttons.addButton(&pb3);

    qRegisterMetaType<QAbstractButton *>("QAbstractButton *");
    QSignalSpy clickedSpy(&buttons, SIGNAL(buttonClicked(QAbstractButton*)));
    QSignalSpy clickedIdSpy(&buttons, SIGNAL(buttonClicked(int)));
    QSignalSpy pressedSpy(&buttons, SIGNAL(buttonPressed(QAbstractButton*)));
    QSignalSpy pressedIdSpy(&buttons, SIGNAL(buttonPressed(int)));
    QSignalSpy releasedSpy(&buttons, SIGNAL(buttonReleased(QAbstractButton*)));
    QSignalSpy releasedIdSpy(&buttons, SIGNAL(buttonReleased(int)));

    pb1.animateClick();
    QTestEventLoop::instance().enterLoop(1);

    QCOMPARE(clickedSpy.count(), 1);
    QCOMPARE(clickedIdSpy.count(), 1);

    int expectedId = -2;

    QVERIFY(clickedIdSpy.takeFirst().at(0).toInt() == expectedId);
    QCOMPARE(pressedSpy.count(), 1);
    QCOMPARE(pressedIdSpy.count(), 1);
    QVERIFY(pressedIdSpy.takeFirst().at(0).toInt() == expectedId);
    QCOMPARE(releasedSpy.count(), 1);
    QCOMPARE(releasedIdSpy.count(), 1);
    QVERIFY(releasedIdSpy.takeFirst().at(0).toInt() == expectedId);

    clickedSpy.clear();
    clickedIdSpy.clear();
    pressedSpy.clear();
    pressedIdSpy.clear();
    releasedSpy.clear();
    releasedIdSpy.clear();

    pb2.animateClick();
    QTestEventLoop::instance().enterLoop(1);

    QCOMPARE(clickedSpy.count(), 1);
    QCOMPARE(clickedIdSpy.count(), 1);
    QVERIFY(clickedIdSpy.takeFirst().at(0).toInt() == 23);
    QCOMPARE(pressedSpy.count(), 1);
    QCOMPARE(pressedIdSpy.count(), 1);
    QVERIFY(pressedIdSpy.takeFirst().at(0).toInt() == 23);
    QCOMPARE(releasedSpy.count(), 1);
    QCOMPARE(releasedIdSpy.count(), 1);
    QVERIFY(releasedIdSpy.takeFirst().at(0).toInt() == 23);
}

void tst_QButtonGroup::task106609()
{
    // task is:
    // sometimes, only one of the two signals in QButtonGroup get emitted
    // they get emitted when using the mouse, but when using the keyboard, only one is
    //

    QDialog dlg(0);
    QButtonGroup *buttons = new QButtonGroup(&dlg);
    QVBoxLayout *vbox = new QVBoxLayout(&dlg);

    SpecialRadioButton *radio1 = new SpecialRadioButton(&dlg);
    radio1->setText("radio1");
    SpecialRadioButton *radio2 = new SpecialRadioButton(&dlg);
    radio2->setText("radio2");
    QRadioButton *radio3 = new QRadioButton(&dlg);
    radio3->setText("radio3");

    buttons->addButton(radio1, 1);
    vbox->addWidget(radio1);
    buttons->addButton(radio2, 2);
    vbox->addWidget(radio2);
    buttons->addButton(radio3, 3);
    vbox->addWidget(radio3);
    dlg.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dlg));

    qRegisterMetaType<QAbstractButton*>("QAbstractButton*");
    QSignalSpy spy1(buttons, SIGNAL(buttonClicked(QAbstractButton*)));
    QSignalSpy spy2(buttons, SIGNAL(buttonClicked(int)));

    QApplication::setActiveWindow(&dlg);
    QTRY_COMPARE(QApplication::activeWindow(), static_cast<QWidget*>(&dlg));

    radio1->setFocus();
    radio1->setChecked(true);
    QTestEventLoop::instance().enterLoop(1);

    //qDebug() << "int:" << spy2.count() << "QAbstractButton*:" << spy1.count();
    QCOMPARE(spy2.count(), 2);
    QCOMPARE(spy1.count(), 2);
}

void tst_QButtonGroup::checkedButton()
{
    QButtonGroup buttons;
    buttons.setExclusive(false);
    QPushButton pb1;
    pb1.setCheckable(true);
    QPushButton pb2;
    pb2.setCheckable(true);
    buttons.addButton(&pb1);
    buttons.addButton(&pb2, 23);

    QVERIFY(buttons.checkedButton() == 0);
    pb1.setChecked(true);
    QVERIFY(buttons.checkedButton() == &pb1);
    pb2.setChecked(true);
    QVERIFY(buttons.checkedButton() == &pb2);
    pb2.setChecked(false);
    QVERIFY(buttons.checkedButton() == &pb1);
    pb1.setChecked(false);
    QVERIFY(buttons.checkedButton() == 0);

    buttons.setExclusive(true);
    QVERIFY(buttons.checkedButton() == 0);
    pb1.setChecked(true);
    QVERIFY(buttons.checkedButton() == &pb1);
    pb2.setChecked(true);
    QVERIFY(buttons.checkedButton() == &pb2);
    // checked button cannot be unchecked
    pb2.setChecked(false);
    QVERIFY(buttons.checkedButton() == &pb2);
}

class task209485_ButtonDeleter : public QObject
{
    Q_OBJECT

public:
    task209485_ButtonDeleter(QButtonGroup *group, bool deleteButton)
        : group(group)
        , deleteButton(deleteButton)
    {
        connect(group, SIGNAL(buttonClicked(int)), SLOT(buttonClicked(int)));
    }

private slots:
    void buttonClicked(int)
    {
        if (deleteButton)
            group->removeButton(group->buttons().first());
    }

private:
    QButtonGroup *group;
    bool deleteButton;
};

void tst_QButtonGroup::task209485_removeFromGroupInEventHandler_data()
{
    QTest::addColumn<bool>("deleteButton");
    QTest::addColumn<int>("signalCount");
    QTest::newRow("buttonPress 1") << true << 1;
    QTest::newRow("buttonPress 2") << false << 2;
}

void tst_QButtonGroup::task209485_removeFromGroupInEventHandler()
{
    QFETCH(bool, deleteButton);
    QFETCH(int, signalCount);
    qRegisterMetaType<QAbstractButton *>("QAbstractButton *");

    QPushButton *button = new QPushButton;
    QButtonGroup group;
    group.addButton(button);

    task209485_ButtonDeleter buttonDeleter(&group, deleteButton);

    QSignalSpy spy1(&group, SIGNAL(buttonClicked(QAbstractButton*)));
    QSignalSpy spy2(&group, SIGNAL(buttonClicked(int)));

    // NOTE: Reintroducing the bug of this task will cause the following line to crash:
    QTest::mouseClick(button, Qt::LeftButton);

    QCOMPARE(spy1.count() + spy2.count(), signalCount);
}

void tst_QButtonGroup::autoIncrementId()
{
    QDialog dlg(0);
    QButtonGroup *buttons = new QButtonGroup(&dlg);
    QVBoxLayout *vbox = new QVBoxLayout(&dlg);

    QRadioButton *radio1 = new QRadioButton(&dlg);
    radio1->setText("radio1");
    QRadioButton *radio2 = new QRadioButton(&dlg);
    radio2->setText("radio2");
    QRadioButton *radio3 = new QRadioButton(&dlg);
    radio3->setText("radio3");

    buttons->addButton(radio1);
    vbox->addWidget(radio1);
    buttons->addButton(radio2);
    vbox->addWidget(radio2);
    buttons->addButton(radio3);
    vbox->addWidget(radio3);

    radio1->setChecked(true);

    QVERIFY(buttons->id(radio1) == -2);
    QVERIFY(buttons->id(radio2) == -3);
    QVERIFY(buttons->id(radio3) == -4);

    dlg.show();
}

QTEST_MAIN(tst_QButtonGroup)
#include "tst_qbuttongroup.moc"
