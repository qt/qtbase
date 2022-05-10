// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtCore>


static QVariant callMyDBusFunction() { return QVariant(); }

int main()
{
    QDBusMessage myDBusMessage;

//! [0]
    QList<QVariant> arguments;
    arguments << QVariant(42) << QVariant::fromValue(QDBusVariant(43)) << QVariant("hello");
    myDBusMessage.setArguments(arguments);
//! [0]

//! [1]
    // call a D-Bus function that returns a D-Bus variant
    QVariant v = callMyDBusFunction();
    // retrieve the D-Bus variant
    QDBusVariant dbusVariant = qvariant_cast<QDBusVariant>(v);
    // retrieve the actual value stored in the D-Bus variant
    QVariant result = dbusVariant.variant();
//! [1]

    return 0;
}
