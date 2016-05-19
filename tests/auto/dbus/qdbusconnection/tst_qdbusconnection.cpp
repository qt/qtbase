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

#include "tst_qdbusconnection.h"

#include <qcoreapplication.h>
#include <qdebug.h>

#include <QtTest/QtTest>
#include <QtDBus/QtDBus>

void MyObject::method(const QDBusMessage &msg)
{
    path = msg.path();
    ++callCount;
    //qDebug() << msg;
}

void MyObjectWithoutInterface::method(const QDBusMessage &msg)
{
    path = msg.path();
    interface = msg.interface();
    ++callCount;
    //qDebug() << msg;
}

int tst_QDBusConnection::hookCallCount;
tst_QDBusConnection::tst_QDBusConnection()
{
#ifdef HAS_HOOKSETUPFUNCTION
#  define QCOMPARE_HOOKCOUNT(n)         QCOMPARE(hookCallCount, n); hookCallCount = 0
#  define QVERIFY_HOOKCALLED()          QCOMPARE(hookCallCount, 1); hookCallCount = 0
    hookSetupFunction();
#else
#  define QCOMPARE_HOOKCOUNT(n)         qt_noop()
#  define QVERIFY_HOOKCALLED()          qt_noop()
#endif
}

// called before each testcase
void tst_QDBusConnection::init()
{
    hookCallCount = 0;
}

void tst_QDBusConnection::cleanup()
{
    QVERIFY2(!hookCallCount, "Unchecked call");
}


void tst_QDBusConnection::noConnection()
{
    QDBusConnection con = QDBusConnection::connectToBus("unix:path=/dev/null", "testconnection");
    QVERIFY(!con.isConnected());

    // try sending a message. This should fail
    QDBusMessage msg = QDBusMessage::createMethodCall("org.kde.selftest", "/org/kde/selftest",
                                                      "org.kde.selftest", "Ping");
    msg << QLatin1String("ping");

    QVERIFY(!con.send(msg));

    QDBusSpy spy;
    QVERIFY(con.callWithCallback(msg, &spy, SLOT(asyncReply)) == 0);

    QDBusMessage reply = con.call(msg);
    QCOMPARE(reply.type(), QDBusMessage::ErrorMessage);

    QDBusReply<void> voidreply(reply);
    QVERIFY(!voidreply.isValid());

    QDBusConnection::disconnectFromBus("testconnection");
}

void tst_QDBusConnection::sendSignal()
{
    QDBusConnection con = QDBusConnection::sessionBus();

    QVERIFY(con.isConnected());

    QDBusMessage msg = QDBusMessage::createSignal("/org/kde/selftest", "org.kde.selftest",
                                                  "Ping");
    msg << QLatin1String("ping");

    QVERIFY(con.send(msg));
}

void tst_QDBusConnection::sendSignalToName()
{
    if (!QCoreApplication::instance())
        QSKIP("Test requires a QCoreApplication");  // because of the qWait()

    QDBusSpy spy;

    QDBusConnection con = QDBusConnection::sessionBus();

    con.connect(con.baseService(), "/org/kde/selftest", "org.kde.selftest", "ping", &spy,
                SLOT(handlePing(QString)));

    QDBusMessage msg =
        QDBusMessage::createTargetedSignal(con.baseService(), "/org/kde/selftest",
                                           "org.kde.selftest", "ping");
    msg << QLatin1String("ping");

    QVERIFY(con.send(msg));

    QTest::qWait(1000);

    QCOMPARE(spy.args.count(), 1);
    QCOMPARE(spy.args.at(0).toString(), QString("ping"));
}

void tst_QDBusConnection::sendSignalToOtherName()
{
    if (!QCoreApplication::instance())
        QSKIP("Test requires a QCoreApplication");  // because of the qWait()

    QDBusSpy spy;

    QDBusConnection con = QDBusConnection::sessionBus();

    con.connect(con.baseService(), "/org/kde/selftest", "org.kde.selftest", "ping", &spy,
                SLOT(handlePing(QString)));

    QDBusMessage msg =
        QDBusMessage::createTargetedSignal("some.other.service", "/org/kde/selftest",
                                           "org.kde.selftest", "ping");
    msg << QLatin1String("ping");

    QVERIFY(con.send(msg));

    QTest::qWait(1000);

    QCOMPARE(spy.args.count(), 0);
}

void tst_QDBusConnection::send()
{
    QDBusConnection con = QDBusConnection::sessionBus();

    QVERIFY(con.isConnected());

    QDBusMessage msg = QDBusMessage::createMethodCall("org.freedesktop.DBus",
        "/org/freedesktop/DBus", "org.freedesktop.DBus", "ListNames");

    QDBusMessage reply = con.call(msg);

    QCOMPARE(reply.arguments().count(), 1);
    QCOMPARE(reply.arguments().at(0).typeName(), "QStringList");
    QVERIFY(reply.arguments().at(0).toStringList().contains(con.baseService()));
}

void tst_QDBusConnection::sendWithGui()
{
    if (!QCoreApplication::instance())
        QSKIP("Test requires a QCoreApplication");

    QDBusConnection con = QDBusConnection::sessionBus();

    QVERIFY(con.isConnected());

    QDBusMessage msg = QDBusMessage::createMethodCall("org.freedesktop.DBus",
        "/org/freedesktop/DBus", "org.freedesktop.DBus", "ListNames");

    QDBusMessage reply = con.call(msg, QDBus::BlockWithGui);

    QCOMPARE(reply.arguments().count(), 1);
    QCOMPARE(reply.arguments().at(0).typeName(), "QStringList");
    QVERIFY(reply.arguments().at(0).toStringList().contains(con.baseService()));
}

void tst_QDBusConnection::sendAsync()
{
    if (!QCoreApplication::instance())
        QSKIP("Test requires a QCoreApplication");

    QDBusConnection con = QDBusConnection::sessionBus();
    QVERIFY(con.isConnected());

    QDBusSpy spy;

    QDBusMessage msg = QDBusMessage::createMethodCall("org.freedesktop.DBus",
            "/org/freedesktop/DBus", "org.freedesktop.DBus", "ListNames");
    QVERIFY(con.callWithCallback(msg, &spy, SLOT(asyncReply(QDBusMessage))));

    QTest::qWait(1000);

    QCOMPARE(spy.args.value(0).typeName(), "QStringList");
    QVERIFY(spy.args.at(0).toStringList().contains(con.baseService()));
}

