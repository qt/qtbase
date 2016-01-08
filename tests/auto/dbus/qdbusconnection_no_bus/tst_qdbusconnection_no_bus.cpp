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

#include <QtTest/QtTest>
#include <QtDBus/QtDBus>

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

#include "tst_qdbusconnection_no_bus.moc"

