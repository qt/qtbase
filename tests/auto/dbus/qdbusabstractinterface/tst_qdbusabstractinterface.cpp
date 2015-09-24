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
#include <qcoreapplication.h>
#include <qdebug.h>
#include <qsharedpointer.h>

#include <QtTest/QtTest>

#include <QtDBus>

#include "interface.h"
#include "pinger_interface.h"

static const char serviceName[] = "org.qtproject.autotests.qpinger";
static const char objectPath[] = "/org/qtproject/qpinger";
static const char *interfaceName = serviceName;

typedef QSharedPointer<org::qtproject::QtDBus::Pinger> Pinger;

class tst_QDBusAbstractInterface: public QObject
{
    Q_OBJECT
    Interface targetObj;
    QString peerAddress;

    Pinger getPinger(QString service = "", const QString &path = "/")
    {
        QDBusConnection con = QDBusConnection::sessionBus();
        if (!con.isConnected())
            return Pinger();
        if (service.isEmpty() && !service.isNull())
            service = con.baseService();
        return Pinger(new org::qtproject::QtDBus::Pinger(service, path, con));
    }

    Pinger getPingerPeer(const QString &path = "/", const QString &service = "")
    {
        QDBusConnection con = QDBusConnection("peer");
        if (!con.isConnected())
            return Pinger();
        return Pinger(new org::qtproject::QtDBus::Pinger(service, path, con));
    }

    void resetServer()
    {
        QDBusMessage req = QDBusMessage::createMethodCall(serviceName, objectPath, interfaceName, "reset");
        QDBusConnection::sessionBus().send(req);
    }

public:
    tst_QDBusAbstractInterface();

private slots:
    void initTestCase();
    void cleanupTestCase();

    void init();
    void cleanup();

    void makeVoidCall();
    void makeStringCall();
    void makeComplexCall();
    void makeMultiOutCall();

    void makeVoidCallPeer();
    void makeStringCallPeer();
    void makeComplexCallPeer();
    void makeMultiOutCallPeer();

    void makeAsyncVoidCall();
    void makeAsyncStringCall();
    void makeAsyncComplexCall();
    void makeAsyncMultiOutCall();

    void makeAsyncVoidCallPeer();
    void makeAsyncStringCallPeer();
    void makeAsyncComplexCallPeer();
    void makeAsyncMultiOutCallPeer();

    void callWithTimeout();

    void stringPropRead();
    void stringPropWrite();
    void variantPropRead();
    void variantPropWrite();
    void complexPropRead();
    void complexPropWrite();

    void stringPropReadPeer();
    void stringPropWritePeer();
    void variantPropReadPeer();
    void variantPropWritePeer();
    void complexPropReadPeer();
    void complexPropWritePeer();

    void stringPropDirectRead();
    void stringPropDirectWrite();
    void variantPropDirectRead();
    void variantPropDirectWrite();
    void complexPropDirectRead();
    void complexPropDirectWrite();

    void stringPropDirectReadPeer();
    void stringPropDirectWritePeer();
    void variantPropDirectReadPeer();
    void variantPropDirectWritePeer();
    void complexPropDirectReadPeer();
    void complexPropDirectWritePeer();

    void getVoidSignal_data();
    void getVoidSignal();
    void getStringSignal_data();
    void getStringSignal();
    void getComplexSignal_data();
    void getComplexSignal();

    void getVoidSignalPeer_data();
    void getVoidSignalPeer();
    void getStringSignalPeer_data();
    void getStringSignalPeer();
    void getComplexSignalPeer_data();
    void getComplexSignalPeer();

    void followSignal();

    void connectDisconnect_data();
    void connectDisconnect();
    void connectDisconnectPeer_data();
    void connectDisconnectPeer();

    void createErrors_data();
    void createErrors();

    void createErrorsPeer_data();
    void createErrorsPeer();

    void callErrors_data();
    void callErrors();
    void asyncCallErrors_data();
    void asyncCallErrors();

    void callErrorsPeer_data();
    void callErrorsPeer();
    void asyncCallErrorsPeer_data();
    void asyncCallErrorsPeer();

    void propertyReadErrors_data();
    void propertyReadErrors();
    void propertyWriteErrors_data();
    void propertyWriteErrors();
    void directPropertyReadErrors_data();
    void directPropertyReadErrors();
    void directPropertyWriteErrors_data();
    void directPropertyWriteErrors();

    void propertyReadErrorsPeer_data();
    void propertyReadErrorsPeer();
    void propertyWriteErrorsPeer_data();
    void propertyWriteErrorsPeer();
    void directPropertyReadErrorsPeer_data();
    void directPropertyReadErrorsPeer();
    void directPropertyWriteErrorsPeer_data();
    void directPropertyWriteErrorsPeer();

    void validity_data();
    void validity();

private:
    QProcess proc;
};

class SignalReceiver : public QObject
{
    Q_OBJECT
public:
    int callCount;
    SignalReceiver() : callCount(0) {}
public slots:
    void receive() { ++callCount; }
};

tst_QDBusAbstractInterface::tst_QDBusAbstractInterface()
{
    // register the meta types
    qDBusRegisterMetaType<RegisteredType>();
    qRegisterMetaType<UnregisteredType>();
}

void tst_QDBusAbstractInterface::initTestCase()
{
    // enable debugging temporarily:
    //putenv("QDBUS_DEBUG=1");

    // register the object
    QDBusConnection con = QDBusConnection::sessionBus();
    QVERIFY(con.isConnected());
    con.registerObject("/", &targetObj, QDBusConnection::ExportScriptableContents);

    // verify service isn't registered by something else
    // (e.g. a left over qpinger from a previous test run)
    QVERIFY(!con.interface()->isServiceRegistered(serviceName));

    // start peer server
#ifdef Q_OS_WIN
#  define EXE ".exe"
#else
#  define EXE ""
#endif
    proc.setProcessChannelMode(QProcess::ForwardedErrorChannel);
    proc.start(QFINDTESTDATA("qpinger/qpinger" EXE));
    QVERIFY2(proc.waitForStarted(), qPrintable(proc.errorString()));
    QVERIFY(proc.waitForReadyRead());

    // verify service is now registered
    QTRY_VERIFY(con.interface()->isServiceRegistered(serviceName));

    // get peer server address
    QDBusMessage req = QDBusMessage::createMethodCall(serviceName, objectPath, interfaceName, "address");
    QDBusMessage rpl = con.call(req);
    QVERIFY(rpl.type() == QDBusMessage::ReplyMessage);
    peerAddress = rpl.arguments().at(0).toString();
}

