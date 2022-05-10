// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <QDBusContext>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>

const char errorName[] = "org.qtproject.tst_QDBusContext.Error";
const char errorMsg[] = "A generic error";

class TestObject: public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.qtproject.tst_QDBusContext.TestObject")
public:
    inline TestObject(QObject *parent) : QObject(parent) { }
public Q_SLOTS:
    void generateError();
};

class tst_QDBusContext: public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void sendErrorReply();
};

void TestObject::generateError()
{
    sendErrorReply(errorName, errorMsg);
}

void tst_QDBusContext::initTestCase()
{
    TestObject *obj = new TestObject(this);
    QVERIFY(QDBusConnection::sessionBus().isConnected());
    QVERIFY(QDBusConnection::sessionBus().registerObject("/TestObject", obj,
                                                          QDBusConnection::ExportAllSlots));
}

void tst_QDBusContext::sendErrorReply()
{
    QDBusInterface iface(QDBusConnection::sessionBus().baseService(), "/TestObject");
    QVERIFY(iface.isValid());

    QDBusReply<void> reply = iface.call("generateError");
    QVERIFY(!reply.isValid());

    const QDBusError &error = reply.error();
    QCOMPARE(error.name(), QString::fromLatin1(errorName));
    QCOMPARE(error.message(), QString::fromLatin1(errorMsg));
}

QTEST_MAIN(tst_QDBusContext)

#include "tst_qdbuscontext.moc"
