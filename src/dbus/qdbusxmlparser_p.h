// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QDBUSXMLPARSER_P_H
#define QDBUSXMLPARSER_P_H

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
#include <QtCore/qloggingcategory.h>
#include <QtCore/qmap.h>
#include "qdbusintrospection_p.h"

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(dbusParser)

/*!
    \internal
*/
class QDBusXmlParser
{
    QString m_service;
    QString m_path;
    QSharedDataPointer<QDBusIntrospection::Object> m_object;
    QDBusIntrospection::Interfaces m_interfaces;

public:
    QDBusXmlParser(const QString& service, const QString& path,
                   const QString& xmlData);

    inline QDBusIntrospection::Interfaces interfaces() const { return m_interfaces; }
    inline QSharedDataPointer<QDBusIntrospection::Object> object() const { return m_object; }
};

QT_END_NAMESPACE

#endif // QT_NO_DBUS
#endif
