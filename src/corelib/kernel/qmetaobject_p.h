// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2014 Olivier Goffart <ogoffart@woboq.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMETAOBJECT_P_H
#define QMETAOBJECT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of moc.  This header file may change from version to version without notice,
// or even be removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>
#include <QtCore/qobjectdefs.h>
#include <QtCore/qmutex.h>
#include <QtCore/qmetaobject.h>
#ifndef QT_NO_QOBJECT
#include <private/qobject_p.h> // For QObjectPrivate::Connection
#endif
#include <private/qtools_p.h>
#include <QtCore/qvarlengtharray.h>

QT_BEGIN_NAMESPACE
// ### TODO - QTBUG-87869: wrap in a proper Q_NAMESPACE and use scoped enums, to avoid name clashes

using namespace QtMiscUtils;

enum PropertyFlags {
    Invalid = 0x00000000,
    Readable = 0x00000001,
    Writable = 0x00000002,
    Resettable = 0x00000004,
    EnumOrFlag = 0x00000008,
    Alias = 0x00000010,
    // Reserved for future usage = 0x00000020,
    StdCppSet = 0x00000100,
    Constant = 0x00000400,
    Final = 0x00000800,
    Designable = 0x00001000,
    Scriptable = 0x00004000,
    Stored = 0x00010000,
    User = 0x00100000,
    Required = 0x01000000,
    Bindable = 0x02000000
};

enum MethodFlags {
    AccessPrivate = 0x00,
    AccessProtected = 0x01,
    AccessPublic = 0x02,
    AccessMask = 0x03, // mask

    MethodMethod = 0x00,
    MethodSignal = 0x04,
    MethodSlot = 0x08,
    MethodConstructor = 0x0c,
    MethodTypeMask = 0x0c,

    MethodCompatibility = 0x10,
    MethodCloned = 0x20,
    MethodScriptable = 0x40,
    MethodRevisioned = 0x80,

    MethodIsConst = 0x100, // no use case for volatile so far
};

