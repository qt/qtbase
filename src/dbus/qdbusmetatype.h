/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtDBus module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QDBUSMETATYPE_H
#define QDBUSMETATYPE_H

#include <QtDBus/qtdbusglobal.h>
#include "QtCore/qmetatype.h"
#include <QtDBus/qdbusargument.h>

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE


class Q_DBUS_EXPORT QDBusMetaType
{
public:
    typedef void (*MarshallFunction)(QDBusArgument &, const void *);
    typedef void (*DemarshallFunction)(const QDBusArgument &, void *);

    static void registerMarshallOperators(int typeId, MarshallFunction, DemarshallFunction);
    static bool marshall(QDBusArgument &, int id, const void *data);
    static bool demarshall(const QDBusArgument &, int id, void *data);

    static int signatureToType(const char *signature);
    static const char *typeToSignature(int type);
};

template<typename T>
void qDBusMarshallHelper(QDBusArgument &arg, const T *t)
{ arg << *t; }

template<typename T>
void qDBusDemarshallHelper(const QDBusArgument &arg, T *t)
{ arg >> *t; }

template<typename T>
int qDBusRegisterMetaType(
#ifndef Q_QDOC
    T * /* dummy */ = nullptr
#endif
)
{
    void (*mf)(QDBusArgument &, const T *) = qDBusMarshallHelper<T>;
    void (*df)(const QDBusArgument &, T *) = qDBusDemarshallHelper<T>;

    int id = qMetaTypeId<T>(); // make sure it's registered
    QDBusMetaType::registerMarshallOperators(id,
        reinterpret_cast<QDBusMetaType::MarshallFunction>(mf),
        reinterpret_cast<QDBusMetaType::DemarshallFunction>(df));
    return id;
}

QT_END_NAMESPACE

#endif // QT_NO_DBUS
#endif