void tst_QDBusAbstractInterface::cleanupTestCase()
{
    QDBusMessage msg = QDBusMessage::createMethodCall(serviceName, objectPath, interfaceName, "quit");
    QDBusConnection::sessionBus().call(msg);
    proc.waitForFinished(200);
    proc.close();
}

void tst_QDBusAbstractInterface::init()
{
    QDBusConnection con = QDBusConnection::sessionBus();
    QVERIFY(con.isConnected());

    // connect to peer server
    QDBusConnection peercon = QDBusConnection::connectToPeer(peerAddress, "peer");
    QVERIFY(peercon.isConnected());

    QDBusMessage req2 = QDBusMessage::createMethodCall(serviceName, objectPath, interfaceName, "waitForConnected");
    QDBusMessage rpl2 = con.call(req2);
    QVERIFY2(rpl2.type() == QDBusMessage::ReplyMessage, rpl2.errorMessage().toLatin1());
}

void tst_QDBusAbstractInterface::cleanup()
{
    QDBusConnection::disconnectFromPeer("peer");

    // Reset the object exported by this process
    targetObj.m_stringProp = QString();
    targetObj.m_variantProp = QDBusVariant();
    targetObj.m_complexProp = RegisteredType();

    QDBusMessage resetCall = QDBusMessage::createMethodCall(serviceName, objectPath, interfaceName, "reset");
    QVERIFY(QDBusConnection::sessionBus().call(resetCall).type() == QDBusMessage::ReplyMessage);
}

void tst_QDBusAbstractInterface::makeVoidCall()
{
    Pinger p = getPinger();
    QVERIFY2(p, "Not connected to D-Bus");

    QDBusReply<void> r = p->voidMethod();
    QVERIFY(r.isValid());
}

void tst_QDBusAbstractInterface::makeStringCall()
{
    Pinger p = getPinger();
    QVERIFY2(p, "Not connected to D-Bus");

    QDBusReply<QString> r = p->stringMethod();
    QVERIFY(r.isValid());
    QCOMPARE(r.value(), targetObj.stringMethod());
}

static QHash<QString, QVariant> complexMethodArgs()
{
    QHash<QString, QVariant> args;
    args.insert("arg1", "Hello world");
    args.insert("arg2", 12345);
    return args;
}

void tst_QDBusAbstractInterface::makeComplexCall()
{
    Pinger p = getPinger();
    QVERIFY2(p, "Not connected to D-Bus");

    QDBusReply<RegisteredType> r = p->complexMethod(complexMethodArgs());
    QVERIFY(r.isValid());
    QCOMPARE(r.value(), targetObj.complexMethod(complexMethodArgs()));
}

void tst_QDBusAbstractInterface::makeMultiOutCall()
{
    Pinger p = getPinger();
    QVERIFY2(p, "Not connected to D-Bus");

    int value;
    QDBusReply<QString> r = p->multiOutMethod(value);
    QVERIFY(r.isValid());

    int expectedValue;
    QCOMPARE(r.value(), targetObj.multiOutMethod(expectedValue));
    QCOMPARE(value, expectedValue);
}

void tst_QDBusAbstractInterface::makeVoidCallPeer()
{
    Pinger p = getPingerPeer();
    QVERIFY2(p, "Not connected to D-Bus");

    QDBusReply<void> r = p->voidMethod();
    QVERIFY(r.isValid());
}

void tst_QDBusAbstractInterface::makeStringCallPeer()
{
    Pinger p = getPingerPeer();
    QVERIFY2(p, "Not connected to D-Bus");

    QDBusReply<QString> r = p->stringMethod();
    QVERIFY(r.isValid());
    QCOMPARE(r.value(), targetObj.stringMethod());
}

void tst_QDBusAbstractInterface::makeComplexCallPeer()
{
    Pinger p = getPingerPeer();
    QVERIFY2(p, "Not connected to D-Bus");

    QDBusReply<RegisteredType> r = p->complexMethod(complexMethodArgs());
    QVERIFY(r.isValid());
    QCOMPARE(r.value(), targetObj.complexMethod(complexMethodArgs()));
}

void tst_QDBusAbstractInterface::makeMultiOutCallPeer()
{
    Pinger p = getPingerPeer();
    QVERIFY2(p, "Not connected to D-Bus");

    int value;
    QDBusReply<QString> r = p->multiOutMethod(value);
    QVERIFY(r.isValid());

    int expectedValue;
    QCOMPARE(r.value(), targetObj.multiOutMethod(expectedValue));
    QCOMPARE(value, expectedValue);
}

void tst_QDBusAbstractInterface::makeAsyncVoidCall()
{
    Pinger p = getPinger();
    QVERIFY2(p, "Not connected to D-Bus");

    QDBusPendingReply<void> r = p->voidMethod();
    r.waitForFinished();
    QVERIFY(r.isValid());
}

void tst_QDBusAbstractInterface::makeAsyncStringCall()
{
    Pinger p = getPinger();
    QVERIFY2(p, "Not connected to D-Bus");

    QDBusPendingReply<QString> r = p->stringMethod();
    r.waitForFinished();
    QVERIFY(r.isValid());
    QCOMPARE(r.value(), targetObj.stringMethod());
}

void tst_QDBusAbstractInterface::makeAsyncComplexCall()
{
    Pinger p = getPinger();
    QVERIFY2(p, "Not connected to D-Bus");

    QDBusPendingReply<RegisteredType> r = p->complexMethod(complexMethodArgs());
    r.waitForFinished();
    QVERIFY(r.isValid());
    QCOMPARE(r.value(), targetObj.complexMethod(complexMethodArgs()));
}

