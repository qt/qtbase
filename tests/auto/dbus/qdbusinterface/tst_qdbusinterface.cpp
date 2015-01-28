/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/
/* -*- C++ -*-
 */

#include <qcoreapplication.h>
#include <qmetatype.h>
#include <QtTest/QtTest>
#include <QtCore/qvariant.h>
#include <QtDBus/QtDBus>
#include <qdebug.h>
#include "../qdbusmarshall/common.h"
#include "myobject.h"

#define TEST_INTERFACE_NAME "org.qtproject.QtDBus.MyObject"
#define TEST_SIGNAL_NAME "somethingHappened"

static const char serviceName[] = "org.qtproject.autotests.qmyserver";
static const char objectPath[] = "/org/qtproject/qmyserver";
static const char *interfaceName = serviceName;

int MyObject::callCount = 0;
QVariantList MyObject::callArgs;

class MyObjectUnknownType: public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.qtproject.QtDBus.MyObject")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.qtproject.QtDBus.MyObjectUnknownTypes\" >\n"
"    <property access=\"readwrite\" type=\"~\" name=\"prop1\" />\n"
"    <signal name=\"somethingHappened\" >\n"
"      <arg direction=\"out\" type=\"~\" />\n"
"    </signal>\n"
"    <method name=\"ping\" >\n"
"      <arg direction=\"in\" type=\"~\" name=\"ping\" />\n"
"      <arg direction=\"out\" type=\"~\" name=\"ping\" />\n"
"    </method>\n"
"    <method name=\"regularMethod\" />\n"
"  </interface>\n"
                "")
};

class Spy: public QObject
{
    Q_OBJECT
public:
    QString received;
    int count;

    Spy() : count(0)
    { }

public slots:
    void spySlot(const QString& arg)
    {
        received = arg;
        ++count;
    }
};

class DerivedFromQDBusInterface: public QDBusInterface
{
    Q_OBJECT
public:
    DerivedFromQDBusInterface()
        : QDBusInterface("com.example.Test", "/")
    {}

public slots:
    void method() {}
};

// helper function
void emitSignal(const QString &interface, const QString &name, const QString &arg)
{
    QDBusMessage msg = QDBusMessage::createSignal("/", interface, name);
    msg << arg;
    QDBusConnection::sessionBus().send(msg);

    QTest::qWait(1000);
}

void emitSignalPeer(const QString &interface, const QString &name, const QString &arg)
{
    QDBusMessage req = QDBusMessage::createMethodCall(serviceName, objectPath, interfaceName, "emitSignal");
    req << interface;
    req << name;
    req << arg;
    QDBusConnection::sessionBus().send(req);

    QTest::qWait(1000);
}

int callCountPeer()
{
    QDBusMessage req = QDBusMessage::createMethodCall(serviceName, objectPath, interfaceName, "callCount");
    QDBusMessage reply = QDBusConnection::sessionBus().call(req);
    return reply.arguments().at(0).toInt();
}

QVariantList callArgsPeer()
{
    QDBusMessage req = QDBusMessage::createMethodCall(serviceName, objectPath, interfaceName, "callArgs");
    QDBusMessage reply = QDBusConnection::sessionBus().call(req);
    return qdbus_cast<QVariantList>(reply.arguments().at(0));
}

void setProp1Peer(int val)
{
    QDBusMessage req = QDBusMessage::createMethodCall(serviceName, objectPath, interfaceName, "setProp1");
    req << val;
    QDBusMessage reply = QDBusConnection::sessionBus().call(req);
}

int prop1Peer()
{
    QDBusMessage req = QDBusMessage::createMethodCall(serviceName, objectPath, interfaceName, "prop1");
    QDBusMessage reply = QDBusConnection::sessionBus().call(req);
    return reply.arguments().at(0).toInt();
}

void setComplexPropPeer(QList<int> val)
{
    QDBusMessage req = QDBusMessage::createMethodCall(serviceName, objectPath, interfaceName, "setComplexProp");
    req << QVariant::fromValue(val);
    QDBusMessage reply = QDBusConnection::sessionBus().call(req);
}

QList<int> complexPropPeer()
{
    QDBusMessage req = QDBusMessage::createMethodCall(serviceName, objectPath, interfaceName, "complexProp");
    QDBusMessage reply = QDBusConnection::sessionBus().call(req);
    return qdbus_cast<QList<int> >(reply.arguments().at(0));
}

void resetPeer()
{
    QDBusMessage req = QDBusMessage::createMethodCall(serviceName, objectPath, interfaceName, "reset");
    QDBusConnection::sessionBus().call(req);
}

class tst_QDBusInterface: public QObject
{
    Q_OBJECT
    MyObject obj;

public slots:
    void testServiceOwnerChanged(const QString &service)
    {
        if (service == "com.example.Test")
            QTestEventLoop::instance().exitLoop();
    }

private slots:
    void initTestCase();
    void cleanupTestCase();

    void notConnected();
    void notValid();
    void notValidDerived();
    void invalidAfterServiceOwnerChanged();
    void introspect();
    void introspectUnknownTypes();
    void introspectVirtualObject();
    void callMethod();
    void invokeMethod();
    void invokeMethodWithReturn();
    void invokeMethodWithMultiReturn();
    void invokeMethodWithComplexReturn();

    void introspectPeer();
    void callMethodPeer();
    void invokeMethodPeer();
    void invokeMethodWithReturnPeer();
    void invokeMethodWithMultiReturnPeer();
    void invokeMethodWithComplexReturnPeer();

