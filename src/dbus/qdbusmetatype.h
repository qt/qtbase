/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

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

    static void registerMarshallOperators(QMetaType typeId, MarshallFunction, DemarshallFunction);
    static bool marshall(QDBusArgument &, QMetaType id, const void *data);
    static bool demarshall(const QDBusArgument &, QMetaType id, void *data);

    static QMetaType signatureToMetaType(const char *signature);
    static const char *typeToSignature(QMetaType type);
};

template<typename T>
QMetaType qDBusRegisterMetaType()
{
    auto mf = [](QDBusArgument &arg, const void *t) { arg << *static_cast<const T *>(t); };
    auto df = [](const QDBusArgument &arg, void *t) { arg >> *static_cast<T *>(t); };

    QMetaType metaType = QMetaType::fromType<T>();
    QDBusMetaType::registerMarshallOperators(metaType, mf, df);
    return metaType;
}

QT_END_NAMESPACE

#endif // QT_NO_DBUS
#endif
