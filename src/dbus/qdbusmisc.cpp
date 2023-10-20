// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <string.h>

#ifndef QT_BOOTSTRAPPED
#include <QtCore/qcoreapplication.h>
#include <QtCore/qlist.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qvariant.h>

#include "qdbusutil_p.h"
#include "qdbusconnection_p.h"
#include "qdbusabstractadaptor_p.h" // for QCLASSINFO_DBUS_*
#endif
#include "qdbusmetatype_p.h"

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

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
        interface = QLatin1StringView(mo->classInfo(idx).value());
    } else {
        interface = QLatin1StringView(mo->className());
        interface.replace("::"_L1, "."_L1);

        if (interface.startsWith("QDBus"_L1)) {
            interface.prepend("org.qtproject.QtDBus."_L1);
        } else if (interface.startsWith(u'Q') &&
                   interface.size() >= 2 && interface.at(1).isUpper()) {
            // assume it's Qt
            interface.prepend("org.qtproject.Qt."_L1);
        } else if (!QCoreApplication::instance()||
                   QCoreApplication::instance()->applicationName().isEmpty()) {
            interface.prepend("local."_L1);
         } else {
            interface.prepend(u'.').prepend(QCoreApplication::instance()->applicationName());
            const QString organizationDomain = QCoreApplication::instance()->organizationDomain();
            const auto domainName = QStringView{organizationDomain}.split(u'.', Qt::SkipEmptyParts);
            if (domainName.isEmpty()) {
                 interface.prepend("local."_L1);
            } else {
                QString composedDomain;
                // + 1 for additional dot, e.g. organizationDomain equals "example.com",
                // then composedDomain will be equal "com.example."
                composedDomain.reserve(organizationDomain.size() + 1);
                for (auto it = domainName.rbegin(), end = domainName.rend(); it != end; ++it)
                    composedDomain += *it + u'.';

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
int qDBusParametersForMethod(const QMetaMethod &mm, QList<QMetaType> &metaTypes, QString &errorMsg)
{
    QList<QByteArray> parameterTypes;
    parameterTypes.reserve(mm.parameterCount());

    // Not using QMetaMethod::parameterTypes() since we call QMetaType::fromName below
    // where we need any typedefs resolved already.
    for (int i = 0; i < mm.parameterCount(); ++i) {
        QByteArray typeName = mm.parameterMetaType(i).name();
        if (typeName.isEmpty())
            typeName = mm.parameterTypeName(i);
        parameterTypes.append(typeName);
    }

    return qDBusParametersForMethod(parameterTypes, metaTypes, errorMsg);
}

#endif // QT_BOOTSTRAPPED

int qDBusParametersForMethod(const QList<QByteArray> &parameterTypes, QList<QMetaType> &metaTypes,
                             QString &errorMsg)
{
    QDBusMetaTypeId::init();
    metaTypes.clear();

    metaTypes.append(QMetaType());        // return type
    int inputCount = 0;
    bool seenMessage = false;
    for (QByteArray type : parameterTypes) {
        if (type.endsWith('*')) {
            errorMsg = "Pointers are not supported: "_L1 + QLatin1StringView(type);
            return -1;
        }

        if (type.endsWith('&')) {
            QByteArray basictype = type;
            basictype.truncate(type.size() - 1);

            QMetaType id = QMetaType::fromName(basictype);
            if (!id.isValid()) {
                errorMsg = "Unregistered output type in parameter list: "_L1 + QLatin1StringView(type);
                return -1;
            } else if (QDBusMetaType::typeToSignature(id) == nullptr)
                return -1;

            metaTypes.append(id);
            seenMessage = true; // it cannot appear anymore anyways
            continue;
        }

        if (seenMessage) {      // && !type.endsWith('&')
            errorMsg = "Invalid method, non-output parameters after message or after output parameters: "_L1 + QLatin1StringView(type);
            return -1;          // not allowed
        }

        if (type.startsWith("QVector<"))
            type = "QList<" + type.mid(sizeof("QVector<") - 1);

        QMetaType id = QMetaType::fromName(type);
#ifdef QT_BOOTSTRAPPED
        // in bootstrap mode QDBusMessage isn't included, thus we need to resolve it manually here
        if (type == "QDBusMessage") {
            id = QDBusMetaTypeId::message();
        }
#endif

        if (!id.isValid()) {
            errorMsg = "Unregistered input type in parameter list: "_L1 + QLatin1StringView(type);
            return -1;
        }

        if (id == QDBusMetaTypeId::message())
            seenMessage = true;
        else if (QDBusMetaType::typeToSignature(id) == nullptr) {
            errorMsg = "Type not registered with QtDBus in parameter list: "_L1 + QLatin1StringView(type);
            return -1;
        }

        metaTypes.append(id);
        ++inputCount;
    }

    return inputCount;
}

QT_END_NAMESPACE

#endif // QT_NO_DBUS
