/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
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

#include <qcoreapplication.h>
#include <qdebug.h>
#include <qsystemtrayicon.h>
#include <qmenu.h>

//TESTED_CLASS=
//TESTED_FILES=

class tst_QSystemTrayIcon: public QObject
{
Q_OBJECT

public:
    tst_QSystemTrayIcon();
    virtual ~tst_QSystemTrayIcon();

private slots:
    void getSetCheck();
    void showHide();
    void showMessage();
    void supportsMessages();
    void lastWindowClosed();
};

tst_QSystemTrayIcon::tst_QSystemTrayIcon()
{
}

tst_QSystemTrayIcon::~tst_QSystemTrayIcon()
{
}

// Testing get/set functions
void tst_QSystemTrayIcon::showHide()
{
    QSystemTrayIcon icon;
    icon.setIcon(QIcon("icons/icon.png"));
    icon.show();
    icon.setIcon(QIcon("icons/icon.png"));
    icon.hide();
}

// Testing get/set functions
void tst_QSystemTrayIcon::showMessage()
{
    QSystemTrayIcon icon;
    icon.setIcon(QIcon("icons/icon.png"));

    icon.showMessage("Title", "Messagecontents");
    icon.showMessage("Title", "Messagecontents", QSystemTrayIcon::NoIcon);
    icon.showMessage("Title", "Messagecontents", QSystemTrayIcon::Warning);
    icon.showMessage("Title", "Messagecontents", QSystemTrayIcon::Critical);

    icon.show();
    icon.showMessage("Title", "Messagecontents");
    icon.showMessage("Title", "Messagecontents", QSystemTrayIcon::NoIcon);
    icon.showMessage("Title", "Messagecontents", QSystemTrayIcon::Warning);
    icon.showMessage("Title", "Messagecontents", QSystemTrayIcon::Critical);
}

// Testing get/set functions
void tst_QSystemTrayIcon::getSetCheck()
{
    QSystemTrayIcon icon;
    QCOMPARE(true, icon.toolTip().isEmpty());
    icon.setToolTip("testToolTip");
    QCOMPARE(true, "testToolTip" == icon.toolTip());

    QCOMPARE(true, icon.icon().isNull());
    icon.setIcon(QIcon("icons/icon.png"));
    QCOMPARE(false, icon.icon().isNull());

    QMenu menu;
    QCOMPARE(true, icon.contextMenu() == 0);
    icon.setContextMenu(&menu);
    QCOMPARE(false, icon.contextMenu() == 0);
}

void tst_QSystemTrayIcon::supportsMessages()
{
#if !defined(Q_WS_QWS)
    QCOMPARE(QSystemTrayIcon::supportsMessages(), true );
#else
    QCOMPARE(QSystemTrayIcon::supportsMessages(), false );
#endif

}

void tst_QSystemTrayIcon::lastWindowClosed()
{
    QSignalSpy spy(qApp, SIGNAL(lastWindowClosed()));
    QWidget window;
    QSystemTrayIcon icon;
    icon.setIcon(QIcon("whatever.png"));
    icon.show();
    window.show();
    QTimer::singleShot(2500, &window, SLOT(close()));
    QTimer::singleShot(20000, qApp, SLOT(quit())); // in case the test fails
    qApp->exec();
    QVERIFY(spy.count() == 1);
}

QTEST_MAIN(tst_QSystemTrayIcon)
#include "tst_qsystemtrayicon.moc"
