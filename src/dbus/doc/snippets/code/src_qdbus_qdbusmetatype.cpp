// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtCore/qlist.h>
typedef QList<QString> MyClass;

//! [0-0]
#include <QDBusMetaType>
//! [0-0]
void dbus() {
//! [0-1]
qDBusRegisterMetaType<MyClass>();
//! [0-1]
}