void tst_QDBusAbstractInterface::makeAsyncMultiOutCall()
{
    Pinger p = getPinger();
    QVERIFY2(p, "Not connected to D-Bus");

    QDBusPendingReply<QString, int> r = p->multiOutMethod();
    r.waitForFinished();
    QVERIFY(r.isValid());

    int expectedValue;
    QCOMPARE(r.value(), targetObj.multiOutMethod(expectedValue));
    QCOMPARE(r.argumentAt<1>(), expectedValue);
}

void tst_QDBusAbstractInterface::makeAsyncVoidCallPeer()
{
    Pinger p = getPingerPeer();
    QVERIFY2(p, "Not connected to D-Bus");

    QDBusPendingReply<void> r = p->voidMethod();
    r.waitForFinished();
    QVERIFY(r.isValid());
}
void tst_QDBusAbstractInterface::makeAsyncStringCallPeer()
{
    Pinger p = getPingerPeer();
    QVERIFY2(p, "Not connected to D-Bus");

    QDBusMessage reply = p->call(QDBus::BlockWithGui, QLatin1String("voidMethod"));
    QVERIFY(reply.type() == QDBusMessage::ReplyMessage);

    QDBusPendingReply<QString> r = p->stringMethod();
    r.waitForFinished();
    QVERIFY(r.isValid());
    QCOMPARE(r.value(), targetObj.stringMethod());
}

void tst_QDBusAbstractInterface::makeAsyncComplexCallPeer()
{
    Pinger p = getPingerPeer();
    QVERIFY2(p, "Not connected to D-Bus");

    QDBusPendingReply<RegisteredType> r = p->complexMethod(complexMethodArgs());
    r.waitForFinished();
    QVERIFY(r.isValid());
    QCOMPARE(r.value(), targetObj.complexMethod(complexMethodArgs()));
}

void tst_QDBusAbstractInterface::makeAsyncMultiOutCallPeer()
{
    Pinger p = getPingerPeer();
    QVERIFY2(p, "Not connected to D-Bus");

    QDBusPendingReply<QString, int> r = p->multiOutMethod();
    r.waitForFinished();
    QVERIFY(r.isValid());

    int expectedValue;
    QCOMPARE(r.value(), targetObj.multiOutMethod(expectedValue));
    QCOMPARE(r.argumentAt<1>(), expectedValue);
    QCoreApplication::instance()->processEvents();
}

static const char server_serviceName[] = "org.qtproject.autotests.dbusserver";
static const char server_objectPath[] = "/org/qtproject/server";
static const char server_interfaceName[] = "org.qtproject.QtDBus.Pinger";

class DBusServerThread : public QThread
{
public:
    DBusServerThread() {
        start();
        m_ready.acquire();
    }
    ~DBusServerThread() {
        quit();
        wait();
    }

    void run()
    {
        QDBusConnection con = QDBusConnection::connectToBus(QDBusConnection::SessionBus, "ThreadConnection");
        if (!con.isConnected())
            qWarning("Error registering to DBus");
        if (!con.registerService(server_serviceName))
            qWarning("Error registering service name");
        Interface targetObj;
        con.registerObject(server_objectPath, &targetObj, QDBusConnection::ExportScriptableContents);
        m_ready.release();
        exec();

        QDBusConnection::disconnectFromBus( con.name() );
    }
private:
    QSemaphore m_ready;
};

void tst_QDBusAbstractInterface::callWithTimeout()
{
    QDBusConnection con = QDBusConnection::sessionBus();
    QVERIFY2(con.isConnected(), "Not connected to D-Bus");

    DBusServerThread serverThread;

    QDBusMessage msg = QDBusMessage::createMethodCall(server_serviceName,
                                                      server_objectPath, server_interfaceName, "sleepMethod");
    msg << 100; // sleep 100 ms

    {
       // Call with no timeout -> works
        QDBusMessage reply = con.call(msg);
        QCOMPARE((int)reply.type(), (int)QDBusMessage::ReplyMessage);
        QCOMPARE(reply.arguments().at(0).toInt(), 42);
    }

    {
        // Call with 1 msec timeout -> fails
        QDBusMessage reply = con.call(msg, QDBus::Block, 1);
        QCOMPARE(reply.type(), QDBusMessage::ErrorMessage);
    }

    // Now using QDBusInterface

    QDBusInterface iface(server_serviceName, server_objectPath, server_interfaceName, con);
    {
        // Call with no timeout
        QDBusMessage reply = iface.call("sleepMethod", 100);
        QCOMPARE(reply.type(), QDBusMessage::ReplyMessage);
        QCOMPARE(reply.arguments().at(0).toInt(), 42);
    }
    {
        // Call with 1 msec timeout -> fails
        iface.setTimeout(1);
        QDBusMessage reply = iface.call("sleepMethod", 100);
        QCOMPARE(reply.type(), QDBusMessage::ErrorMessage);
    }
    {
        // Call with 300 msec timeout -> works
        iface.setTimeout(300);
        QDBusMessage reply = iface.call("sleepMethod", 100);
        QCOMPARE(reply.arguments().at(0).toInt(), 42);
    }

    // Now using generated code
    org::qtproject::QtDBus::Pinger p(server_serviceName, server_objectPath, QDBusConnection::sessionBus());
    {
        // Call with no timeout
        QDBusReply<int> reply = p.sleepMethod(100);
        QVERIFY(reply.isValid());
        QCOMPARE(int(reply), 42);
    }
    {
        // Call with 1 msec timeout -> fails
        p.setTimeout(1);
        QDBusReply<int> reply = p.sleepMethod(100);
        QVERIFY(!reply.isValid());
    }
}

void tst_QDBusAbstractInterface::stringPropRead()
{
    Pinger p = getPinger();
    QVERIFY2(p, "Not connected to D-Bus");

    QString expectedValue = targetObj.m_stringProp = "This is a test";
    QVariant v = p->property("stringProp");
    QVERIFY(v.isValid());
    QCOMPARE(v.toString(), expectedValue);
}

void tst_QDBusAbstractInterface::stringPropWrite()
{
    Pinger p = getPinger();
    QVERIFY2(p, "Not connected to D-Bus");

    QString expectedValue = "This is a value";
    QVERIFY(p->setProperty("stringProp", expectedValue));
    QCOMPARE(targetObj.m_stringProp, expectedValue);
}

