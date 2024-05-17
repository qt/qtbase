// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <qcoreapplication.h>
#include <qdebug.h>

#include <QTest>


#include <stdlib.h>

/* This test uses an appless main, to ensure that no D-Bus stuff is implicitly done
   It also sets the magic "QT_SIMULATE_DBUS_LIBFAIL" env variable, that is only available
   in developer builds. That env variable simulates a D-Bus library load fail.

   In no case should the QDBus module crash because D-Bus libs couldn't be loaded */

class tst_QDBusConnectionNoBus : public QObject
{
    Q_OBJECT

public:
    tst_QDBusConnectionNoBus()
    {
        qputenv("DBUS_SESSION_BUS_ADDRESS", "unix:abstract=/tmp/does_not_exist");
#ifdef SIMULATE_LOAD_FAIL
        qputenv("QT_SIMULATE_DBUS_LIBFAIL", "1");
#endif
    }

private slots:
    void connectToBus();
};


void tst_QDBusConnectionNoBus::connectToBus()
{
    int argc = 0;
    QCoreApplication app(argc, 0);

    QDBusConnection con = QDBusConnection::sessionBus();

    QVERIFY(!con.isConnected()); // if we didn't crash here, the test passed :)
}

QTEST_APPLESS_MAIN(tst_QDBusConnectionNoBus)

#include "tst_qdbusconnection_no_libdbus.moc"

