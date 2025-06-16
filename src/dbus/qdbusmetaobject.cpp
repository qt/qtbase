// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qdbusmetaobject_p.h"

#include <QtCore/qbytearray.h>
#include <QtCore/qhash.h>
#include <QtCore/qstring.h>
#include <QtCore/qvarlengtharray.h>

#include "qdbusutil_p.h"
#include "qdbuserror.h"
#include "qdbusmetatype.h"
#include "qdbusargument.h"
#include "qdbusintrospection_p.h"
#include "qdbusabstractinterface_p.h"

#include <private/qmetaobject_p.h>
#include <private/qmetaobjectbuilder_p.h>

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class QDBusMetaObjectGenerator
{
public:
    QDBusMetaObjectGenerator(const QString &interface,
                             const QDBusIntrospection::Interface *parsedData);
    void write(QDBusMetaObject *obj);
    void writeWithoutXml(QDBusMetaObject *obj);

private:
    struct Method {
        QList<QByteArray> parameterNames;
        QByteArray tag;
        QByteArray name;
        QVarLengthArray<int, 4> inputTypes;
        QVarLengthArray<int, 4> outputTypes;
        QByteArray rawReturnType;
        quint32 flags;
    };

    struct Property {
        QByteArray typeName;
        QByteArray signature;
        int type;
        quint32 flags;
    };
    struct Type {
        int id;
        QByteArray name;
    };

    using MethodMap = QMap<QByteArray, Method>;
    MethodMap signals_;
    MethodMap methods;
    QMap<QByteArray, Property> properties;

    const QDBusIntrospection::Interface *data;
    QString interface;

    Type findType(const QByteArray &signature,
                  const QDBusIntrospection::Annotations &annotations,
                  const char *direction = "Out", int id = -1);

    void parseMethods();
    void parseSignals();
    void parseProperties();

    static qsizetype aggregateParameterCount(const MethodMap &map);
};

static const qsizetype intsPerProperty = 2;
static const qsizetype intsPerMethod = 2;

struct QDBusMetaObjectPrivate : public QMetaObjectPrivate
{
    int propertyDBusData;
    int methodDBusData;
};

QDBusMetaObjectGenerator::QDBusMetaObjectGenerator(const QString &interfaceName,
                                                   const QDBusIntrospection::Interface *parsedData)
    : data(parsedData), interface(interfaceName)
{
    if (data) {
        parseProperties();
        parseSignals();             // call parseSignals first so that slots override signals
        parseMethods();
    }
}

static int registerComplexDBusType(const QByteArray &typeName)
{
    struct QDBusRawTypeHandler : QtPrivate::QMetaTypeInterface
    {
        const QByteArray name;
        QDBusRawTypeHandler(const QByteArray &name)
            : QtPrivate::QMetaTypeInterface {
                0, sizeof(void *), sizeof(void *), QMetaType::RelocatableType, 0, nullptr,
                name.constData(),
                nullptr, nullptr, nullptr, nullptr,
                nullptr, nullptr, nullptr,
                nullptr, nullptr, nullptr
            },
            name(name)
        {}
    };

    Q_CONSTINIT static QBasicMutex mutex;
    Q_CONSTINIT static struct Hash : QHash<QByteArray, QMetaType>
    {
        ~Hash()
        {
            for (QMetaType entry : std::as_const(*this))
                QMetaType::unregisterMetaType(entry);
        }
    } hash;
    QMutexLocker lock(&mutex);
    QMetaType &metatype = hash[typeName];
    if (!metatype.isValid())
        metatype = QMetaType(new QDBusRawTypeHandler(typeName));
    return metatype.id();
}

Q_DBUS_EXPORT bool qt_dbus_metaobject_skip_annotations = false;

