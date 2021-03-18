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

#include <string.h>

#ifndef QT_BOOTSTRAPPED
#include <QtCore/qcoreapplication.h>
#include <QtCore/qvariant.h>
#include <QtCore/qmetaobject.h>

#include "qdbusutil_p.h"
#include "qdbusconnection_p.h"
#include "qdbusabstractadaptor_p.h" // for QCLASSINFO_DBUS_*
#endif
#include <QtCore/qvector.h>
#include "qdbusmetatype_p.h"

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

bool qDBusCheckAsyncTag(const char *tag)
{
    static const char noReplyTag[] = "Q_NOREPLY";
    if (!tag || !*tag)
        return false;

    const char *p = strstr(tag, noReplyTag);
    if (p != nullptr &&
        (p == tag || *(p-1) == ' ') &&
        (p[sizeof noReplyTag - 1] == '\0' || p[sizeof noReplyTag - 1] == ' '))
        return true;

    return false;
}

#ifndef QT_BOOTSTRAPPED

QString qDBusInterfaceFromMetaObject(const QMetaObject *mo)
{
    QString interface;

    int idx = mo->indexOfClassInfo(QCLASSINFO_DBUS_INTERFACE);
    if (idx >= mo->classInfoOffset()) {
        interface = QLatin1String(mo->classInfo(idx).value());
    } else {
        interface = QLatin1String(mo->className());
        interface.replace(QLatin1String("::"), QLatin1String("."));

        if (interface.startsWith(QLatin1String("QDBus"))) {
            interface.prepend(QLatin1String("org.qtproject.QtDBus."));
        } else if (interface.startsWith(QLatin1Char('Q')) &&
                   interface.length() >= 2 && interface.at(1).isUpper()) {
            // assume it's Qt
            interface.prepend(QLatin1String("org.qtproject.Qt."));
        } else if (!QCoreApplication::instance()||
                   QCoreApplication::instance()->applicationName().isEmpty()) {
            interface.prepend(QLatin1String("local."));
         } else {
            interface.prepend(QLatin1Char('.')).prepend(QCoreApplication::instance()->applicationName());
            const QString organizationDomain = QCoreApplication::instance()->organizationDomain();
            const auto domainName = organizationDomain.splitRef(QLatin1Char('.'), Qt::SkipEmptyParts);
            if (domainName.isEmpty()) {
                 interface.prepend(QLatin1String("local."));
            } else {
                QString composedDomain;
                // + 1 for additional dot, e.g. organizationDomain equals "example.com",
                // then composedDomain will be equal "com.example."
                composedDomain.reserve(organizationDomain.size() + 1);
                for (auto it = domainName.rbegin(), end = domainName.rend(); it != end; ++it)
                    composedDomain += *it + QLatin1Char('.');

                interface.prepend(composedDomain);
            }
         }
     }

    return interface;
}

bool qDBusInterfaceInObject(QObject *obj, const QString &interface_name)
{
    const QMetaObject *mo = obj->metaObject();
    for ( ; mo != &QObject::staticMetaObject; mo = mo->superClass())
        if (interface_name == qDBusInterfaceFromMetaObject(mo))
            return true;
    return false;
}

// calculates the metatypes for the method
// the slot must have the parameters in the following form:
//  - zero or more value or const-ref parameters of any kind
//  - zero or one const ref of QDBusMessage
//  - zero or more non-const ref parameters
// No parameter may be a template.
// this function returns -1 if the parameters don't match the above form
// this function returns the number of *input* parameters, including the QDBusMessage one if any
// this function does not check the return type, so metaTypes[0] is always 0 and always present
// metaTypes.count() >= retval + 1 in all cases
//
// sig must be the normalised signature for the method
int qDBusParametersForMethod(const QMetaMethod &mm, QVector<int> &metaTypes, QString &errorMsg)
{
    return qDBusParametersForMethod(mm.parameterTypes(), metaTypes, errorMsg);
}

#endif // QT_BOOTSTRAPPED

int qDBusParametersForMethod(const QList<QByteArray> &parameterTypes, QVector<int>& metaTypes, QString &errorMsg)
{
    QDBusMetaTypeId::init();
    metaTypes.clear();

    metaTypes.append(0);        // return type
    int inputCount = 0;
    bool seenMessage = false;
    QList<QByteArray>::ConstIterator it = parameterTypes.constBegin();
    QList<QByteArray>::ConstIterator end = parameterTypes.constEnd();
    for ( ; it != end; ++it) {
        const QByteArray &type = *it;
        if (type.endsWith('*')) {
            errorMsg = QLatin1String("Pointers are not supported: ") + QLatin1String(type);
            return -1;
        }

        if (type.endsWith('&')) {
            QByteArray basictype = type;
            basictype.truncate(type.length() - 1);

            int id = QMetaType::type(basictype);
            if (id == 0) {
                errorMsg = QLatin1String("Unregistered output type in parameter list: ") + QLatin1String(type);
                return -1;
            } else if (QDBusMetaType::typeToSignature(id) == nullptr)
                return -1;

            metaTypes.append( id );
            seenMessage = true; // it cannot appear anymore anyways
            continue;
        }

        if (seenMessage) {      // && !type.endsWith('&')
            errorMsg = QLatin1String("Invalid method, non-output parameters after message or after output parameters: ") + QLatin1String(type);
            return -1;          // not allowed
        }

        int id = QMetaType::type(type);
#ifdef QT_BOOTSTRAPPED
        // in bootstrap mode QDBusMessage isn't included, thus we need to resolve it manually here
        if (type == "QDBusMessage") {
            id = QDBusMetaTypeId::message();
        }
#endif

        if (id == QMetaType::UnknownType) {
            errorMsg = QLatin1String("Unregistered input type in parameter list: ") + QLatin1String(type);
            return -1;
        }

        if (id == QDBusMetaTypeId::message())
            seenMessage = true;
        else if (QDBusMetaType::typeToSignature(id) == nullptr) {
            errorMsg = QLatin1String("Type not registered with QtDBus in parameter list: ") + QLatin1String(type);
            return -1;
        }

        metaTypes.append(id);
        ++inputCount;
    }

    return inputCount;
}

QT_END_NAMESPACE

#endif // QT_NO_DBUS