    void signal();
    void signalPeer();

    void propertyRead();
    void propertyWrite();
    void complexPropertyRead();
    void complexPropertyWrite();

    void propertyReadPeer();
    void propertyWritePeer();
    void complexPropertyReadPeer();
    void complexPropertyWritePeer();
private:
    QProcess proc;
};

class WaitForQMyServer: public QObject
{
    Q_OBJECT
public:
    WaitForQMyServer();
    bool ok();
public Q_SLOTS:
    void ownerChange(const QString &name)
    {
        if (name == serviceName)
            loop.quit();
    }

private:
    QEventLoop loop;
};

WaitForQMyServer::WaitForQMyServer()
{
    QDBusConnection con = QDBusConnection::sessionBus();
    if (!ok()) {
        connect(con.interface(), SIGNAL(serviceOwnerChanged(QString,QString,QString)),
                SLOT(ownerChange(QString)));
        QTimer::singleShot(2000, &loop, SLOT(quit()));
        loop.exec();
    }
}

bool WaitForQMyServer::ok()
{
    return QDBusConnection::sessionBus().isConnected() &&
        QDBusConnection::sessionBus().interface()->isServiceRegistered(serviceName);
}

void tst_QDBusInterface::initTestCase()
{
    QDBusConnection con = QDBusConnection::sessionBus();
    QVERIFY(con.isConnected());
    QTest::qWait(500);

    con.registerObject("/", &obj, QDBusConnection::ExportAllProperties
                       | QDBusConnection::ExportAllSlots
                       | QDBusConnection::ExportAllInvokables);

#ifdef Q_OS_WIN
#  define EXE ".exe"
#else
#  define EXE ""
#endif
    proc.setProcessChannelMode(QProcess::ForwardedErrorChannel);
    proc.start(QFINDTESTDATA("qmyserver/qmyserver" EXE));
    QVERIFY2(proc.waitForStarted(), qPrintable(proc.errorString()));
    QVERIFY(proc.waitForReadyRead());

    WaitForQMyServer w;
    QVERIFY(w.ok());
    //QTest::qWait(2000);

    // get peer server address
    QDBusMessage req = QDBusMessage::createMethodCall(serviceName, objectPath, interfaceName, "address");
    QDBusMessage rpl = con.call(req);
    QVERIFY(rpl.type() == QDBusMessage::ReplyMessage);
    QString address = rpl.arguments().at(0).toString();

    // connect to peer server
    QDBusConnection peercon = QDBusConnection::connectToPeer(address, "peer");
    QVERIFY(peercon.isConnected());

    QDBusMessage req2 = QDBusMessage::createMethodCall(serviceName, objectPath, interfaceName, "waitForConnected");
    QDBusMessage rpl2 = con.call(req2);
    QVERIFY(rpl2.type() == QDBusMessage::ReplyMessage);
    QVERIFY2(rpl2.type() == QDBusMessage::ReplyMessage, rpl2.errorMessage().toLatin1());
}

void tst_QDBusInterface::cleanupTestCase()
{
    QDBusMessage msg = QDBusMessage::createMethodCall(serviceName, objectPath, interfaceName, "quit");
    QDBusConnection::sessionBus().call(msg);
    proc.waitForFinished(200);
    proc.close();
}

void tst_QDBusInterface::notConnected()
{
    QDBusConnection connection("");
    QVERIFY(!connection.isConnected());

    QDBusInterface interface("org.freedesktop.DBus", "/", "org.freedesktop.DBus",
                             connection);

    QVERIFY(!interface.isValid());
    QVERIFY(!QMetaObject::invokeMethod(&interface, "ListNames", Qt::DirectConnection));
}

void tst_QDBusInterface::notValid()
{
    QDBusConnection connection("");
    QVERIFY(!connection.isConnected());

    QDBusInterface interface("com.example.Test", QString(), "org.example.Test",
                             connection);

    QVERIFY(!interface.isValid());
    QVERIFY(!QMetaObject::invokeMethod(&interface, "ListNames", Qt::DirectConnection));
}

void tst_QDBusInterface::notValidDerived()
{
    DerivedFromQDBusInterface c;
    QVERIFY(!c.isValid());
    QMetaObject::invokeMethod(&c, "method", Qt::DirectConnection);
}

void tst_QDBusInterface::invalidAfterServiceOwnerChanged()
{
    // this test is technically the same as tst_QDBusAbstractInterface::followSignal
    QDBusConnection conn = QDBusConnection::sessionBus();
    QDBusConnectionInterface *connIface = conn.interface();

    QDBusInterface validInterface(conn.baseService(), "/");
    QVERIFY(validInterface.isValid());
    QDBusInterface invalidInterface("com.example.Test", "/");
    QVERIFY(!invalidInterface.isValid());

    QTestEventLoop::instance().connect(connIface, SIGNAL(serviceOwnerChanged(QString,QString,QString)),
                                       SLOT(exitLoop()));
    QVERIFY(connIface->registerService("com.example.Test") == QDBusConnectionInterface::ServiceRegistered);

    QTestEventLoop::instance().enterLoop(5);

    QVERIFY(!QTestEventLoop::instance().timeout());
    QVERIFY(invalidInterface.isValid());
}

