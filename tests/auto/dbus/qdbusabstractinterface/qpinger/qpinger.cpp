// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QCoreApplication>

#include <QDBusServer>
#include <QDBusContext>
#include <QDBusMetaType>
#include <QDBusConnection>
#include <QDBusMessage>

#include "../interface.h"

static const char serviceName[] = "org.qtproject.autotests.qpinger";
static const char objectPath[] = "/org/qtproject/qpinger";
//static const char *interfaceName = serviceName;

class PingerServer : public QDBusServer, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.qtproject.autotests.qpinger")
public:
    PingerServer(QObject *parent = nullptr)
        : QDBusServer(parent),
          m_conn("none")
    {
        connect(this, SIGNAL(newConnection(QDBusConnection)), SLOT(handleConnection(QDBusConnection)));
        reset();
    }

public slots:
    QString address() const
    {
        if (!QDBusServer::isConnected())
            sendErrorReply(QDBusServer::lastError().name(), QDBusServer::lastError().message());
        return QDBusServer::address();
    }

    void waitForConnected()
    {
        if (callPendingReply.type() != QDBusMessage::InvalidMessage) {
            sendErrorReply(QDBusError::NotSupported, "One call already pending!");
            return;
        }
        if (m_conn.isConnected())
            return;
        // not connected, we'll reply later
        setDelayedReply(true);
        callPendingReply = message();
    }

    void reset()
    {
        targetObj.m_stringProp = "This is a test";
        targetObj.m_variantProp = QDBusVariant(QVariant(42));
        targetObj.m_complexProp = RegisteredType("This is a test");
    }

    void voidSignal()
    {
        emit targetObj.voidSignal();
    }

    void stringSignal(const QString& value)
    {
        emit targetObj.stringSignal(value);
    }

    void complexSignal(const QString& value)
    {
        RegisteredType reg(value);
        emit targetObj.complexSignal(reg);
    }

    void quit()
    {
        qApp->quit();
    }

private slots:
    void handleConnection(const QDBusConnection& con)
    {
        m_conn = con;
        m_conn.registerObject("/", &targetObj, QDBusConnection::ExportScriptableContents);
        if (callPendingReply.type() != QDBusMessage::InvalidMessage) {
            QDBusConnection::sessionBus().send(callPendingReply.createReply());
            callPendingReply = QDBusMessage();
        }
    }

private:
    Interface targetObj;
    QDBusConnection m_conn;
    QDBusMessage callPendingReply;
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // register the meta types
    qDBusRegisterMetaType<RegisteredType>();
    qRegisterMetaType<UnregisteredType>();

    QDBusConnection con = QDBusConnection::sessionBus();
    if (!con.isConnected())
        exit(1);

    if (!con.registerService(serviceName))
        exit(2);

    PingerServer server;
    con.registerObject(objectPath, &server, QDBusConnection::ExportAllSlots);

    printf("ready.\n");
    fflush(stdout);

    return app.exec();
}

#include "qpinger.moc"