enum MetaObjectFlag {
    DynamicMetaObject = 0x01,
    RequiresVariantMetaObject = 0x02,
    PropertyAccessInStaticMetaCall = 0x04 // since Qt 5.5, property code is in the static metacall
};
Q_DECLARE_FLAGS(MetaObjectFlags, MetaObjectFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS(MetaObjectFlags)

enum MetaDataFlags {
    IsUnresolvedType = 0x80000000,
    TypeNameIndexMask = 0x7FFFFFFF,
    IsUnresolvedSignal = 0x70000000
};

enum EnumFlags {
    EnumIsFlag = 0x1,
    EnumIsScoped = 0x2
};

Q_CORE_EXPORT int qMetaTypeTypeInternal(const char *);

class QArgumentType
{
public:
    QArgumentType(int type)
        : _type(type)
    {}
    QArgumentType(const QByteArray &name)
        : _type(qMetaTypeTypeInternal(name.constData())), _name(name)
    {}
    QArgumentType()
        : _type(0)
    {}
    int type() const
    { return _type; }
    QByteArray name() const
    {
        if (_type && _name.isEmpty())
            const_cast<QArgumentType *>(this)->_name = QMetaType(_type).name();
        return _name;
    }
    bool operator==(const QArgumentType &other) const
    {
        if (_type && other._type)
            return _type == other._type;
        else
            return name() == other.name();
    }
    bool operator!=(const QArgumentType &other) const
    {
        if (_type && other._type)
            return _type != other._type;
        else
            return name() != other.name();
    }

private:
    int _type;
    QByteArray _name;
};
Q_DECLARE_TYPEINFO(QArgumentType, Q_RELOCATABLE_TYPE);

typedef QVarLengthArray<QArgumentType, 10> QArgumentTypeArray;

namespace { class QMetaMethodPrivate; }
class QMetaMethodInvoker : public QMetaMethod
{
    QMetaMethodInvoker() = delete;

public:
    enum class InvokeFailReason : int {
        // negative values mean a match was found but the invocation failed
        // (and a warning has been printed)
        ReturnTypeMismatch = -1,
        DeadLockDetected = -2,
        CallViaVirtualFailed = -3,  // no warning
        ConstructorCallOnObject = -4,
        ConstructorCallWithoutResult = -5,
        ConstructorCallFailed = -6, // no warning

        CouldNotQueueParameter = -0x1000,

        // zero is success
        None = 0,

        // positive values mean the parameters did not match
        TooFewArguments,
        FormalParameterMismatch = 0x1000,
    };

    // shadows the public function
    static InvokeFailReason Q_CORE_EXPORT
    invokeImpl(QMetaMethod self, void *target, Qt::ConnectionType, qsizetype paramCount,
               const void *const *parameters, const char *const *typeNames,
               const QtPrivate::QMetaTypeInterface *const *metaTypes);
};

struct QMetaObjectPrivate
{
    // revision 7 is Qt 5.0 everything lower is not supported
    // revision 8 is Qt 5.12: It adds the enum name to QMetaEnum
    // revision 9 is Qt 6.0: It adds the metatype of properties and methods
    // revision 10 is Qt 6.2: The metatype of the metaobject is stored in the metatypes array
    //                        and metamethods store a flag stating whether they are const
    // revision 11 is Qt 6.5: The metatype for void is stored in the metatypes array
    enum { OutputRevision = 11 }; // Used by moc, qmetaobjectbuilder and qdbus
    enum { IntsPerMethod = QMetaMethod::Data::Size };
    enum { IntsPerEnum = QMetaEnum::Data::Size };
    enum { IntsPerProperty = QMetaProperty::Data::Size };

    int revision;
    int className;
    int classInfoCount, classInfoData;
    int methodCount, methodData;
    int propertyCount, propertyData;
    int enumeratorCount, enumeratorData;
    int constructorCount, constructorData;
    int flags;
    int signalCount;

    static inline const QMetaObjectPrivate *get(const QMetaObject *metaobject)
    { return reinterpret_cast<const QMetaObjectPrivate*>(metaobject->d.data); }

    static int originalClone(const QMetaObject *obj, int local_method_index);

    static QByteArray decodeMethodSignature(const char *signature,
                                            QArgumentTypeArray &types);
    static int indexOfSignalRelative(const QMetaObject **baseObject,
                                     const QByteArray &name, int argc,
                                     const QArgumentType *types);
    static int indexOfSlotRelative(const QMetaObject **m,
                                   const QByteArray &name, int argc,
                                   const QArgumentType *types);
    static int indexOfSignal(const QMetaObject *m, const QByteArray &name,
                             int argc, const QArgumentType *types);
    static int indexOfSlot(const QMetaObject *m, const QByteArray &name,
                           int argc, const QArgumentType *types);
    static int indexOfMethod(const QMetaObject *m, const QByteArray &name,
                             int argc, const QArgumentType *types);
    static int indexOfConstructor(const QMetaObject *m, const QByteArray &name,
                                  int argc, const QArgumentType *types);
    Q_CORE_EXPORT static QMetaMethod signal(const QMetaObject *m, int signal_index);
    static inline int signalOffset(const QMetaObject *m)
    {
        Q_ASSERT(m != nullptr);
        int offset = 0;
        for (m = m->d.superdata; m; m = m->d.superdata)
            offset += reinterpret_cast<const QMetaObjectPrivate *>(m->d.data)->signalCount;
        return offset;
    }
    Q_CORE_EXPORT static int absoluteSignalCount(const QMetaObject *m);
    Q_CORE_EXPORT static int signalIndex(const QMetaMethod &m);
    static bool checkConnectArgs(int signalArgc, const QArgumentType *signalTypes,
                                 int methodArgc, const QArgumentType *methodTypes);
    static bool checkConnectArgs(const QMetaMethodPrivate *signal,
                                 const QMetaMethodPrivate *method);

    static QList<QByteArray> parameterTypeNamesFromSignature(const char *signature);

#ifndef QT_NO_QOBJECT
    // defined in qobject.cpp
    enum DisconnectType { DisconnectAll, DisconnectOne };
    static void memberIndexes(const QObject *obj, const QMetaMethod &member,
                              int *signalIndex, int *methodIndex);
    static QObjectPrivate::Connection *connect(const QObject *sender, int signal_index,
                        const QMetaObject *smeta,
                        const QObject *receiver, int method_index_relative,
                        const QMetaObject *rmeta = nullptr,
                        int type = 0, int *types = nullptr);
    static bool disconnect(const QObject *sender, int signal_index,
                           const QMetaObject *smeta,
                           const QObject *receiver, int method_index, void **slot,
                           DisconnectType = DisconnectAll);
    static inline bool disconnectHelper(QObjectPrivate::ConnectionData *connections, int signalIndex,
                                        const QObject *receiver, int method_index, void **slot,
                                        QBasicMutex *senderMutex, DisconnectType = DisconnectAll);
#endif

    template<int MethodType>
    static inline int indexOfMethodRelative(const QMetaObject **baseObject,
                                            const QByteArray &name, int argc,
                                            const QArgumentType *types);

    static bool methodMatch(const QMetaObject *m, const QMetaMethod &method,
                            const QByteArray &name, int argc,
                            const QArgumentType *types);
    Q_CORE_EXPORT static QMetaMethod firstMethod(const QMetaObject *baseObject, QByteArrayView name);

};

// For meta-object generators

enum { MetaObjectPrivateFieldCount = sizeof(QMetaObjectPrivate) / sizeof(int) };

#ifndef UTILS_H
// mirrored in moc's utils.h
static inline bool is_ident_char(char s)
{
    return isAsciiLetterOrNumber(s) || s == '_';
}

static inline bool is_space(char s)
{
    return (s == ' ' || s == '\t');
}
#endif

/*
    This function is shared with moc.cpp. The implementation lives in qmetaobject_moc_p.h, which
    should be included where needed. The declaration here is not used to avoid warnings from
    the compiler about unused functions.

static QByteArray normalizeTypeInternal(const char *t, const char *e, bool fixScope = false, bool adjustConst = true);
*/

QT_END_NAMESPACE

#endif