void tst_QDBusInterface::introspect()
{
    QDBusInterface iface(QDBusConnection::sessionBus().baseService(), QLatin1String("/"),
                         TEST_INTERFACE_NAME);

    const QMetaObject *mo = iface.metaObject();

    QCOMPARE(mo->methodCount() - mo->methodOffset(), 7);
    QVERIFY(mo->indexOfSignal(TEST_SIGNAL_NAME "(QString)") != -1);

    QCOMPARE(mo->propertyCount() - mo->propertyOffset(), 2);
    QVERIFY(mo->indexOfProperty("prop1") != -1);
    QVERIFY(mo->indexOfProperty("complexProp") != -1);
}

void tst_QDBusInterface::introspectUnknownTypes()
{
    QDBusConnection con = QDBusConnection::sessionBus();
    MyObjectUnknownType obj;
    con.registerObject("/unknownTypes", &obj, QDBusConnection::ExportAllContents);
    QDBusInterface iface(QDBusConnection::sessionBus().baseService(), QLatin1String("/unknownTypes"),
                         "org.qtproject.QtDBus.MyObjectUnknownTypes");

    const QMetaObject *mo = iface.metaObject();
    QVERIFY(mo->indexOfMethod("regularMethod()") != -1); // this is the control
    QVERIFY(mo->indexOfMethod("somethingHappened(QDBusRawType<0x7e>*)") != -1);

    QVERIFY(mo->indexOfMethod("ping(QDBusRawType<0x7e>*)") != -1);
    int midx = mo->indexOfMethod("ping(QDBusRawType<0x7e>*)");
    QCOMPARE(mo->method(midx).typeName(), "QDBusRawType<0x7e>*");

    QVERIFY(mo->indexOfProperty("prop1") != -1);
    int pidx = mo->indexOfProperty("prop1");
    QCOMPARE(mo->property(pidx).typeName(), "QDBusRawType<0x7e>*");



    QDBusMessage message = QDBusMessage::createMethodCall(con.baseService(), "/unknownTypes", "org.freedesktop.DBus.Introspectable", "Introspect");
    QDBusMessage reply = con.call(message, QDBus::Block, 5000);
    qDebug() << "REPL: " << reply.arguments();

}


class VirtualObject: public QDBusVirtualObject
{
    Q_OBJECT
public:
    VirtualObject() :success(true) {}

    QString introspect(const QString &path) const {
        if (path == "/some/path/superNode")
            return "zitroneneis";
        if (path == "/some/path/superNode/foo")
            return  "  <interface name=\"org.qtproject.QtDBus.VirtualObject\">\n"
                    "    <method name=\"klingeling\" />\n"
                    "  </interface>\n" ;
        return QString();
    }

    bool handleMessage(const QDBusMessage &message, const QDBusConnection &connection) {
        ++callCount;
        lastMessage = message;

        if (success) {
            QDBusMessage reply = message.createReply(replyArguments);
            connection.send(reply);
        }
        emit messageReceived(message);
        return success;
    }
signals:
    void messageReceived(const QDBusMessage &message) const;

public:
    mutable QDBusMessage lastMessage;
    QVariantList replyArguments;
    mutable int callCount;
    bool success;
};

void tst_QDBusInterface::introspectVirtualObject()
{
    QDBusConnection con = QDBusConnection::sessionBus();
    QVERIFY(con.isConnected());
    VirtualObject obj;

    obj.success = false;

    QString path = "/some/path/superNode";
    QVERIFY(con.registerVirtualObject(path, &obj, QDBusConnection::SubPath));

    QDBusMessage message = QDBusMessage::createMethodCall(con.baseService(), path, "org.freedesktop.DBus.Introspectable", "Introspect");
    QDBusMessage reply = con.call(message, QDBus::Block, 5000);
    QVERIFY(reply.arguments().at(0).toString().contains(
        QRegExp("<node>.*zitroneneis.*<interface name=") ));

    QDBusMessage message2 = QDBusMessage::createMethodCall(con.baseService(), path + "/foo", "org.freedesktop.DBus.Introspectable", "Introspect");
    QDBusMessage reply2 = con.call(message2, QDBus::Block, 5000);
    QVERIFY(reply2.arguments().at(0).toString().contains(
        QRegExp("<node>.*<interface name=\"org.qtproject.QtDBus.VirtualObject\">"
                ".*<method name=\"klingeling\" />\n"
                ".*</interface>.*<interface name=") ));
}

void tst_QDBusInterface::callMethod()
{
    QDBusConnection con = QDBusConnection::sessionBus();
    QDBusInterface iface(QDBusConnection::sessionBus().baseService(), QLatin1String("/"),
                         TEST_INTERFACE_NAME);

    MyObject::callCount = 0;

    // call a SLOT method
    QDBusMessage reply = iface.call("ping", QVariant::fromValue(QDBusVariant("foo")));
    QCOMPARE(MyObject::callCount, 1);
    QCOMPARE(reply.type(), QDBusMessage::ReplyMessage);

    // verify what the callee received
    QCOMPARE(MyObject::callArgs.count(), 1);
    QVariant v = MyObject::callArgs.at(0);
    QDBusVariant dv = qdbus_cast<QDBusVariant>(v);
    QCOMPARE(dv.variant().type(), QVariant::String);
    QCOMPARE(dv.variant().toString(), QString("foo"));

    // verify reply
    QCOMPARE(reply.arguments().count(), 1);
    v = reply.arguments().at(0);
    dv = qdbus_cast<QDBusVariant>(v);
    QCOMPARE(dv.variant().type(), QVariant::String);
    QCOMPARE(dv.variant().toString(), QString("foo"));

    // call an INVOKABLE method
    reply = iface.call("ping_invokable", QVariant::fromValue(QDBusVariant("bar")));
    QCOMPARE(MyObject::callCount, 2);
    QCOMPARE(reply.type(), QDBusMessage::ReplyMessage);

    // verify what the callee received
    QCOMPARE(MyObject::callArgs.count(), 1);
    v = MyObject::callArgs.at(0);
    dv = qdbus_cast<QDBusVariant>(v);
    QCOMPARE(dv.variant().type(), QVariant::String);
    QCOMPARE(dv.variant().toString(), QString("bar"));

    // verify reply
    QCOMPARE(reply.arguments().count(), 1);
    v = reply.arguments().at(0);
    dv = qdbus_cast<QDBusVariant>(v);
    QCOMPARE(dv.variant().type(), QVariant::String);
    QCOMPARE(dv.variant().toString(), QString("bar"));
}