void tst_QDBusConnection::connect()
{
    QDBusSpy spy;

    QDBusConnection con = QDBusConnection::sessionBus();

    if (!QCoreApplication::instance())
        return;         // cannot receive signals in this thread without QCoreApplication

    con.connect(con.baseService(), "/org/kde/selftest", "org.kde.selftest", "ping", &spy,
                 SLOT(handlePing(QString)));

    QDBusMessage msg = QDBusMessage::createSignal("/org/kde/selftest", "org.kde.selftest",
                                                  "ping");
    msg << QLatin1String("ping");

    QVERIFY(con.send(msg));

    QTest::qWait(1000);

    QCOMPARE(spy.args.count(), 1);
    QCOMPARE(spy.args.at(0).toString(), QString("ping"));
}

void tst_QDBusConnection::connectToBus()
{
    {
        QDBusConnection con = QDBusConnection::connectToBus(
                QDBusConnection::SessionBus, "bubu");

        QVERIFY(con.isConnected());
        QVERIFY(!con.lastError().isValid());

        QDBusConnection con2("foo");
        QVERIFY(!con2.isConnected());
        QVERIFY(con2.lastError().isValid());

        con2 = con;
        QVERIFY(con.isConnected());
        QVERIFY(con2.isConnected());
        QVERIFY(!con.lastError().isValid());
        QVERIFY(!con2.lastError().isValid());
    }

    {
        QDBusConnection con("bubu");
        QVERIFY(con.isConnected());
        QVERIFY(!con.lastError().isValid());
    }

    QDBusConnection::disconnectFromPeer("bubu");

    {
        QDBusConnection con("bubu");
        QVERIFY(con.isConnected());
        QVERIFY(!con.lastError().isValid());
    }

    QDBusConnection::disconnectFromBus("bubu");

    {
        QDBusConnection con("bubu");
        QVERIFY(!con.isConnected());
        QVERIFY(con.lastError().isValid());
    }

    QByteArray address = qgetenv("DBUS_SESSION_BUS_ADDRESS");
    if (!address.isEmpty()) {
        QDBusConnection con = QDBusConnection::connectToBus(address, "newconn");
        QVERIFY(con.isConnected());
        QVERIFY(!con.lastError().isValid());

        QDBusConnection::disconnectFromBus("newconn");
    }
}

void tst_QDBusConnection::connectToPeer()
{
    {
        QDBusConnection con = QDBusConnection::connectToPeer(
                "", "newconn");
        QVERIFY(!con.isConnected());
        QVERIFY(con.lastError().isValid());
        QDBusConnection::disconnectFromPeer("newconn");
    }

    QDBusServer server;

    {
        QDBusConnection con = QDBusConnection::connectToPeer(
                "unix:abstract=/tmp/dbus-XXXXXXXXXX,guid=00000000000000000000000000000000", "newconn2");
        QVERIFY(!con.isConnected());
        QVERIFY(con.lastError().isValid());
        QDBusConnection::disconnectFromPeer("newconn2");
    }

    {
        QDBusConnection con = QDBusConnection::connectToPeer(
                server.address(), "bubu");

        QVERIFY(con.isConnected());
        QVERIFY(!con.lastError().isValid());

        QDBusConnection con2("foo");
        QVERIFY(!con2.isConnected());
        QVERIFY(con2.lastError().isValid());

        con2 = con;
        QVERIFY(con.isConnected());
        QVERIFY(con2.isConnected());
        QVERIFY(!con.lastError().isValid());
        QVERIFY(!con2.lastError().isValid());
    }

    {
        QDBusConnection con("bubu");
        QVERIFY(con.isConnected());
        QVERIFY(!con.lastError().isValid());
    }

    QDBusConnection::disconnectFromBus("bubu");

    {
        QDBusConnection con("bubu");
        QVERIFY(con.isConnected());
        QVERIFY(!con.lastError().isValid());
    }

    QDBusConnection::disconnectFromPeer("bubu");

    {
        QDBusConnection con("bubu");
        QVERIFY(!con.isConnected());
        QVERIFY(con.lastError().isValid());
    }
}

void tst_QDBusConnection::registerObject_data()
{
    QTest::addColumn<QString>("path");

    QTest::newRow("/") << "/";
    QTest::newRow("/p1") << "/p1";
    QTest::newRow("/p2") << "/p2";
    QTest::newRow("/p1/q") << "/p1/q";
    QTest::newRow("/p1/q/r") << "/p1/q/r";
}

void tst_QDBusConnection::registerObject()
{
    QFETCH(QString, path);

    QDBusConnection con = QDBusConnection::sessionBus();
    QVERIFY(con.isConnected());

    //QVERIFY(!callMethod(con, path));
    {
        // register one object at root:
        MyObject obj;
        QVERIFY(con.registerObject(path, &obj, QDBusConnection::ExportAllSlots));
        QCOMPARE(con.objectRegisteredAt(path), static_cast<QObject *>(&obj));
        QVERIFY(callMethod(con, path));
        QCOMPARE(obj.path, path);
        QVERIFY_HOOKCALLED();
    }
    // make sure it's gone
    QVERIFY(!callMethod(con, path));
    QVERIFY_HOOKCALLED();
}

void tst_QDBusConnection::registerObjectWithInterface_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<QString>("interface");

    QTest::newRow("/") << "/" << "org.foo";
    QTest::newRow("/p1") << "/p1" << "org.foo";
    QTest::newRow("/p2") << "/p2" << "org.foo";
    QTest::newRow("/p1/q") << "/p1/q" << "org.foo";
    QTest::newRow("/p1/q/r") << "/p1/q/r" << "org.foo";

}

void tst_QDBusConnection::registerObjectWithInterface()
{
    QFETCH(QString, path);
    QFETCH(QString, interface);

    QDBusConnection con = QDBusConnection::sessionBus();
    QVERIFY(con.isConnected());

    {
        // register one object at root:
        MyObjectWithoutInterface obj;
        QVERIFY(con.registerObject(path, interface, &obj, QDBusConnection::ExportAllSlots));
        QCOMPARE(con.objectRegisteredAt(path), static_cast<QObject *>(&obj));
        QVERIFY(callMethod(con, path, interface));
        QCOMPARE(obj.path, path);
        QCOMPARE(obj.interface, interface);
        QVERIFY_HOOKCALLED();
    }
    // make sure it's gone
    QVERIFY(!callMethod(con, path, interface));
    QVERIFY_HOOKCALLED();
}

void tst_QDBusConnection::registerObjectPeer_data()
{
    QTest::addColumn<QString>("path");

    QTest::newRow("/") << "/";
    QTest::newRow("/p1") << "/p1";
    QTest::newRow("/p2") << "/p2";
    QTest::newRow("/p1/q") << "/p1/q";
    QTest::newRow("/p1/q/r") << "/p1/q/r";
}