void tst_QDBusAbstractInterface::variantPropRead()
{
    Pinger p = getPinger();
    QVERIFY2(p, "Not connected to D-Bus");

    QDBusVariant expectedValue = targetObj.m_variantProp = QDBusVariant(QVariant(42));
    QVariant v = p->property("variantProp");
    QVERIFY(v.isValid());
    QDBusVariant value = v.value<QDBusVariant>();
    QCOMPARE(value.variant().userType(), expectedValue.variant().userType());
    QCOMPARE(value.variant(), expectedValue.variant());
}

void tst_QDBusAbstractInterface::variantPropWrite()
{
    Pinger p = getPinger();
    QVERIFY2(p, "Not connected to D-Bus");

    QDBusVariant expectedValue = QDBusVariant(Q_INT64_C(-47));
    QVERIFY(p->setProperty("variantProp", QVariant::fromValue(expectedValue)));
    QCOMPARE(targetObj.m_variantProp.variant(), expectedValue.variant());
}

void tst_QDBusAbstractInterface::complexPropRead()
{
    Pinger p = getPinger();
    QVERIFY2(p, "Not connected to D-Bus");

    RegisteredType expectedValue = targetObj.m_complexProp = RegisteredType("This is a test");
    QVariant v = p->property("complexProp");
    QVERIFY(v.userType() == qMetaTypeId<RegisteredType>());
    QCOMPARE(v.value<RegisteredType>(), targetObj.m_complexProp);
}

void tst_QDBusAbstractInterface::complexPropWrite()
{
    Pinger p = getPinger();
    QVERIFY2(p, "Not connected to D-Bus");

    RegisteredType expectedValue = RegisteredType("This is a value");
    QVERIFY(p->setProperty("complexProp", QVariant::fromValue(expectedValue)));
    QCOMPARE(targetObj.m_complexProp, expectedValue);
}

void tst_QDBusAbstractInterface::stringPropReadPeer()
{
    Pinger p = getPingerPeer();
    QVERIFY2(p, "Not connected to D-Bus");
    resetServer();

    QString expectedValue = "This is a test";
    QVariant v = p->property("stringProp");
    QVERIFY(v.isValid());
    QCOMPARE(v.toString(), expectedValue);
}

void tst_QDBusAbstractInterface::stringPropWritePeer()
{
    Pinger p = getPingerPeer();
    QVERIFY2(p, "Not connected to D-Bus");
    resetServer();

    QString expectedValue = "This is a value";
    QVERIFY(p->setProperty("stringProp", expectedValue));
    QEXPECT_FAIL("", "QTBUG-24262 peer tests are broken", Abort);
    QCOMPARE(targetObj.m_stringProp, expectedValue);
}

void tst_QDBusAbstractInterface::variantPropReadPeer()
{
    Pinger p = getPingerPeer();
    QVERIFY2(p, "Not connected to D-Bus");
    resetServer();

    QDBusVariant expectedValue = QDBusVariant(QVariant(42));
    QVariant v = p->property("variantProp");
    QVERIFY(v.isValid());
    QDBusVariant value = v.value<QDBusVariant>();
    QCOMPARE(value.variant().userType(), expectedValue.variant().userType());
    QCOMPARE(value.variant(), expectedValue.variant());
}

void tst_QDBusAbstractInterface::variantPropWritePeer()
{
    Pinger p = getPingerPeer();
    QVERIFY2(p, "Not connected to D-Bus");
    resetServer();

    QDBusVariant expectedValue = QDBusVariant(Q_INT64_C(-47));
    QVERIFY(p->setProperty("variantProp", QVariant::fromValue(expectedValue)));
    QEXPECT_FAIL("", "QTBUG-24262 peer tests are broken", Abort);
    QCOMPARE(targetObj.m_variantProp.variant(), expectedValue.variant());
}

void tst_QDBusAbstractInterface::complexPropReadPeer()
{
    Pinger p = getPingerPeer();
    QVERIFY2(p, "Not connected to D-Bus");
    resetServer();

    RegisteredType expectedValue = RegisteredType("This is a test");
    QVariant v = p->property("complexProp");
    QVERIFY(v.userType() == qMetaTypeId<RegisteredType>());
    QCOMPARE(v.value<RegisteredType>(), expectedValue);
}

void tst_QDBusAbstractInterface::complexPropWritePeer()
{
    Pinger p = getPingerPeer();
    QVERIFY2(p, "Not connected to D-Bus");
    resetServer();

    RegisteredType expectedValue = RegisteredType("This is a value");
    QVERIFY(p->setProperty("complexProp", QVariant::fromValue(expectedValue)));
    QEXPECT_FAIL("", "QTBUG-24262 peer tests are broken", Abort);
    QCOMPARE(targetObj.m_complexProp, expectedValue);
}

void tst_QDBusAbstractInterface::stringPropDirectRead()
{
    Pinger p = getPinger();
    QVERIFY2(p, "Not connected to D-Bus");

    QString expectedValue = targetObj.m_stringProp = "This is a test";
    QCOMPARE(p->stringProp(), expectedValue);
}

void tst_QDBusAbstractInterface::stringPropDirectWrite()
{
    Pinger p = getPinger();
    QVERIFY2(p, "Not connected to D-Bus");

    QString expectedValue = "This is a value";
    p->setStringProp(expectedValue);
    QCOMPARE(targetObj.m_stringProp, expectedValue);
}

void tst_QDBusAbstractInterface::variantPropDirectRead()
{
    Pinger p = getPinger();
    QVERIFY2(p, "Not connected to D-Bus");

    QDBusVariant expectedValue = targetObj.m_variantProp = QDBusVariant(42);
    QCOMPARE(p->variantProp().variant(), expectedValue.variant());
}

void tst_QDBusAbstractInterface::variantPropDirectWrite()
{
    Pinger p = getPinger();
    QVERIFY2(p, "Not connected to D-Bus");

    QDBusVariant expectedValue = QDBusVariant(Q_INT64_C(-47));
    p->setVariantProp(expectedValue);
    QCOMPARE(targetObj.m_variantProp.variant().userType(), expectedValue.variant().userType());
    QCOMPARE(targetObj.m_variantProp.variant(), expectedValue.variant());
}

