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

#include <QtDBus/QDBusServiceWatcher>
#include <QtDBus>
#include <QtTest>

class tst_QDBusServiceWatcher: public QObject
{
    Q_OBJECT
    QString serviceName;
    int testCounter;
public:
    tst_QDBusServiceWatcher();

private slots:
    void initTestCase();
    void init();

    void watchForCreation();
    void watchForDisappearance();
    void watchForOwnerChange();
    void modeChange();
};

tst_QDBusServiceWatcher::tst_QDBusServiceWatcher()
    : testCounter(0)
{
}

void tst_QDBusServiceWatcher::initTestCase()
{
    QDBusConnection con = QDBusConnection::sessionBus();
    QVERIFY(con.isConnected());
}

void tst_QDBusServiceWatcher::init()
{
    // change the service name from test to test
    serviceName = "com.example.TestService" + QString::number(testCounter++);
}

void tst_QDBusServiceWatcher::watchForCreation()
{
    QDBusConnection con = QDBusConnection::sessionBus();
    QVERIFY(con.isConnected());

    QDBusServiceWatcher watcher(serviceName, con, QDBusServiceWatcher::WatchForRegistration);

    QSignalSpy spyR(&watcher, SIGNAL(serviceRegistered(QString)));
    QSignalSpy spyU(&watcher, SIGNAL(serviceUnregistered(QString)));
    QSignalSpy spyO(&watcher, SIGNAL(serviceOwnerChanged(QString,QString,QString)));
    QTestEventLoop::instance().connect(&watcher, SIGNAL(serviceRegistered(QString)), SLOT(exitLoop()));

    // register a name
    QVERIFY(con.registerService(serviceName));

    QTestEventLoop::instance().enterLoop(1);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QCOMPARE(spyR.count(), 1);
    QCOMPARE(spyR.at(0).at(0).toString(), serviceName);

    QCOMPARE(spyU.count(), 0);

    QCOMPARE(spyO.count(), 1);
    QCOMPARE(spyO.at(0).at(0).toString(), serviceName);
    QVERIFY(spyO.at(0).at(1).toString().isEmpty());
    QCOMPARE(spyO.at(0).at(2).toString(), con.baseService());

    spyR.clear();
    spyU.clear();
    spyO.clear();

    // unregister it:
    con.unregisterService(serviceName);

    // and register again
    QVERIFY(con.registerService(serviceName));

    QTestEventLoop::instance().enterLoop(1);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QCOMPARE(spyR.count(), 1);
    QCOMPARE(spyR.at(0).at(0).toString(), serviceName);

    QCOMPARE(spyU.count(), 0);

    QCOMPARE(spyO.count(), 1);
    QCOMPARE(spyO.at(0).at(0).toString(), serviceName);
    QVERIFY(spyO.at(0).at(1).toString().isEmpty());
    QCOMPARE(spyO.at(0).at(2).toString(), con.baseService());
}

void tst_QDBusServiceWatcher::watchForDisappearance()
{
    QDBusConnection con = QDBusConnection::sessionBus();
    QVERIFY(con.isConnected());

    QDBusServiceWatcher watcher(serviceName, con, QDBusServiceWatcher::WatchForUnregistration);
    watcher.setObjectName("watcher for disappearance");

    QSignalSpy spyR(&watcher, SIGNAL(serviceRegistered(QString)));
    QSignalSpy spyU(&watcher, SIGNAL(serviceUnregistered(QString)));
    QSignalSpy spyO(&watcher, SIGNAL(serviceOwnerChanged(QString,QString,QString)));
    QTestEventLoop::instance().connect(&watcher, SIGNAL(serviceUnregistered(QString)), SLOT(exitLoop()));

    // register a name
    QVERIFY(con.registerService(serviceName));

    // unregister it:
    con.unregisterService(serviceName);

    QTestEventLoop::instance().enterLoop(1);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QCOMPARE(spyR.count(), 0);

    QCOMPARE(spyU.count(), 1);
    QCOMPARE(spyU.at(0).at(0).toString(), serviceName);

    QCOMPARE(spyO.count(), 1);
    QCOMPARE(spyO.at(0).at(0).toString(), serviceName);
    QCOMPARE(spyO.at(0).at(1).toString(), con.baseService());
    QVERIFY(spyO.at(0).at(2).toString().isEmpty());
}