void tst_QDBusConnection::registerObjectPeer()
{
    if (!QCoreApplication::instance())
        QSKIP("Test requires a QCoreApplication");

    QFETCH(QString, path);

    MyServer server(path);

    QDBusConnection::connectToPeer(server.address(), "beforeFoo");
    QTestEventLoop::instance().enterLoop(2);
    QVERIFY(!QTestEventLoop::instance().timeout());

    {
        QDBusConnection con = QDBusConnection::connectToPeer(server.address(), "foo");

        QTestEventLoop::instance().enterLoop(2);
        QVERIFY(!QTestEventLoop::instance().timeout());
        QVERIFY(con.isConnected());

        MyObject obj;
        QVERIFY(callMethodPeer(con, path));
        QCOMPARE(obj.path, path);
        QVERIFY_HOOKCALLED();
    }

    QDBusConnection::connectToPeer(server.address(), "afterFoo");
    QTestEventLoop::instance().enterLoop(2);

    {
        QDBusConnection con("foo");
        QVERIFY(con.isConnected());
        QVERIFY(callMethodPeer(con, path));
        QVERIFY_HOOKCALLED();
    }

    server.unregisterObject();

    {
        QDBusConnection con("foo");
        QVERIFY(con.isConnected());
        QVERIFY(!callMethodPeer(con, path));
        QVERIFY_HOOKCALLED();
    }

    server.registerObject();

    {
        QDBusConnection con("foo");
        QVERIFY(con.isConnected());
        QVERIFY(callMethodPeer(con, path));
        QVERIFY_HOOKCALLED();
    }

    QDBusConnection::disconnectFromPeer("foo");

    {
        QDBusConnection con("foo");
        QVERIFY(!con.isConnected());
        QVERIFY(!callMethodPeer(con, path));
    }

    QDBusConnection::disconnectFromPeer("beforeFoo");
    QDBusConnection::disconnectFromPeer("afterFoo");
}

void tst_QDBusConnection::registerObject2()
{
    QDBusConnection con = QDBusConnection::sessionBus();
    QVERIFY(con.isConnected());

    // make sure nothing is using our paths:
     QVERIFY(!callMethod(con, "/"));
     QVERIFY_HOOKCALLED();
     QVERIFY(!callMethod(con, "/p1"));
     QVERIFY_HOOKCALLED();
     QVERIFY(!callMethod(con, "/p2"));
     QVERIFY_HOOKCALLED();
     QVERIFY(!callMethod(con, "/p1/q"));
     QVERIFY_HOOKCALLED();
     QVERIFY(!callMethod(con, "/p1/q/r"));
     QVERIFY_HOOKCALLED();

    {
        // register one object at root:
        MyObject obj;
        QVERIFY(con.registerObject("/", &obj, QDBusConnection::ExportAllSlots));
        QVERIFY(callMethod(con, "/"));
        QCOMPARE(obj.path, QString("/"));
        QVERIFY_HOOKCALLED();
    }
    // make sure it's gone
    QVERIFY(!callMethod(con, "/"));
    QVERIFY_HOOKCALLED();

    {
        // register one at an element:
        MyObject obj;
        QVERIFY(con.registerObject("/p1", &obj, QDBusConnection::ExportAllSlots));
        QVERIFY(!callMethod(con, "/"));
        QVERIFY_HOOKCALLED();
        QVERIFY(callMethod(con, "/p1"));
        QCOMPARE(obj.path, QString("/p1"));
        QVERIFY_HOOKCALLED();

        // re-register it somewhere else
        QVERIFY(con.registerObject("/p2", &obj, QDBusConnection::ExportAllSlots));
        QVERIFY(callMethod(con, "/p1"));
        QCOMPARE(obj.path, QString("/p1"));
        QVERIFY_HOOKCALLED();
        QVERIFY(callMethod(con, "/p2"));
        QCOMPARE(obj.path, QString("/p2"));
        QVERIFY_HOOKCALLED();
    }
    // make sure it's gone
    QVERIFY(!callMethod(con, "/p1"));
    QVERIFY_HOOKCALLED();
    QVERIFY(!callMethod(con, "/p2"));
    QVERIFY_HOOKCALLED();

    {
        // register at a deep path
        MyObject obj;
        QVERIFY(con.registerObject("/p1/q/r", &obj, QDBusConnection::ExportAllSlots));
        QVERIFY(!callMethod(con, "/"));
        QVERIFY_HOOKCALLED();
        QVERIFY(!callMethod(con, "/p1"));
        QVERIFY_HOOKCALLED();
        QVERIFY(!callMethod(con, "/p1/q"));
        QVERIFY_HOOKCALLED();
        QVERIFY(callMethod(con, "/p1/q/r"));
        QCOMPARE(obj.path, QString("/p1/q/r"));
        QVERIFY_HOOKCALLED();
    }

    // make sure it's gone
    QVERIFY(!callMethod(con, "/p1/q/r"));
    QVERIFY_HOOKCALLED();

    {
        MyObject obj;
        QVERIFY(con.registerObject("/p1/q2", &obj, QDBusConnection::ExportAllSlots));
        QVERIFY(callMethod(con, "/p1/q2"));
        QCOMPARE(obj.path, QString("/p1/q2"));
        QVERIFY_HOOKCALLED();

        // try unregistering
        con.unregisterObject("/p1/q2");
        QVERIFY(!callMethod(con, "/p1/q2"));
        QVERIFY_HOOKCALLED();

        // register it again
        QVERIFY(con.registerObject("/p1/q2", &obj, QDBusConnection::ExportAllSlots));
        QVERIFY(callMethod(con, "/p1/q2"));
        QCOMPARE(obj.path, QString("/p1/q2"));
        QVERIFY_HOOKCALLED();

        // now try removing things around it:
        con.unregisterObject("/p2");
        QVERIFY(callMethod(con, "/p1/q2")); // unrelated object shouldn't affect
        QVERIFY_HOOKCALLED();

        con.unregisterObject("/p1");
        QVERIFY(callMethod(con, "/p1/q2")); // unregistering just the parent shouldn't affect it
        QVERIFY_HOOKCALLED();

        con.unregisterObject("/p1/q2/r");
        QVERIFY(callMethod(con, "/p1/q2")); // unregistering non-existing child shouldn't affect it either
        QVERIFY_HOOKCALLED();

        con.unregisterObject("/p1/q");
        QVERIFY(callMethod(con, "/p1/q2")); // unregistering sibling (before) shouldn't affect
        QVERIFY_HOOKCALLED();

        con.unregisterObject("/p1/r");
        QVERIFY(callMethod(con, "/p1/q2")); // unregistering sibling (after) shouldn't affect
        QVERIFY_HOOKCALLED();

        // now remove it:
        con.unregisterObject("/p1", QDBusConnection::UnregisterTree);
        QVERIFY(!callMethod(con, "/p1/q2")); // we removed the full tree
        QVERIFY_HOOKCALLED();
    }
}

