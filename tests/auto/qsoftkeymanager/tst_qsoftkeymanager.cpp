/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>

#include "qevent.h"
#include "qdialog.h"
#include "qpushbutton.h"
#include "qdialogbuttonbox.h"
#include "private/qsoftkeymanager_p.h"

#ifdef Q_OS_SYMBIAN
#include "qsymbianevent.h"
#endif

#ifdef Q_WS_S60
static const int s60CommandStart = 6000;
#endif


class tst_QSoftKeyManager : public QObject
{
Q_OBJECT

public:
    tst_QSoftKeyManager();
    virtual ~tst_QSoftKeyManager();

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
private slots:
    void updateSoftKeysCompressed();
    void handleCommand();
    void checkSoftkeyEnableStates();
    void noMergingOverWindowBoundary();

private: // utils
    inline void simulateSymbianCommand(int command)
    {
        QSymbianEvent event1(QSymbianEvent::CommandEvent, command);
        qApp->symbianProcessEvent(&event1);
    };
};

class EventListener : public QObject
{
public:
    EventListener(QObject *listenTo)
    {
        resetCounts();
        if (listenTo)
            listenTo->installEventFilter(this);
    }

    void resetCounts()
    {
        numUpdateSoftKeys = 0;
    }

    int numUpdateSoftKeys;

protected:
    bool eventFilter(QObject * /*object*/, QEvent *event)
    {
        if (event->type() == QEvent::UpdateSoftKeys)
            numUpdateSoftKeys++;
        return false;
    }
};

tst_QSoftKeyManager::tst_QSoftKeyManager() : QObject()
{
}

tst_QSoftKeyManager::~tst_QSoftKeyManager()
{
}

void tst_QSoftKeyManager::initTestCase()
{
}

void tst_QSoftKeyManager::cleanupTestCase()
{
}

void tst_QSoftKeyManager::init()
{
}

void tst_QSoftKeyManager::cleanup()
{
}

/*
    This tests that we only get one UpdateSoftKeys event even though
    multiple events that trigger soft keys occur.
*/
void tst_QSoftKeyManager::updateSoftKeysCompressed()
{
    QWidget w;
    EventListener listener(qApp);

    QList<QAction *> softKeys;
    for (int i = 0; i < 10; ++i) {
        QAction *action = new QAction("foo", &w);
        action->setSoftKeyRole(QAction::PositiveSoftKey);
        softKeys << action;
    }
    w.addActions(softKeys);

    QApplication::processEvents();

    QVERIFY(listener.numUpdateSoftKeys == 1);
}

/*
    This tests that when the S60 environment sends us a command
    that it actually gets mapped to the correct action.
*/
void tst_QSoftKeyManager::handleCommand()
{
    QDialog w;
    QDialogButtonBox *buttons = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
            Qt::Horizontal,
            &w);

    w.show();
    QApplication::processEvents();

    QCOMPARE(w.actions().count(), 2);

    QSignalSpy spy0(w.actions()[0], SIGNAL(triggered()));
    QSignalSpy spy1(w.actions()[1], SIGNAL(triggered()));

    // These should work eventually, but do not yet
//    QTest::keyPress(&w, Qt::Key_Context1);
//    QTest::keyPress(&w, Qt::Key_Context2);

    simulateSymbianCommand(s60CommandStart); //0 = LSK position
    simulateSymbianCommand(s60CommandStart + 2); //2 = RSK position

    QApplication::processEvents();

    QCOMPARE(spy0.count(), 1);
    QCOMPARE(spy1.count(), 1);
}

