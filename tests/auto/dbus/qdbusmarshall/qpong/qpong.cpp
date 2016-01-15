/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

static const char serviceName[] = "org.qtproject.autotests.qpong";
static const char objectPath[] = "/org/qtproject/qpong";
//static const char *interfaceName = serviceName;

class Pong: public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.qtproject.autotests.qpong")
public slots:

    void ping(QDBusMessage msg)
    {
        msg.setDelayedReply(true);
        if (!QDBusConnection::sessionBus().send(msg.createReply(msg.arguments())))
            exit(1);
    }

    void quit()
    {
        qApp->quit();
    }
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QDBusConnection con = QDBusConnection::sessionBus();
    if (!con.isConnected())
        exit(1);

    if (!con.registerService(serviceName))
        exit(2);

    Pong pong;
    con.registerObject(objectPath, &pong, QDBusConnection::ExportAllSlots);

    printf("ready.\n");
    fflush(stdout);

    return app.exec();
}

#include "qpong.moc"