void tst_QDBusInterface::invokeMethod()
{
    QDBusConnection con = QDBusConnection::sessionBus();
    QDBusInterface iface(QDBusConnection::sessionBus().baseService(), QLatin1String("/"),
                         TEST_INTERFACE_NAME);

    MyObject::callCount = 0;

    // make the SLOT call without a return type
    QDBusVariant arg("foo");
    QVERIFY(QMetaObject::invokeMethod(&iface, "ping", Q_ARG(QDBusVariant, arg)));
    QCOMPARE(MyObject::callCount, 1);

    // verify what the callee received
    QCOMPARE(MyObject::callArgs.count(), 1);
    QVariant v = MyObject::callArgs.at(0);
    QDBusVariant dv = qdbus_cast<QDBusVariant>(v);
    QCOMPARE(dv.variant().type(), QVariant::String);
    QCOMPARE(dv.variant().toString(), QString("foo"));

    // make the INVOKABLE call without a return type
    QDBusVariant arg2("bar");
    QVERIFY(QMetaObject::invokeMethod(&iface, "ping_invokable", Q_ARG(QDBusVariant, arg2)));
    QCOMPARE(MyObject::callCount, 2);

    // verify what the callee received
    QCOMPARE(MyObject::callArgs.count(), 1);
    v = MyObject::callArgs.at(0);
    dv = qdbus_cast<QDBusVariant>(v);
    QCOMPARE(dv.variant().type(), QVariant::String);
    QCOMPARE(dv.variant().toString(), QString("bar"));
}

void tst_QDBusInterface::invokeMethodWithReturn()
{
    QDBusConnection con = QDBusConnection::sessionBus();
    QDBusInterface iface(QDBusConnection::sessionBus().baseService(), QLatin1String("/"),
                         TEST_INTERFACE_NAME);

    MyObject::callCount = 0;
    QDBusVariant retArg;

    // make the SLOT call without a return type
    QDBusVariant arg("foo");
    QVERIFY(QMetaObject::invokeMethod(&iface, "ping", Q_RETURN_ARG(QDBusVariant, retArg), Q_ARG(QDBusVariant, arg)));
    QCOMPARE(MyObject::callCount, 1);

    // verify what the callee received
    QCOMPARE(MyObject::callArgs.count(), 1);
    QVariant v = MyObject::callArgs.at(0);
    QDBusVariant dv = qdbus_cast<QDBusVariant>(v);
    QCOMPARE(dv.variant().type(), QVariant::String);
    QCOMPARE(dv.variant().toString(), arg.variant().toString());

    // verify that we got the reply as expected
    QCOMPARE(retArg.variant(), arg.variant());

    // make the INVOKABLE call without a return type
    QDBusVariant arg2("bar");
    QVERIFY(QMetaObject::invokeMethod(&iface, "ping_invokable", Q_RETURN_ARG(QDBusVariant, retArg), Q_ARG(QDBusVariant, arg2)));
    QCOMPARE(MyObject::callCount, 2);

    // verify what the callee received
    QCOMPARE(MyObject::callArgs.count(), 1);
    v = MyObject::callArgs.at(0);
    dv = qdbus_cast<QDBusVariant>(v);
    QCOMPARE(dv.variant().type(), QVariant::String);
    QCOMPARE(dv.variant().toString(), arg2.variant().toString());

    // verify that we got the reply as expected
    QCOMPARE(retArg.variant(), arg2.variant());
}