void tst_QDBusAbstractInterface::complexPropDirectRead()
{
    Pinger p = getPinger();
    QVERIFY2(p, "Not connected to D-Bus");

    RegisteredType expectedValue = targetObj.m_complexProp = RegisteredType("This is a test");
    QCOMPARE(p->complexProp(), targetObj.m_complexProp);
}

void tst_QDBusAbstractInterface::complexPropDirectWrite()
{
    Pinger p = getPinger();
    QVERIFY2(p, "Not connected to D-Bus");

    RegisteredType expectedValue = RegisteredType("This is a value");
    p->setComplexProp(expectedValue);
    QCOMPARE(targetObj.m_complexProp, expectedValue);
}

void tst_QDBusAbstractInterface::stringPropDirectReadPeer()
{
    Pinger p = getPingerPeer();
    QVERIFY2(p, "Not connected to D-Bus");
    resetServer();

    QString expectedValue = "This is a test";
    QCOMPARE(p->stringProp(), expectedValue);
}

void tst_QDBusAbstractInterface::stringPropDirectWritePeer()
{
    Pinger p = getPingerPeer();
    QVERIFY2(p, "Not connected to D-Bus");
    resetServer();

    QString expectedValue = "This is a value";
    p->setStringProp(expectedValue);
    QEXPECT_FAIL("", "QTBUG-24262 peer tests are broken", Abort);
    QCOMPARE(targetObj.m_stringProp, expectedValue);
}

void tst_QDBusAbstractInterface::variantPropDirectReadPeer()
{
    Pinger p = getPingerPeer();
    QVERIFY2(p, "Not connected to D-Bus");
    resetServer();

    QDBusVariant expectedValue = QDBusVariant(42);
    QCOMPARE(p->variantProp().variant(), expectedValue.variant());
}

void tst_QDBusAbstractInterface::variantPropDirectWritePeer()
{
    Pinger p = getPingerPeer();
    QVERIFY2(p, "Not connected to D-Bus");
    resetServer();

    QDBusVariant expectedValue = QDBusVariant(Q_INT64_C(-47));
    p->setVariantProp(expectedValue);
    QEXPECT_FAIL("", "QTBUG-24262 peer tests are broken", Abort);
    QCOMPARE(targetObj.m_variantProp.variant().userType(), expectedValue.variant().userType());
    QCOMPARE(targetObj.m_variantProp.variant(), expectedValue.variant());
}

void tst_QDBusAbstractInterface::complexPropDirectReadPeer()
{
    Pinger p = getPingerPeer();
    QVERIFY2(p, "Not connected to D-Bus");
    resetServer();

    RegisteredType expectedValue = RegisteredType("This is a test");
    QCOMPARE(p->complexProp(), expectedValue);
}

void tst_QDBusAbstractInterface::complexPropDirectWritePeer()
{
    Pinger p = getPingerPeer();
    QVERIFY2(p, "Not connected to D-Bus");
    resetServer();

    RegisteredType expectedValue = RegisteredType("This is a value");
    p->setComplexProp(expectedValue);
    QEXPECT_FAIL("", "QTBUG-24262 peer tests are broken", Abort);
    QCOMPARE(targetObj.m_complexProp, expectedValue);
}

void tst_QDBusAbstractInterface::getVoidSignal_data()
{
    QTest::addColumn<QString>("service");
    QTest::addColumn<QString>("path");

    QTest::newRow("specific") << QDBusConnection::sessionBus().baseService() << "/";
    QTest::newRow("service-wildcard") << QString() << "/";
    QTest::newRow("path-wildcard") << QDBusConnection::sessionBus().baseService() << QString();
    QTest::newRow("full-wildcard") << QString() << QString();
}

void tst_QDBusAbstractInterface::getVoidSignal()
{
    QFETCH(QString, service);
    QFETCH(QString, path);
    Pinger p = getPinger(service, path);
    QVERIFY2(p, "Not connected to D-Bus");

    // we need to connect the signal somewhere in order for D-Bus to enable the rules
    QTestEventLoop::instance().connect(p.data(), SIGNAL(voidSignal()), SLOT(exitLoop()));
    QSignalSpy s(p.data(), SIGNAL(voidSignal()));

    emit targetObj.voidSignal();
    QTestEventLoop::instance().enterLoop(2);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QVERIFY(s.size() == 1);
    QVERIFY(s.at(0).size() == 0);
}

void tst_QDBusAbstractInterface::getStringSignal_data()
{
    getVoidSignal_data();
}

void tst_QDBusAbstractInterface::getStringSignal()
{
    QFETCH(QString, service);
    QFETCH(QString, path);
    Pinger p = getPinger(service, path);
    QVERIFY2(p, "Not connected to D-Bus");

    // we need to connect the signal somewhere in order for D-Bus to enable the rules
    QTestEventLoop::instance().connect(p.data(), SIGNAL(stringSignal(QString)), SLOT(exitLoop()));
    QSignalSpy s(p.data(), SIGNAL(stringSignal(QString)));

    QString expectedValue = "Good morning";
    emit targetObj.stringSignal(expectedValue);
    QTestEventLoop::instance().enterLoop(2);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QVERIFY(s.size() == 1);
    QVERIFY(s[0].size() == 1);
    QCOMPARE(s[0][0].userType(), int(QVariant::String));
    QCOMPARE(s[0][0].toString(), expectedValue);
}

void tst_QDBusAbstractInterface::getComplexSignal_data()
{
    getVoidSignal_data();
}

void tst_QDBusAbstractInterface::getComplexSignal()
{
    QFETCH(QString, service);
    QFETCH(QString, path);
    Pinger p = getPinger(service, path);
    QVERIFY2(p, "Not connected to D-Bus");

    // we need to connect the signal somewhere in order for D-Bus to enable the rules
    QTestEventLoop::instance().connect(p.data(), SIGNAL(complexSignal(RegisteredType)), SLOT(exitLoop()));
    QSignalSpy s(p.data(), SIGNAL(complexSignal(RegisteredType)));

    RegisteredType expectedValue("Good evening");
    emit targetObj.complexSignal(expectedValue);
    QTestEventLoop::instance().enterLoop(2);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QVERIFY(s.size() == 1);
    QVERIFY(s[0].size() == 1);
    QCOMPARE(s[0][0].userType(), qMetaTypeId<RegisteredType>());
    QCOMPARE(s[0][0].value<RegisteredType>(), expectedValue);
}