void tst_QDBusConnection::registerObjectPeer2()
{
    if (!QCoreApplication::instance())
        QSKIP("Test requires a QCoreApplication");

    MyServer2 server;
    QDBusConnection con = QDBusConnection::connectToPeer(server.address(), "foo");
    QTestEventLoop::instance().enterLoop(2);
    QVERIFY(!QTestEventLoop::instance().timeout());
    QVERIFY(con.isConnected());

    QDBusConnection srv_con = server.connection();

    // make sure nothing is using our paths:
    QVERIFY(!callMethodPeer(srv_con, "/"));
    QVERIFY_HOOKCALLED();
    QVERIFY(!callMethodPeer(srv_con, "/p1"));
    QVERIFY_HOOKCALLED();
    QVERIFY(!callMethodPeer(srv_con, "/p2"));
    QVERIFY_HOOKCALLED();
    QVERIFY(!callMethodPeer(srv_con, "/p1/q"));
    QVERIFY_HOOKCALLED();
    QVERIFY(!callMethodPeer(srv_con, "/p1/q/r"));
    QVERIFY_HOOKCALLED();

    {
        // register one object at root:
        MyObject obj;
        QVERIFY(con.registerObject("/", &obj, QDBusConnection::ExportAllSlots));
        QVERIFY(callMethodPeer(srv_con, "/"));
        QCOMPARE(obj.path, QString("/"));
        QVERIFY_HOOKCALLED();
    }

    // make sure it's gone
    QVERIFY(!callMethodPeer(srv_con, "/"));
    QVERIFY_HOOKCALLED();

    {
        // register one at an element:
        MyObject obj;
        QVERIFY(con.registerObject("/p1", &obj, QDBusConnection::ExportAllSlots));
        QVERIFY(!callMethodPeer(srv_con, "/"));
        QVERIFY_HOOKCALLED();
        QVERIFY(callMethodPeer(srv_con, "/p1"));
        QCOMPARE(obj.path, QString("/p1"));
        QVERIFY_HOOKCALLED();

        // re-register it somewhere else
        QVERIFY(con.registerObject("/p2", &obj, QDBusConnection::ExportAllSlots));
        QVERIFY(callMethodPeer(srv_con, "/p1"));
        QCOMPARE(obj.path, QString("/p1"));
        QVERIFY_HOOKCALLED();
        QVERIFY(callMethodPeer(srv_con, "/p2"));
        QCOMPARE(obj.path, QString("/p2"));
        QVERIFY_HOOKCALLED();
    }

    // make sure it's gone
    QVERIFY(!callMethodPeer(srv_con, "/p1"));
    QVERIFY_HOOKCALLED();
    QVERIFY(!callMethodPeer(srv_con, "/p2"));
    QVERIFY_HOOKCALLED();

    {
        // register at a deep path
        MyObject obj;
        QVERIFY(con.registerObject("/p1/q/r", &obj, QDBusConnection::ExportAllSlots));
        QVERIFY(!callMethodPeer(srv_con, "/"));
        QVERIFY_HOOKCALLED();
        QVERIFY(!callMethodPeer(srv_con, "/p1"));
        QVERIFY_HOOKCALLED();
        QVERIFY(!callMethodPeer(srv_con, "/p1/q"));
        QVERIFY_HOOKCALLED();
        QVERIFY(callMethodPeer(srv_con, "/p1/q/r"));
        QCOMPARE(obj.path, QString("/p1/q/r"));
        QVERIFY_HOOKCALLED();
    }

    // make sure it's gone
    QVERIFY(!callMethodPeer(srv_con, "/p1/q/r"));
    QVERIFY_HOOKCALLED();

    {
        MyObject obj;
        QVERIFY(con.registerObject("/p1/q2", &obj, QDBusConnection::ExportAllSlots));
        QVERIFY(callMethodPeer(srv_con, "/p1/q2"));
        QCOMPARE(obj.path, QString("/p1/q2"));
        QVERIFY_HOOKCALLED();

        // try unregistering
        con.unregisterObject("/p1/q2");
        QVERIFY(!callMethodPeer(srv_con, "/p1/q2"));
        QVERIFY_HOOKCALLED();

        // register it again
        QVERIFY(con.registerObject("/p1/q2", &obj, QDBusConnection::ExportAllSlots));
        QVERIFY(callMethodPeer(srv_con, "/p1/q2"));
        QCOMPARE(obj.path, QString("/p1/q2"));
        QVERIFY_HOOKCALLED();

        // now try removing things around it:
        con.unregisterObject("/p2");
        QVERIFY(callMethodPeer(srv_con, "/p1/q2")); // unrelated object shouldn't affect
        QVERIFY_HOOKCALLED();

        con.unregisterObject("/p1");
        QVERIFY(callMethodPeer(srv_con, "/p1/q2")); // unregistering just the parent shouldn't affect it
        QVERIFY_HOOKCALLED();

        con.unregisterObject("/p1/q2/r");
        QVERIFY(callMethodPeer(srv_con, "/p1/q2")); // unregistering non-existing child shouldn't affect it either
        QVERIFY_HOOKCALLED();

        con.unregisterObject("/p1/q");
        QVERIFY(callMethodPeer(srv_con, "/p1/q2")); // unregistering sibling (before) shouldn't affect
        QVERIFY_HOOKCALLED();

        con.unregisterObject("/p1/r");
        QVERIFY(callMethodPeer(srv_con, "/p1/q2")); // unregistering sibling (after) shouldn't affect
        QVERIFY_HOOKCALLED();

        // now remove it:
        con.unregisterObject("/p1", QDBusConnection::UnregisterTree);
        QVERIFY(!callMethodPeer(srv_con, "/p1/q2")); // we removed the full tree
        QVERIFY_HOOKCALLED();
    }

    QDBusConnection::disconnectFromPeer("foo");
}