void tst_QDBusInterface::invokeMethodWithMultiReturn()
{
    QDBusConnection con = QDBusConnection::sessionBus();
    QDBusInterface iface(QDBusConnection::sessionBus().baseService(), QLatin1String("/"),
                         TEST_INTERFACE_NAME);

    MyObject::callCount = 0;
    QDBusVariant retArg, retArg2;

    // make the SLOT call without a return type
    QDBusVariant arg("foo"), arg2("bar");
    QVERIFY(QMetaObject::invokeMethod(&iface, "ping",
                                      Q_RETURN_ARG(QDBusVariant, retArg),
                                      Q_ARG(QDBusVariant, arg),
                                      Q_ARG(QDBusVariant, arg2),
                                      Q_ARG(QDBusVariant&, retArg2)));
    QCOMPARE(MyObject::callCount, 1);

    // verify what the callee received
    QCOMPARE(MyObject::callArgs.count(), 2);
    QVariant v = MyObject::callArgs.at(0);
    QDBusVariant dv = qdbus_cast<QDBusVariant>(v);
    QCOMPARE(dv.variant().type(), QVariant::String);
    QCOMPARE(dv.variant().toString(), arg.variant().toString());

    v = MyObject::callArgs.at(1);
    dv = qdbus_cast<QDBusVariant>(v);
    QCOMPARE(dv.variant().type(), QVariant::String);
    QCOMPARE(dv.variant().toString(), arg2.variant().toString());

    // verify that we got the replies as expected
    QCOMPARE(retArg.variant(), arg.variant());
    QCOMPARE(retArg2.variant(), arg2.variant());

    // make the INVOKABLE call without a return type
    QDBusVariant arg3("hello"), arg4("world");
    QVERIFY(QMetaObject::invokeMethod(&iface, "ping_invokable",
                                      Q_RETURN_ARG(QDBusVariant, retArg),
                                      Q_ARG(QDBusVariant, arg3),
                                      Q_ARG(QDBusVariant, arg4),
                                      Q_ARG(QDBusVariant&, retArg2)));
    QCOMPARE(MyObject::callCount, 2);

    // verify what the callee received
    QCOMPARE(MyObject::callArgs.count(), 2);
    v = MyObject::callArgs.at(0);
    dv = qdbus_cast<QDBusVariant>(v);
    QCOMPARE(dv.variant().type(), QVariant::String);
    QCOMPARE(dv.variant().toString(), arg3.variant().toString());

    v = MyObject::callArgs.at(1);
    dv = qdbus_cast<QDBusVariant>(v);
    QCOMPARE(dv.variant().type(), QVariant::String);
    QCOMPARE(dv.variant().toString(), arg4.variant().toString());

    // verify that we got the replies as expected
    QCOMPARE(retArg.variant(), arg3.variant());
    QCOMPARE(retArg2.variant(), arg4.variant());
}

void tst_QDBusInterface::invokeMethodWithComplexReturn()
{
    QDBusConnection con = QDBusConnection::sessionBus();
    QDBusInterface iface(QDBusConnection::sessionBus().baseService(), QLatin1String("/"),
                         TEST_INTERFACE_NAME);

    MyObject::callCount = 0;
    QList<int> retArg;

    // make the SLOT call without a return type
    QList<int> arg = QList<int>() << 42 << -47;
    QVERIFY(QMetaObject::invokeMethod(&iface, "ping", Q_RETURN_ARG(QList<int>, retArg), Q_ARG(QList<int>, arg)));
    QCOMPARE(MyObject::callCount, 1);

    // verify what the callee received
    QCOMPARE(MyObject::callArgs.count(), 1);
    QVariant v = MyObject::callArgs.at(0);
    QCOMPARE(v.userType(), qMetaTypeId<QDBusArgument>());
    QCOMPARE(qdbus_cast<QList<int> >(v), arg);

    // verify that we got the reply as expected
    QCOMPARE(retArg, arg);

    // make the INVOKABLE call without a return type
    QList<int> arg2 = QList<int>() << 24 << -74;
    QVERIFY(QMetaObject::invokeMethod(&iface, "ping", Q_RETURN_ARG(QList<int>, retArg), Q_ARG(QList<int>, arg2)));
    QCOMPARE(MyObject::callCount, 2);

    // verify what the callee received
    QCOMPARE(MyObject::callArgs.count(), 1);
    v = MyObject::callArgs.at(0);
    QCOMPARE(v.userType(), qMetaTypeId<QDBusArgument>());
    QCOMPARE(qdbus_cast<QList<int> >(v), arg2);

    // verify that we got the reply as expected
    QCOMPARE(retArg, arg2);
}

void tst_QDBusInterface::introspectPeer()
{
    QDBusConnection con("peer");
    QDBusInterface iface(QString(), QLatin1String("/"),
                         TEST_INTERFACE_NAME, con);

    const QMetaObject *mo = iface.metaObject();

    QCOMPARE(mo->methodCount() - mo->methodOffset(), 7);
    QVERIFY(mo->indexOfSignal(TEST_SIGNAL_NAME "(QString)") != -1);

    QCOMPARE(mo->propertyCount() - mo->propertyOffset(), 2);
    QVERIFY(mo->indexOfProperty("prop1") != -1);
    QVERIFY(mo->indexOfProperty("complexProp") != -1);
}

void tst_QDBusInterface::callMethodPeer()
{
    QDBusConnection con("peer");
    QDBusInterface iface(QString(), QLatin1String("/"),
                         TEST_INTERFACE_NAME, con);

    resetPeer();

    // call a SLOT method
    QDBusMessage reply = iface.call("ping", QVariant::fromValue(QDBusVariant("foo")));
    QCOMPARE(callCountPeer(), 1);
    QCOMPARE(reply.type(), QDBusMessage::ReplyMessage);

    // verify what the callee received
    QVariantList callArgs = callArgsPeer();
    QCOMPARE(callArgs.count(), 1);
    QVariant v = callArgs.at(0);
    QDBusVariant dv = qdbus_cast<QDBusVariant>(v);
    QCOMPARE(dv.variant().type(), QVariant::String);
    QCOMPARE(dv.variant().toString(), QString("foo"));

    // verify reply
    QCOMPARE(reply.arguments().count(), 1);
    v = reply.arguments().at(0);
    dv = qdbus_cast<QDBusVariant>(v);
    QCOMPARE(dv.variant().type(), QVariant::String);
    QCOMPARE(dv.variant().toString(), QString("foo"));

    // call an INVOKABLE method
    reply = iface.call("ping_invokable", QVariant::fromValue(QDBusVariant("bar")));
    QCOMPARE(callCountPeer(), 2);
    QCOMPARE(reply.type(), QDBusMessage::ReplyMessage);

    // verify what the callee received
    callArgs = callArgsPeer();
    QCOMPARE(callArgs.count(), 1);
    v = callArgs.at(0);
    dv = qdbus_cast<QDBusVariant>(v);
    QCOMPARE(dv.variant().type(), QVariant::String);
    QCOMPARE(dv.variant().toString(), QString("bar"));

    // verify reply
    QCOMPARE(reply.arguments().count(), 1);
    v = reply.arguments().at(0);
    dv = qdbus_cast<QDBusVariant>(v);
    QCOMPARE(dv.variant().type(), QVariant::String);
    QCOMPARE(dv.variant().toString(), QString("bar"));
}