void tst_QDBusAbstractInterface::getVoidSignalPeer_data()
{
    QTest::addColumn<QString>("path");

    QTest::newRow("specific") << "/";
    QTest::newRow("wildcard") << QString();
}

void tst_QDBusAbstractInterface::getVoidSignalPeer()
{
    QFETCH(QString, path);
    Pinger p = getPingerPeer(path);
    QVERIFY2(p, "Not connected to D-Bus");

    // we need to connect the signal somewhere in order for D-Bus to enable the rules
    QTestEventLoop::instance().connect(p.data(), SIGNAL(voidSignal()), SLOT(exitLoop()));
    QSignalSpy s(p.data(), SIGNAL(voidSignal()));

    QDBusMessage req = QDBusMessage::createMethodCall(serviceName, objectPath, interfaceName, "voidSignal");
    QVERIFY(QDBusConnection::sessionBus().send(req));
    QTestEventLoop::instance().enterLoop(2);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QVERIFY(s.size() == 1);
    QVERIFY(s.at(0).size() == 0);
}

void tst_QDBusAbstractInterface::getStringSignalPeer_data()
{
    getVoidSignalPeer_data();
}

void tst_QDBusAbstractInterface::getStringSignalPeer()
{
    QFETCH(QString, path);
    Pinger p = getPingerPeer(path);
    QVERIFY2(p, "Not connected to D-Bus");

    // we need to connect the signal somewhere in order for D-Bus to enable the rules
    QTestEventLoop::instance().connect(p.data(), SIGNAL(stringSignal(QString)), SLOT(exitLoop()));
    QSignalSpy s(p.data(), SIGNAL(stringSignal(QString)));

    QString expectedValue = "Good morning";
    QDBusMessage req = QDBusMessage::createMethodCall(serviceName, objectPath, interfaceName, "stringSignal");
    req << expectedValue;
    QVERIFY(QDBusConnection::sessionBus().send(req));
    QTestEventLoop::instance().enterLoop(2);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QVERIFY(s.size() == 1);
    QVERIFY(s[0].size() == 1);
    QCOMPARE(s[0][0].userType(), int(QVariant::String));
    QCOMPARE(s[0][0].toString(), expectedValue);
}

void tst_QDBusAbstractInterface::getComplexSignalPeer_data()
{
    getVoidSignalPeer_data();
}

void tst_QDBusAbstractInterface::getComplexSignalPeer()
{
    QFETCH(QString, path);
    Pinger p = getPingerPeer(path);
    QVERIFY2(p, "Not connected to D-Bus");

    // we need to connect the signal somewhere in order for D-Bus to enable the rules
    QTestEventLoop::instance().connect(p.data(), SIGNAL(complexSignal(RegisteredType)), SLOT(exitLoop()));
    QSignalSpy s(p.data(), SIGNAL(complexSignal(RegisteredType)));

    RegisteredType expectedValue("Good evening");
    QDBusMessage req = QDBusMessage::createMethodCall(serviceName, objectPath, interfaceName, "complexSignal");
    req << "Good evening";
    QVERIFY(QDBusConnection::sessionBus().send(req));
    QTestEventLoop::instance().enterLoop(2);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QVERIFY(s.size() == 1);
    QVERIFY(s[0].size() == 1);
    QCOMPARE(s[0][0].userType(), qMetaTypeId<RegisteredType>());
    QCOMPARE(s[0][0].value<RegisteredType>(), expectedValue);
}

void tst_QDBusAbstractInterface::followSignal()
{
    const QString serviceToFollow = "org.qtproject.tst_qdbusabstractinterface.FollowMe";
    Pinger p = getPinger(serviceToFollow);
    QVERIFY2(p, "Not connected to D-Bus");

    QDBusConnection con = p->connection();
    QVERIFY(!con.interface()->isServiceRegistered(serviceToFollow));
    Pinger control = getPinger("");

    // connect our test signal
    // FRAGILE CODE AHEAD:
    // Connection order is important: we connect the control first because that
    // needs to be delivered last, to ensure that we don't exitLoop() before
    // the signal delivery to QSignalSpy is posted to the current thread. That
    // happens because QDBusConnectionPrivate runs in a separate thread and
    // uses a QMultiHash and insertMulti prepends to the list of items with the
    // same key.
    QTestEventLoop::instance().connect(control.data(), SIGNAL(voidSignal()), SLOT(exitLoop()));
    QSignalSpy s(p.data(), SIGNAL(voidSignal()));

    emit targetObj.voidSignal();
    QTestEventLoop::instance().enterLoop(200);
    QVERIFY(!QTestEventLoop::instance().timeout());

    // signal must not have been received because the service isn't registered
    QVERIFY(s.isEmpty());

    // now register the service
    QDBusReply<QDBusConnectionInterface::RegisterServiceReply> r =
            con.interface()->registerService(serviceToFollow, QDBusConnectionInterface::DontQueueService,
                                             QDBusConnectionInterface::DontAllowReplacement);
    QVERIFY(r.isValid() && r.value() == QDBusConnectionInterface::ServiceRegistered);
    QVERIFY(con.interface()->isServiceRegistered(serviceToFollow));
    QCoreApplication::instance()->processEvents();

    // emit the signal again:
    emit targetObj.voidSignal();
    QTestEventLoop::instance().enterLoop(2);
    QVERIFY(!QTestEventLoop::instance().timeout());

    // now the signal must have been received:
    QCOMPARE(s.size(), 1);
    QVERIFY(s.at(0).size() == 0);

    // cleanup:
    con.interface()->unregisterService(serviceToFollow);
}

void tst_QDBusAbstractInterface::connectDisconnect_data()
{
    QTest::addColumn<int>("connectCount");
    QTest::addColumn<int>("disconnectCount");

    // we don't actually need multiple disconnects
    // QObject::disconnect() disconnects all matching rules
    // we'd have to use QMetaObject::disconnectOne if we wanted just one
    QTest::newRow("null") << 0 << 0;
    QTest::newRow("connect-disconnect") << 1 << 1;
    QTest::newRow("connect-disconnect-wildcard") << 1 << -1;
    QTest::newRow("connect-twice") << 2 << 0;
    QTest::newRow("connect-twice-disconnect") << 2 << 1;
    QTest::newRow("connect-twice-disconnect-wildcard") << 2 << -1;
}