QDBusMetaObjectGenerator::Type
QDBusMetaObjectGenerator::findType(const QByteArray &signature,
                                   const QDBusIntrospection::Annotations &annotations,
                                   const char *direction, int id)
{
    Type result;
    result.id = QMetaType::UnknownType;

    int type = QDBusMetaType::signatureToMetaType(signature).id();
    if (type == QMetaType::UnknownType && !qt_dbus_metaobject_skip_annotations) {
        // it's not a type normally handled by our meta type system
        // it must contain an annotation
        QString annotationName = QString::fromLatin1("org.qtproject.QtDBus.QtTypeName");
        if (id >= 0)
            annotationName += QString::fromLatin1(".%1%2")
                              .arg(QLatin1StringView(direction))
                              .arg(id);

        // extract from annotations:
        auto annotation = annotations.value(annotationName);
        QByteArray typeName = annotation.value.toLatin1();

        // verify that it's a valid one
        if (typeName.isEmpty()) {
            // try the old annotation from Qt 4
            annotationName = QString::fromLatin1("com.trolltech.QtDBus.QtTypeName");
            if (id >= 0)
                annotationName += QString::fromLatin1(".%1%2")
                                  .arg(QLatin1StringView(direction))
                                  .arg(id);
            annotation = annotations.value(annotationName);
            typeName = annotation.value.toLatin1();
        }

        if (!typeName.isEmpty()) {
            // type name found
            type = QMetaType::fromName(typeName).id();
        }

        if (type == QMetaType::UnknownType || signature != QDBusMetaType::typeToSignature(QMetaType(type))) {
            // type is still unknown or doesn't match back to the signature that it
            // was expected to, so synthesize a fake type
            typeName = "QDBusRawType<0x" + signature.toHex() + ">*";
            type = registerComplexDBusType(typeName);
        }

        result.name = typeName;
    } else if (type == QMetaType::UnknownType) {
        // this case is used only by the qdbus command-line tool
        // invalid, let's create an impossible type that contains the signature

        if (signature == "av") {
            result.name = "QVariantList";
            type = QMetaType::QVariantList;
        } else if (signature == "a{sv}") {
            result.name = "QVariantMap";
            type = QMetaType::QVariantMap;
        } else if (signature == "a{ss}") {
            result.name = "QMap<QString,QString>";
            type = qMetaTypeId<QMap<QString, QString> >();
        } else if (signature == "aay") {
            result.name = "QByteArrayList";
            type = qMetaTypeId<QByteArrayList>();
        } else {
            result.name = "{D-Bus type \"" + signature + "\"}";
            type = registerComplexDBusType(result.name);
        }
    } else {
        result.name = QMetaType(type).name();
    }

    result.id = type;
    return result;              // success
}

void QDBusMetaObjectGenerator::parseMethods()
{
    //
    // TODO:
    //  Add cloned methods when the remote object has return types
    //

    for (const QDBusIntrospection::Method &m : std::as_const(data->methods)) {
        Method mm;

        mm.name = m.name.toLatin1();
        QByteArray prototype = mm.name;
        prototype += '(';

        bool ok = true;

        // build the input argument list
        for (qsizetype i = 0; i < m.inputArgs.size(); ++i) {
            const QDBusIntrospection::Argument &arg = m.inputArgs.at(i);

            Type type = findType(arg.type.toLatin1(), m.annotations, "In", i);
            if (type.id == QMetaType::UnknownType) {
                ok = false;
                break;
            }

            mm.inputTypes.append(type.id);

            mm.parameterNames.append(arg.name.toLatin1());

            prototype.append(type.name);
            prototype.append(',');
        }
        if (!ok) continue;

        // build the output argument list:
        for (qsizetype i = 0; i < m.outputArgs.size(); ++i) {
            const QDBusIntrospection::Argument &arg = m.outputArgs.at(i);

            Type type = findType(arg.type.toLatin1(), m.annotations, "Out", i);
            if (type.id == QMetaType::UnknownType) {
                ok = false;
                break;
            }

            mm.outputTypes.append(type.id);

            if (i == 0 && type.id == -1) {
                mm.rawReturnType = type.name;
            }
            if (i != 0) {
                // non-const ref parameter
                mm.parameterNames.append(arg.name.toLatin1());

                prototype.append(type.name);
                prototype.append("&,");
            }
        }
        if (!ok) continue;

        // convert the last commas:
        if (!mm.parameterNames.isEmpty())
            prototype[prototype.size() - 1] = ')';
        else
            prototype.append(')');

        // check the async tag
        if (m.annotations.value(ANNOTATION_NO_WAIT ""_L1).value == "true"_L1)
            mm.tag = "Q_NOREPLY";

        // meta method flags
        mm.flags = AccessPublic | MethodSlot | MethodScriptable;

        // add
        methods.insert(QMetaObject::normalizedSignature(prototype), mm);
    }
}