void tst_QDBusInterface::invokeMethodPeer()
{
    QDBusConnection con("peer");
    QDBusInterface iface(QString(), QLatin1String("/"),
                         TEST_INTERFACE_NAME, con);

    resetPeer();

    // make the SLOT call without a return type
    QDBusVariant arg("foo");
    QVERIFY(QMetaObject::invokeMethod(&iface, "ping", Q_ARG(QDBusVariant, arg)));
    QCOMPARE(callCountPeer(), 1);

    // verify what the callee received
    QVariantList callArgs = callArgsPeer();
    QCOMPARE(callArgs.count(), 1);
    QVariant v = callArgs.at(0);
    QDBusVariant dv = qdbus_cast<QDBusVariant>(v);
    QCOMPARE(dv.variant().type(), QVariant::String);
    QCOMPARE(dv.variant().toString(), QString("foo"));

    // make the INVOKABLE call without a return type
    QDBusVariant arg2("bar");
    QVERIFY(QMetaObject::invokeMethod(&iface, "ping_invokable", Q_ARG(QDBusVariant, arg2)));
    QCOMPARE(callCountPeer(), 2);

    // verify what the callee received
    callArgs = callArgsPeer();
    QCOMPARE(callArgs.count(), 1);
    v = callArgs.at(0);
    dv = qdbus_cast<QDBusVariant>(v);
    QCOMPARE(dv.variant().type(), QVariant::String);
    QCOMPARE(dv.variant().toString(), QString("bar"));
}

void tst_QDBusInterface::invokeMethodWithReturnPeer()
{
    QDBusConnection con("peer");
    QDBusInterface iface(QString(), QLatin1String("/"),
                         TEST_INTERFACE_NAME, con);

    resetPeer();
    QDBusVariant retArg;

    // make the SLOT call without a return type
    QDBusVariant arg("foo");
    QVERIFY(QMetaObject::invokeMethod(&iface, "ping", Q_RETURN_ARG(QDBusVariant, retArg), Q_ARG(QDBusVariant, arg)));
    QCOMPARE(callCountPeer(), 1);

    // verify what the callee received
    QVariantList callArgs = callArgsPeer();
    QCOMPARE(callArgs.count(), 1);
    QVariant v = callArgs.at(0);
    QDBusVariant dv = qdbus_cast<QDBusVariant>(v);
    QCOMPARE(dv.variant().type(), QVariant::String);
    QCOMPARE(dv.variant().toString(), arg.variant().toString());

    // verify that we got the reply as expected
    QCOMPARE(retArg.variant(), arg.variant());

    // make the INVOKABLE call without a return type
    QDBusVariant arg2("bar");
    QVERIFY(QMetaObject::invokeMethod(&iface, "ping_invokable", Q_RETURN_ARG(QDBusVariant, retArg), Q_ARG(QDBusVariant, arg2)));
    QCOMPARE(callCountPeer(), 2);

    // verify what the callee received
    callArgs = callArgsPeer();
    QCOMPARE(callArgs.count(), 1);
    v = callArgs.at(0);
    dv = qdbus_cast<QDBusVariant>(v);
    QCOMPARE(dv.variant().type(), QVariant::String);
    QCOMPARE(dv.variant().toString(), arg2.variant().toString());

    // verify that we got the reply as expected
    QCOMPARE(retArg.variant(), arg2.variant());
}

void tst_QDBusInterface::invokeMethodWithMultiReturnPeer()
{
    QDBusConnection con("peer");
    QDBusInterface iface(QString(), QLatin1String("/"),
                         TEST_INTERFACE_NAME, con);

    resetPeer();
    QDBusVariant retArg, retArg2;

    // make the SLOT call without a return type
    QDBusVariant arg("foo"), arg2("bar");
    QVERIFY(QMetaObject::invokeMethod(&iface, "ping",
                                      Q_RETURN_ARG(QDBusVariant, retArg),
                                      Q_ARG(QDBusVariant, arg),
                                      Q_ARG(QDBusVariant, arg2),
                                      Q_ARG(QDBusVariant&, retArg2)));
    QCOMPARE(callCountPeer(), 1);

    // verify what the callee received
    QVariantList callArgs = callArgsPeer();
    QCOMPARE(callArgs.count(), 2);
    QVariant v = callArgs.at(0);
    QDBusVariant dv = qdbus_cast<QDBusVariant>(v);
    QCOMPARE(dv.variant().type(), QVariant::String);
    QCOMPARE(dv.variant().toString(), arg.variant().toString());

    v = callArgs.at(1);
    dv = qdbus_cast<QDBusVariant>(v);
    QCOMPARE(dv.variant().type(), QVariant::String);
    QCOMPARE(dv.variant().toString(), arg2.variant().toString());

    // verify that we got the replies as expected
    QCOMPARE(retArg.variant(), arg.variant());
    QCOMPARE(retArg2.variant(), arg2.variant());

    // make the INVOKABLE call without a return type
    QDBusVariant arg3("hello"), arg4("world");
    QVERIFY(QMetaObject::invokeMethod(&iface, "ping_invokable",
                                      Q_RETURN_ARG(QDBusVariant, retArg),
                                      Q_ARG(QDBusVariant, arg3),
                                      Q_ARG(QDBusVariant, arg4),
                                      Q_ARG(QDBusVariant&, retArg2)));
    QCOMPARE(callCountPeer(), 2);

    // verify what the callee received
    callArgs = callArgsPeer();
    QCOMPARE(callArgs.count(), 2);
    v = callArgs.at(0);
    dv = qdbus_cast<QDBusVariant>(v);
    QCOMPARE(dv.variant().type(), QVariant::String);
    QCOMPARE(dv.variant().toString(), arg3.variant().toString());

    v = callArgs.at(1);
    dv = qdbus_cast<QDBusVariant>(v);
    QCOMPARE(dv.variant().type(), QVariant::String);
    QCOMPARE(dv.variant().toString(), arg4.variant().toString());

    // verify that we got the replies as expected
    QCOMPARE(retArg.variant(), arg3.variant());
    QCOMPARE(retArg2.variant(), arg4.variant());
}

