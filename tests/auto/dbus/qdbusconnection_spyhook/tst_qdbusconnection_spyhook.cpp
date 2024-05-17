// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
