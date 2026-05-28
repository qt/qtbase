// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtCore/qmetaobject.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qdebug.h>

#include "qdbusinterface_p.h"   // for ANNOTATION_NO_WAIT
#include "qdbusabstractadaptor_p.h" // for QCLASSINFO_DBUS_*
#include "qdbusconnection_p.h"  // for the flags
#include "qdbusmetatype_p.h"
#include "qdbusmetatype.h"
#include "qdbusutil_p.h"

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

extern Q_DBUS_EXPORT QString qDBusGenerateMetaObjectXml(QString interface, const QMetaObject *mo,
                                                       const QMetaObject *base, int flags);

static inline QString typeNameToXml(const char *typeName)
{
    // ### copied from qtextdocument.cpp
    // ### move this into Qt Core at some point
    const QLatin1StringView plain(typeName);
    QString rich;
    rich.reserve(int(plain.size() * 1.1));
    for (int i = 0; i < plain.size(); ++i) {
        if (plain.at(i) == u'<')
            rich += "&lt;"_L1;
        else if (plain.at(i) == u'>')
            rich += "&gt;"_L1;
        else if (plain.at(i) == u'&')
            rich += "&amp;"_L1;
        else
            rich += plain.at(i);
    }
    return rich;
}

static inline QLatin1StringView accessAsString(bool read, bool write)
{
    if (read)
        return write ? "readwrite"_L1 : "read"_L1 ;
    else
        return write ? "write"_L1 : ""_L1 ;
}

// implement the D-Bus org.freedesktop.DBus.Introspectable interface
// we do that by analysing the metaObject of all the adaptor interfaces

