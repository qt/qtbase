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
#include <QtCore/QtCore>
#include <QtDBus/QtDBus>
#include "../interface.h"

static const char serviceName[] = "org.qtproject.autotests.qpinger";
static const char objectPath[] = "/org/qtproject/qpinger";
//static const char *interfaceName = serviceName;

class PingerServer : public QDBusServer, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.qtproject.autotests.qpinger")
public:
    PingerServer(QObject* parent = 0)
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
