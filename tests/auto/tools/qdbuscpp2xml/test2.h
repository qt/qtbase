// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#ifndef TEST2_H
#define TEST2_H

#include <QObject>
#include <QtDBus/QtDBus>

// Regression test for QTBUG-34550
class Test2 : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "net.company.object")
    Q_PROPERTY(QDBusObjectPath objectProperty READ objectProperty)
    Q_PROPERTY(QList<QDBusObjectPath> objectPropertyList READ objectPropertyList)
public:
    Test2(QObject *parent = nullptr) : QObject(parent) { }

    QDBusObjectPath objectProperty() { return {}; }

    QList<QDBusObjectPath> objectPropertyList() { return {}; }
};

#endif // TEST2_H
