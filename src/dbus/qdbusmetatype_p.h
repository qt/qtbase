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

#ifndef QDBUSMETATYPE_P_H
#define QDBUSMETATYPE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QLibrary class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtDBus/private/qtdbusglobal_p.h>
#include <qdbusmetatype.h>

#include <qdbusmessage.h>
#include <qdbusargument.h>
#include <qdbusextratypes.h>
#include <qdbuserror.h>
#include <qdbusunixfiledescriptor.h>

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

struct QDBusMetaTypeId
{
    static int message();         // QDBusMessage
    static int argument();        // QDBusArgument
    static int variant();         // QDBusVariant
    static int objectpath();      // QDBusObjectPath
    static int signature();       // QDBusSignature
    static int error();           // QDBusError
    static int unixfd();          // QDBusUnixFileDescriptor

    static void init();
};

inline int QDBusMetaTypeId::message()
#ifdef QT_BOOTSTRAPPED
{ return qDBusRegisterMetaType<QList<QDBusUnixFileDescriptor> >() + 1; }
#else
{ return qMetaTypeId<QDBusMessage>(); }
#endif

inline int QDBusMetaTypeId::argument()
{ return qMetaTypeId<QDBusArgument>(); }

inline int QDBusMetaTypeId::variant()
{ return qMetaTypeId<QDBusVariant>(); }

inline int QDBusMetaTypeId::objectpath()
{ return qMetaTypeId<QDBusObjectPath>(); }

inline int QDBusMetaTypeId::signature()
{ return qMetaTypeId<QDBusSignature>(); }

inline int QDBusMetaTypeId::error()
#ifdef QT_BOOTSTRAPPED
{ return qDBusRegisterMetaType<QList<QDBusUnixFileDescriptor> >() + 2; }
#else
{ return qMetaTypeId<QDBusError>(); }
#endif

inline int QDBusMetaTypeId::unixfd()
{ return qMetaTypeId<QDBusUnixFileDescriptor>(); }

QT_END_NAMESPACE

#endif // QT_NO_DBUS
#endif