void tst_QDBusConnection::registerQObjectChildren()
{
    // make sure no one is there
    QDBusConnection con = QDBusConnection::sessionBus();
    QVERIFY(!callMethod(con, "/p1"));
    QVERIFY_HOOKCALLED();

    {
        MyObject obj, *a, *b, *c, *cc;

        a = new MyObject(&obj);
        a->setObjectName("a");

        b = new MyObject(&obj);
        b->setObjectName("b");

        c = new MyObject(&obj);
        c->setObjectName("c");

        cc = new MyObject(c);
        cc->setObjectName("cc");

        con.registerObject("/p1", &obj, QDBusConnection::ExportAllSlots |
                           QDBusConnection::ExportChildObjects);

        // make calls
        QVERIFY(callMethod(con, "/p1"));
        QCOMPARE(obj.callCount, 1);
        QVERIFY_HOOKCALLED();
        QVERIFY(callMethod(con, "/p1/a"));
        QCOMPARE(a->callCount, 1);
        QVERIFY_HOOKCALLED();
        QVERIFY(callMethod(con, "/p1/b"));
        QCOMPARE(b->callCount, 1);
        QVERIFY_HOOKCALLED();
        QVERIFY(callMethod(con, "/p1/c"));
        QCOMPARE(c->callCount, 1);
        QVERIFY_HOOKCALLED();
        QVERIFY(callMethod(con, "/p1/c/cc"));
        QCOMPARE(cc->callCount, 1);
        QVERIFY_HOOKCALLED();

        QVERIFY(!callMethod(con, "/p1/d"));
        QVERIFY_HOOKCALLED();
        QVERIFY(!callMethod(con, "/p1/c/abc"));
        QVERIFY_HOOKCALLED();

        // pull an object, see if it goes away:
        delete b;
        QVERIFY(!callMethod(con, "/p1/b"));
        QVERIFY_HOOKCALLED();

        delete c;
        QVERIFY(!callMethod(con, "/p1/c"));
        QVERIFY_HOOKCALLED();
        QVERIFY(!callMethod(con, "/p1/c/cc"));
        QVERIFY_HOOKCALLED();
    }

    QVERIFY(!callMethod(con, "/p1"));
    QVERIFY_HOOKCALLED();
    QVERIFY(!callMethod(con, "/p1/a"));
    QVERIFY_HOOKCALLED();
    QVERIFY(!callMethod(con, "/p1/b"));
    QVERIFY_HOOKCALLED();
    QVERIFY(!callMethod(con, "/p1/c"));
    QVERIFY_HOOKCALLED();
    QVERIFY(!callMethod(con, "/p1/c/cc"));
    QVERIFY_HOOKCALLED();
}

void tst_QDBusConnection::registerQObjectChildrenPeer()
{
    if (!QCoreApplication::instance())
        QSKIP("Test requires a QCoreApplication");

    MyServer2 server;
    QDBusConnection con = QDBusConnection::connectToPeer(server.address(), "foo");
    QTestEventLoop::instance().enterLoop(2);
    QVERIFY(!QTestEventLoop::instance().timeout());
    QCoreApplication::processEvents();
    QVERIFY(con.isConnected());

    QDBusConnection srv_con = server.connection();

    QVERIFY(!callMethodPeer(srv_con, "/p1"));
    QVERIFY_HOOKCALLED();

    {
        MyObject obj, *a, *b, *c, *cc;

        a = new MyObject(&obj);
        a->setObjectName("a");

        b = new MyObject(&obj);
        b->setObjectName("b");

        c = new MyObject(&obj);
        c->setObjectName("c");

        cc = new MyObject(c);
        cc->setObjectName("cc");

        con.registerObject("/p1", &obj, QDBusConnection::ExportAllSlots |
                           QDBusConnection::ExportChildObjects);

        // make calls
        QVERIFY(callMethodPeer(srv_con, "/p1"));
        QCOMPARE(obj.callCount, 1);
        QVERIFY_HOOKCALLED();
        QVERIFY(callMethodPeer(srv_con, "/p1/a"));
        QCOMPARE(a->callCount, 1);
        QVERIFY_HOOKCALLED();
        QVERIFY(callMethodPeer(srv_con, "/p1/b"));
        QCOMPARE(b->callCount, 1);
        QVERIFY_HOOKCALLED();
        QVERIFY(callMethodPeer(srv_con, "/p1/c"));
        QCOMPARE(c->callCount, 1);
        QVERIFY_HOOKCALLED();
        QVERIFY(callMethodPeer(srv_con, "/p1/c/cc"));
        QCOMPARE(cc->callCount, 1);
        QVERIFY_HOOKCALLED();

        QVERIFY(!callMethodPeer(srv_con, "/p1/d"));
        QVERIFY_HOOKCALLED();
        QVERIFY(!callMethodPeer(srv_con, "/p1/c/abc"));
        QVERIFY_HOOKCALLED();

        // pull an object, see if it goes away:
        delete b;
        QVERIFY(!callMethodPeer(srv_con, "/p1/b"));
        QVERIFY_HOOKCALLED();

        delete c;
        QVERIFY(!callMethodPeer(srv_con, "/p1/c"));
        QVERIFY_HOOKCALLED();
        QVERIFY(!callMethodPeer(srv_con, "/p1/c/cc"));
        QVERIFY_HOOKCALLED();
    }

    QVERIFY(!callMethodPeer(srv_con, "/p1"));
    QVERIFY_HOOKCALLED();
    QVERIFY(!callMethodPeer(srv_con, "/p1/a"));
    QVERIFY_HOOKCALLED();
    QVERIFY(!callMethodPeer(srv_con, "/p1/b"));
    QVERIFY_HOOKCALLED();
    QVERIFY(!callMethodPeer(srv_con, "/p1/c"));
    QVERIFY_HOOKCALLED();
    QVERIFY(!callMethodPeer(srv_con, "/p1/c/cc"));
    QVERIFY_HOOKCALLED();

    QDBusConnection::disconnectFromPeer("foo");
}

bool tst_QDBusConnection::callMethod(const QDBusConnection &conn, const QString &path)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(conn.baseService(), path, "", "method");
    QDBusMessage reply = conn.call(msg, QDBus::Block/*WithGui*/);
    if (reply.type() != QDBusMessage::ReplyMessage)
        return false;
    QTest::qCompare(MyObject::path, path, "MyObject::path", "path", __FILE__, __LINE__);
    return (MyObject::path == path);
}

bool tst_QDBusConnection::callMethod(const QDBusConnection &conn, const QString &path, const QString &interface)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(conn.baseService(), path, interface, "method");
    QDBusMessage reply = conn.call(msg, QDBus::Block/*WithGui*/);
    if (reply.type() != QDBusMessage::ReplyMessage)
        return false;
    QTest::qCompare(MyObjectWithoutInterface::path, path, "MyObjectWithoutInterface::path", "path", __FILE__, __LINE__);
    return (MyObjectWithoutInterface::path == path) && MyObjectWithoutInterface::interface == interface;
}

