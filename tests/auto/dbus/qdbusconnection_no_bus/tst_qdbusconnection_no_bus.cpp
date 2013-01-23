/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
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
        ::setenv("DBUS_SESSION_BUS_ADDRESS", "unix:abstract=/tmp/does_not_exist", 1);
        ::setenv("QT_SIMULATE_DBUS_LIBFAIL", "1", 1);
    }

private slots:
    void connectToBus();
};


void tst_QDBusConnectionNoBus::connectToBus()
{
    int argc = 0;
    QCoreApplication app(argc, 0);

    QDBusConnection con = QDBusConnection::sessionBus();

    QVERIFY(true); // if we didn't crash here, the test passed :)
}

QTEST_APPLESS_MAIN(tst_QDBusConnectionNoBus)

#include "tst_qdbusconnection_no_bus.moc"