void tst_QDBusAbstractInterface::connectDisconnect()
{
    QFETCH(int, connectCount);
    QFETCH(int, disconnectCount);

    Pinger p = getPinger();
    QVERIFY2(p, "Not connected to D-Bus");

    // connect the exitLoop slot first
    // if the disconnect() below does something weird, we'll get a timeout
    QTestEventLoop::instance().connect(p.data(), SIGNAL(voidSignal()), SLOT(exitLoop()));

    SignalReceiver sr;
    for (int i = 0; i < connectCount; ++i)
        sr.connect(p.data(), SIGNAL(voidSignal()), SLOT(receive()));
    if (disconnectCount)
        QObject::disconnect(p.data(), disconnectCount > 0 ? SIGNAL(voidSignal()) : 0, &sr, SLOT(receive()));

    emit targetObj.voidSignal();
    QTestEventLoop::instance().enterLoop(2);
    QVERIFY(!QTestEventLoop::instance().timeout());

    if (disconnectCount != 0)
        QCOMPARE(sr.callCount, 0);
    else
        QCOMPARE(sr.callCount, connectCount);
}

void tst_QDBusAbstractInterface::connectDisconnectPeer_data()
{
    connectDisconnect_data();
}

void tst_QDBusAbstractInterface::connectDisconnectPeer()
{
    QFETCH(int, connectCount);
    QFETCH(int, disconnectCount);

    Pinger p = getPingerPeer();
    QVERIFY2(p, "Not connected to D-Bus");

    // connect the exitLoop slot first
    // if the disconnect() below does something weird, we'll get a timeout
    QTestEventLoop::instance().connect(p.data(), SIGNAL(voidSignal()), SLOT(exitLoop()));

    SignalReceiver sr;
    for (int i = 0; i < connectCount; ++i)
        sr.connect(p.data(), SIGNAL(voidSignal()), SLOT(receive()));
    if (disconnectCount)
        QObject::disconnect(p.data(), disconnectCount > 0 ? SIGNAL(voidSignal()) : 0, &sr, SLOT(receive()));

    QDBusMessage req = QDBusMessage::createMethodCall(serviceName, objectPath, interfaceName, "voidSignal");
    QVERIFY(QDBusConnection::sessionBus().send(req));
    QTestEventLoop::instance().enterLoop(2);
    QVERIFY(!QTestEventLoop::instance().timeout());

    if (disconnectCount != 0)
        QCOMPARE(sr.callCount, 0);
    else
        QCOMPARE(sr.callCount, connectCount);
}

void tst_QDBusAbstractInterface::createErrors_data()
{
    QTest::addColumn<QString>("service");
    QTest::addColumn<QString>("path");
    QTest::addColumn<QString>("errorName");

    QTest::newRow("invalid-service") << "this isn't valid" << "/" << "org.qtproject.QtDBus.Error.InvalidService";
    QTest::newRow("invalid-path") << QDBusConnection::sessionBus().baseService() << "this isn't valid"
            << "org.qtproject.QtDBus.Error.InvalidObjectPath";
}

void tst_QDBusAbstractInterface::createErrors()
{
    QFETCH(QString, service);
    QFETCH(QString, path);
    Pinger p = getPinger(service, path);
    QVERIFY2(p, "Not connected to D-Bus");

    QVERIFY(!p->isValid());
    QTEST(p->lastError().name(), "errorName");
}

void tst_QDBusAbstractInterface::createErrorsPeer_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<QString>("errorName");

    QTest::newRow("invalid-path") << "this isn't valid" << "org.qtproject.QtDBus.Error.InvalidObjectPath";
}

void tst_QDBusAbstractInterface::createErrorsPeer()
{
    QFETCH(QString, path);
    Pinger p = getPingerPeer(path);
    QVERIFY2(p, "Not connected to D-Bus");

    QVERIFY(!p->isValid());
    QTEST(p->lastError().name(), "errorName");
}

void tst_QDBusAbstractInterface::callErrors_data()
{
    createErrors_data();
    QTest::newRow("service-wildcard") << QString() << "/" << "org.qtproject.QtDBus.Error.InvalidService";
    QTest::newRow("path-wildcard") << QDBusConnection::sessionBus().baseService() << QString()
            << "org.qtproject.QtDBus.Error.InvalidObjectPath";
    QTest::newRow("full-wildcard") << QString() << QString() << "org.qtproject.QtDBus.Error.InvalidService";
}

void tst_QDBusAbstractInterface::callErrors()
{
    QFETCH(QString, service);
    QFETCH(QString, path);
    Pinger p = getPinger(service, path);
    QVERIFY2(p, "Not connected to D-Bus");

    // we shouldn't be able to make this call:
    QDBusReply<QString> r = p->stringMethod();
    QVERIFY(!r.isValid());
    QTEST(r.error().name(), "errorName");
    QCOMPARE(p->lastError().name(), r.error().name());
}

void tst_QDBusAbstractInterface::asyncCallErrors_data()
{
    callErrors_data();
}

void tst_QDBusAbstractInterface::asyncCallErrors()
{
    QFETCH(QString, service);
    QFETCH(QString, path);
    Pinger p = getPinger(service, path);
    QVERIFY2(p, "Not connected to D-Bus");

    // we shouldn't be able to make this call:
    QDBusPendingReply<QString> r = p->stringMethod();
    QVERIFY(r.isError());
    QTEST(r.error().name(), "errorName");
    QCOMPARE(p->lastError().name(), r.error().name());
}

void tst_QDBusAbstractInterface::callErrorsPeer_data()
{
    createErrorsPeer_data();
    QTest::newRow("path-wildcard") << QString() << "org.qtproject.QtDBus.Error.InvalidObjectPath";
}