bool tst_QDBusConnection::callMethodPeer(const QDBusConnection &conn, const QString &path)
{
    QDBusMessage msg = QDBusMessage::createMethodCall("", path, "", "method");
    QDBusMessage reply = conn.call(msg, QDBus::BlockWithGui);

    if (reply.type() != QDBusMessage::ReplyMessage)
        return false;
    QTest::qCompare(MyObject::path, path, "MyObject::path", "path", __FILE__, __LINE__);
    return (MyObject::path == path);
}

void tst_QDBusConnection::callSelf()
{
    TestObject testObject;
    QDBusConnection connection = QDBusConnection::sessionBus();
    QVERIFY(connection.registerObject("/test", &testObject,
            QDBusConnection::ExportAllContents));
    QCOMPARE(connection.objectRegisteredAt("/test"), static_cast<QObject *>(&testObject));
    QVERIFY(connection.registerService(serviceName()));
    QDBusInterface interface(serviceName(), "/test");
    QVERIFY(interface.isValid());
    QVERIFY_HOOKCALLED();

    interface.call(QDBus::Block, "test0");
    QCOMPARE(testObject.func, QString("test0"));
    QVERIFY_HOOKCALLED();
    interface.call(QDBus::Block, "test1", 42);
    QCOMPARE(testObject.func, QString("test1 42"));
    QVERIFY_HOOKCALLED();
    QDBusMessage reply = interface.call(QDBus::Block, "test2");
    QCOMPARE(testObject.func, QString("test2"));
    QCOMPARE(reply.arguments().value(0).toInt(), 43);
    QVERIFY_HOOKCALLED();

    QDBusMessage msg = QDBusMessage::createMethodCall(serviceName(), "/test",
                                                      QString(), "test3");
    msg << 44;
    reply = connection.call(msg);
    QCOMPARE(reply.arguments().value(0).toInt(), 45);
    QVERIFY_HOOKCALLED();
}

void tst_QDBusConnection::callSelfByAnotherName_data()
{
    QTest::addColumn<int>("registerMethod");
    QTest::newRow("connection") << 0;
    QTest::newRow("connection-interface") << 1;
    QTest::newRow("direct") << 2;
}

void tst_QDBusConnection::callSelfByAnotherName()
{
    if (!QCoreApplication::instance())
        QSKIP("Test requires a QCoreApplication");

    static int counter = 0;
    QString sname = serviceName() + QString::number(counter++);

    QDBusConnection con = QDBusConnection::sessionBus();
    QVERIFY(con.isConnected());

    TestObject testObject;
    QVERIFY(con.registerObject("/test", &testObject,
            QDBusConnection::ExportAllContents));
    con.connect("org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus", "NameOwnerChanged",
                QStringList() << sname << "",
                QString(), &QTestEventLoop::instance(), SLOT(exitLoop()));

    // register the name
    QFETCH(int, registerMethod);
    switch (registerMethod) {
    case 0:
        QVERIFY(con.registerService(sname));
        break;

    case 1:
        QCOMPARE(con.interface()->registerService(sname).value(), QDBusConnectionInterface::ServiceRegistered);
        break;

    case 2: {
            // flag is DBUS_NAME_FLAG_DO_NOT_QUEUE = 0x04
            // reply is DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER = 1
            QDBusReply<uint> reply = con.interface()->call("RequestName", sname, 4u);
            QCOMPARE(reply.value(), uint(1));
        }
    }

    struct Deregisterer {
        QDBusConnection con;
        QString sname;
        Deregisterer(const QDBusConnection &con, const QString &sname) : con(con), sname(sname) {}
        ~Deregisterer() { con.interface()->unregisterService(sname); }
    } deregisterer(con, sname);

    // give the connection a chance to find out that we're good to go
    QTestEventLoop::instance().enterLoop(2);
    con.disconnect("org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus", "NameOwnerChanged",
                 QStringList() << sname << "",
                 QString(), &QTestEventLoop::instance(), SLOT(exitLoop()));
    QVERIFY(!QTestEventLoop::instance().timeout());

    // make the call
    QDBusMessage msg = QDBusMessage::createMethodCall(sname, "/test",
                                                      QString(), "test0");
    QDBusMessage reply = con.call(msg, QDBus::Block, 1000);

    QCOMPARE(reply.type(), QDBusMessage::ReplyMessage);
    QVERIFY_HOOKCALLED();
}

void tst_QDBusConnection::multipleInterfacesInQObject()
{
    QDBusConnection con = QDBusConnection::sessionBus();
    QVERIFY(!callMethod(con, "/p1"));
    QVERIFY_HOOKCALLED();

    MyObject obj;
    con.registerObject("/p1", &obj, QDBusConnection::ExportAllSlots);

    // check if we can call the BaseObject's interface
    QDBusMessage msg = QDBusMessage::createMethodCall(con.baseService(), "/p1",
                                                      "local.BaseObject", "anotherMethod");
    QDBusMessage reply = con.call(msg, QDBus::Block);
    QCOMPARE(reply.type(), QDBusMessage::ReplyMessage);
    QCOMPARE(reply.arguments().count(), 0);
    QVERIFY_HOOKCALLED();
}

void tst_QDBusConnection::slotsWithLessParameters()
{
    if (!QCoreApplication::instance())
        QSKIP("Test requires a QCoreApplication");

    QDBusConnection con = QDBusConnection::sessionBus();

    QDBusMessage signal = QDBusMessage::createSignal("/", "org.qtproject.TestCase",
                                                     "oneSignal");
    signal << "one parameter";

    signalsReceived = 0;
    QVERIFY(con.connect(con.baseService(), signal.path(), signal.interface(),
                        signal.member(), this, SLOT(oneSlot())));
    QVERIFY(con.send(signal));
    QTest::qWait(100);
    QCOMPARE(signalsReceived, 1);

    // disconnect and try with a signature
    signalsReceived = 0;
    QVERIFY(con.disconnect(con.baseService(), signal.path(), signal.interface(),
                           signal.member(), this, SLOT(oneSlot())));
    QVERIFY(con.connect(con.baseService(), signal.path(), signal.interface(),
                        signal.member(), "s", this, SLOT(oneSlot())));
    QVERIFY(con.send(signal));
    QTest::qWait(100);
    QCOMPARE(signalsReceived, 1);
}

void tst_QDBusConnection::secondCallWithCallback()
{
    QDBusConnection con = QDBusConnection::sessionBus();
    QDBusMessage msg = QDBusMessage::createMethodCall(con.baseService(), "/test", QString(),
                                                      "test0");
    con.callWithCallback(msg, this, SLOT(exitLoop()), SLOT(secondCallWithCallback()));
}