void QDBusMetaObjectGenerator::parseSignals()
{
    for (const QDBusIntrospection::Signal &s : std::as_const(data->signals_)) {
        Method mm;

        mm.name = s.name.toLatin1();
        QByteArray prototype = mm.name;
        prototype += '(';

        bool ok = true;

        // build the output argument list
        for (qsizetype i = 0; i < s.outputArgs.size(); ++i) {
            const QDBusIntrospection::Argument &arg = s.outputArgs.at(i);

            Type type = findType(arg.type.toLatin1(), s.annotations, "Out", i);
            if (type.id == QMetaType::UnknownType) {
                ok = false;
                break;
            }

            mm.inputTypes.append(type.id);

            mm.parameterNames.append(arg.name.toLatin1());

            prototype.append(type.name);
            prototype.append(',');
        }
        if (!ok) continue;

        // convert the last commas:
        if (!mm.parameterNames.isEmpty())
            prototype[prototype.size() - 1] = ')';
        else
            prototype.append(')');

        // meta method flags
        mm.flags = AccessPublic | MethodSignal | MethodScriptable;

        // add
        signals_.insert(QMetaObject::normalizedSignature(prototype), mm);
    }
}

void QDBusMetaObjectGenerator::parseProperties()
{
    for (const QDBusIntrospection::Property &p : std::as_const(data->properties)) {
        Property mp;
        Type type = findType(p.type.toLatin1(), p.annotations);
        if (type.id == QMetaType::UnknownType)
            continue;

        QByteArray name = p.name.toLatin1();
        mp.signature = p.type.toLatin1();
        mp.type = type.id;
        mp.typeName = type.name;

        // build the flags:
        mp.flags = StdCppSet | Scriptable | Stored | Designable;
        if (p.access != QDBusIntrospection::Property::Write)
            mp.flags |= Readable;
        if (p.access != QDBusIntrospection::Property::Read)
            mp.flags |= Writable;

        // add the property:
        properties.insert(name, mp);
    }
}

// Returns the sum of all parameters (including return type) for the given
// \a map of methods. This is needed for calculating the size of the methods'
// parameter type/name meta-data.
qsizetype QDBusMetaObjectGenerator::aggregateParameterCount(const MethodMap &map)
{
    qsizetype sum = 0;
    for (const Method &m : map)
        sum += m.inputTypes.size() + qMax(qsizetype(1), m.outputTypes.size());
    return sum;
}

