/****************************************************************************
**
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

#include <QtDBus/QDBusMessage>

#define HAS_HOOKSETUPFUNCTION       1
static void hookSetupFunction();

// Ugly hack, look away
#include "../qdbusconnection/tst_qdbusconnection.cpp"

QT_BEGIN_NAMESPACE
extern Q_DBUS_EXPORT void qDBusAddSpyHook(void (*Hook)(const QDBusMessage&));
QT_END_NAMESPACE

static void hookFunction(const QDBusMessage &)
{
//    qDebug() << "hook called";
    ++tst_QDBusConnection::hookCallCount;
}

static void hookSetupFunction()
{
    QT_PREPEND_NAMESPACE(qDBusAddSpyHook)(hookFunction);
}

QTEST_MAIN(tst_QDBusConnection_SpyHook)