void tst_QDBusConnection::nestedCallWithCallback()
{
    if (!QCoreApplication::instance())
        QSKIP("Test requires a QCoreApplication");

    TestObject testObject;
    QDBusConnection connection = QDBusConnection::sessionBus();
    QVERIFY(connection.registerObject("/test", &testObject,
            QDBusConnection::ExportAllContents));

    QDBusMessage msg = QDBusMessage::createMethodCall(connection.baseService(), "/test", QString(),
                                                      "ThisFunctionDoesntExist");
    signalsReceived = 0;

    connection.callWithCallback(msg, this, SLOT(exitLoop()), SLOT(secondCallWithCallback()), 10);
    QTestEventLoop::instance().enterLoop(15);
    QVERIFY(!QTestEventLoop::instance().timeout());
    QCOMPARE(signalsReceived, 1);
    QCOMPARE_HOOKCOUNT(2);
}

void tst_QDBusConnection::serviceRegistrationRaceCondition()
{
    if (!QCoreApplication::instance())
        QSKIP("Test requires a QCoreApplication");

    // There was a race condition in the updating of list of name owners in
    // Qt D-Bus. When the user connects to a signal coming from a given
    // service, we must listen for NameOwnerChanged signals relevant to that
    // name and update when the owner changes. However, it's possible that we
    // receive in one chunk from the server both the NameOwnerChanged signal
    // about the service and the signal we're interested in. Since Qt D-Bus
    // posts events in order to handle the incoming signals, the update
    // happens too late.

    const QString connectionName = "testConnectionName";
    const QString serviceName = "org.example.SecondaryName";

    QDBusConnection session = QDBusConnection::sessionBus();
    QVERIFY(!session.interface()->isServiceRegistered(serviceName));

    // connect to the signal:
    RaceConditionSignalWaiter recv;
    session.connect(serviceName, "/", "org.qtproject.TestCase", "oneSignal", &recv, SLOT(countUp()));

    // create a secondary connection and register a name
    QDBusConnection connection = QDBusConnection::connectToBus(QDBusConnection::SessionBus, connectionName);
    QDBusConnection::disconnectFromBus(connectionName); // disconnection happens when "connection" goes out of scope
    QVERIFY(connection.isConnected());
    QVERIFY(connection.registerService(serviceName));

    // send a signal
    QDBusMessage msg = QDBusMessage::createSignal("/", "org.qtproject.TestCase", "oneSignal");
    connection.send(msg);

    // make a blocking call just to be sure that the buffer was flushed
    msg = QDBusMessage::createMethodCall("org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus",
                                         "NameHasOwner");
    msg << connectionName;
    connection.call(msg); // ignore result

    // Now here's the race condition (more info on task QTBUG-15651):
    // the bus has most likely queued three signals for us to work on:
    // 1) NameOwnerChanged for the connection we created above
    // 2) NameOwnerChanged for the service we registered above
    // 3) The "oneSignal" signal we sent
    //
    // We'll most likely receive all three in one go from the server. We must
    // update the owner of serviceName before we start processing the
    // "oneSignal" signal.

    QTestEventLoop::instance().connect(&recv, SIGNAL(done()), SLOT(exitLoop()));
    QTestEventLoop::instance().enterLoop(1);
    QVERIFY(!QTestEventLoop::instance().timeout());
    QCOMPARE(recv.count, 1);
}

void tst_QDBusConnection::registerVirtualObject()
{
    QDBusConnection con = QDBusConnection::sessionBus();
    QVERIFY(con.isConnected());

    QString path = "/tree/node";
    QString childPath = "/tree/node/child";
    QString childChildPath = "/tree/node/child/another";

    {
        // Register VirtualObject that handles child paths. Unregister by going out of scope.
        VirtualObject obj;
        QVERIFY(con.registerVirtualObject(path, &obj, QDBusConnection::SubPath));
        QCOMPARE(con.objectRegisteredAt(path), static_cast<QObject *>(&obj));
        QCOMPARE(con.objectRegisteredAt(childPath), static_cast<QObject *>(&obj));
        QCOMPARE(con.objectRegisteredAt(childChildPath), static_cast<QObject *>(&obj));
    }
    QCOMPARE(con.objectRegisteredAt(path), static_cast<QObject *>(0));
    QCOMPARE(con.objectRegisteredAt(childPath), static_cast<QObject *>(0));

    {
        // Register VirtualObject that handles child paths. Unregister by calling unregister.
        VirtualObject obj;
        QVERIFY(con.registerVirtualObject(path, &obj, QDBusConnection::SubPath));
        QCOMPARE(con.objectRegisteredAt(path), static_cast<QObject *>(&obj));
        QCOMPARE(con.objectRegisteredAt(childPath), static_cast<QObject *>(&obj));
        QCOMPARE(con.objectRegisteredAt(childChildPath), static_cast<QObject *>(&obj));
        con.unregisterObject(path);
        QCOMPARE(con.objectRegisteredAt(path), static_cast<QObject *>(0));
        QCOMPARE(con.objectRegisteredAt(childPath), static_cast<QObject *>(0));
    }

    {
        // Single node has no sub path handling.
        VirtualObject obj;
        QVERIFY(con.registerVirtualObject(path, &obj, QDBusConnection::SingleNode));
        QCOMPARE(con.objectRegisteredAt(path), static_cast<QObject *>(&obj));
        QCOMPARE(con.objectRegisteredAt(childPath), static_cast<QObject *>(0));
    }

    {
        // Register VirtualObject that handles child paths. Try to register an object on a child path of that.
        VirtualObject obj;
        QVERIFY(con.registerVirtualObject(path, &obj, QDBusConnection::SubPath));
        QCOMPARE(con.objectRegisteredAt(path), static_cast<QObject *>(&obj));

        QObject objectAtSubPath;
        QVERIFY(!con.registerObject(path, &objectAtSubPath));
        QVERIFY(!con.registerObject(childPath, &objectAtSubPath));
        QCOMPARE(con.objectRegisteredAt(childPath), static_cast<QObject *>(&obj));
    }

    {
        // Register object, make sure no SubPath handling object can be registered on a parent path.
        QObject objectAtSubPath;
        QVERIFY(con.registerObject(childPath, &objectAtSubPath));
        QCOMPARE(con.objectRegisteredAt(childPath), static_cast<QObject *>(&objectAtSubPath));

        VirtualObject obj;
        QVERIFY(!con.registerVirtualObject(path, &obj, QDBusConnection::SubPath));
        QCOMPARE(con.objectRegisteredAt(path), static_cast<QObject *>(0));
    }

    {
        // Register object, make sure no SubPath handling object can be registered on a parent path.
        // (same as above, but deeper)
        QObject objectAtSubPath;
        QVERIFY(con.registerObject(childChildPath, &objectAtSubPath));
        QCOMPARE(con.objectRegisteredAt(childChildPath), static_cast<QObject *>(&objectAtSubPath));

        VirtualObject obj;
        QVERIFY(!con.registerVirtualObject(path, &obj, QDBusConnection::SubPath));
        QCOMPARE(con.objectRegisteredAt(path), static_cast<QObject *>(0));
    }

    QCOMPARE(con.objectRegisteredAt(path), static_cast<QObject *>(0));
    QCOMPARE(con.objectRegisteredAt(childPath), static_cast<QObject *>(0));
    QCOMPARE(con.objectRegisteredAt(childChildPath), static_cast<QObject *>(0));
}