void QDBusMetaObjectGenerator::write(QDBusMetaObject *obj)
{
    // this code here is mostly copied from qaxbase.cpp
    // with a few modifications to make it cleaner

    QString className = interface;
    className.replace(u'.', "::"_L1);
    if (className.isEmpty())
        className = "QDBusInterface"_L1;

    QVarLengthArray<uint> idata;
    idata.resize(sizeof(QDBusMetaObjectPrivate) / sizeof(uint));

    qsizetype methodParametersDataSize =
            ((aggregateParameterCount(signals_)
             + aggregateParameterCount(methods)) * 2) // types and parameter names
            - signals_.size() // return "parameters" don't have names
            - methods.size(); // ditto

    QDBusMetaObjectPrivate *header = reinterpret_cast<QDBusMetaObjectPrivate *>(idata.data());
    static_assert(QMetaObjectPrivate::OutputRevision == 13, "QtDBus meta-object generator should generate the same version as moc");
    header->revision = QMetaObjectPrivate::OutputRevision;
    header->className = 0;
    header->classInfoCount = 0;
    header->classInfoData = 0;
    header->methodCount = int(signals_.size() + methods.size());
    header->methodData = int(idata.size());
    header->propertyCount = int(properties.size());
    header->propertyData = int(header->methodData + header->methodCount *
                               QMetaObjectPrivate::IntsPerMethod + methodParametersDataSize);
    header->enumeratorCount = 0;
    header->enumeratorData = 0;
    header->constructorCount = 0;
    header->constructorData = 0;
    header->flags = RequiresVariantMetaObject | AllocatedMetaObject;
    header->signalCount = signals_.size();
    // These are specific to QDBusMetaObject:
    header->propertyDBusData = int(header->propertyData + header->propertyCount
                                   * QMetaObjectPrivate::IntsPerProperty);
    header->methodDBusData = int(header->propertyDBusData + header->propertyCount * intsPerProperty);

    qsizetype data_size = idata.size() +
                    (header->methodCount * (QMetaObjectPrivate::IntsPerMethod+intsPerMethod)) + methodParametersDataSize +
                    (header->propertyCount * (QMetaObjectPrivate::IntsPerProperty+intsPerProperty));

    // Signals must be added before other methods, to match moc.
    std::array<std::reference_wrapper<const MethodMap>, 2> methodMaps = { signals_, methods };

    for (const auto &methodMap : methodMaps) {
        for (const Method &mm : methodMap.get())
            data_size += 2 + mm.inputTypes.size() + mm.outputTypes.size();
    }
    idata.resize(data_size + 1);

    QMetaStringTable strings(className.toLatin1());

    qsizetype offset = header->methodData;
    qsizetype parametersOffset = offset + header->methodCount * QMetaObjectPrivate::IntsPerMethod;
    qsizetype signatureOffset = header->methodDBusData;
    qsizetype typeidOffset = header->methodDBusData + header->methodCount * intsPerMethod;
    idata[typeidOffset++] = 0;                           // eod

    qsizetype totalMetaTypeCount = properties.size();
    ++totalMetaTypeCount; // + 1 for metatype of dynamic metaobject
    for (const auto &methodMap : methodMaps) {
        for (const Method &mm : methodMap.get()) {
            qsizetype argc = mm.inputTypes.size() + qMax(qsizetype(0), mm.outputTypes.size() - 1);
            totalMetaTypeCount += argc + 1;
        }
    }
    QMetaType *metaTypes = new QMetaType[totalMetaTypeCount];
    int propertyId = 0;

    // add each method:
    qsizetype currentMethodMetaTypeOffset = properties.size() + 1;

    for (const auto &methodMap : methodMaps) {
        for (const Method &mm : methodMap.get()) {
            qsizetype argc = mm.inputTypes.size() + qMax(qsizetype(0), mm.outputTypes.size() - 1);

            idata[offset++] = strings.enter(mm.name);
            idata[offset++] = argc;
            idata[offset++] = parametersOffset;
            idata[offset++] = strings.enter(mm.tag);
            idata[offset++] = mm.flags;
            idata[offset++] = currentMethodMetaTypeOffset;

            // Parameter types
            for (qsizetype i = -1; i < argc; ++i) {
                int type;
                QByteArray typeName;
                if (i < 0) { // Return type
                    if (!mm.outputTypes.isEmpty()) {
                        type = mm.outputTypes.first();
                        if (type == -1) {
                            type = IsUnresolvedType | strings.enter(mm.rawReturnType);
                        }
                    } else {
                        type = QMetaType::Void;
                    }
                } else if (i < mm.inputTypes.size()) {
                    type = mm.inputTypes.at(i);
                } else {
                    Q_ASSERT(mm.outputTypes.size() > 1);
                    type = mm.outputTypes.at(i - mm.inputTypes.size() + 1);
                    // Output parameters are references; type id not available
                    typeName = QMetaType(type).name();
                    typeName.append('&');
                    type = QMetaType::UnknownType;
                }
                int typeInfo;
                if (!typeName.isEmpty())
                    typeInfo = IsUnresolvedType | strings.enter(typeName);
                else
                    typeInfo = type;
                metaTypes[currentMethodMetaTypeOffset++] = QMetaType(type);
                idata[parametersOffset++] = typeInfo;
            }
            // Parameter names
            for (qsizetype i = 0; i < argc; ++i)
                idata[parametersOffset++] = strings.enter(mm.parameterNames.at(i));

            idata[signatureOffset++] = typeidOffset;
            idata[typeidOffset++] = mm.inputTypes.size();
            memcpy(idata.data() + typeidOffset, mm.inputTypes.data(), mm.inputTypes.size() * sizeof(uint));
            typeidOffset += mm.inputTypes.size();

            idata[signatureOffset++] = typeidOffset;
            idata[typeidOffset++] = mm.outputTypes.size();
            memcpy(idata.data() + typeidOffset, mm.outputTypes.data(), mm.outputTypes.size() * sizeof(uint));
            typeidOffset += mm.outputTypes.size();
        }
    }

    Q_ASSERT(offset == header->methodData + header->methodCount * QMetaObjectPrivate::IntsPerMethod);
    Q_ASSERT(parametersOffset == header->propertyData);
    Q_ASSERT(signatureOffset == header->methodDBusData + header->methodCount * intsPerMethod);
    Q_ASSERT(typeidOffset == idata.size());
    offset += methodParametersDataSize;
    Q_ASSERT(offset == header->propertyData);

    // add each property
    signatureOffset = header->propertyDBusData;
    for (const auto &[name, mp] : std::as_const(properties).asKeyValueRange()) {
        // form is name, typeinfo, flags
        idata[offset++] = strings.enter(name);
        Q_ASSERT(mp.type != QMetaType::UnknownType);
        idata[offset++] = mp.type;
        idata[offset++] = mp.flags;
        idata[offset++] = -1; // notify index
        idata[offset++] = 0; // revision

        idata[signatureOffset++] = strings.enter(mp.signature);
        idata[signatureOffset++] = mp.type;

        metaTypes[propertyId++] = QMetaType(mp.type);
    }
    metaTypes[propertyId] = QMetaType(); // we can't know our own metatype

    Q_ASSERT(offset == header->propertyDBusData);
    Q_ASSERT(signatureOffset == header->methodDBusData);

    char *string_data = new char[strings.blobSize()];
    strings.writeBlob(string_data);

    uint *uint_data = new uint[idata.size()];
    memcpy(uint_data, idata.data(), idata.size() * sizeof(uint));

    // put the metaobject together
    obj->d.data = uint_data;
    obj->d.relatedMetaObjects = nullptr;
    obj->d.static_metacall = nullptr;
    obj->d.extradata = nullptr;
    obj->d.stringdata = reinterpret_cast<const uint *>(string_data);
    obj->d.superdata = &QDBusAbstractInterface::staticMetaObject;
    obj->d.metaTypes = reinterpret_cast<QtPrivate::QMetaTypeInterface *const *>(metaTypes);
}