static QString generateInterfaceXml(const QMetaObject *mo, int flags, int methodOffset, int propOffset)
{
    QString retval;

    // start with properties:
    if (flags & (QDBusConnection::ExportScriptableProperties |
                 QDBusConnection::ExportNonScriptableProperties)) {
        for (int i = propOffset; i < mo->propertyCount(); ++i) {

            QMetaProperty mp = mo->property(i);

            if (!((mp.isScriptable() && (flags & QDBusConnection::ExportScriptableProperties)) ||
                  (!mp.isScriptable() && (flags & QDBusConnection::ExportNonScriptableProperties))))
                continue;

            QMetaType type = mp.metaType();
            if (!type.isValid())
                continue;
            const char *signature = QDBusMetaType::typeToSignature(type);
            if (!signature)
                continue;

            retval += "    <property name=\"%1\" type=\"%2\" access=\"%3\""_L1
                      .arg(QLatin1StringView(mp.name()),
                           QLatin1StringView(signature),
                           accessAsString(mp.isReadable(), mp.isWritable()));

            if (!QDBusMetaType::signatureToMetaType(signature).isValid()) {
                const char *typeName = type.name();
                retval += ">\n      <annotation name=\"org.qtproject.QtDBus.QtTypeName\" value=\"%3\"/>\n    </property>\n"_L1
                          .arg(typeNameToXml(typeName));
            } else {
                retval += "/>\n"_L1;
            }
        }
    }

    // now add methods:
    for (int i = methodOffset; i < mo->methodCount(); ++i) {
        QMetaMethod mm = mo->method(i);

        bool isSignal = false;
        bool isSlot = false;
        if (mm.methodType() == QMetaMethod::Signal)
            // adding a signal
            isSignal = true;
        else if (mm.access() == QMetaMethod::Public && mm.methodType() == QMetaMethod::Slot)
            isSlot = true;
        else if (mm.access() == QMetaMethod::Public && mm.methodType() == QMetaMethod::Method)
            ; // invokable, neither signal nor slot
        else
            continue;           // neither signal nor public method/slot

        if (isSignal && !(flags & (QDBusConnection::ExportScriptableSignals |
                                   QDBusConnection::ExportNonScriptableSignals)))
            continue;           // we're not exporting any signals
        if (!isSignal && (!(flags & (QDBusConnection::ExportScriptableSlots | QDBusConnection::ExportNonScriptableSlots)) &&
                          !(flags & (QDBusConnection::ExportScriptableInvokables | QDBusConnection::ExportNonScriptableInvokables))))
            continue;           // we're not exporting any slots or invokables

        // we want to skip non-scriptable stuff as early as possible to avoid bogus warning
        // for methods that are not being exported at all
        bool isScriptable = mm.attributes() & QMetaMethod::Scriptable;
        if (!isScriptable && !(flags & (isSignal ? QDBusConnection::ExportNonScriptableSignals : QDBusConnection::ExportNonScriptableInvokables | QDBusConnection::ExportNonScriptableSlots)))
            continue;

        QString xml = QString::asprintf("    <%s name=\"%s\">\n",
                                        isSignal ? "signal" : "method", mm.name().constData());

        // check the return type first
        QMetaType typeId = mm.returnMetaType();
        if (typeId.isValid() && typeId.id() != QMetaType::Void) {
            const char *typeName = QDBusMetaType::typeToSignature(typeId);
            if (typeName) {
                xml += "      <arg type=\"%1\" direction=\"out\"/>\n"_L1
                       .arg(typeNameToXml(typeName));

                // do we need to describe this argument?
                if (!QDBusMetaType::signatureToMetaType(typeName).isValid())
                    xml += "      <annotation name=\"org.qtproject.QtDBus.QtTypeName.Out0\" value=\"%1\"/>\n"_L1
                        .arg(typeNameToXml(QMetaType(typeId).name()));
            } else {
                qWarning() << "Unsupported return type" << typeId.id() << typeId.name() << "in method" << mm.name();
                continue;
            }
        }
        else if (!typeId.isValid()) {
            qWarning() << "Invalid return type in method" << mm.name();
            continue;           // wasn't a valid type
        }

        QList<QByteArray> names = mm.parameterNames();
        QList<QMetaType> types;
        QString errorMsg;
        int inputCount = qDBusParametersForMethod(mm, types, errorMsg);
        if (inputCount == -1) {
            qWarning() << "Skipped method" << mm.name() << ":" << qPrintable(errorMsg);
            continue;           // invalid form
        }
        if (isSignal && inputCount + 1 != types.size())
            continue;           // signal with output arguments?
        if (isSignal && types.at(inputCount) == QDBusMetaTypeId::message())
            continue;           // signal with QDBusMessage argument?
        if (isSignal && mm.attributes() & QMetaMethod::Cloned)
            continue;           // cloned signal?

        int j;
        for (j = 1; j < types.size(); ++j) {
            // input parameter for a slot or output for a signal
            if (types.at(j) == QDBusMetaTypeId::message()) {
                isScriptable = true;
                continue;
            }

            QString name;
            if (!names.at(j - 1).isEmpty())
                name = "name=\"%1\" "_L1.arg(QLatin1StringView(names.at(j - 1)));

            bool isOutput = isSignal || j > inputCount;

            const char *signature = QDBusMetaType::typeToSignature(types.at(j));
            xml += QString::asprintf("      <arg %lstype=\"%s\" direction=\"%s\"/>\n",
                                     qUtf16Printable(name), signature, isOutput ? "out" : "in");

            // do we need to describe this argument?
            if (!QDBusMetaType::signatureToMetaType(signature).isValid()) {
                const char *typeName = QMetaType(types.at(j)).name();
                xml += QString::fromLatin1("      <annotation name=\"org.qtproject.QtDBus.QtTypeName.%1%2\" value=\"%3\"/>\n")
                       .arg(isOutput ? "Out"_L1 : "In"_L1)
                       .arg(isOutput && !isSignal ? j - inputCount : j - 1)
                       .arg(typeNameToXml(typeName));
            }
        }

        int wantedMask;
        if (isScriptable)
            wantedMask = isSignal ? QDBusConnection::ExportScriptableSignals
                                  : isSlot ? QDBusConnection::ExportScriptableSlots
                                           : QDBusConnection::ExportScriptableInvokables;
        else
            wantedMask = isSignal ? QDBusConnection::ExportNonScriptableSignals
                                  : isSlot ? QDBusConnection::ExportNonScriptableSlots
                                           : QDBusConnection::ExportNonScriptableInvokables;
        if ((flags & wantedMask) != wantedMask)
            continue;

        if (qDBusCheckAsyncTag(mm.tag()))
            // add the no-reply annotation
            xml += "      <annotation name=\"" ANNOTATION_NO_WAIT "\" value=\"true\"/>\n"_L1;

        retval += xml;
        retval += "    </%1>\n"_L1.arg(isSignal ? "signal"_L1 : "method"_L1);
    }

    return retval;
}

QString qDBusGenerateMetaObjectXml(QString interface, const QMetaObject *mo,
                                   const QMetaObject *base, int flags)
{
    if (interface.isEmpty())
        // generate the interface name from the meta object
        interface = qDBusInterfaceFromMetaObject(mo);

    QString xml;
    int idx = mo->indexOfClassInfo(QCLASSINFO_DBUS_INTROSPECTION);
    if (idx >= mo->classInfoOffset())
        return QString::fromUtf8(mo->classInfo(idx).value());
    else
        xml = generateInterfaceXml(mo, flags, base->methodCount(), base->propertyCount());

    if (xml.isEmpty())
        return QString();       // don't add an empty interface
    return "  <interface name=\"%1\">\n%2  </interface>\n"_L1
        .arg(interface, xml);
}

QT_END_NAMESPACE

#endif // QT_NO_DBUS