void tst_QDBusConnection::callVirtualObject()
{
    if (!QCoreApplication::instance())
        QSKIP("Test requires a QCoreApplication");

    QDBusConnection con = QDBusConnection::sessionBus();
    QVERIFY(con.isConnected());

    QDBusConnection con2 = QDBusConnection::connectToBus(QDBusConnection::SessionBus, "con2");

    QString path = "/tree/node";
    QString childPath = "/tree/node/child";

    // register one object at root:
    VirtualObject obj;
    QVERIFY(con.registerVirtualObject(path, &obj, QDBusConnection::SubPath));
    obj.callCount = 0;
    obj.replyArguments << 42 << 47u;

    QObject::connect(&obj, SIGNAL(messageReceived(QDBusMessage)), &QTestEventLoop::instance(), SLOT(exitLoop()));

    QDBusMessage message = QDBusMessage::createMethodCall(con.baseService(), path, QString(), "hello");
    QDBusPendingCall reply = con2.asyncCall(message);

    QTestEventLoop::instance().enterLoop(5);
    QVERIFY(!QTestEventLoop::instance().timeout());
    QVERIFY_HOOKCALLED();

    QCOMPARE(obj.callCount, 1);
    QCOMPARE(obj.lastMessage.service(), con2.baseService());
    QCOMPARE(obj.lastMessage.interface(), QString());
    QCOMPARE(obj.lastMessage.path(), path);
    reply.waitForFinished();
    QVERIFY(reply.isValid());
    QCOMPARE(reply.reply().arguments(), obj.replyArguments);

    // call sub path
    QDBusMessage childMessage = QDBusMessage::createMethodCall(con.baseService(), childPath, QString(), "helloChild");
    obj.replyArguments.clear();
    obj.replyArguments << 99;
    QDBusPendingCall childReply = con2.asyncCall(childMessage);

    QTestEventLoop::instance().enterLoop(5);
    QVERIFY(!QTestEventLoop::instance().timeout());
    QVERIFY_HOOKCALLED();

    QCOMPARE(obj.callCount, 2);
    QCOMPARE(obj.lastMessage.service(), con2.baseService());
    QCOMPARE(obj.lastMessage.interface(), QString());
    QCOMPARE(obj.lastMessage.path(), childPath);

    childReply.waitForFinished();
    QVERIFY(childReply.isValid());
    QCOMPARE(childReply.reply().arguments(), obj.replyArguments);

    // let the call fail by having the virtual object return false
    obj.success = false;
    QDBusMessage errorMessage = QDBusMessage::createMethodCall(con.baseService(), childPath, QString(), "someFunc");
    QDBusPendingCall errorReply = con2.asyncCall(errorMessage);

    QTestEventLoop::instance().enterLoop(5);
    QVERIFY(!QTestEventLoop::instance().timeout());
    QVERIFY_HOOKCALLED();
    QTest::qWait(100);
    QVERIFY(errorReply.isError());
    QCOMPARE(errorReply.reply().errorName(), QString("org.freedesktop.DBus.Error.UnknownObject"));

    QDBusConnection::disconnectFromBus("con2");
}

void tst_QDBusConnection::callVirtualObjectLocal()
{
    QDBusConnection con = QDBusConnection::sessionBus();
    QVERIFY(con.isConnected());

    QString path = "/tree/node";
    QString childPath = "/tree/node/child";

    // register one object at root:
    VirtualObject obj;
    QVERIFY(con.registerVirtualObject(path, &obj, QDBusConnection::SubPath));
    obj.callCount = 0;
    obj.replyArguments << 42 << 47u;

    QDBusMessage message = QDBusMessage::createMethodCall(con.baseService(), path, QString(), "hello");
    QDBusMessage reply = con.call(message, QDBus::Block, 5000);
    QCOMPARE(obj.callCount, 1);
    QCOMPARE(obj.lastMessage.service(), con.baseService());
    QCOMPARE(obj.lastMessage.interface(), QString());
    QCOMPARE(obj.lastMessage.path(), path);
    QCOMPARE(obj.replyArguments, reply.arguments());
    QVERIFY_HOOKCALLED();

    obj.replyArguments << QString("alien abduction");
    QDBusMessage subPathMessage = QDBusMessage::createMethodCall(con.baseService(), childPath, QString(), "hello");
    QDBusMessage subPathReply = con.call(subPathMessage , QDBus::Block, 5000);
    QCOMPARE(obj.callCount, 2);
    QCOMPARE(obj.lastMessage.service(), con.baseService());
    QCOMPARE(obj.lastMessage.interface(), QString());
    QCOMPARE(obj.lastMessage.path(), childPath);
    QCOMPARE(obj.replyArguments, subPathReply.arguments());
    QVERIFY_HOOKCALLED();
}

void tst_QDBusConnection::pendingCallWhenDisconnected()
{
    if (!QCoreApplication::instance())
        QSKIP("Test requires a QCoreApplication");

    QDBusServer *server = new QDBusServer;
    QDBusConnection con = QDBusConnection::connectToPeer(server->address(), "disconnect");
    QTestEventLoop::instance().enterLoop(2);
    QVERIFY(con.isConnected());
    QDBusMessage message = QDBusMessage::createMethodCall("", "/", QString(), "method");
    QDBusPendingCall reply = con.asyncCall(message);

    delete server;

    QTestEventLoop::instance().enterLoop(2);
    QVERIFY(!con.isConnected());
    QVERIFY(reply.isFinished());
    QVERIFY(reply.isError());
    QVERIFY(reply.error().type() == QDBusError::Disconnected);
}

QString MyObject::path;
QString MyObjectWithoutInterface::path;
QString MyObjectWithoutInterface::interface;

#ifndef tst_QDBusConnection
QTEST_MAIN(tst_QDBusConnection)
#endif