void tst_QDBusServiceWatcher::watchForOwnerChange()
{
    QDBusConnection con = QDBusConnection::sessionBus();
    QVERIFY(con.isConnected());

    QDBusServiceWatcher watcher(serviceName, con, QDBusServiceWatcher::WatchForOwnerChange);

    QSignalSpy spyR(&watcher, SIGNAL(serviceRegistered(QString)));
    QSignalSpy spyU(&watcher, SIGNAL(serviceUnregistered(QString)));
    QSignalSpy spyO(&watcher, SIGNAL(serviceOwnerChanged(QString,QString,QString)));
    QTestEventLoop::instance().connect(&watcher, SIGNAL(serviceRegistered(QString)), SLOT(exitLoop()));

    // register a name
    QVERIFY(con.registerService(serviceName));

    QTestEventLoop::instance().enterLoop(1);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QCOMPARE(spyR.count(), 1);
    QCOMPARE(spyR.at(0).at(0).toString(), serviceName);

    QCOMPARE(spyU.count(), 0);

    QCOMPARE(spyO.count(), 1);
    QCOMPARE(spyO.at(0).at(0).toString(), serviceName);
    QVERIFY(spyO.at(0).at(1).toString().isEmpty());
    QCOMPARE(spyO.at(0).at(2).toString(), con.baseService());

    spyR.clear();
    spyU.clear();
    spyO.clear();

    // unregister it:
    con.unregisterService(serviceName);

    // and register again
    QVERIFY(con.registerService(serviceName));

    QTestEventLoop::instance().enterLoop(1);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QCOMPARE(spyR.count(), 1);
    QCOMPARE(spyR.at(0).at(0).toString(), serviceName);

    QCOMPARE(spyU.count(), 1);
    QCOMPARE(spyU.at(0).at(0).toString(), serviceName);

    QCOMPARE(spyO.count(), 2);
    QCOMPARE(spyO.at(0).at(0).toString(), serviceName);
    QCOMPARE(spyO.at(0).at(1).toString(), con.baseService());
    QVERIFY(spyO.at(0).at(2).toString().isEmpty());
    QCOMPARE(spyO.at(1).at(0).toString(), serviceName);
    QVERIFY(spyO.at(1).at(1).toString().isEmpty());
    QCOMPARE(spyO.at(1).at(2).toString(), con.baseService());
}

void tst_QDBusServiceWatcher::modeChange()
{
    QDBusConnection con = QDBusConnection::sessionBus();
    QVERIFY(con.isConnected());

    QDBusServiceWatcher watcher(serviceName, con, QDBusServiceWatcher::WatchForRegistration);

    QSignalSpy spyR(&watcher, SIGNAL(serviceRegistered(QString)));
    QSignalSpy spyU(&watcher, SIGNAL(serviceUnregistered(QString)));
    QSignalSpy spyO(&watcher, SIGNAL(serviceOwnerChanged(QString,QString,QString)));
    QTestEventLoop::instance().connect(&watcher, SIGNAL(serviceRegistered(QString)), SLOT(exitLoop()));

    // register a name
    QVERIFY(con.registerService(serviceName));

    QTestEventLoop::instance().enterLoop(1);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QCOMPARE(spyR.count(), 1);
    QCOMPARE(spyR.at(0).at(0).toString(), serviceName);

    QCOMPARE(spyU.count(), 0);

    QCOMPARE(spyO.count(), 1);
    QCOMPARE(spyO.at(0).at(0).toString(), serviceName);
    QVERIFY(spyO.at(0).at(1).toString().isEmpty());
    QCOMPARE(spyO.at(0).at(2).toString(), con.baseService());

    spyR.clear();
    spyU.clear();
    spyO.clear();

    watcher.setWatchMode(QDBusServiceWatcher::WatchForUnregistration);

    // unregister it:
    con.unregisterService(serviceName);

    QTestEventLoop::instance().connect(&watcher, SIGNAL(serviceUnregistered(QString)), SLOT(exitLoop()));
    QTestEventLoop::instance().enterLoop(1);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QCOMPARE(spyR.count(), 0);

    QCOMPARE(spyU.count(), 1);
    QCOMPARE(spyU.at(0).at(0).toString(), serviceName);

    QCOMPARE(spyO.count(), 1);
    QCOMPARE(spyO.at(0).at(0).toString(), serviceName);
    QCOMPARE(spyO.at(0).at(1).toString(), con.baseService());
    QVERIFY(spyO.at(0).at(2).toString().isEmpty());
}

QTEST_MAIN(tst_QDBusServiceWatcher)
#include "tst_qdbusservicewatcher.moc"