void tst_QDBusInterface::invokeMethodWithComplexReturnPeer()
{
    QDBusConnection con("peer");
    QDBusInterface iface(QString(), QLatin1String("/"),
                         TEST_INTERFACE_NAME, con);

    resetPeer();
    QList<int> retArg;

    // make the SLOT call without a return type
    QList<int> arg = QList<int>() << 42 << -47;
    QVERIFY(QMetaObject::invokeMethod(&iface, "ping", Q_RETURN_ARG(QList<int>, retArg), Q_ARG(QList<int>, arg)));
    QCOMPARE(callCountPeer(), 1);

    // verify what the callee received
    QVariantList callArgs = callArgsPeer();
    QCOMPARE(callArgs.count(), 1);
    QVariant v = callArgs.at(0);
    QCOMPARE(v.userType(), qMetaTypeId<QDBusArgument>());
    QCOMPARE(qdbus_cast<QList<int> >(v), arg);

    // verify that we got the reply as expected
    QCOMPARE(retArg, arg);

    // make the INVOKABLE call without a return type
    QList<int> arg2 = QList<int>() << 24 << -74;
    QVERIFY(QMetaObject::invokeMethod(&iface, "ping", Q_RETURN_ARG(QList<int>, retArg), Q_ARG(QList<int>, arg2)));
    QCOMPARE(callCountPeer(), 2);

    // verify what the callee received
    callArgs = callArgsPeer();
    QCOMPARE(callArgs.count(), 1);
    v = callArgs.at(0);
    QCOMPARE(v.userType(), qMetaTypeId<QDBusArgument>());
    QCOMPARE(qdbus_cast<QList<int> >(v), arg2);

    // verify that we got the reply as expected
    QCOMPARE(retArg, arg2);
}

void tst_QDBusInterface::signal()
{
    QDBusConnection con = QDBusConnection::sessionBus();
    QDBusInterface iface(QDBusConnection::sessionBus().baseService(), QLatin1String("/"),
                         TEST_INTERFACE_NAME);

    QString arg = "So long and thanks for all the fish";
    {
        Spy spy;
        spy.connect(&iface, SIGNAL(somethingHappened(QString)), SLOT(spySlot(QString)));

        emitSignal(TEST_INTERFACE_NAME, TEST_SIGNAL_NAME, arg);
        QCOMPARE(spy.count, 1);
        QCOMPARE(spy.received, arg);
    }

    QDBusInterface iface2(QDBusConnection::sessionBus().baseService(), QLatin1String("/"),
                          TEST_INTERFACE_NAME);
    {
        Spy spy;
        spy.connect(&iface, SIGNAL(somethingHappened(QString)), SLOT(spySlot(QString)));
        spy.connect(&iface2, SIGNAL(somethingHappened(QString)), SLOT(spySlot(QString)));

        emitSignal(TEST_INTERFACE_NAME, TEST_SIGNAL_NAME, arg);
        QCOMPARE(spy.count, 2);
        QCOMPARE(spy.received, arg);
    }

    {
        Spy spy, spy2;
        spy.connect(&iface, SIGNAL(somethingHappened(QString)), SLOT(spySlot(QString)));
        spy2.connect(&iface2, SIGNAL(somethingHappened(QString)), SLOT(spySlot(QString)));

        emitSignal(TEST_INTERFACE_NAME, TEST_SIGNAL_NAME, arg);
        QCOMPARE(spy.count, 1);
        QCOMPARE(spy.received, arg);
        QCOMPARE(spy2.count, 1);
        QCOMPARE(spy2.received, arg);
    }
}

