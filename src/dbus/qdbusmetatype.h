/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtDBus module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
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
