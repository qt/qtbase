/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
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

#include <QtDBus/QDBusServiceWatcher>
#include <QtDBus>
#include <QtTest>

class tst_QDBusServiceWatcher: public QObject
{
    Q_OBJECT
    int testCounter;
public:
    tst_QDBusServiceWatcher();

private slots:
    void initTestCase();
    void watchForCreation_data();
    void watchForCreation();
    void watchForDisappearance_data();
    void watchForDisappearance();
    void watchForDisappearanceUniqueConnection();
    void watchForOwnerChange_data();
    void watchForOwnerChange();
    void modeChange_data();
    void modeChange();
    void disconnectedConnection();
    void setConnection_data();
    void setConnection();

private:
    QString generateServiceName();
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

QString tst_QDBusServiceWatcher::generateServiceName() {
    return "com.example.TestService" + QString::number(testCounter++);
}

void tst_QDBusServiceWatcher::watchForCreation_data()
{
    QTest::addColumn<QString>("watchedName");
    QTest::addColumn<QString>("registeredName");

    //com.example.TestService5 matches com.example.TestService5
    QString name = generateServiceName();
    QTest::newRow("normal") << name << name;

    //com.example* matches com.example.TestService5
    name = generateServiceName();
    QTest::newRow("wildcard") << "com.example*" << name;

    //com.example.TestService5* matches com.example.TestService5
    name = generateServiceName();
    QTest::newRow("wildcard_exact") << name+"*" << name;

    //com.example.TestService5* matches com.example.TestService5.Foo
    name = generateServiceName();
    QTest::newRow("wildcard_subdomain") << name+"*" << name + ".Foo";

    //com.example.TestService5* matches com.example.TestService5.Foo.Bar
    name = generateServiceName();
    QTest::newRow("wildcard_subsubdomain") << name+"*" << name + ".Foo.Bar";
}

void tst_QDBusServiceWatcher::watchForCreation()
{
    QFETCH(QString, watchedName);
    QFETCH(QString, registeredName);

    QDBusConnection con = QDBusConnection::sessionBus();
    QVERIFY(con.isConnected());

    QDBusServiceWatcher watcher(watchedName, con, QDBusServiceWatcher::WatchForRegistration);

    QSignalSpy spyR(&watcher, SIGNAL(serviceRegistered(QString)));
    QSignalSpy spyU(&watcher, SIGNAL(serviceUnregistered(QString)));
    QSignalSpy spyO(&watcher, SIGNAL(serviceOwnerChanged(QString,QString,QString)));
    QTestEventLoop::instance().connect(&watcher, SIGNAL(serviceRegistered(QString)), SLOT(exitLoop()));

    // register a name
    QVERIFY(con.registerService(registeredName));

    QTestEventLoop::instance().enterLoop(1);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QCOMPARE(spyR.count(), 1);
    QCOMPARE(spyR.at(0).at(0).toString(), registeredName);

    QCOMPARE(spyU.count(), 0);

    QCOMPARE(spyO.count(), 1);
    QCOMPARE(spyO.at(0).at(0).toString(), registeredName);
    QVERIFY(spyO.at(0).at(1).toString().isEmpty());
    QCOMPARE(spyO.at(0).at(2).toString(), con.baseService());

    spyR.clear();
    spyU.clear();
    spyO.clear();

    // unregister it:
    con.unregisterService(registeredName);

    // and register again
    QVERIFY(con.registerService(registeredName));

    QTestEventLoop::instance().enterLoop(1);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QCOMPARE(spyR.count(), 1);
    QCOMPARE(spyR.at(0).at(0).toString(), registeredName);

    QCOMPARE(spyU.count(), 0);

    QCOMPARE(spyO.count(), 1);
    QCOMPARE(spyO.at(0).at(0).toString(), registeredName);
    QVERIFY(spyO.at(0).at(1).toString().isEmpty());
    QCOMPARE(spyO.at(0).at(2).toString(), con.baseService());
}

void tst_QDBusServiceWatcher::watchForDisappearance_data()
{
    tst_QDBusServiceWatcher::watchForCreation_data();
}

void tst_QDBusServiceWatcher::watchForDisappearance()
{
    QFETCH(QString, watchedName);
    QFETCH(QString, registeredName);

    QDBusConnection con = QDBusConnection::sessionBus();
    QVERIFY(con.isConnected());

    QDBusServiceWatcher watcher(watchedName, con, QDBusServiceWatcher::WatchForUnregistration);
    watcher.setObjectName("watcher for disappearance");

    QSignalSpy spyR(&watcher, SIGNAL(serviceRegistered(QString)));
    QSignalSpy spyU(&watcher, SIGNAL(serviceUnregistered(QString)));
    QSignalSpy spyO(&watcher, SIGNAL(serviceOwnerChanged(QString,QString,QString)));
    QTestEventLoop::instance().connect(&watcher, SIGNAL(serviceUnregistered(QString)), SLOT(exitLoop()));

    // register a name
    QVERIFY(con.registerService(registeredName));

    // unregister it:
    con.unregisterService(registeredName);

    QTestEventLoop::instance().enterLoop(1);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QCOMPARE(spyR.count(), 0);

    QCOMPARE(spyU.count(), 1);
    QCOMPARE(spyU.at(0).at(0).toString(), registeredName);

    QCOMPARE(spyO.count(), 1);
    QCOMPARE(spyO.at(0).at(0).toString(), registeredName);
    QCOMPARE(spyO.at(0).at(1).toString(), con.baseService());
    QVERIFY(spyO.at(0).at(2).toString().isEmpty());
}

void tst_QDBusServiceWatcher::watchForDisappearanceUniqueConnection()
{
    QDBusConnection con = QDBusConnection::sessionBus();
    QVERIFY(con.isConnected());

    // second connection
    QString watchedName = QDBusConnection::connectToBus(QDBusConnection::SessionBus, "session2").baseService();
    QVERIFY(!watchedName.isEmpty());

    QDBusServiceWatcher watcher(watchedName, con, QDBusServiceWatcher::WatchForUnregistration);
    watcher.setObjectName("watcher for disappearance");

    QSignalSpy spyR(&watcher, SIGNAL(serviceRegistered(QString)));
    QSignalSpy spyU(&watcher, SIGNAL(serviceUnregistered(QString)));
    QSignalSpy spyO(&watcher, SIGNAL(serviceOwnerChanged(QString,QString,QString)));
    QTestEventLoop::instance().connect(&watcher, SIGNAL(serviceUnregistered(QString)), SLOT(exitLoop()));

    // unregister it:
    QDBusConnection::disconnectFromBus("session2");

    QTestEventLoop::instance().enterLoop(1);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QCOMPARE(spyR.count(), 0);

    QCOMPARE(spyU.count(), 1);
    QCOMPARE(spyU.at(0).at(0).toString(), watchedName);

    QCOMPARE(spyO.count(), 1);
    QCOMPARE(spyO.at(0).at(0).toString(), watchedName);
    QCOMPARE(spyO.at(0).at(1).toString(), watchedName);
    QVERIFY(spyO.at(0).at(2).toString().isEmpty());
}

void tst_QDBusServiceWatcher::watchForOwnerChange_data()
{
    watchForCreation_data();
}

void tst_QDBusServiceWatcher::watchForOwnerChange()
{
    QFETCH(QString, watchedName);
    QFETCH(QString, registeredName);

    QDBusConnection con = QDBusConnection::sessionBus();
    QVERIFY(con.isConnected());

    QDBusServiceWatcher watcher(watchedName, con, QDBusServiceWatcher::WatchForOwnerChange);

    QSignalSpy spyR(&watcher, SIGNAL(serviceRegistered(QString)));
    QSignalSpy spyU(&watcher, SIGNAL(serviceUnregistered(QString)));
    QSignalSpy spyO(&watcher, SIGNAL(serviceOwnerChanged(QString,QString,QString)));
    QTestEventLoop::instance().connect(&watcher, SIGNAL(serviceRegistered(QString)), SLOT(exitLoop()));

    // register a name
    QVERIFY(con.registerService(registeredName));

    QTestEventLoop::instance().enterLoop(1);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QCOMPARE(spyR.count(), 1);
    QCOMPARE(spyR.at(0).at(0).toString(), registeredName);

    QCOMPARE(spyU.count(), 0);

    QCOMPARE(spyO.count(), 1);
    QCOMPARE(spyO.at(0).at(0).toString(), registeredName);
    QVERIFY(spyO.at(0).at(1).toString().isEmpty());
    QCOMPARE(spyO.at(0).at(2).toString(), con.baseService());

    spyR.clear();
    spyU.clear();
    spyO.clear();

    // unregister it:
    con.unregisterService(registeredName);

    // and register again
    QVERIFY(con.registerService(registeredName));

    QTestEventLoop::instance().enterLoop(1);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QCOMPARE(spyR.count(), 1);
    QCOMPARE(spyR.at(0).at(0).toString(), registeredName);

    QCOMPARE(spyU.count(), 1);
    QCOMPARE(spyU.at(0).at(0).toString(), registeredName);

    QCOMPARE(spyO.count(), 2);
    QCOMPARE(spyO.at(0).at(0).toString(), registeredName);
    QCOMPARE(spyO.at(0).at(1).toString(), con.baseService());
    QVERIFY(spyO.at(0).at(2).toString().isEmpty());
    QCOMPARE(spyO.at(1).at(0).toString(), registeredName);
    QVERIFY(spyO.at(1).at(1).toString().isEmpty());
    QCOMPARE(spyO.at(1).at(2).toString(), con.baseService());
}

void tst_QDBusServiceWatcher::modeChange_data()
{
    watchForCreation_data();
}

void tst_QDBusServiceWatcher::modeChange()
{
    QFETCH(QString, watchedName);
    QFETCH(QString, registeredName);

    QDBusConnection con = QDBusConnection::sessionBus();
    QVERIFY(con.isConnected());

    QDBusServiceWatcher watcher(watchedName, con, QDBusServiceWatcher::WatchForRegistration);

    QSignalSpy spyR(&watcher, SIGNAL(serviceRegistered(QString)));
    QSignalSpy spyU(&watcher, SIGNAL(serviceUnregistered(QString)));
    QSignalSpy spyO(&watcher, SIGNAL(serviceOwnerChanged(QString,QString,QString)));
    QTestEventLoop::instance().connect(&watcher, SIGNAL(serviceRegistered(QString)), SLOT(exitLoop()));

    // register a name
    QVERIFY(con.registerService(registeredName));

    QTestEventLoop::instance().enterLoop(1);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QCOMPARE(spyR.count(), 1);
    QCOMPARE(spyR.at(0).at(0).toString(), registeredName);

    QCOMPARE(spyU.count(), 0);

    QCOMPARE(spyO.count(), 1);
    QCOMPARE(spyO.at(0).at(0).toString(), registeredName);
    QVERIFY(spyO.at(0).at(1).toString().isEmpty());
    QCOMPARE(spyO.at(0).at(2).toString(), con.baseService());

    spyR.clear();
    spyU.clear();
    spyO.clear();

    watcher.setWatchMode(QDBusServiceWatcher::WatchForUnregistration);

    // unregister it:
    con.unregisterService(registeredName);

    QTestEventLoop::instance().connect(&watcher, SIGNAL(serviceUnregistered(QString)), SLOT(exitLoop()));
    QTestEventLoop::instance().enterLoop(1);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QCOMPARE(spyR.count(), 0);

    QCOMPARE(spyU.count(), 1);
    QCOMPARE(spyU.at(0).at(0).toString(), registeredName);

    QCOMPARE(spyO.count(), 1);
    QCOMPARE(spyO.at(0).at(0).toString(), registeredName);
    QCOMPARE(spyO.at(0).at(1).toString(), con.baseService());
    QVERIFY(spyO.at(0).at(2).toString().isEmpty());
}

void tst_QDBusServiceWatcher::disconnectedConnection()
{
    QDBusConnection con("");
    QVERIFY(!con.isConnected());

    QDBusServiceWatcher watcher(generateServiceName(), con, QDBusServiceWatcher::WatchForRegistration);
    watcher.addWatchedService("com.example.somethingelse");
    watcher.addWatchedService("org.freedesktop.DBus");

    watcher.setWatchMode(QDBusServiceWatcher::WatchForUnregistration);
    watcher.setWatchMode(QDBusServiceWatcher::WatchForOwnerChange);

    watcher.setWatchedServices(QStringList());
}

void tst_QDBusServiceWatcher::setConnection_data()
{
    QTest::addColumn<QString>("serviceName");
    QTest::newRow("normal") << generateServiceName();
}

void tst_QDBusServiceWatcher::setConnection()
{
    QFETCH(QString, serviceName);
    // begin with a disconnected connection
    QDBusConnection con("");
    QVERIFY(!con.isConnected());

    QDBusServiceWatcher watcher(serviceName, con, QDBusServiceWatcher::WatchForRegistration);

    QSignalSpy spyR(&watcher, SIGNAL(serviceRegistered(QString)));
    QSignalSpy spyU(&watcher, SIGNAL(serviceUnregistered(QString)));
    QTestEventLoop::instance().connect(&watcher, SIGNAL(serviceOwnerChanged(QString,QString,QString)), SLOT(exitLoop()));

    // move to the session bus
    con = QDBusConnection::sessionBus();
    QVERIFY(con.isConnected());
    watcher.setConnection(con);

    // register a name
    QVERIFY(con.registerService(serviceName));

    QTestEventLoop::instance().enterLoop(1);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QCOMPARE(spyR.count(), 1);
    QCOMPARE(spyR.at(0).at(0).toString(), serviceName);
    QCOMPARE(spyU.count(), 0);

    // is the system bus available?
    if (!QDBusConnection::systemBus().isConnected())
        return;

    // connect to the system bus and ask to watch that base service
    QString watchedName = QDBusConnection::connectToBus(QDBusConnection::SystemBus, "system2").baseService();
    watcher.setWatchedServices(QStringList() << watchedName);
    watcher.setWatchMode(QDBusServiceWatcher::WatchForUnregistration);

    // move to the system bus
    watcher.setConnection(QDBusConnection::systemBus());
    spyR.clear();
    spyU.clear();

    QDBusConnection::disconnectFromBus("system2");

    QTestEventLoop::instance().enterLoop(1);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QCOMPARE(spyR.count(), 0);

    QCOMPARE(spyU.count(), 1);
    QCOMPARE(spyU.at(0).at(0).toString(), watchedName);
}

QTEST_MAIN(tst_QDBusServiceWatcher)
#include "tst_qdbusservicewatcher.moc"