void tst_QDBusInterface::signalPeer()
{
    QDBusConnection con("peer");
    QDBusInterface iface(QString(), QLatin1String("/"),
                         TEST_INTERFACE_NAME, con);

    QString arg = "So long and thanks for all the fish";
    {
        Spy spy;
        spy.connect(&iface, SIGNAL(somethingHappened(QString)), SLOT(spySlot(QString)));

        emitSignalPeer(TEST_INTERFACE_NAME, TEST_SIGNAL_NAME, arg);
        QCOMPARE(spy.count, 1);
        QCOMPARE(spy.received, arg);
    }

    QDBusInterface iface2(QString(), QLatin1String("/"),
                          TEST_INTERFACE_NAME, con);
    {
        Spy spy;
        spy.connect(&iface, SIGNAL(somethingHappened(QString)), SLOT(spySlot(QString)));
        spy.connect(&iface2, SIGNAL(somethingHappened(QString)), SLOT(spySlot(QString)));

        emitSignalPeer(TEST_INTERFACE_NAME, TEST_SIGNAL_NAME, arg);
        QCOMPARE(spy.count, 2);
        QCOMPARE(spy.received, arg);
    }

    {
        Spy spy, spy2;
        spy.connect(&iface, SIGNAL(somethingHappened(QString)), SLOT(spySlot(QString)));
        spy2.connect(&iface2, SIGNAL(somethingHappened(QString)), SLOT(spySlot(QString)));

        emitSignalPeer(TEST_INTERFACE_NAME, TEST_SIGNAL_NAME, arg);
        QCOMPARE(spy.count, 1);
        QCOMPARE(spy.received, arg);
        QCOMPARE(spy2.count, 1);
        QCOMPARE(spy2.received, arg);
    }
}

void tst_QDBusInterface::propertyRead()
{
    QDBusConnection con = QDBusConnection::sessionBus();
    QDBusInterface iface(QDBusConnection::sessionBus().baseService(), QLatin1String("/"),
                         TEST_INTERFACE_NAME);

    int arg = obj.m_prop1 = 42;
    MyObject::callCount = 0;

    QVariant v = iface.property("prop1");
    QVERIFY(v.isValid());
    QCOMPARE(v.userType(), int(QVariant::Int));
    QCOMPARE(v.toInt(), arg);
    QCOMPARE(MyObject::callCount, 1);
}

void tst_QDBusInterface::propertyWrite()
{
    QDBusConnection con = QDBusConnection::sessionBus();
    QDBusInterface iface(QDBusConnection::sessionBus().baseService(), QLatin1String("/"),
                         TEST_INTERFACE_NAME);

    int arg = 42;
    obj.m_prop1 = 0;
    MyObject::callCount = 0;

    QVERIFY(iface.setProperty("prop1", arg));
    QCOMPARE(MyObject::callCount, 1);
    QCOMPARE(obj.m_prop1, arg);
}

void tst_QDBusInterface::complexPropertyRead()
{
    QDBusConnection con = QDBusConnection::sessionBus();
    QDBusInterface iface(QDBusConnection::sessionBus().baseService(), QLatin1String("/"),
                         TEST_INTERFACE_NAME);

    QList<int> arg = obj.m_complexProp = QList<int>() << 42 << -47;
    MyObject::callCount = 0;

    QVariant v = iface.property("complexProp");
    QVERIFY(v.isValid());
    QCOMPARE(v.userType(), qMetaTypeId<QList<int> >());
    QCOMPARE(v.value<QList<int> >(), arg);
    QCOMPARE(MyObject::callCount, 1);
}

void tst_QDBusInterface::complexPropertyWrite()
{
    QDBusConnection con = QDBusConnection::sessionBus();
    QDBusInterface iface(QDBusConnection::sessionBus().baseService(), QLatin1String("/"),
                         TEST_INTERFACE_NAME);

    QList<int> arg = QList<int>() << -47 << 42;
    obj.m_complexProp.clear();
    MyObject::callCount = 0;

    QVERIFY(iface.setProperty("complexProp", QVariant::fromValue(arg)));
    QCOMPARE(MyObject::callCount, 1);
    QCOMPARE(obj.m_complexProp, arg);
}

void tst_QDBusInterface::propertyReadPeer()
{
    QDBusConnection con("peer");
    QDBusInterface iface(QString(), QLatin1String("/"),
                         TEST_INTERFACE_NAME, con);

    resetPeer();
    int arg = 42;
    setProp1Peer(42);

    QVariant v = iface.property("prop1");
    QVERIFY(v.isValid());
    QCOMPARE(v.userType(), int(QVariant::Int));
    QCOMPARE(v.toInt(), arg);
    QCOMPARE(callCountPeer(), 1);
}

void tst_QDBusInterface::propertyWritePeer()
{
    QDBusConnection con("peer");
    QDBusInterface iface(QString(), QLatin1String("/"),
                         TEST_INTERFACE_NAME, con);

    resetPeer();
    int arg = 42;
    setProp1Peer(0);

    QVERIFY(iface.setProperty("prop1", arg));
    QCOMPARE(callCountPeer(), 1);
    QCOMPARE(prop1Peer(), arg);
}

void tst_QDBusInterface::complexPropertyReadPeer()
{
    QDBusConnection con("peer");
    QDBusInterface iface(QString(), QLatin1String("/"),
                         TEST_INTERFACE_NAME, con);

    resetPeer();
    QList<int> arg = QList<int>() << 42 << -47;
    setComplexPropPeer(arg);

    QVariant v = iface.property("complexProp");
    QVERIFY(v.isValid());
    QCOMPARE(v.userType(), qMetaTypeId<QList<int> >());
    QCOMPARE(v.value<QList<int> >(), arg);
    QCOMPARE(callCountPeer(), 1);
}

void tst_QDBusInterface::complexPropertyWritePeer()
{
    QDBusConnection con("peer");
    QDBusInterface iface(QString(), QLatin1String("/"),
                         TEST_INTERFACE_NAME, con);

    resetPeer();
    QList<int> arg = QList<int>() << -47 << 42;

    QVERIFY(iface.setProperty("complexProp", QVariant::fromValue(arg)));
    QCOMPARE(callCountPeer(), 1);
    QCOMPARE(complexPropPeer(), arg);
}

QTEST_MAIN(tst_QDBusInterface)

#include "tst_qdbusinterface.moc"