#if 0
void QDBusMetaObjectGenerator::writeWithoutXml(const QString &interface)
{
    // no XML definition
    QString tmp(interface);
    tmp.replace(u'.', "::"_L1);
    QByteArray name(tmp.toLatin1());

    QDBusMetaObjectPrivate *header = new QDBusMetaObjectPrivate;
    memset(header, 0, sizeof *header);
    header->revision = 1;
    // leave the rest with 0

    char *stringdata = new char[name.length() + 1];
    stringdata[name.length()] = '\0';

    d.data = reinterpret_cast<uint*>(header);
    d.relatedMetaObjects = 0;
    d.static_metacall = 0;
    d.extradata = 0;
    d.stringdata = stringdata;
    d.superdata = &QDBusAbstractInterface::staticMetaObject;
    cached = false;
}
#endif

/////////
// class QDBusMetaObject

QDBusMetaObject *QDBusMetaObject::createMetaObject(const QString &interface, const QString &xml,
                                                   QHash<QString, QDBusMetaObject *> &cache,
                                                   QDBusError &error)
{
    error = QDBusError();
    QDBusIntrospection::Interfaces parsed = QDBusIntrospection::parseInterfaces(xml);

    QDBusMetaObject *we = nullptr;
    QDBusIntrospection::Interfaces::ConstIterator it = parsed.constBegin();
    QDBusIntrospection::Interfaces::ConstIterator end = parsed.constEnd();
    for ( ; it != end; ++it) {
        // check if it's in the cache
        bool us = it.key() == interface;

        QDBusMetaObject *obj = cache.value(it.key(), 0);
        if (!obj && (us || !interface.startsWith("local."_L1 ))) {
            // not in cache; create
            obj = new QDBusMetaObject;
            QDBusMetaObjectGenerator generator(it.key(), it.value().constData());
            generator.write(obj);

            if ((obj->cached = !it.key().startsWith("local."_L1)))
                // cache it
                cache.insert(it.key(), obj);
            else if (!us)
                delete obj;

        }

        if (us)
            // it's us
            we = obj;
    }

    if (we)
        return we;
    // still nothing?

    if (parsed.isEmpty()) {
        // object didn't return introspection
        we = new QDBusMetaObject;
        QDBusMetaObjectGenerator generator(interface, nullptr);
        generator.write(we);
        we->cached = false;
        return we;
    } else if (interface.isEmpty()) {
        // merge all interfaces
        it = parsed.constBegin();
        QDBusIntrospection::Interface merged = *it.value().constData();

        for (++it; it != end; ++it) {
            merged.annotations.insert(it.value()->annotations);
            merged.methods.unite(it.value()->methods);
            merged.signals_.unite(it.value()->signals_);
            merged.properties.insert(it.value()->properties);
        }

        merged.name = "local.Merged"_L1;
        merged.introspection.clear();

        we = new QDBusMetaObject;
        QDBusMetaObjectGenerator generator(merged.name, &merged);
        generator.write(we);
        we->cached = false;
        return we;
    }

    // mark as an error
    error = QDBusError(QDBusError::UnknownInterface,
                       "Interface '%1' was not found"_L1.arg(interface));
    return nullptr;
}