void tst_QDBusAbstractInterface::callErrorsPeer()
{
    QFETCH(QString, path);
    Pinger p = getPingerPeer(path);
    QVERIFY2(p, "Not connected to D-Bus");

    // we shouldn't be able to make this call:
    QDBusReply<QString> r = p->stringMethod();
    QVERIFY(!r.isValid());
    QTEST(r.error().name(), "errorName");
    QCOMPARE(p->lastError().name(), r.error().name());
}

void tst_QDBusAbstractInterface::asyncCallErrorsPeer_data()
{
    callErrorsPeer_data();
}

void tst_QDBusAbstractInterface::asyncCallErrorsPeer()
{
    QFETCH(QString, path);
    Pinger p = getPingerPeer(path);
    QVERIFY2(p, "Not connected to D-Bus");

    // we shouldn't be able to make this call:
    QDBusPendingReply<QString> r = p->stringMethod();
    QVERIFY(r.isError());
    QTEST(r.error().name(), "errorName");
    QCOMPARE(p->lastError().name(), r.error().name());
}

void tst_QDBusAbstractInterface::propertyReadErrors_data()
{
    callErrors_data();
}

void tst_QDBusAbstractInterface::propertyReadErrors()
{
    QFETCH(QString, service);
    QFETCH(QString, path);
    Pinger p = getPinger(service, path);
    QVERIFY2(p, "Not connected to D-Bus");

    // we shouldn't be able to get this value:
    QVariant v = p->property("stringProp");
    QVERIFY(v.isNull());
    QVERIFY(!v.isValid());
    QTEST(p->lastError().name(), "errorName");
}

void tst_QDBusAbstractInterface::propertyWriteErrors_data()
{
    callErrors_data();
}

void tst_QDBusAbstractInterface::propertyWriteErrors()
{
    QFETCH(QString, service);
    QFETCH(QString, path);
    Pinger p = getPinger(service, path);
    QVERIFY2(p, "Not connected to D-Bus");

    // we shouldn't be able to get this value:
    if (p->isValid())
        QCOMPARE(int(p->lastError().type()), int(QDBusError::NoError));
    QVERIFY(!p->setProperty("stringProp", ""));
    QTEST(p->lastError().name(), "errorName");
}

void tst_QDBusAbstractInterface::directPropertyReadErrors_data()
{
    callErrors_data();
}

void tst_QDBusAbstractInterface::directPropertyReadErrors()
{
    QFETCH(QString, service);
    QFETCH(QString, path);
    Pinger p = getPinger(service, path);
    QVERIFY2(p, "Not connected to D-Bus");

    // we shouldn't be able to get this value:
    QString v = p->stringProp();
    QVERIFY(v.isNull());
    QTEST(p->lastError().name(), "errorName");
}

void tst_QDBusAbstractInterface::directPropertyWriteErrors_data()
{
    callErrors_data();
}

void tst_QDBusAbstractInterface::directPropertyWriteErrors()
{
    QFETCH(QString, service);
    QFETCH(QString, path);
    Pinger p = getPinger(service, path);
    QVERIFY2(p, "Not connected to D-Bus");

    // we shouldn't be able to get this value:
    // but there's no direct way of verifying that the setting failed
    if (p->isValid())
        QCOMPARE(int(p->lastError().type()), int(QDBusError::NoError));
    p->setStringProp("");
    QTEST(p->lastError().name(), "errorName");
}

void tst_QDBusAbstractInterface::propertyReadErrorsPeer_data()
{
    callErrorsPeer_data();
}

void tst_QDBusAbstractInterface::propertyReadErrorsPeer()
{
    QFETCH(QString, path);
    Pinger p = getPingerPeer(path);
    QVERIFY2(p, "Not connected to D-Bus");

    // we shouldn't be able to get this value:
    QVariant v = p->property("stringProp");
    QVERIFY(v.isNull());
    QVERIFY(!v.isValid());
    QTEST(p->lastError().name(), "errorName");
}

void tst_QDBusAbstractInterface::propertyWriteErrorsPeer_data()
{
    callErrorsPeer_data();
}

void tst_QDBusAbstractInterface::propertyWriteErrorsPeer()
{
    QFETCH(QString, path);
    Pinger p = getPingerPeer(path);
    QVERIFY2(p, "Not connected to D-Bus");

    // we shouldn't be able to get this value:
    if (p->isValid())
        QCOMPARE(int(p->lastError().type()), int(QDBusError::NoError));
    QVERIFY(!p->setProperty("stringProp", ""));
    QTEST(p->lastError().name(), "errorName");
}

void tst_QDBusAbstractInterface::directPropertyReadErrorsPeer_data()
{
    callErrorsPeer_data();
}

void tst_QDBusAbstractInterface::directPropertyReadErrorsPeer()
{
    QFETCH(QString, path);
    Pinger p = getPingerPeer(path);
    QVERIFY2(p, "Not connected to D-Bus");

    // we shouldn't be able to get this value:
    QString v = p->stringProp();
    QVERIFY(v.isNull());
    QTEST(p->lastError().name(), "errorName");
}

void tst_QDBusAbstractInterface::directPropertyWriteErrorsPeer_data()
{
    callErrorsPeer_data();
}

void tst_QDBusAbstractInterface::directPropertyWriteErrorsPeer()
{
    QFETCH(QString, path);
    Pinger p = getPingerPeer(path);
    QVERIFY2(p, "Not connected to D-Bus");

    // we shouldn't be able to get this value:
    // but there's no direct way of verifying that the setting failed
    if (p->isValid())
        QCOMPARE(int(p->lastError().type()), int(QDBusError::NoError));
    p->setStringProp("");
    QTEST(p->lastError().name(), "errorName");
}

void tst_QDBusAbstractInterface::validity_data()
{
    QTest::addColumn<QString>("service");

    QTest::newRow("null-service") << "";
    QTest::newRow("ignored-service") << "org.example.anyservice";
}

void tst_QDBusAbstractInterface::validity()
{
    /* Test case for QTBUG-32374 */
    QFETCH(QString, service);
    Pinger p = getPingerPeer("/", service);
    QVERIFY2(p, "Not connected to D-Bus");

    QVERIFY(p->isValid());
}

QTEST_MAIN(tst_QDBusAbstractInterface)
#include "tst_qdbusabstractinterface.moc"
