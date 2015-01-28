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
#include <QtCore/QtCore>
#include <QtDBus/QtDBus>

#include "../myobject.h"

static const char serviceName[] = "org.qtproject.autotests.qmyserver";
static const char objectPath[] = "/org/qtproject/qmyserver";
//static const char *interfaceName = serviceName;

const char *slotSpy;
QString valueSpy;

Q_DECLARE_METATYPE(QDBusConnection::RegisterOptions)

class MyServer : public QDBusServer, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.qtproject.autotests.qmyserver")

public:
    MyServer(QObject* parent = 0)
        : QDBusServer(parent),
          m_conn("none"),
          obj(NULL)
    {
        connect(this, SIGNAL(newConnection(QDBusConnection)), SLOT(handleConnection(QDBusConnection)));
    }

    ~MyServer()
    {
        if (obj)
            obj->deleteLater();
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

    Q_NOREPLY void requestSync(const QString &seq)
    {
        emit syncReceived(seq);
    }

    void emitSignal(const QString& interface, const QString& name, const QDBusVariant& parameter)
    {
        if (interface.endsWith('2'))
            obj->if2->emitSignal(name, parameter.variant());
        else if (interface.endsWith('3'))
            obj->if3->emitSignal(name, parameter.variant());
        else if (interface.endsWith('4'))
            obj->if4->emitSignal(name, parameter.variant());
        else
            obj->emitSignal(name, parameter.variant());
    }

    void emitSignal2(const QString& interface, const QString& name)
    {
        if (interface.endsWith('2'))
            obj->if2->emitSignal(name, QVariant());
        else if (interface.endsWith('3'))
            obj->if3->emitSignal(name, QVariant());
        else if (interface.endsWith('4'))
            obj->if4->emitSignal(name, QVariant());
        else
            obj->emitSignal(name, QVariant());
    }

    void newMyObject(int nInterfaces = 4)
    {
        if (obj)
            obj->deleteLater();
        obj = new MyObject(nInterfaces);
    }

    void registerMyObject(const QString & path, int options)
    {
        m_conn.registerObject(path, obj, (QDBusConnection::RegisterOptions)options);
    }

    QString slotSpyServer()
    {
        return QLatin1String(slotSpy);
    }

    QString valueSpyServer()
    {
        return valueSpy;
    }

    void clearValueSpy()
    {
        valueSpy.clear();
    }

    void quit()
    {
        qApp->quit();
    }

signals:
    Q_SCRIPTABLE void syncReceived(const QString &sequence);

private slots:
    void handleConnection(QDBusConnection con)
    {
        m_conn = con;
        con.registerObject(objectPath, this, QDBusConnection::ExportScriptableSignals);

        if (callPendingReply.type() != QDBusMessage::InvalidMessage) {
            QDBusConnection::sessionBus().send(callPendingReply.createReply());
            callPendingReply = QDBusMessage();
        }
    }

private:
    QDBusConnection m_conn;
    QDBusMessage callPendingReply;
    MyObject* obj;
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QDBusConnection con = QDBusConnection::sessionBus();
    if (!con.isConnected())
        exit(1);

    if (!con.registerService(serviceName))
        exit(2);

    MyServer server;
    con.registerObject(objectPath, &server, QDBusConnection::ExportAllSlots);

    printf("ready.\n");
    fflush(stdout);

    return app.exec();
}

#include "qmyserver.moc"