QDBusMetaObject::QDBusMetaObject()
{
}

static inline const QDBusMetaObjectPrivate *priv(const uint* data)
{
    return reinterpret_cast<const QDBusMetaObjectPrivate *>(data);
}

const int *QDBusMetaObject::inputTypesForMethod(int id) const
{
    //id -= methodOffset();
    if (id >= 0 && id < priv(d.data)->methodCount) {
        int handle = priv(d.data)->methodDBusData + id*intsPerMethod;
        return reinterpret_cast<const int*>(d.data + d.data[handle]);
    }
    return nullptr;
}

const int *QDBusMetaObject::outputTypesForMethod(int id) const
{
    //id -= methodOffset();
    if (id >= 0 && id < priv(d.data)->methodCount) {
        int handle = priv(d.data)->methodDBusData + id*intsPerMethod;
        return reinterpret_cast<const int*>(d.data + d.data[handle + 1]);
    }
    return nullptr;
}

int QDBusMetaObject::propertyMetaType(int id) const
{
    //id -= propertyOffset();
    if (id >= 0 && id < priv(d.data)->propertyCount) {
        int handle = priv(d.data)->propertyDBusData + id*intsPerProperty;
        return d.data[handle + 1];
    }
    return 0;
}

QT_END_NAMESPACE

#endif // QT_NO_DBUS