/*
    This tests that the state of a widget that owns softkey action is respected when handling the softkey
    command.
*/
void tst_QSoftKeyManager::checkSoftkeyEnableStates()
{
    QDialog w;
    QDialogButtonBox *buttons = new QDialogButtonBox(
        QDialogButtonBox::RestoreDefaults | QDialogButtonBox::Help,
        Qt::Horizontal,
        &w);
    QPushButton *pBDefaults = buttons->button(QDialogButtonBox::RestoreDefaults);
    QPushButton *pBHelp = buttons->button(QDialogButtonBox::Help);
    pBHelp->setEnabled(false);
    w.show();
    QApplication::processEvents();

    //According to StandardButton enum in QDialogButtonBox the Help action
    //is inserted before RestoreDefaults and thus help action is in index 0
    QSignalSpy spy0(w.actions()[0], SIGNAL(triggered())); //disabled help action
    QSignalSpy spy1(w.actions()[1], SIGNAL(triggered())); //restore defaults action

    //Verify that enabled button gets all the action trigger signals and
    //disabled button gets none.
    for (int i = 0; i < 10; i++) {
        //simulate "help" softkey press
        simulateSymbianCommand(s60CommandStart);
        //simulate "Restore Defaults" softkey press
        simulateSymbianCommand(s60CommandStart + 2);
    }
    QApplication::processEvents();
    //Restore defaults button is enabled and its signals are recorded to spy1
    QCOMPARE(spy0.count(), 0);
    QCOMPARE(spy1.count(), 10);

    spy0.clear();
    spy1.clear();

    for (int i = 0; i < 10; i++) {
        //simulate "help" softkey press
        simulateSymbianCommand(s60CommandStart);
        //simulate "Restore Defaults" softkey press
        simulateSymbianCommand(s60CommandStart + 2);
        //switch enabled button to disabled and vice versa
        pBHelp->setEnabled(!pBHelp->isEnabled());
        pBDefaults->setEnabled(!pBDefaults->isEnabled());
    }
    QApplication::processEvents();
    QCOMPARE(spy0.count(), 5);
    QCOMPARE(spy1.count(), 5);
}

/*
    This tests that the softkeys are not merged over window boundaries. I.e. dialogs
    don't get softkeys of base widget by default - QTBUG-6163.
*/
void tst_QSoftKeyManager::noMergingOverWindowBoundary()
{
    // Create base window against which the dialog softkeys will ve verified
    QWidget base;

    QAction* baseLeft = new QAction(tr("BaseLeft"), &base);
    baseLeft->setSoftKeyRole(QAction::PositiveSoftKey);
    base.addAction(baseLeft);

    QAction* baseRight = new QAction(tr("BaseRight"), &base);
    baseRight->setSoftKeyRole(QAction::NegativeSoftKey);
    base.addAction(baseRight);

    base.showMaximized();
    QApplication::processEvents();

    QSignalSpy baseLeftSpy(baseLeft, SIGNAL(triggered()));
    QSignalSpy baseRightSpy(baseRight, SIGNAL(triggered()));

    //Verify that both base softkeys emit triggered signals
    simulateSymbianCommand(s60CommandStart);
    simulateSymbianCommand(s60CommandStart + 2);

    QCOMPARE(baseLeftSpy.count(), 1);
    QCOMPARE(baseRightSpy.count(), 1);
    baseLeftSpy.clear();
    baseRightSpy.clear();

    // Verify that no softkey merging when using dialog without parent
    QDialog dlg;
    dlg.show();

    QApplication::processEvents();

    simulateSymbianCommand(s60CommandStart);
    simulateSymbianCommand(s60CommandStart + 2);

    QCOMPARE(baseLeftSpy.count(), 0);
    QCOMPARE(baseRightSpy.count(), 0);

    // Ensure base view has focus again
    dlg.hide();
    base.showMaximized();

    // Verify that no softkey merging when using dialog with parent
    QDialog dlg2(&base);
    dlg2.show();

    QApplication::processEvents();

    simulateSymbianCommand(s60CommandStart);
    simulateSymbianCommand(s60CommandStart + 2);

    QCOMPARE(baseLeftSpy.count(), 0);
    QCOMPARE(baseRightSpy.count(), 0);
}

QTEST_MAIN(tst_QSoftKeyManager)
#include "tst_qsoftkeymanager.moc"
