// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2015 Olivier Goffart <ogoffart@woboq.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmetaobject.h"
#include "qmetatype.h"
#include "qobject.h"
#include "qmetaobject_p.h"
#include "qmetatype_p.h"

#include <qcoreapplication.h>
#include <qcoreevent.h>
#include <qdatastream.h>
#include <qstringlist.h>
#include <qthread.h>
#include <qvariant.h>
#include <qdebug.h>
#if QT_CONFIG(thread)
#include <qsemaphore.h>
#endif

#include "private/qobject_p.h"
#include "private/qmetaobject_p.h"
#include "private/qthread_p.h"

// for normalizeTypeInternal
#include "private/qmetaobject_moc_p.h"

#include <ctype.h>
#include <memory>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
    \class QMetaObject
    \inmodule QtCore

    \brief The QMetaObject class contains meta-information about Qt
    objects.

    \ingroup objectmodel

    The Qt \l{Meta-Object System} in Qt is responsible for the
    signals and slots inter-object communication mechanism, runtime
    type information, and the Qt property system. A single
    QMetaObject instance is created for each QObject subclass that is
    used in an application, and this instance stores all the
    meta-information for the QObject subclass. This object is
    available as QObject::metaObject().

    This class is not normally required for application programming,
    but it is useful if you write meta-applications, such as scripting
    engines or GUI builders.

    The functions you are most likely to find useful are these:
    \list
    \li className() returns the name of a class.
    \li superClass() returns the superclass's meta-object.
    \li method() and methodCount() provide information
       about a class's meta-methods (signals, slots and other
       \l{Q_INVOKABLE}{invokable} member functions).
    \li enumerator() and enumeratorCount() and provide information about
       a class's enumerators.
    \li propertyCount() and property() provide information about a
       class's properties.
    \li constructor() and constructorCount() provide information
       about a class's meta-constructors.
    \endlist

    The index functions indexOfConstructor(), indexOfMethod(),
    indexOfEnumerator(), and indexOfProperty() map names of constructors,
    member functions, enumerators, or properties to indexes in the
    meta-object. For example, Qt uses indexOfMethod() internally when you
    connect a signal to a slot.

    Classes can also have a list of \e{name}--\e{value} pairs of
    additional class information, stored in QMetaClassInfo objects.
    The number of pairs is returned by classInfoCount(), single pairs
    are returned by classInfo(), and you can search for pairs with
    indexOfClassInfo().

    \note Operations that use the meta object system are generally thread-
    safe, as QMetaObjects are typically static read-only instances
    generated at compile time. However, if meta objects are dynamically
    modified by the application (for instance, when using QQmlPropertyMap),
    then the application has to explicitly synchronize access to the
    respective meta object.

    \sa QMetaClassInfo, QMetaEnum, QMetaMethod, QMetaProperty, QMetaType,
        {Meta-Object System}
*/

/*!
    \enum QMetaObject::Call

    \internal

    \value InvokeMetaMethod
    \value ReadProperty
    \value WriteProperty
    \value ResetProperty
    \value CreateInstance
    \value IndexOfMethod
    \value RegisterPropertyMetaType
    \value RegisterMethodArgumentMetaType
    \value BindableProperty
    \value CustomCall
    \value ConstructInPlace
*/

/*!
    \enum QMetaMethod::Access

    This enum describes the access level of a method, following the conventions used in C++.

    \value Private
    \value Protected
    \value Public
*/

static inline const QMetaObjectPrivate *priv(const uint* data)
{ return reinterpret_cast<const QMetaObjectPrivate*>(data); }

static inline const char *rawStringData(const QMetaObject *mo, int index)
{
    Q_ASSERT(priv(mo->d.data)->revision >= 7);
    uint offset = mo->d.stringdata[2*index];
    return reinterpret_cast<const char *>(mo->d.stringdata) + offset;
}

static inline QLatin1StringView stringDataView(const QMetaObject *mo, int index)
{
    Q_ASSERT(priv(mo->d.data)->revision >= 7);
    uint offset = mo->d.stringdata[2*index];
    uint length = mo->d.stringdata[2*index + 1];
    const char *string = reinterpret_cast<const char *>(mo->d.stringdata) + offset;
    return {string, qsizetype(length)};
}

static inline QByteArray stringData(const QMetaObject *mo, int index)
{
    const auto view = stringDataView(mo, index);
    return QByteArray::fromRawData(view.data(), view.size());
}

static inline const char *rawTypeNameFromTypeInfo(const QMetaObject *mo, uint typeInfo)
{
    if (typeInfo & IsUnresolvedType) {
        return rawStringData(mo, typeInfo & TypeNameIndexMask);
    } else {
        return QMetaType(typeInfo).name();
    }
}

static inline QByteArray typeNameFromTypeInfo(const QMetaObject *mo, uint typeInfo)
{
    if (typeInfo & IsUnresolvedType) {
        return stringData(mo, typeInfo & TypeNameIndexMask);
    } else {
        return QMetaType(typeInfo).name();
    }
}

static inline int typeFromTypeInfo(const QMetaObject *mo, uint typeInfo)
{
    if (!(typeInfo & IsUnresolvedType))
        return typeInfo;
    return QMetaType::fromName(rawStringData(mo, typeInfo & TypeNameIndexMask)).id();
}

namespace {
class QMetaMethodPrivate : public QMetaMethodInvoker
{
public:
    static const QMetaMethodPrivate *get(const QMetaMethod *q)
    { return static_cast<const QMetaMethodPrivate *>(q); }

    inline QByteArray signature() const;
    inline QByteArray name() const;
    inline int typesDataIndex() const;
    inline const char *rawReturnTypeName() const;
    inline int returnType() const;
    inline int parameterCount() const;
    inline int parametersDataIndex() const;
    inline uint parameterTypeInfo(int index) const;
    inline int parameterType(int index) const;
    inline void getParameterTypes(int *types) const;
    inline const QtPrivate::QMetaTypeInterface *returnMetaTypeInterface() const;
    inline const QtPrivate::QMetaTypeInterface *const *parameterMetaTypeInterfaces() const;
    inline QByteArray parameterTypeName(int index) const;
    inline QList<QByteArray> parameterTypes() const;
    inline QList<QByteArray> parameterNames() const;
    inline QByteArray tag() const;
    inline int ownMethodIndex() const;
    inline int ownConstructorMethodIndex() const;

private:
    void checkMethodMetaTypeConsistency(const QtPrivate::QMetaTypeInterface *iface, int index) const;
    QMetaMethodPrivate();
};
} // unnamed namespace

enum { MaximumParamCount = 11 }; // up to 10 arguments + 1 return value

#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
/*!
    \since 4.5
    \obsolete [6.5] Please use the variadic overload of this function

    Constructs a new instance of this class. You can pass up to ten arguments
    (\a val0, \a val1, \a val2, \a val3, \a val4, \a val5, \a val6, \a val7,
    \a val8, and \a val9) to the constructor. Returns the new object, or
    \nullptr if no suitable constructor is available.

    Note that only constructors that are declared with the Q_INVOKABLE
    modifier are made available through the meta-object system.

    \sa Q_ARG(), constructor()
*/
QObject *QMetaObject::newInstance(QGenericArgument val0,
                                  QGenericArgument val1,
                                  QGenericArgument val2,
                                  QGenericArgument val3,
                                  QGenericArgument val4,
                                  QGenericArgument val5,
                                  QGenericArgument val6,
                                  QGenericArgument val7,
                                  QGenericArgument val8,
                                  QGenericArgument val9) const
{
    const char *typeNames[] = {
        nullptr,
        val0.name(), val1.name(), val2.name(), val3.name(), val4.name(),
        val5.name(), val6.name(), val7.name(), val8.name(), val9.name()
    };
    const void *parameters[] = {
        nullptr,
        val0.data(), val1.data(), val2.data(), val3.data(), val4.data(),
        val5.data(), val6.data(), val7.data(), val8.data(), val9.data()
    };

    int paramCount;
    for (paramCount = 1; paramCount < MaximumParamCount; ++paramCount) {
        int len = int(qstrlen(typeNames[paramCount]));
        if (len <= 0)
            break;
    }

    return newInstanceImpl(this, paramCount, parameters, typeNames, nullptr);
}
#endif

/*!
    \fn template <typename... Args> QObject *QMetaObject::newInstance(Args &&... arguments) const
    \since 6.5

    Constructs a new instance of this class and returns the new object, or
    \nullptr if no suitable constructor is available. The types of the
    arguments \a arguments will be used to find a matching constructor, and then
    forwarded to it the same way signal-slot connections do.

    Note that only constructors that are declared with the Q_INVOKABLE
    modifier are made available through the meta-object system.

    \sa constructor()
*/

QObject *QMetaObject::newInstanceImpl(const QMetaObject *mobj, qsizetype paramCount,
                                      const void **parameters, const char **typeNames,
                                      const QtPrivate::QMetaTypeInterface **metaTypes)
{
    if (!mobj->inherits(&QObject::staticMetaObject)) {
        qWarning("QMetaObject::newInstance: type %s does not inherit QObject", mobj->className());
        return nullptr;
    }

QT_WARNING_PUSH
#if Q_CC_GNU >= 1200
QT_WARNING_DISABLE_GCC("-Wdangling-pointer")
#endif

    // set the return type
    QObject *returnValue = nullptr;
    QMetaType returnValueMetaType = QMetaType::fromType<decltype(returnValue)>();
    parameters[0] = &returnValue;
    typeNames[0] = returnValueMetaType.name();
    if (metaTypes)
        metaTypes[0] = returnValueMetaType.iface();

QT_WARNING_POP

    // find the constructor
    auto priv = QMetaObjectPrivate::get(mobj);
    for (int i = 0; i < priv->constructorCount; ++i) {
        QMetaMethod m = QMetaMethod::fromRelativeConstructorIndex(mobj, i);
        if (m.parameterCount() != (paramCount - 1))
            continue;

        // attempt to call
        QMetaMethodPrivate::InvokeFailReason r =
                QMetaMethodPrivate::invokeImpl(m, nullptr, Qt::DirectConnection, paramCount,
                                               parameters, typeNames, metaTypes);
        if (r == QMetaMethodPrivate::InvokeFailReason::None)
            return returnValue;
        if (int(r) < 0)
            return nullptr;
    }

    return returnValue;
}

/*!
    \internal
*/
int QMetaObject::static_metacall(Call cl, int idx, void **argv) const
{
    Q_ASSERT(priv(d.data)->revision >= 6);
    if (!d.static_metacall)
        return 0;
    d.static_metacall(nullptr, cl, idx, argv);
    return -1;
}

/*!
    \internal
*/
int QMetaObject::metacall(QObject *object, Call cl, int idx, void **argv)
{
    if (object->d_ptr->metaObject)
        return object->d_ptr->metaObject->metaCall(object, cl, idx, argv);
    else
        return object->qt_metacall(cl, idx, argv);
}

static inline const char *objectClassName(const QMetaObject *m)
{
    return rawStringData(m, priv(m->d.data)->className);
}

/*!
    Returns the class name.

    \sa superClass()
*/
const char *QMetaObject::className() const
{
    return objectClassName(this);
}

/*!
    \fn QMetaObject *QMetaObject::superClass() const

    Returns the meta-object of the superclass, or \nullptr if there is
    no such object.

    \sa className()
*/

/*!
    Returns \c true if the class described by this QMetaObject inherits
    the type described by \a metaObject; otherwise returns false.

    A type is considered to inherit itself.

    \since 5.7
*/
bool QMetaObject::inherits(const QMetaObject *metaObject) const noexcept
{
    const QMetaObject *m = this;
    do {
        if (metaObject == m)
            return true;
    } while ((m = m->d.superdata));
    return false;
}

/*!
    \fn QObject *QMetaObject::cast(QObject *obj) const
    \internal

    Returns \a obj if object \a obj inherits from this
    meta-object; otherwise returns \nullptr.
*/

/*!
    \internal

    Returns \a obj if object \a obj inherits from this
    meta-object; otherwise returns \nullptr.
*/
const QObject *QMetaObject::cast(const QObject *obj) const
{
    return (obj && obj->metaObject()->inherits(this)) ? obj : nullptr;
}

#ifndef QT_NO_TRANSLATION
/*!
    \internal
*/
QString QMetaObject::tr(const char *s, const char *c, int n) const
{
    return QCoreApplication::translate(objectClassName(this), s, c, n);
}
#endif // QT_NO_TRANSLATION

/*!
    \since 6.2
    Returns the metatype corresponding to this metaobject.
    If the metaobject originates from a namespace, an invalid metatype is returned.
 */
QMetaType QMetaObject::metaType() const
{

    const QMetaObjectPrivate *d = priv(this->d.data);
    if (d->revision < 10) {
        // before revision 10, we did not store the metatype in the metatype array
        return QMetaType::fromName(className());
    } else {
        /* in the metatype array, we store

         | index                               | data                           |
         |----------------------------------------------------------------------|
         | 0                                   | QMetaType(property0)           |
         | ...                                 | ...                            |
         | propertyCount - 1                   | QMetaType(propertyCount - 1)   |
         | propertyCount                       | QMetaType(enumerator0)         |
         | ...                                 | ...                            |
         | propertyCount + enumeratorCount - 1 | QMetaType(enumeratorCount - 1) |
         | propertyCount + enumeratorCount     | QMetaType(class)               |

        */
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
        // Before revision 12 we only stored metatypes for enums if they showed
        // up as types of properties or method arguments or return values.
        // From revision 12 on, we always store them in a predictable place.
        const qsizetype offset = d->revision < 12
                ? d->propertyCount
                : d->propertyCount + d->enumeratorCount;
#else
        const qsizetype offset = d->propertyCount + d->enumeratorCount;
#endif

        auto iface = this->d.metaTypes[offset];
        if (iface && QtMetaTypePrivate::isInterfaceFor<void>(iface))
            return QMetaType(); // return invalid meta-type for namespaces
        if (iface)
            return QMetaType(iface);
        else // in case of a dynamic metaobject, we might have no metatype stored
            return QMetaType::fromName(className()); // try lookup by name in that case
    }
}

/*!
    Returns the method offset for this class; i.e. the index position
    of this class's first member function.

    The offset is the sum of all the methods in the class's
    superclasses (which is always positive since QObject has the
    deleteLater() slot and a destroyed() signal).

    \sa method(), methodCount(), indexOfMethod()
*/
int QMetaObject::methodOffset() const
{
    int offset = 0;
    const QMetaObject *m = d.superdata;
    while (m) {
        offset += priv(m->d.data)->methodCount;
        m = m->d.superdata;
    }
    return offset;
}


/*!
    Returns the enumerator offset for this class; i.e. the index
    position of this class's first enumerator.

    If the class has no superclasses with enumerators, the offset is
    0; otherwise the offset is the sum of all the enumerators in the
    class's superclasses.

    \sa enumerator(), enumeratorCount(), indexOfEnumerator()
*/
int QMetaObject::enumeratorOffset() const
{
    int offset = 0;
    const QMetaObject *m = d.superdata;
    while (m) {
        offset += priv(m->d.data)->enumeratorCount;
        m = m->d.superdata;
    }
    return offset;
}

/*!
    Returns the property offset for this class; i.e. the index
    position of this class's first property.

    The offset is the sum of all the properties in the class's
    superclasses (which is always positive since QObject has the
    name() property).

    \sa property(), propertyCount(), indexOfProperty()
*/
int QMetaObject::propertyOffset() const
{
    int offset = 0;
    const QMetaObject *m = d.superdata;
    while (m) {
        offset += priv(m->d.data)->propertyCount;
        m = m->d.superdata;
    }
    return offset;
}

/*!
    Returns the class information offset for this class; i.e. the
    index position of this class's first class information item.

    If the class has no superclasses with class information, the
    offset is 0; otherwise the offset is the sum of all the class
    information items in the class's superclasses.

    \sa classInfo(), classInfoCount(), indexOfClassInfo()
*/
int QMetaObject::classInfoOffset() const
{
    int offset = 0;
    const QMetaObject *m = d.superdata;
    while (m) {
        offset += priv(m->d.data)->classInfoCount;
        m = m->d.superdata;
    }
    return offset;
}

/*!
    \since 4.5

    Returns the number of constructors in this class.

    \sa constructor(), indexOfConstructor()
*/
int QMetaObject::constructorCount() const
{
    Q_ASSERT(priv(d.data)->revision >= 2);
    return priv(d.data)->constructorCount;
}

/*!
    Returns the number of methods in this class, including the number of
    methods provided by each base class. These include signals and slots
    as well as normal member functions.

    Use code like the following to obtain a QStringList containing the methods
    specific to a given class:

    \snippet code/src_corelib_kernel_qmetaobject.cpp methodCount

    \sa method(), methodOffset(), indexOfMethod()
*/
int QMetaObject::methodCount() const
{
    int n = priv(d.data)->methodCount;
    const QMetaObject *m = d.superdata;
    while (m) {
        n += priv(m->d.data)->methodCount;
        m = m->d.superdata;
    }
    return n;
}

/*!
    Returns the number of enumerators in this class.

    \sa enumerator(), enumeratorOffset(), indexOfEnumerator()
*/
int QMetaObject::enumeratorCount() const
{
    int n = priv(d.data)->enumeratorCount;
    const QMetaObject *m = d.superdata;
    while (m) {
        n += priv(m->d.data)->enumeratorCount;
        m = m->d.superdata;
    }
    return n;
}

/*!
    Returns the number of properties in this class, including the number of
    properties provided by each base class.

    Use code like the following to obtain a QStringList containing the properties
    specific to a given class:

    \snippet code/src_corelib_kernel_qmetaobject.cpp propertyCount

    \sa property(), propertyOffset(), indexOfProperty()
*/
int QMetaObject::propertyCount() const
{
    int n = priv(d.data)->propertyCount;
    const QMetaObject *m = d.superdata;
    while (m) {
        n += priv(m->d.data)->propertyCount;
        m = m->d.superdata;
    }
    return n;
}

/*!
    Returns the number of items of class information in this class.

    \sa classInfo(), classInfoOffset(), indexOfClassInfo()
*/
int QMetaObject::classInfoCount() const
{
    int n = priv(d.data)->classInfoCount;
    const QMetaObject *m = d.superdata;
    while (m) {
        n += priv(m->d.data)->classInfoCount;
        m = m->d.superdata;
    }
    return n;
}

// Returns \c true if the method defined by the given meta-object&meta-method
// matches the given name, argument count and argument types, otherwise
// returns \c false.
bool QMetaObjectPrivate::methodMatch(const QMetaObject *m, const QMetaMethod &method,
                        const QByteArray &name, int argc,
                        const QArgumentType *types)
{
    const QMetaMethod::Data &data = method.data;
    auto priv = QMetaMethodPrivate::get(&method);
    if (priv->parameterCount() != argc)
        return false;

    if (stringData(m, data.name()) != name)
        return false;

    const QtPrivate::QMetaTypeInterface * const *ifaces = priv->parameterMetaTypeInterfaces();
    int paramsIndex = data.parameters() + 1;
    for (int i = 0; i < argc; ++i) {
        uint typeInfo = m->d.data[paramsIndex + i];
        if (int id = types[i].type()) {
            if (id == QMetaType(ifaces[i]).id())
                continue;
            if (id != typeFromTypeInfo(m, typeInfo))
                return false;
        } else {
            if (types[i].name() == QMetaType(ifaces[i]).name())
                continue;
            if (types[i].name() != typeNameFromTypeInfo(m, typeInfo))
                return false;
        }
    }

    return true;
}

/*!
   \internal
   Returns the first method with name \a name found in \a baseObject
 */
QMetaMethod QMetaObjectPrivate::firstMethod(const QMetaObject *baseObject, QByteArrayView name)
{
    for (const QMetaObject *currentObject = baseObject; currentObject; currentObject = currentObject->superClass()) {
        const int start = priv(currentObject->d.data)->methodCount - 1;
        const int end = 0;
        for (int i = start; i >= end; --i) {
            auto candidate = QMetaMethod::fromRelativeMethodIndex(currentObject, i);
            if (name == candidate.name())
                return candidate;
        }
    }
    return QMetaMethod{};
}

/**
* \internal
* helper function for indexOf{Method,Slot,Signal}, returns the relative index of the method within
* the baseObject
* \a MethodType might be MethodSignal or MethodSlot, or \nullptr to match everything.
*/
template<int MethodType>
inline int QMetaObjectPrivate::indexOfMethodRelative(const QMetaObject **baseObject,
                                        const QByteArray &name, int argc,
                                        const QArgumentType *types)
{
    for (const QMetaObject *m = *baseObject; m; m = m->d.superdata) {
        Q_ASSERT(priv(m->d.data)->revision >= 7);
        int i = (MethodType == MethodSignal)
                 ? (priv(m->d.data)->signalCount - 1) : (priv(m->d.data)->methodCount - 1);
        const int end = (MethodType == MethodSlot)
                        ? (priv(m->d.data)->signalCount) : 0;

        for (; i >= end; --i) {
            auto data = QMetaMethod::fromRelativeMethodIndex(m, i);
            if (methodMatch(m, data, name, argc, types)) {
                *baseObject = m;
                return i;
            }
        }
    }
    return -1;
}


/*!
    \since 4.5

    Finds \a constructor and returns its index; otherwise returns -1.

    Note that the \a constructor has to be in normalized form, as returned
    by normalizedSignature().

    \sa constructor(), constructorCount(), normalizedSignature()
*/
int QMetaObject::indexOfConstructor(const char *constructor) const
{
    Q_ASSERT(priv(d.data)->revision >= 7);
    QArgumentTypeArray types;
    QByteArray name = QMetaObjectPrivate::decodeMethodSignature(constructor, types);
    return QMetaObjectPrivate::indexOfConstructor(this, name, types.size(), types.constData());
}

/*!
    Finds \a method and returns its index; otherwise returns -1.

    Note that the \a method has to be in normalized form, as returned
    by normalizedSignature().

    \sa method(), methodCount(), methodOffset(), normalizedSignature()
*/
int QMetaObject::indexOfMethod(const char *method) const
{
    const QMetaObject *m = this;
    int i;
    Q_ASSERT(priv(m->d.data)->revision >= 7);
    QArgumentTypeArray types;
    QByteArray name = QMetaObjectPrivate::decodeMethodSignature(method, types);
    i = QMetaObjectPrivate::indexOfMethodRelative<0>(&m, name, types.size(), types.constData());
    if (i >= 0)
        i += m->methodOffset();
    return i;
}

// Parses a string of comma-separated types into QArgumentTypes.
// No normalization of the type names is performed.
static void argumentTypesFromString(const char *str, const char *end,
                                    QArgumentTypeArray &types)
{
    Q_ASSERT(str <= end);
    while (str != end) {
        if (!types.isEmpty())
            ++str; // Skip comma
        const char *begin = str;
        int level = 0;
        while (str != end && (level > 0 || *str != ',')) {
            if (*str == '<')
                ++level;
            else if (*str == '>')
                --level;
            ++str;
        }
        QByteArray argType(begin, str - begin);
        argType.replace("QVector<", "QList<");
        types += QArgumentType(std::move(argType));
    }
}

// Given a method \a signature (e.g. "foo(int,double)"), this function
// populates the argument \a types array and returns the method name.
QByteArray QMetaObjectPrivate::decodeMethodSignature(
        const char *signature, QArgumentTypeArray &types)
{
    Q_ASSERT(signature != nullptr);
    const char *lparens = strchr(signature, '(');
    if (!lparens)
        return QByteArray();
    const char *rparens = strrchr(lparens + 1, ')');
    if (!rparens || *(rparens+1))
        return QByteArray();
    int nameLength = lparens - signature;
    argumentTypesFromString(lparens + 1, rparens, types);
    return QByteArray::fromRawData(signature, nameLength);
}

/*!
    Finds \a signal and returns its index; otherwise returns -1.

    This is the same as indexOfMethod(), except that it will return
    -1 if the method exists but isn't a signal.

    Note that the \a signal has to be in normalized form, as returned
    by normalizedSignature().

    \sa indexOfMethod(), normalizedSignature(), method(), methodCount(), methodOffset()
*/
int QMetaObject::indexOfSignal(const char *signal) const
{
    const QMetaObject *m = this;
    int i;
    Q_ASSERT(priv(m->d.data)->revision >= 7);
    QArgumentTypeArray types;
    QByteArray name = QMetaObjectPrivate::decodeMethodSignature(signal, types);
    i = QMetaObjectPrivate::indexOfSignalRelative(&m, name, types.size(), types.constData());
    if (i >= 0)
        i += m->methodOffset();
    return i;
}

/*!
    \internal
    Same as QMetaObject::indexOfSignal, but the result is the local offset to the base object.

    \a baseObject will be adjusted to the enclosing QMetaObject, or \nullptr if the signal is not found
*/
int QMetaObjectPrivate::indexOfSignalRelative(const QMetaObject **baseObject,
                                              const QByteArray &name, int argc,
                                              const QArgumentType *types)
{
    int i = indexOfMethodRelative<MethodSignal>(baseObject, name, argc, types);
#ifndef QT_NO_DEBUG
    const QMetaObject *m = *baseObject;
    if (i >= 0 && m && m->d.superdata) {
        int conflict = indexOfMethod(m->d.superdata, name, argc, types);
        if (conflict >= 0) {
            QMetaMethod conflictMethod = m->d.superdata->method(conflict);
            qWarning("QMetaObject::indexOfSignal: signal %s from %s redefined in %s",
                     conflictMethod.methodSignature().constData(),
                     objectClassName(m->d.superdata), objectClassName(m));
        }
     }
 #endif
     return i;
}

/*!
    Finds \a slot and returns its index; otherwise returns -1.

    This is the same as indexOfMethod(), except that it will return
    -1 if the method exists but isn't a slot.

    \sa indexOfMethod(), method(), methodCount(), methodOffset()
*/
int QMetaObject::indexOfSlot(const char *slot) const
{
    const QMetaObject *m = this;
    int i;
    Q_ASSERT(priv(m->d.data)->revision >= 7);
    QArgumentTypeArray types;
    QByteArray name = QMetaObjectPrivate::decodeMethodSignature(slot, types);
    i = QMetaObjectPrivate::indexOfSlotRelative(&m, name, types.size(), types.constData());
    if (i >= 0)
        i += m->methodOffset();
    return i;
}

// same as indexOfSignalRelative but for slots.
int QMetaObjectPrivate::indexOfSlotRelative(const QMetaObject **m,
                                            const QByteArray &name, int argc,
                                            const QArgumentType *types)
{
    return indexOfMethodRelative<MethodSlot>(m, name, argc, types);
}

int QMetaObjectPrivate::indexOfSignal(const QMetaObject *m, const QByteArray &name,
                                      int argc, const QArgumentType *types)
{
    int i = indexOfSignalRelative(&m, name, argc, types);
    if (i >= 0)
        i += m->methodOffset();
    return i;
}

int QMetaObjectPrivate::indexOfSlot(const QMetaObject *m, const QByteArray &name,
                                    int argc, const QArgumentType *types)
{
    int i = indexOfSlotRelative(&m, name, argc, types);
    if (i >= 0)
        i += m->methodOffset();
    return i;
}

int QMetaObjectPrivate::indexOfMethod(const QMetaObject *m, const QByteArray &name,
                                      int argc, const QArgumentType *types)
{
    int i = indexOfMethodRelative<0>(&m, name, argc, types);
    if (i >= 0)
        i += m->methodOffset();
    return i;
}

int QMetaObjectPrivate::indexOfConstructor(const QMetaObject *m, const QByteArray &name,
                                           int argc, const QArgumentType *types)
{
    for (int i = priv(m->d.data)->constructorCount-1; i >= 0; --i) {
        const QMetaMethod method = QMetaMethod::fromRelativeConstructorIndex(m, i);
        if (methodMatch(m, method, name, argc, types))
            return i;
    }
    return -1;
}

/*!
    \fn int QMetaObjectPrivate::signalOffset(const QMetaObject *m)
    \internal
    \since 5.0

    Returns the signal offset for the class \a m; i.e., the index position
    of the class's first signal.

    Similar to QMetaObject::methodOffset(), but non-signal methods are
    excluded.
*/

/*!
    \internal
    \since 5.0

    Returns the number of signals for the class \a m, including the signals
    for the base class.

    Similar to QMetaObject::methodCount(), but non-signal methods are
    excluded.
*/
int QMetaObjectPrivate::absoluteSignalCount(const QMetaObject *m)
{
    Q_ASSERT(m != nullptr);
    int n = priv(m->d.data)->signalCount;
    for (m = m->d.superdata; m; m = m->d.superdata)
        n += priv(m->d.data)->signalCount;
    return n;
}

/*!
    \internal
    \since 5.0

    Returns the index of the signal method \a m.

    Similar to QMetaMethod::methodIndex(), but non-signal methods are
    excluded.
*/
int QMetaObjectPrivate::signalIndex(const QMetaMethod &m)
{
    if (!m.mobj)
        return -1;
    return QMetaMethodPrivate::get(&m)->ownMethodIndex() + signalOffset(m.mobj);
}

/*!
    \internal
    \since 5.0

    Returns the signal for the given meta-object \a m at \a signal_index.

    It it different from QMetaObject::method(); the index should not include
    non-signal methods.
*/
QMetaMethod QMetaObjectPrivate::signal(const QMetaObject *m, int signal_index)
{
    if (signal_index < 0)
        return QMetaMethod();

    Q_ASSERT(m != nullptr);
    int i = signal_index;
    i -= signalOffset(m);
    if (i < 0 && m->d.superdata)
        return signal(m->d.superdata, signal_index);


    if (i >= 0 && i < priv(m->d.data)->signalCount)
        return QMetaMethod::fromRelativeMethodIndex(m, i);
    return QMetaMethod();
}

/*!
    \internal

    Returns \c true if the \a signalTypes and \a methodTypes are
    compatible; otherwise returns \c false.
*/
bool QMetaObjectPrivate::checkConnectArgs(int signalArgc, const QArgumentType *signalTypes,
                                          int methodArgc, const QArgumentType *methodTypes)
{
    if (signalArgc < methodArgc)
        return false;
    for (int i = 0; i < methodArgc; ++i) {
        if (signalTypes[i] != methodTypes[i])
            return false;
    }
    return true;
}

/*!
    \internal

    Returns \c true if the \a signal and \a method arguments are
    compatible; otherwise returns \c false.
*/
bool QMetaObjectPrivate::checkConnectArgs(const QMetaMethodPrivate *signal,
                                          const QMetaMethodPrivate *method)
{
    if (signal->methodType() != QMetaMethod::Signal)
        return false;
    if (signal->parameterCount() < method->parameterCount())
        return false;
    const QMetaObject *smeta = signal->enclosingMetaObject();
    const QMetaObject *rmeta = method->enclosingMetaObject();
    for (int i = 0; i < method->parameterCount(); ++i) {
        uint sourceTypeInfo = signal->parameterTypeInfo(i);
        uint targetTypeInfo = method->parameterTypeInfo(i);
        if ((sourceTypeInfo & IsUnresolvedType)
            || (targetTypeInfo & IsUnresolvedType)) {
            QByteArray sourceName = typeNameFromTypeInfo(smeta, sourceTypeInfo);
            QByteArray targetName = typeNameFromTypeInfo(rmeta, targetTypeInfo);
            if (sourceName != targetName)
                return false;
        } else {
            int sourceType = typeFromTypeInfo(smeta, sourceTypeInfo);
            int targetType = typeFromTypeInfo(rmeta, targetTypeInfo);
            if (sourceType != targetType)
                return false;
        }
    }
    return true;
}

static const QMetaObject *QMetaObject_findMetaObject(const QMetaObject *self, const char *name)
{
    while (self) {
        if (strcmp(objectClassName(self), name) == 0)
            return self;
        if (self->d.relatedMetaObjects) {
            Q_ASSERT(priv(self->d.data)->revision >= 2);
            const auto *e = self->d.relatedMetaObjects;
            if (e) {
                while (*e) {
                    if (const QMetaObject *m =QMetaObject_findMetaObject((*e), name))
                        return m;
                    ++e;
                }
            }
        }
        self = self->d.superdata;
    }
    return self;
}

/*!
    Finds enumerator \a name and returns its index; otherwise returns
    -1.

    \sa enumerator(), enumeratorCount(), enumeratorOffset()
*/
int QMetaObject::indexOfEnumerator(const char *name) const
{
    const QMetaObject *m = this;
    while (m) {
        const QMetaObjectPrivate *d = priv(m->d.data);
        for (int i = 0; i < d->enumeratorCount; ++i) {
            const QMetaEnum e(m, i);
            const char *prop = rawStringData(m, e.data.name());
            if (strcmp(name, prop) == 0) {
                i += m->enumeratorOffset();
                return i;
            }
        }
        m = m->d.superdata;
    }
    // Check alias names:
    m = this;
    while (m) {
        const QMetaObjectPrivate *d = priv(m->d.data);
        for (int i = 0; i < d->enumeratorCount; ++i) {
            const QMetaEnum e(m, i);
            const char *prop = rawStringData(m, e.data.alias());
            if (strcmp(name, prop) == 0) {
                i += m->enumeratorOffset();
                return i;
            }
        }
        m = m->d.superdata;
    }
    return -1;
}

/*!
    Finds property \a name and returns its index; otherwise returns
    -1.

    \sa property(), propertyCount(), propertyOffset()
*/
int QMetaObject::indexOfProperty(const char *name) const
{
    const QMetaObject *m = this;
    while (m) {
        const QMetaObjectPrivate *d = priv(m->d.data);
        for (int i = 0; i < d->propertyCount; ++i) {
            const QMetaProperty::Data data = QMetaProperty::getMetaPropertyData(m, i);
            const char *prop = rawStringData(m, data.name());
            if (strcmp(name, prop) == 0) {
                i += m->propertyOffset();
                return i;
            }
        }
        m = m->d.superdata;
    }

    if (priv(this->d.data)->flags & DynamicMetaObject) {
        QAbstractDynamicMetaObject *me =
            const_cast<QAbstractDynamicMetaObject *>(static_cast<const QAbstractDynamicMetaObject *>(this));

        return me->createProperty(name, nullptr);
    }

    return -1;
}

/*!
    Finds class information item \a name and returns its index;
    otherwise returns -1.

    \sa classInfo(), classInfoCount(), classInfoOffset()
*/
int QMetaObject::indexOfClassInfo(const char *name) const
{
    int i = -1;
    const QMetaObject *m = this;
    while (m && i < 0) {
        for (i = priv(m->d.data)->classInfoCount-1; i >= 0; --i)
            if (strcmp(name, rawStringData(m, m->d.data[priv(m->d.data)->classInfoData + 2*i])) == 0) {
                i += m->classInfoOffset();
                break;
            }
        m = m->d.superdata;
    }
    return i;
}

/*!
    \since 4.5

    Returns the meta-data for the constructor with the given \a index.

    \sa constructorCount(), newInstance()
*/
QMetaMethod QMetaObject::constructor(int index) const
{
    int i = index;
    if (i >= 0 && i < priv(d.data)->constructorCount)
        return QMetaMethod::fromRelativeConstructorIndex(this, i);
    return QMetaMethod();
}

/*!
    Returns the meta-data for the method with the given \a index.

    \sa methodCount(), methodOffset(), indexOfMethod()
*/
QMetaMethod QMetaObject::method(int index) const
{
    int i = index;
    i -= methodOffset();
    if (i < 0 && d.superdata)
        return d.superdata->method(index);

    if (i >= 0 && i < priv(d.data)->methodCount)
        return QMetaMethod::fromRelativeMethodIndex(this, i);
    return QMetaMethod();
}

/*!
    Returns the meta-data for the enumerator with the given \a index.

    \sa enumeratorCount(), enumeratorOffset(), indexOfEnumerator()
*/
QMetaEnum QMetaObject::enumerator(int index) const
{
    int i = index;
    i -= enumeratorOffset();
    if (i < 0 && d.superdata)
        return d.superdata->enumerator(index);

    if (i >= 0 && i < priv(d.data)->enumeratorCount)
        return QMetaEnum(this, i);
    return QMetaEnum();
}

/*!
    Returns the meta-data for the property with the given \a index.
    If no such property exists, a null QMetaProperty is returned.

    \sa propertyCount(), propertyOffset(), indexOfProperty()
*/
QMetaProperty QMetaObject::property(int index) const
{
    int i = index;
    i -= propertyOffset();
    if (i < 0 && d.superdata)
        return d.superdata->property(index);

    if (i >= 0 && i < priv(d.data)->propertyCount)
        return QMetaProperty(this, i);
    return QMetaProperty();
}

/*!
    \since 4.2

    Returns the property that has the \c USER flag set to true.

    \sa QMetaProperty::isUser()
*/
QMetaProperty QMetaObject::userProperty() const
{
    const int propCount = propertyCount();
    for (int i = propCount - 1; i >= 0; --i) {
        const QMetaProperty prop = property(i);
        if (prop.isUser())
            return prop;
    }
    return QMetaProperty();
}

/*!
    Returns the meta-data for the item of class information with the
    given \a index.

    Example:

    \snippet code/src_corelib_kernel_qmetaobject.cpp 0

    \sa classInfoCount(), classInfoOffset(), indexOfClassInfo()
 */
QMetaClassInfo QMetaObject::classInfo(int index) const
{
    int i = index;
    i -= classInfoOffset();
    if (i < 0 && d.superdata)
        return d.superdata->classInfo(index);

    QMetaClassInfo result;
    if (i >= 0 && i < priv(d.data)->classInfoCount) {
        result.mobj = this;
        result.data = { d.data + priv(d.data)->classInfoData + i * QMetaClassInfo::Data::Size };
    }
    return result;
}

/*!
    Returns \c true if the \a signal and \a method arguments are
    compatible; otherwise returns \c false.

    Both \a signal and \a method are expected to be normalized.

    \sa normalizedSignature()
*/
bool QMetaObject::checkConnectArgs(const char *signal, const char *method)
{
    const char *s1 = signal;
    const char *s2 = method;
    while (*s1++ != '(') { }                        // scan to first '('
    while (*s2++ != '(') { }
    if (*s2 == ')' || qstrcmp(s1,s2) == 0)        // method has no args or
        return true;                                //   exact match
    const auto s1len = qstrlen(s1);
    const auto s2len = qstrlen(s2);
    if (s2len < s1len && strncmp(s1,s2,s2len-1)==0 && s1[s2len-1]==',')
        return true;                                // method has less args
    return false;
}

/*!
    \since 5.0
    \overload

    Returns \c true if the \a signal and \a method arguments are
    compatible; otherwise returns \c false.
*/
bool QMetaObject::checkConnectArgs(const QMetaMethod &signal,
                                   const QMetaMethod &method)
{
    return QMetaObjectPrivate::checkConnectArgs(
            QMetaMethodPrivate::get(&signal),
            QMetaMethodPrivate::get(&method));
}

static void qRemoveWhitespace(const char *s, char *d)
{
    char last = 0;
    while (*s && is_space(*s))
        s++;
    while (*s) {
        while (*s && !is_space(*s))
            last = *d++ = *s++;
        while (*s && is_space(*s))
            s++;
        if (*s && ((is_ident_char(*s) && is_ident_char(last))
                   || ((*s == ':') && (last == '<')))) {
            last = *d++ = ' ';
        }
    }
    *d = '\0';
}

static char *qNormalizeType(char *d, int &templdepth, QByteArray &result)
{
    const char *t = d;
    while (*d && (templdepth
                   || (*d != ',' && *d != ')'))) {
        if (*d == '<')
            ++templdepth;
        if (*d == '>')
            --templdepth;
        ++d;
    }
    // "void" should only be removed if this is part of a signature that has
    // an explicit void argument; e.g., "void foo(void)" --> "void foo()"
    if (strncmp("void)", t, d - t + 1) != 0)
        result += normalizeTypeInternal(t, d);

    return d;
}


/*!
    \since 4.2

    Normalizes a \a type.

    See QMetaObject::normalizedSignature() for a description on how
    Qt normalizes.

    Example:

    \snippet code/src_corelib_kernel_qmetaobject.cpp 1

    \sa normalizedSignature()
 */
QByteArray QMetaObject::normalizedType(const char *type)
{
    return normalizeTypeInternal(type, type + qstrlen(type));
}

/*!
    Normalizes the signature of the given \a method.

    Qt uses normalized signatures to decide whether two given signals
    and slots are compatible. Normalization reduces whitespace to a
    minimum, moves 'const' to the front where appropriate, removes
    'const' from value types and replaces const references with
    values.

    \sa checkConnectArgs(), normalizedType()
 */
QByteArray QMetaObject::normalizedSignature(const char *method)
{
    QByteArray result;
    if (!method || !*method)
        return result;
    int len = int(strlen(method));
    QVarLengthArray<char> stackbuf(len + 1);
    char *d = stackbuf.data();
    qRemoveWhitespace(method, d);

    result.reserve(len);

    int argdepth = 0;
    int templdepth = 0;
    while (*d) {
        if (argdepth == 1) {
            d = qNormalizeType(d, templdepth, result);
            if (!*d) //most likely an invalid signature.
                break;
        }
        if (*d == '(')
            ++argdepth;
        if (*d == ')')
            --argdepth;
        result += *d++;
    }

    return result;
}

Q_DECL_COLD_FUNCTION static inline bool
printMethodNotFoundWarning(const QMetaObject *meta, QLatin1StringView name, qsizetype paramCount,
                           const char *const *names,
                           const QtPrivate::QMetaTypeInterface * const *metaTypes)
{
    // now find the candidates we couldn't use
    QByteArray candidateMessage;
    for (int i = 0; i < meta->methodCount(); ++i) {
        const QMetaMethod method = meta->method(i);
        if (method.name() == QByteArrayView(name))
            candidateMessage += "    " + method.methodSignature() + '\n';
    }
    if (!candidateMessage.isEmpty()) {
        candidateMessage.prepend("\nCandidates are:\n");
        candidateMessage.chop(1);
    }

    QVarLengthArray<char, 512> sig;
    for (qsizetype i = 1; i < paramCount; ++i) {
        if (names[i])
            sig.append(names[i], qstrlen(names[i]));
        else
            sig.append(metaTypes[i]->name, qstrlen(metaTypes[i]->name));
        sig.append(',');
    }
    if (paramCount != 1)
        sig.resize(sig.size() - 1);

    qWarning("QMetaObject::invokeMethod: No such method %s::%.*s(%.*s)%.*s",
             meta->className(), int(name.size()), name.constData(),
             int(sig.size()), sig.constData(),
             int(candidateMessage.size()), candidateMessage.constData());
    return false;
}

/*!
    \fn template <typename... Args> bool QMetaObject::invokeMethod(QObject *obj, const char *member, Qt::ConnectionType type, QMetaMethodReturnArgument ret, Args &&... args)
    \fn template <typename... Args> bool QMetaObject::invokeMethod(QObject *obj, const char *member, QMetaMethodReturnArgument ret, Args &&... args)
    \fn template <typename... Args> bool QMetaObject::invokeMethod(QObject *obj, const char *member, Qt::ConnectionType type, Args &&... args)
    \fn template <typename... Args> bool QMetaObject::invokeMethod(QObject *obj, const char *member, Args &&... args)
    \since 6.5
    \threadsafe

    Invokes the \a member (a signal or a slot name) on the object \a
    obj. Returns \c true if the member could be invoked. Returns \c false
    if there is no such member or the parameters did not match.

    For the overloads with a QMetaMethodReturnArgument parameter, the return
    value of the \a member function call is placed in \a ret. For the overloads
    without such a member, the return value of the called function (if any)
    will be discarded. QMetaMethodReturnArgument is an internal type you should
    not use directly. Instead, use the qReturnArg() function.

    The overloads with a Qt::ConnectionType \a type parameter allow explicitly
    selecting whether the invocation will be synchronous or not:

    \list
    \li If \a type is Qt::DirectConnection, the member will be invoked immediately
       in the current thread.

    \li If \a type is Qt::QueuedConnection, a QEvent will be sent and the
       member is invoked as soon as the application enters the event loop in the
       thread that the \a obj was created in or was moved to.

    \li If \a type is Qt::BlockingQueuedConnection, the method will be invoked in
       the same way as for Qt::QueuedConnection, except that the current thread
       will block until the event is delivered. Using this connection type to
       communicate between objects in the same thread will lead to deadlocks.

    \li If \a type is Qt::AutoConnection, the member is invoked synchronously
        if \a obj lives in the same thread as the caller; otherwise it will invoke
        the member asynchronously. This is the behavior of the overloads that do
        not have the \a type parameter.
    \endlist

    You only need to pass the name of the signal or slot to this function,
    not the entire signature. For example, to asynchronously invoke
    the \l{QThread::quit()}{quit()} slot on a
    QThread, use the following code:

    \snippet code/src_corelib_kernel_qmetaobject.cpp 2

    With asynchronous method invocations, the parameters must be copyable
    types, because Qt needs to copy the arguments to store them in an event
    behind the scenes. Since Qt 6.5, this function automatically registers the
    types being used; however, as a side-effect, it is not possible to make
    calls using types that are only forward-declared. Additionally, it is not
    possible to make asynchronous calls that use references to
    non-const-qualified types as parameters either.

    To synchronously invoke the \c compute(QString, int, double) slot on
    some arbitrary object \c obj retrieve its return value:

    \snippet code/src_corelib_kernel_qmetaobject.cpp invokemethod-no-macro

    If the "compute" slot does not take exactly one \l QString, one \c int, and
    one \c double in the specified order, the call will fail. Note how it was
    necessary to be explicit about the type of the QString, as the character
    literal is not exactly the right type to match. If the method instead took
    a \l QStringView, a \l qsizetype, and a \c float, the call would need to be
    written as:

    \snippet code/src_corelib_kernel_qmetaobject.cpp invokemethod-no-macro-other-types

    The same call can be executed using the Q_ARG() and Q_RETURN_ARG() macros,
    as in:

    \snippet code/src_corelib_kernel_qmetaobject.cpp 4

    The macros are kept for compatibility with Qt 6.4 and earlier versions, and
    can be freely mixed with parameters that do not use the macro. They may be
    necessary in rare situations when calling a method that used a typedef to
    forward-declared type as a parameter or the return type.

    \sa Q_ARG(), Q_RETURN_ARG(), QMetaMethod::invoke()
*/

/*!
    \threadsafe
    \overload
    \obsolete [6.5] Please use the variadic overload of this function

    Invokes the \a member (a signal or a slot name) on the object \a
    obj. Returns \c true if the member could be invoked. Returns \c false
    if there is no such member or the parameters did not match.

    See the variadic invokeMethod() function for more information. This
    function should behave the same way as that one, with the following
    limitations:

    \list
    \li The number of parameters is limited to 10.
    \li Parameter names may need to be an exact string match.
    \li Meta types are not automatically registered.
    \endlist

    With asynchronous method invocations, the parameters must be of
    types that are already known to Qt's meta-object system, because Qt needs
    to copy the arguments to store them in an event behind the
    scenes. If you try to use a queued connection and get the error
    message

    \snippet code/src_corelib_kernel_qmetaobject.cpp 3

    call qRegisterMetaType() to register the data type before you
    call invokeMethod().

    \sa Q_ARG(), Q_RETURN_ARG(), qRegisterMetaType(), QMetaMethod::invoke()
*/
bool QMetaObject::invokeMethod(QObject *obj,
                               const char *member,
                               Qt::ConnectionType type,
                               QGenericReturnArgument ret,
                               QGenericArgument val0,
                               QGenericArgument val1,
                               QGenericArgument val2,
                               QGenericArgument val3,
                               QGenericArgument val4,
                               QGenericArgument val5,
                               QGenericArgument val6,
                               QGenericArgument val7,
                               QGenericArgument val8,
                               QGenericArgument val9)
{
    if (!obj)
        return false;

    const char *typeNames[] = {ret.name(), val0.name(), val1.name(), val2.name(), val3.name(),
                               val4.name(), val5.name(), val6.name(), val7.name(), val8.name(),
                               val9.name()};
    const void *parameters[] = {ret.data(), val0.data(), val1.data(), val2.data(), val3.data(),
                                val4.data(), val5.data(), val6.data(), val7.data(), val8.data(),
                                val9.data()};
    int paramCount;
    for (paramCount = 1; paramCount < MaximumParamCount; ++paramCount) {
        if (qstrlen(typeNames[paramCount]) <= 0)
            break;
    }
    return invokeMethodImpl(obj, member, type, paramCount, parameters, typeNames, nullptr);
}

bool QMetaObject::invokeMethodImpl(QObject *obj, const char *member, Qt::ConnectionType type,
                                   qsizetype paramCount, const void * const *parameters,
                                   const char * const *typeNames,
                                   const QtPrivate::QMetaTypeInterface * const *metaTypes)
{
    if (!obj)
        return false;

    Q_ASSERT(paramCount >= 1);  // includes the return type
    Q_ASSERT(parameters);
    Q_ASSERT(typeNames);

    // find the method
    QLatin1StringView name(member);
    if (name.isEmpty())
        return false;

    const QMetaObject *meta = obj->metaObject();
    for ( ; meta; meta = meta->superClass()) {
        auto priv = QMetaObjectPrivate::get(meta);
        for (int i = 0; i < priv->methodCount; ++i) {
            QMetaMethod m = QMetaMethod::fromRelativeMethodIndex(meta, i);
            if (m.parameterCount() != (paramCount - 1))
                continue;
            if (name != stringDataView(meta, m.data.name()))
                continue;

            // attempt to call
            QMetaMethodPrivate::InvokeFailReason r =
                    QMetaMethodPrivate::invokeImpl(m, obj, type, paramCount, parameters,
                                                   typeNames, metaTypes);
            if (int(r) <= 0)
                return r == QMetaMethodPrivate::InvokeFailReason::None;
        }
    }

    // This method doesn't belong to us; print out a nice warning with candidates.
    return printMethodNotFoundWarning(obj->metaObject(), name, paramCount, typeNames, metaTypes);
}

bool QMetaObject::invokeMethodImpl(QObject *object, QtPrivate::QSlotObjectBase *slotObj,
                                   Qt::ConnectionType type, void *ret)
{
    auto slot = QtPrivate::SlotObjUniquePtr(slotObj);

    if (! object)
        return false;

    Qt::HANDLE currentThreadId = QThread::currentThreadId();
    QThread *objectThread = object->thread();
    bool receiverInSameThread = false;
    if (objectThread)
        receiverInSameThread = currentThreadId == QThreadData::get2(objectThread)->threadId.loadRelaxed();

    if (type == Qt::AutoConnection)
        type = receiverInSameThread ? Qt::DirectConnection : Qt::QueuedConnection;

    void *argv[] = { ret };

    if (type == Qt::DirectConnection) {
        slot->call(object, argv);
    } else if (type == Qt::QueuedConnection) {
        if (argv[0]) {
            qWarning("QMetaObject::invokeMethod: Unable to invoke methods with return values in "
                     "queued connections");
            return false;
        }

        QCoreApplication::postEvent(object, new QMetaCallEvent(std::move(slot), nullptr, -1, 1));
    } else if (type == Qt::BlockingQueuedConnection) {
#if QT_CONFIG(thread)
        if (receiverInSameThread)
            qWarning("QMetaObject::invokeMethod: Dead lock detected");

        QSemaphore semaphore;
        QCoreApplication::postEvent(object, new QMetaCallEvent(std::move(slot), nullptr, -1, argv, &semaphore));
        semaphore.acquire();
#endif // QT_CONFIG(thread)
    } else {
        qWarning("QMetaObject::invokeMethod: Unknown connection type");
        return false;
    }
    return true;
}

/*! \fn bool QMetaObject::invokeMethod(QObject *obj, const char *member,
                                       QGenericReturnArgument ret,
                                       QGenericArgument val0 = QGenericArgument(0),
                                       QGenericArgument val1 = QGenericArgument(),
                                       QGenericArgument val2 = QGenericArgument(),
                                       QGenericArgument val3 = QGenericArgument(),
                                       QGenericArgument val4 = QGenericArgument(),
                                       QGenericArgument val5 = QGenericArgument(),
                                       QGenericArgument val6 = QGenericArgument(),
                                       QGenericArgument val7 = QGenericArgument(),
                                       QGenericArgument val8 = QGenericArgument(),
                                       QGenericArgument val9 = QGenericArgument());
    \threadsafe
    \obsolete [6.5] Please use the variadic overload of this function.
    \overload invokeMethod()

    This overload always invokes the member using the connection type Qt::AutoConnection.
*/

/*! \fn bool QMetaObject::invokeMethod(QObject *obj, const char *member,
                                       Qt::ConnectionType type,
                                       QGenericArgument val0 = QGenericArgument(0),
                                       QGenericArgument val1 = QGenericArgument(),
                                       QGenericArgument val2 = QGenericArgument(),
                                       QGenericArgument val3 = QGenericArgument(),
                                       QGenericArgument val4 = QGenericArgument(),
                                       QGenericArgument val5 = QGenericArgument(),
                                       QGenericArgument val6 = QGenericArgument(),
                                       QGenericArgument val7 = QGenericArgument(),
                                       QGenericArgument val8 = QGenericArgument(),
                                       QGenericArgument val9 = QGenericArgument())

    \threadsafe
    \obsolete [6.5] Please use the variadic overload of this function.
    \overload invokeMethod()

    This overload can be used if the return value of the member is of no interest.
*/

/*!
    \fn bool QMetaObject::invokeMethod(QObject *obj, const char *member,
                                       QGenericArgument val0 = QGenericArgument(0),
                                       QGenericArgument val1 = QGenericArgument(),
                                       QGenericArgument val2 = QGenericArgument(),
                                       QGenericArgument val3 = QGenericArgument(),
                                       QGenericArgument val4 = QGenericArgument(),
                                       QGenericArgument val5 = QGenericArgument(),
                                       QGenericArgument val6 = QGenericArgument(),
                                       QGenericArgument val7 = QGenericArgument(),
                                       QGenericArgument val8 = QGenericArgument(),
                                       QGenericArgument val9 = QGenericArgument())

    \threadsafe
    \obsolete [6.5] Please use the variadic overload of this function.
    \overload invokeMethod()

    This overload invokes the member using the connection type Qt::AutoConnection and
    ignores return values.
*/

/*!
    \fn  template<typename Functor, typename FunctorReturnType> bool QMetaObject::invokeMethod(QObject *context, Functor &&function, Qt::ConnectionType type, FunctorReturnType *ret)
    \fn  template<typename Functor, typename FunctorReturnType> bool QMetaObject::invokeMethod(QObject *context, Functor &&function, FunctorReturnType *ret)

    \since 5.10
    \threadsafe

    Invokes the \a function in the event loop of \a context. \a function can be a functor
    or a pointer to a member function. Returns \c true if the function could be invoked.
    Returns \c false if there is no such function or the parameters did not match.
    The return value of the function call is placed in \a ret.

    If \a type is set, then the function is invoked using that connection type. Otherwise,
    Qt::AutoConnection will be used.
*/

/*!
    \fn QMetaObject::Connection &QMetaObject::Connection::operator=(Connection &&other)

    Move-assigns \a other to this object, and returns a reference.
*/
/*!
    \fn QMetaObject::Connection::Connection(Connection &&o)

    Move-constructs a Connection instance, making it point to the same object
    that \a o was pointing to.
*/

/*!
    \fn QMetaObject::Connection::swap(Connection &other)
    \since 5.15

    Swaps this Connection instance with \a other. This operation is very fast
    and never fails.
*/

/*!
    \class QMetaMethod
    \inmodule QtCore

    \brief The QMetaMethod class provides meta-data about a member
    function.

    \ingroup objectmodel

    A QMetaMethod has a methodType(), a methodSignature(), a list of
    parameterTypes() and parameterNames(), a return typeName(), a
    tag(), and an access() specifier. You can use invoke() to invoke
    the method on an arbitrary QObject.

    \sa QMetaObject, QMetaEnum, QMetaProperty, {Qt's Property System}
*/

/*!
    \enum QMetaMethod::Attributes

    \internal

    \value Compatibility
    \value Cloned
    \value Scriptable
*/

/*!
    \fn bool QMetaMethod::isValid() const
    \since 5.0

    Returns \c true if this method is valid (can be introspected and
    invoked), otherwise returns \c false.
*/

/*! \fn bool QMetaMethod::operator==(const QMetaMethod &m1, const QMetaMethod &m2)
    \since 5.0
    \overload

    Returns \c true if method \a m1 is equal to method \a m2,
    otherwise returns \c false.
*/

/*! \fn bool QMetaMethod::operator!=(const QMetaMethod &m1, const QMetaMethod &m2)
    \since 5.0
    \overload

    Returns \c true if method \a m1 is not equal to method \a m2,
    otherwise returns \c false.
*/

/*!
    \fn const QMetaObject *QMetaMethod::enclosingMetaObject() const
    \internal
*/

/*!
    \enum QMetaMethod::MethodType

    \value Method  The function is a plain member function.
    \value Signal  The function is a signal.
    \value Slot    The function is a slot.
    \value Constructor The function is a constructor.
*/

/*!
    \fn QMetaMethod::QMetaMethod()
    \internal
*/

/*!
    \internal
*/
QMetaMethod QMetaMethod::fromRelativeMethodIndex(const QMetaObject *mobj, int index)
{
    Q_ASSERT(index >= 0 && index < priv(mobj->d.data)->methodCount);
    QMetaMethod m;
    m.mobj = mobj;
    m.data = { mobj->d.data + priv(mobj->d.data)->methodData + index * Data::Size };
    return m;
}

QMetaMethod QMetaMethod::fromRelativeConstructorIndex(const QMetaObject *mobj, int index)
{
    Q_ASSERT(index >= 0 && index < priv(mobj->d.data)->constructorCount);
    QMetaMethod m;
    m.mobj = mobj;
    m.data = { mobj->d.data + priv(mobj->d.data)->constructorData + index * Data::Size };
    return m;
}

/*!
    \macro Q_METAMETHOD_INVOKE_MAX_ARGS
    \relates QMetaMethod

    Equals maximum number of arguments available for
    execution of the method via QMetaMethod::invoke()
 */

QByteArray QMetaMethodPrivate::signature() const
{
    Q_ASSERT(priv(mobj->d.data)->revision >= 7);
    QByteArray result;
    result.reserve(256);
    result += name();
    result += '(';
    QList<QByteArray> argTypes = parameterTypes();
    for (int i = 0; i < argTypes.size(); ++i) {
        if (i)
            result += ',';
        result += argTypes.at(i);
    }
    result += ')';
    return result;
}

QByteArray QMetaMethodPrivate::name() const
{
    Q_ASSERT(priv(mobj->d.data)->revision >= 7);
    return stringData(mobj, data.name());
}

int QMetaMethodPrivate::typesDataIndex() const
{
    Q_ASSERT(priv(mobj->d.data)->revision >= 7);
    return data.parameters();
}

const char *QMetaMethodPrivate::rawReturnTypeName() const
{
    Q_ASSERT(priv(mobj->d.data)->revision >= 7);
    uint typeInfo = mobj->d.data[typesDataIndex()];
    if (typeInfo & IsUnresolvedType)
        return rawStringData(mobj, typeInfo & TypeNameIndexMask);
    else
        return QMetaType(typeInfo).name();
}

int QMetaMethodPrivate::returnType() const
{
    return parameterType(-1);
}

int QMetaMethodPrivate::parameterCount() const
{
    Q_ASSERT(priv(mobj->d.data)->revision >= 7);
    return data.argc();
}

inline void
QMetaMethodPrivate::checkMethodMetaTypeConsistency(const QtPrivate::QMetaTypeInterface *iface,
                                                   int index) const
{
    uint typeInfo = parameterTypeInfo(index);
    QMetaType mt(iface);
    if (iface) {
        if ((typeInfo & IsUnresolvedType) == 0)
            Q_ASSERT(mt.id() == int(typeInfo & TypeNameIndexMask));
        Q_ASSERT(mt.name());
    } else {
        // The iface can only be null for a parameter if that parameter is a
        // const-ref to a forward-declared type. Since primitive types are
        // never incomplete, we can assert it's not one of them.

#define ASSERT_NOT_PRIMITIVE_TYPE(TYPE, METATYPEID, NAME)           \
        Q_ASSERT(typeInfo != QMetaType::TYPE);
        QT_FOR_EACH_STATIC_PRIMITIVE_NON_VOID_TYPE(ASSERT_NOT_PRIMITIVE_TYPE)
#undef ASSERT_NOT_PRIMITIVE_TYPE
        Q_ASSERT(typeInfo != QMetaType::QObjectStar);

        // Prior to Qt 6.4 we failed to record void and void*
        if (priv(mobj->d.data)->revision >= 11) {
            Q_ASSERT(typeInfo != QMetaType::Void);
            Q_ASSERT(typeInfo != QMetaType::VoidStar);
        }
    }
}

int QMetaMethodPrivate::parametersDataIndex() const
{
    Q_ASSERT(priv(mobj->d.data)->revision >= 7);
    return typesDataIndex() + 1;
}

uint QMetaMethodPrivate::parameterTypeInfo(int index) const
{
    Q_ASSERT(priv(mobj->d.data)->revision >= 7);
    return mobj->d.data[parametersDataIndex() + index];
}

const QtPrivate::QMetaTypeInterface *QMetaMethodPrivate::returnMetaTypeInterface() const
{
    Q_ASSERT(priv(mobj->d.data)->revision >= 7);
    if (methodType() == QMetaMethod::Constructor)
        return nullptr;         // constructors don't have return types

    const QtPrivate::QMetaTypeInterface *iface =  mobj->d.metaTypes[data.metaTypeOffset()];
    checkMethodMetaTypeConsistency(iface, -1);
    return iface;
}

const QtPrivate::QMetaTypeInterface * const *QMetaMethodPrivate::parameterMetaTypeInterfaces() const
{
    Q_ASSERT(priv(mobj->d.data)->revision >= 7);
    int offset = (methodType() == QMetaMethod::Constructor ? 0 : 1);
    const auto ifaces = &mobj->d.metaTypes[data.metaTypeOffset() + offset];

    for (int i = 0; i < parameterCount(); ++i)
        checkMethodMetaTypeConsistency(ifaces[i], i);

    return ifaces;
}

int QMetaMethodPrivate::parameterType(int index) const
{
    Q_ASSERT(priv(mobj->d.data)->revision >= 7);
    return typeFromTypeInfo(mobj, parameterTypeInfo(index));
}

void QMetaMethodPrivate::getParameterTypes(int *types) const
{
    Q_ASSERT(priv(mobj->d.data)->revision >= 7);
    int dataIndex = parametersDataIndex();
    int argc = parameterCount();
    for (int i = 0; i < argc; ++i) {
        int id = typeFromTypeInfo(mobj, mobj->d.data[dataIndex++]);
        *(types++) = id;
    }
}

QByteArray QMetaMethodPrivate::parameterTypeName(int index) const
{
    int paramsIndex = parametersDataIndex();
    return typeNameFromTypeInfo(mobj, mobj->d.data[paramsIndex + index]);
}

QList<QByteArray> QMetaMethodPrivate::parameterTypes() const
{
    Q_ASSERT(priv(mobj->d.data)->revision >= 7);
    int argc = parameterCount();
    QList<QByteArray> list;
    list.reserve(argc);
    int paramsIndex = parametersDataIndex();
    for (int i = 0; i < argc; ++i)
        list += typeNameFromTypeInfo(mobj, mobj->d.data[paramsIndex + i]);
    return list;
}

QList<QByteArray> QMetaMethodPrivate::parameterNames() const
{
    Q_ASSERT(priv(mobj->d.data)->revision >= 7);
    int argc = parameterCount();
    QList<QByteArray> list;
    list.reserve(argc);
    int namesIndex = parametersDataIndex() + argc;
    for (int i = 0; i < argc; ++i)
        list += stringData(mobj, mobj->d.data[namesIndex + i]);
    return list;
}

QByteArray QMetaMethodPrivate::tag() const
{
    Q_ASSERT(priv(mobj->d.data)->revision >= 7);
    return stringData(mobj, data.tag());
}

int QMetaMethodPrivate::ownMethodIndex() const
{
    // recompute the methodIndex by reversing the arithmetic in QMetaObject::method()
    return ( data.d - mobj->d.data - priv(mobj->d.data)->methodData)/Data::Size;
}

int QMetaMethodPrivate::ownConstructorMethodIndex() const
{
    // recompute the methodIndex by reversing the arithmetic in QMetaObject::constructor()
    Q_ASSERT(methodType() == Constructor);
    return ( data.d - mobj->d.data - priv(mobj->d.data)->constructorData)/Data::Size;
}

/*!
    \since 5.0

    Returns the signature of this method (e.g.,
    \c{setValue(double)}).

    \sa parameterTypes(), parameterNames()
*/
QByteArray QMetaMethod::methodSignature() const
{
    if (!mobj)
        return QByteArray();
    return QMetaMethodPrivate::get(this)->signature();
}

/*!
    \since 5.0

    Returns the name of this method.

    \sa methodSignature(), parameterCount()
*/
QByteArray QMetaMethod::name() const
{
    if (!mobj)
        return QByteArray();
    return QMetaMethodPrivate::get(this)->name();
}

/*!
    \since 5.0

    Returns the return type of this method.

    The return value is one of the types that are registered
    with QMetaType, or QMetaType::UnknownType if the type is not registered.

    \sa parameterType(), QMetaType, typeName(), returnMetaType()
*/
int QMetaMethod::returnType() const
 {
     return returnMetaType().id();
}

/*!
    \since 6.0

    Returns the return type of this method.
    \sa parameterMetaType(), QMetaType, typeName()
*/
QMetaType QMetaMethod::returnMetaType() const
{
    if (!mobj || methodType() == QMetaMethod::Constructor)
        return QMetaType{};
    auto mt = QMetaType(mobj->d.metaTypes[data.metaTypeOffset()]);
    if (mt.id() == QMetaType::UnknownType)
        return QMetaType(QMetaMethodPrivate::get(this)->returnType());
    else
        return mt;
}

/*!
    \since 5.0

    Returns the number of parameters of this method.

    \sa parameterType(), parameterNames()
*/
int QMetaMethod::parameterCount() const
{
    if (!mobj)
        return 0;
    return QMetaMethodPrivate::get(this)->parameterCount();
}

/*!
    \since 5.0

    Returns the type of the parameter at the given \a index.

    The return value is one of the types that are registered
    with QMetaType, or QMetaType::UnknownType if the type is not registered.

    \sa parameterCount(), parameterMetaType(), returnType(), QMetaType
*/
int QMetaMethod::parameterType(int index) const
{
    return parameterMetaType(index).id();
}

/*!
    \since 6.0

    Returns the metatype of the parameter at the given \a index.

    If the \a index is smaller than zero or larger than
    parameterCount(), an invalid QMetaType is returned.

    \sa parameterCount(), returnMetaType(), QMetaType
*/
QMetaType QMetaMethod::parameterMetaType(int index) const
{
    if (!mobj || index < 0)
        return {};
    auto priv = QMetaMethodPrivate::get(this);
    if (index >= priv->parameterCount())
        return {};
    // + 1 if there exists a return type
    auto parameterOffset = index + (methodType() == QMetaMethod::Constructor ? 0 : 1);
    auto mt = QMetaType(mobj->d.metaTypes[data.metaTypeOffset() + parameterOffset]);
    if (mt.id() == QMetaType::UnknownType)
        return QMetaType(QMetaMethodPrivate::get(this)->parameterType(index));
    else
        return mt;
}

/*!
    \since 5.0
    \internal

    Gets the parameter \a types of this method. The storage
    for \a types must be able to hold parameterCount() items.

    \sa parameterCount(), returnType(), parameterType()
*/
void QMetaMethod::getParameterTypes(int *types) const
{
    if (!mobj)
        return;
    QMetaMethodPrivate::get(this)->getParameterTypes(types);
}

/*!
    Returns a list of parameter types.

    \sa parameterNames(), methodSignature()
*/
QList<QByteArray> QMetaMethod::parameterTypes() const
{
    if (!mobj)
        return QList<QByteArray>();
    return QMetaMethodPrivate::get(this)->parameterTypes();
}

/*!
   \since 6.0
   Returns the name of the type at position \a index
   If there is no parameter at \a index, returns an empty QByteArray

   \sa parameterNames()
 */
QByteArray QMetaMethod::parameterTypeName(int index) const
{
    if (!mobj || index < 0 || index >= parameterCount())
        return {};
    return QMetaMethodPrivate::get(this)->parameterTypeName(index);
}

/*!
    Returns a list of parameter names.

    \sa parameterTypes(), methodSignature()
*/
QList<QByteArray> QMetaMethod::parameterNames() const
{
    if (!mobj)
        return QList<QByteArray>();
    return QMetaMethodPrivate::get(this)->parameterNames();
}


/*!
    Returns the return type name of this method.

    \sa returnType(), QMetaType::type()
*/
const char *QMetaMethod::typeName() const
{
    if (!mobj)
        return nullptr;
    return QMetaMethodPrivate::get(this)->rawReturnTypeName();
}

/*!
    Returns the tag associated with this method.

    Tags are special macros recognized by \c moc that make it
    possible to add extra information about a method.

    Tag information can be added in the following
    way in the function declaration:

    \snippet code/src_corelib_kernel_qmetaobject.cpp 10

    and the information can be accessed by using:

    \snippet code/src_corelib_kernel_qmetaobject.cpp 11

    For the moment, \c moc will extract and record all tags, but it will not
    handle any of them specially. You can use the tags to annotate your methods
    differently, and treat them according to the specific needs of your
    application.

    \note Since Qt 5.0, \c moc expands preprocessor macros, so it is necessary
    to surround the definition with \c #ifndef \c Q_MOC_RUN, as shown in the
    example above. This was not required in Qt 4. The code as shown above works
    with Qt 4 too.
*/
const char *QMetaMethod::tag() const
{
    if (!mobj)
        return nullptr;
    return QMetaMethodPrivate::get(this)->tag().constData();
}


/*!
    \internal
 */
int QMetaMethod::attributes() const
{
    if (!mobj)
        return false;
    return data.flags() >> 4;
}

/*!
  \since 4.6

  Returns this method's index.
*/
int QMetaMethod::methodIndex() const
{
    if (!mobj)
        return -1;
    return QMetaMethodPrivate::get(this)->ownMethodIndex() + mobj->methodOffset();
}

/*!
  \since 6.0

  Returns this method's local index inside.
*/
int QMetaMethod::relativeMethodIndex() const
{
    if (!mobj)
        return -1;
    return QMetaMethodPrivate::get(this)->ownMethodIndex();
}

// This method has been around for a while, but the documentation was marked \internal until 5.1
/*!
    \since 5.1
    Returns the method revision if one was
    specified by Q_REVISION, otherwise returns 0.
 */
int QMetaMethod::revision() const
{
    if (!mobj)
        return 0;
    if (data.flags() & MethodRevisioned) {
        int offset = priv(mobj->d.data)->methodData
                     + priv(mobj->d.data)->methodCount * Data::Size
                     + QMetaMethodPrivate::get(this)->ownMethodIndex();
        return mobj->d.data[offset];
    }
    return 0;
}

/*!
    \since 6.2

    Returns whether the method is const qualified.

    \note This method might erroneously return \c false for a const method
    if it belongs to a library compiled against an older version of Qt.
 */
bool QMetaMethod::isConst() const
{
    if (!mobj)
        return false;
    if (QMetaObjectPrivate::get(mobj)->revision < 10)
        return false;
    return data.flags() & MethodIsConst;
}

/*!
    Returns the access specification of this method (private,
    protected, or public).

    \note Signals are always public, but you should regard that as an
    implementation detail. It is almost always a bad idea to emit a signal from
    outside its class.

    \sa methodType()
*/
QMetaMethod::Access QMetaMethod::access() const
{
    if (!mobj)
        return Private;
    return (QMetaMethod::Access)(data.flags() & AccessMask);
}

/*!
    Returns the type of this method (signal, slot, or method).

    \sa access()
*/
QMetaMethod::MethodType QMetaMethod::methodType() const
{
    if (!mobj)
        return QMetaMethod::Method;
    return (QMetaMethod::MethodType)((data.flags() & MethodTypeMask)>>2);
}

/*!
    \fn  template <typename PointerToMemberFunction> QMetaMethod QMetaMethod::fromSignal(PointerToMemberFunction signal)
    \since 5.0

    Returns the meta-method that corresponds to the given \a signal, or an
    invalid QMetaMethod if \a signal is not a signal of the class.

    Example:

    \snippet code/src_corelib_kernel_qmetaobject.cpp 9
*/

/*!
    \internal

    Implementation of the fromSignal() function.

    \a metaObject is the class's meta-object
    \a signal is a pointer to a pointer to a member signal of the class
*/
QMetaMethod QMetaMethod::fromSignalImpl(const QMetaObject *metaObject, void **signal)
{
    int i = -1;
    void *args[] = { &i, signal };
    for (const QMetaObject *m = metaObject; m; m = m->d.superdata) {
        m->static_metacall(QMetaObject::IndexOfMethod, 0, args);
        if (i >= 0)
            return QMetaMethod::fromRelativeMethodIndex(m, i);
    }
    return QMetaMethod();
}

/*!
    \fn template <typename... Args> bool QMetaMethod::invoke(QObject *obj, Qt::ConnectionType type, QMetaMethodReturnArgument ret, Args &&... arguments) const
    \fn template <typename... Args> bool QMetaMethod::invoke(QObject *obj, Qt::ConnectionType type, Args &&... arguments) const
    \fn template <typename... Args> bool QMetaMethod::invoke(QObject *obj, QMetaMethodReturnArgument ret, Args &&... arguments) const
    \fn template <typename... Args> bool QMetaMethod::invoke(QObject *obj, Args &&... arguments) const
    \since 6.5

    Invokes this method on the object \a object. Returns \c true if the member could be invoked.
    Returns \c false if there is no such member or the parameters did not match.

    For the overloads with a QMetaMethodReturnArgument parameter, the return
    value of the \a member function call is placed in \a ret. For the overloads
    without such a member, the return value of the called function (if any)
    will be discarded. QMetaMethodReturnArgument is an internal type you should
    not use directly. Instead, use the qReturnArg() function.

    The overloads with a Qt::ConnectionType \a type parameter allow explicitly
    selecting whether the invocation will be synchronous or not:

    \list
    \li If \a type is Qt::DirectConnection, the member will be invoked immediately
       in the current thread.

    \li If \a type is Qt::QueuedConnection, a QEvent will be sent and the
       member is invoked as soon as the application enters the event loop in the
       thread the \a obj was created in or was moved to.

    \li If \a type is Qt::BlockingQueuedConnection, the method will be invoked in
       the same way as for Qt::QueuedConnection, except that the current thread
       will block until the event is delivered. Using this connection type to
       communicate between objects in the same thread will lead to deadlocks.

    \li If \a type is Qt::AutoConnection, the member is invoked synchronously
        if \a obj lives in the same thread as the caller; otherwise it will invoke
        the member asynchronously. This is the behavior of the overloads that do
        not have the \a type parameter.
    \endlist

    To asynchronously invoke the
    \l{QPushButton::animateClick()}{animateClick()} slot on a
    QPushButton:

    \snippet code/src_corelib_kernel_qmetaobject.cpp 6

    With asynchronous method invocations, the parameters must be copyable
    types, because Qt needs to copy the arguments to store them in an event
    behind the scenes. Since Qt 6.5, this function automatically registers the
    types being used; however, as a side-effect, it is not possible to make
    calls using types that are only forward-declared. Additionally, it is not
    possible to make asynchronous calls that use references to
    non-const-qualified types as parameters either.

    To synchronously invoke the \c compute(QString, int, double) slot on
    some arbitrary object \c obj retrieve its return value:

    \snippet code/src_corelib_kernel_qmetaobject.cpp invoke-no-macro

    If the "compute" slot does not take exactly one \l QString, one \c int, and
    one \c double in the specified order, the call will fail. Note how it was
    necessary to be explicit about the type of the QString, as the character
    literal is not exactly the right type to match. If the method instead took
    a \l QByteArray, a \l qint64, and a \c{long double}, the call would need to be
    written as:

    \snippet code/src_corelib_kernel_qmetaobject.cpp invoke-no-macro-other-types

    The same call can be executed using the Q_ARG() and Q_RETURN_ARG() macros,
    as in:

    \snippet code/src_corelib_kernel_qmetaobject.cpp 8

    \warning this method will not test the validity of the arguments: \a object
    must be an instance of the class of the QMetaObject of which this QMetaMethod
    has been constructed with.

    \sa Q_ARG(), Q_RETURN_ARG(), qRegisterMetaType(), QMetaObject::invokeMethod()
*/

/*!
    \obsolete [6.5] Please use the variadic overload of this function

    Invokes this method on the object \a object. Returns \c true if the member could be invoked.
    Returns \c false if there is no such member or the parameters did not match.

    See the variadic invokeMethod() function for more information. This
    function should behave the same way as that one, with the following
    limitations:

    \list
    \li The number of parameters is limited to 10.
    \li Parameter names may need to be an exact string match.
    \li Meta types are not automatically registered.
    \endlist

    With asynchronous method invocations, the parameters must be of
    types that are known to Qt's meta-object system, because Qt needs
    to copy the arguments to store them in an event behind the
    scenes. If you try to use a queued connection and get the error
    message

    \snippet code/src_corelib_kernel_qmetaobject.cpp 7

    call qRegisterMetaType() to register the data type before you
    call QMetaMethod::invoke().

    \warning In addition to the limitations of the variadic invoke() overload,
    the arguments must have the same type as the ones expected by the method,
    else, the behavior is undefined.

    \sa Q_ARG(), Q_RETURN_ARG(), qRegisterMetaType(), QMetaObject::invokeMethod()
*/
bool QMetaMethod::invoke(QObject *object,
                         Qt::ConnectionType connectionType,
                         QGenericReturnArgument returnValue,
                         QGenericArgument val0,
                         QGenericArgument val1,
                         QGenericArgument val2,
                         QGenericArgument val3,
                         QGenericArgument val4,
                         QGenericArgument val5,
                         QGenericArgument val6,
                         QGenericArgument val7,
                         QGenericArgument val8,
                         QGenericArgument val9) const
{
    if (!object || !mobj)
        return false;

    // check argument count (we don't allow invoking a method if given too few arguments)
    const char *typeNames[] = {
        returnValue.name(),
        val0.name(),
        val1.name(),
        val2.name(),
        val3.name(),
        val4.name(),
        val5.name(),
        val6.name(),
        val7.name(),
        val8.name(),
        val9.name()
    };
    void *param[] = {
        returnValue.data(),
        val0.data(),
        val1.data(),
        val2.data(),
        val3.data(),
        val4.data(),
        val5.data(),
        val6.data(),
        val7.data(),
        val8.data(),
        val9.data()
    };

    int paramCount;
    for (paramCount = 1; paramCount < MaximumParamCount; ++paramCount) {
        if (qstrlen(typeNames[paramCount]) <= 0)
            break;
    }
    return invokeImpl(*this, object, connectionType, paramCount, param, typeNames, nullptr);
}

bool QMetaMethod::invokeImpl(QMetaMethod self, void *target, Qt::ConnectionType connectionType,
                             qsizetype paramCount, const void *const *parameters,
                             const char *const *typeNames,
                             const QtPrivate::QMetaTypeInterface *const *metaTypes)
{
    if (!target || !self.mobj)
        return false;
    QMetaMethodPrivate::InvokeFailReason r =
            QMetaMethodPrivate::invokeImpl(self, target, connectionType, paramCount, parameters,
                                           typeNames, metaTypes);
    if (Q_LIKELY(r == QMetaMethodPrivate::InvokeFailReason::None))
        return true;

    if (int(r) >= int(QMetaMethodPrivate::InvokeFailReason::FormalParameterMismatch)) {
        int n = int(r) - int(QMetaMethodPrivate::InvokeFailReason::FormalParameterMismatch);
        qWarning("QMetaMethod::invoke: cannot convert formal parameter %d from %s in call to %s::%s",
                 n, typeNames[n + 1] ? typeNames[n + 1] : metaTypes[n + 1]->name,
                 self.mobj->className(), self.methodSignature().constData());
    }
    if (r == QMetaMethodPrivate::InvokeFailReason::TooFewArguments) {
        qWarning("QMetaMethod::invoke: too few arguments (%d) in call to %s::%s",
                 int(paramCount), self.mobj->className(), self.methodSignature().constData());
    }
    return false;
}

auto QMetaMethodInvoker::invokeImpl(QMetaMethod self, void *target,
                                    Qt::ConnectionType connectionType,
                                    qsizetype paramCount, const void *const *parameters,
                                    const char *const *typeNames,
                                    const QtPrivate::QMetaTypeInterface *const *metaTypes) -> InvokeFailReason
{
    auto object = static_cast<QObject *>(target);
    auto priv = QMetaMethodPrivate::get(&self);
    constexpr bool MetaTypesAreOptional = QT_VERSION < QT_VERSION_CHECK(7, 0, 0);
    auto methodMetaTypes = priv->parameterMetaTypeInterfaces();
    auto param = const_cast<void **>(parameters);

    Q_ASSERT(priv->mobj);
    Q_ASSERT(self.methodType() == Constructor || object);
    Q_ASSERT(self.methodType() == Constructor || connectionType == Qt::ConnectionType(-1) ||
             priv->mobj->cast(object));
    Q_ASSERT(paramCount >= 1);  // includes the return type
    Q_ASSERT(parameters);
    Q_ASSERT(typeNames);
    Q_ASSERT(MetaTypesAreOptional || metaTypes);

    if ((paramCount - 1) < qsizetype(priv->data.argc()))
        return InvokeFailReason::TooFewArguments;

    // 0 is the return type, 1 is the first formal parameter
    auto checkTypesAreCompatible = [=](int idx) {
        uint typeInfo = priv->parameterTypeInfo(idx - 1);
        QLatin1StringView userTypeName(typeNames[idx] ? typeNames[idx] : metaTypes[idx]->name);

        if ((typeInfo & IsUnresolvedType) == 0) {
            // this is a built-in type
            if (MetaTypesAreOptional && !metaTypes)
                return int(typeInfo) == QMetaType::fromName(userTypeName).id();
            return int(typeInfo) == metaTypes[idx]->typeId;
        }

        QLatin1StringView methodTypeName = stringDataView(priv->mobj, typeInfo & TypeNameIndexMask);
        if ((MetaTypesAreOptional && !metaTypes) || !metaTypes[idx]) {
            // compatibility call, compare strings
            if (methodTypeName == userTypeName)
                return true;

            // maybe the user type needs normalization
            QByteArray normalized = normalizeTypeInternal(userTypeName.begin(), userTypeName.end());
            return methodTypeName == QLatin1StringView(normalized);
        }

        QMetaType userType(metaTypes[idx]);
        Q_ASSERT(userType.isValid());
        if (QMetaType(methodMetaTypes[idx - 1]) == userType)
            return true;

        // if the parameter type was NOT only forward-declared, it MUST have
        // matched
        if (methodMetaTypes[idx - 1])
            return false;

        // resolve from the name moc stored for us
        QMetaType resolved = QMetaType::fromName(methodTypeName);
        return resolved == userType;
    };

    // force all types to be registered, just in case
    for (qsizetype i = 0; metaTypes && i < paramCount; ++i)
        QMetaType(metaTypes[i]).registerType();

    // check formal parameters first (overload set)
    for (qsizetype i = 1; i < paramCount; ++i) {
        if (!checkTypesAreCompatible(i))
            return InvokeFailReason(int(InvokeFailReason::FormalParameterMismatch) + i - 1);
    }

    // handle constructors first
    if (self.methodType() == Constructor) {
        if (object) {
            qWarning("QMetaMethod::invokeMethod: cannot call constructor %s on object %p",
                     self.methodSignature().constData(), object);
            return InvokeFailReason::ConstructorCallOnObject;
        }

        if (!parameters[0]) {
            qWarning("QMetaMethod::invokeMethod: constructor call to %s must assign a return type",
                     self.methodSignature().constData());
            return InvokeFailReason::ConstructorCallWithoutResult;
        }

        if (!MetaTypesAreOptional || metaTypes) {
            if (metaTypes[0]->typeId != QMetaType::QObjectStar) {
                qWarning("QMetaMethod::invokeMethod: cannot convert QObject* to %s on constructor call %s",
                         metaTypes[0]->name, self.methodSignature().constData());
                return InvokeFailReason::ReturnTypeMismatch;
            }
        }

        int idx = priv->ownConstructorMethodIndex();
        if (priv->mobj->static_metacall(QMetaObject::CreateInstance, idx, param) >= 0)
            return InvokeFailReason::ConstructorCallFailed;
        return {};
    }

    // regular type - check return type
    if (parameters[0]) {
        if (!checkTypesAreCompatible(0)) {
            const char *retType = typeNames[0] ? typeNames[0] : metaTypes[0]->name;
            qWarning("QMetaMethod::invokeMethod: return type mismatch for method %s::%s:"
                     " cannot convert from %s to %s during invocation",
                     priv->mobj->className(), priv->methodSignature().constData(),
                     priv->rawReturnTypeName(), retType);
            return InvokeFailReason::ReturnTypeMismatch;
        }
    }

    Qt::HANDLE currentThreadId = nullptr;
    QThread *objectThread = nullptr;
    auto receiverInSameThread = [&]() {
        if (!currentThreadId) {
            currentThreadId = QThread::currentThreadId();
            objectThread = object->thread();
        }
        if (objectThread)
            return currentThreadId == QThreadData::get2(objectThread)->threadId.loadRelaxed();
        return false;
    };

    // check connection type
    if (connectionType == Qt::AutoConnection)
        connectionType = receiverInSameThread() ? Qt::DirectConnection : Qt::QueuedConnection;
    else if (connectionType == Qt::ConnectionType(-1))
        connectionType = Qt::DirectConnection;

#if !QT_CONFIG(thread)
    if (connectionType == Qt::BlockingQueuedConnection) {
        connectionType = Qt::DirectConnection;
    }
#endif

    // invoke!
    int idx_relative = priv->ownMethodIndex();
    int idx_offset = priv->mobj->methodOffset();
    QObjectPrivate::StaticMetaCallFunction callFunction = priv->mobj->d.static_metacall;

    if (connectionType == Qt::DirectConnection) {
        if (callFunction)
            callFunction(object, QMetaObject::InvokeMetaMethod, idx_relative, param);
        else if (QMetaObject::metacall(object, QMetaObject::InvokeMetaMethod, idx_relative + idx_offset, param) >= 0)
            return InvokeFailReason::CallViaVirtualFailed;
    } else if (connectionType == Qt::QueuedConnection) {
        if (parameters[0]) {
            qWarning("QMetaMethod::invoke: Unable to invoke methods with return values in "
                     "queued connections");
            return InvokeFailReason::CouldNotQueueParameter;
        }

        auto event = std::make_unique<QMetaCallEvent>(idx_offset, idx_relative, callFunction, nullptr, -1, paramCount);
        QMetaType *types = event->types();
        void **args = event->args();

        // fill in the meta types first
        for (int i = 1; i < paramCount; ++i) {
            types[i] = QMetaType(methodMetaTypes[i - 1]);
            if (!types[i].iface() && (!MetaTypesAreOptional || metaTypes))
                types[i] = QMetaType(metaTypes[i]);
            if (!types[i].iface())
                types[i] = priv->parameterMetaType(i - 1);
            if (!types[i].iface() && typeNames[i])
                types[i] = QMetaType::fromName(typeNames[i]);
            if (!types[i].iface()) {
                qWarning("QMetaMethod::invoke: Unable to handle unregistered datatype '%s'",
                         typeNames[i]);
                return InvokeFailReason(int(InvokeFailReason::CouldNotQueueParameter) - i);
            }
        }

        // now create copies of our parameters using those meta types
        for (int i = 1; i < paramCount; ++i)
            args[i] = types[i].create(parameters[i]);

        QCoreApplication::postEvent(object, event.release());
    } else { // blocking queued connection
#if QT_CONFIG(thread)
        if (receiverInSameThread()) {
            qWarning("QMetaMethod::invoke: Dead lock detected in BlockingQueuedConnection: "
                     "Receiver is %s(%p)", priv->mobj->className(), object);
            return InvokeFailReason::DeadLockDetected;
        }

        QSemaphore semaphore;
        QCoreApplication::postEvent(object, new QMetaCallEvent(idx_offset, idx_relative, callFunction,
                                                        nullptr, -1, param, &semaphore));
        semaphore.acquire();
#endif // QT_CONFIG(thread)
    }
    return {};
}

/*! \fn bool QMetaMethod::invoke(QObject *object,
                                 QGenericReturnArgument returnValue,
                                 QGenericArgument val0 = QGenericArgument(0),
                                 QGenericArgument val1 = QGenericArgument(),
                                 QGenericArgument val2 = QGenericArgument(),
                                 QGenericArgument val3 = QGenericArgument(),
                                 QGenericArgument val4 = QGenericArgument(),
                                 QGenericArgument val5 = QGenericArgument(),
                                 QGenericArgument val6 = QGenericArgument(),
                                 QGenericArgument val7 = QGenericArgument(),
                                 QGenericArgument val8 = QGenericArgument(),
                                 QGenericArgument val9 = QGenericArgument()) const
    \obsolete [6.5] Please use the variadic overload of this function
    \overload invoke()

    This overload always invokes this method using the connection type Qt::AutoConnection.
*/

/*! \fn bool QMetaMethod::invoke(QObject *object,
                                 Qt::ConnectionType connectionType,
                                 QGenericArgument val0 = QGenericArgument(0),
                                 QGenericArgument val1 = QGenericArgument(),
                                 QGenericArgument val2 = QGenericArgument(),
                                 QGenericArgument val3 = QGenericArgument(),
                                 QGenericArgument val4 = QGenericArgument(),
                                 QGenericArgument val5 = QGenericArgument(),
                                 QGenericArgument val6 = QGenericArgument(),
                                 QGenericArgument val7 = QGenericArgument(),
                                 QGenericArgument val8 = QGenericArgument(),
                                 QGenericArgument val9 = QGenericArgument()) const
    \obsolete [6.5] Please use the variadic overload of this function
    \overload invoke()

    This overload can be used if the return value of the member is of no interest.
*/

/*!
    \fn bool QMetaMethod::invoke(QObject *object,
                                 QGenericArgument val0 = QGenericArgument(0),
                                 QGenericArgument val1 = QGenericArgument(),
                                 QGenericArgument val2 = QGenericArgument(),
                                 QGenericArgument val3 = QGenericArgument(),
                                 QGenericArgument val4 = QGenericArgument(),
                                 QGenericArgument val5 = QGenericArgument(),
                                 QGenericArgument val6 = QGenericArgument(),
                                 QGenericArgument val7 = QGenericArgument(),
                                 QGenericArgument val8 = QGenericArgument(),
                                 QGenericArgument val9 = QGenericArgument()) const
    \obsolete [6.5] Please use the variadic overload of this function
    \overload invoke()

    This overload invokes this method using the
    connection type Qt::AutoConnection and ignores return values.
*/

/*!
    \fn template <typename... Args> bool QMetaMethod::invokeOnGadget(void *gadget, QMetaMethodReturnArgument ret, Args &&... arguments) const
    \fn template <typename... Args> bool QMetaMethod::invokeOnGadget(void *gadget, Args &&... arguments) const
    \since 6.5

    Invokes this method on a Q_GADGET. Returns \c true if the member could be invoked.
    Returns \c false if there is no such member or the parameters did not match.

    The pointer \a gadget must point to an instance of the gadget class.

    The invocation is always synchronous.

    For the overload with a QMetaMethodReturnArgument parameter, the return
    value of the \a member function call is placed in \a ret. For the overload
    without it, the return value of the called function (if any) will be
    discarded. QMetaMethodReturnArgument is an internal type you should not use
    directly. Instead, use the qReturnArg() function.

    \warning this method will not test the validity of the arguments: \a gadget
    must be an instance of the class of the QMetaObject of which this QMetaMethod
    has been constructed with.

    \sa Q_ARG(), Q_RETURN_ARG(), qRegisterMetaType(), QMetaObject::invokeMethod()
*/

/*!
    \since 5.5
    \obsolete [6.5] Please use the variadic overload of this function

    Invokes this method on a Q_GADGET. Returns \c true if the member could be invoked.
    Returns \c false if there is no such member or the parameters did not match.

    See the variadic invokeMethod() function for more information. This
    function should behave the same way as that one, with the following
    limitations:

    \list
    \li The number of parameters is limited to 10.
    \li Parameter names may need to be an exact string match.
    \li Meta types are not automatically registered.
    \endlist

    \warning In addition to the limitations of the variadic invoke() overload,
    the arguments must have the same type as the ones expected by the method,
    else, the behavior is undefined.

    \sa Q_ARG(), Q_RETURN_ARG(), qRegisterMetaType(), QMetaObject::invokeMethod()
*/
bool QMetaMethod::invokeOnGadget(void *gadget,
                                 QGenericReturnArgument returnValue,
                                 QGenericArgument val0,
                                 QGenericArgument val1,
                                 QGenericArgument val2,
                                 QGenericArgument val3,
                                 QGenericArgument val4,
                                 QGenericArgument val5,
                                 QGenericArgument val6,
                                 QGenericArgument val7,
                                 QGenericArgument val8,
                                 QGenericArgument val9) const
{
   if (!gadget || !mobj)
        return false;

    // check return type
    if (returnValue.data()) {
        const char *retType = typeName();
        if (qstrcmp(returnValue.name(), retType) != 0) {
            // normalize the return value as well
            QByteArray normalized = QMetaObject::normalizedType(returnValue.name());
            if (qstrcmp(normalized.constData(), retType) != 0) {
                // String comparison failed, try compare the metatype.
                int t = returnType();
                if (t == QMetaType::UnknownType || t != QMetaType::fromName(normalized).id())
                    return false;
            }
        }
    }

    // check argument count (we don't allow invoking a method if given too few arguments)
    const char *typeNames[] = {
        returnValue.name(),
        val0.name(),
        val1.name(),
        val2.name(),
        val3.name(),
        val4.name(),
        val5.name(),
        val6.name(),
        val7.name(),
        val8.name(),
        val9.name()
    };
    int paramCount;
    for (paramCount = 1; paramCount < MaximumParamCount; ++paramCount) {
        if (qstrlen(typeNames[paramCount]) <= 0)
            break;
    }
    if (paramCount <= QMetaMethodPrivate::get(this)->parameterCount())
        return false;

    // invoke!
    void *param[] = {
        returnValue.data(),
        val0.data(),
        val1.data(),
        val2.data(),
        val3.data(),
        val4.data(),
        val5.data(),
        val6.data(),
        val7.data(),
        val8.data(),
        val9.data()
    };
    int idx_relative = QMetaMethodPrivate::get(this)->ownMethodIndex();
    Q_ASSERT(QMetaObjectPrivate::get(mobj)->revision >= 6);
    QObjectPrivate::StaticMetaCallFunction callFunction = mobj->d.static_metacall;
    if (!callFunction)
        return false;
    callFunction(reinterpret_cast<QObject*>(gadget), QMetaObject::InvokeMetaMethod, idx_relative, param);
    return true;
}

/*!
    \fn bool QMetaMethod::invokeOnGadget(void *gadget,
                                         QGenericArgument val0 = QGenericArgument(0),
                                         QGenericArgument val1 = QGenericArgument(),
                                         QGenericArgument val2 = QGenericArgument(),
                                         QGenericArgument val3 = QGenericArgument(),
                                         QGenericArgument val4 = QGenericArgument(),
                                         QGenericArgument val5 = QGenericArgument(),
                                         QGenericArgument val6 = QGenericArgument(),
                                         QGenericArgument val7 = QGenericArgument(),
                                         QGenericArgument val8 = QGenericArgument(),
                                         QGenericArgument val9 = QGenericArgument()) const

    \overload
    \obsolete [6.5] Please use the variadic overload of this function
    \since 5.5

    This overload invokes this method for a \a gadget and ignores return values.
*/

/*!
    \class QMetaEnum
    \inmodule QtCore
    \brief The QMetaEnum class provides meta-data about an enumerator.

    \ingroup objectmodel

    Use name() for the enumerator's name. The enumerator's keys (names
    of each enumerated item) are returned by key(); use keyCount() to find
    the number of keys. isFlag() returns whether the enumerator is
    meant to be used as a flag, meaning that its values can be combined
    using the OR operator.

    The conversion functions keyToValue(), valueToKey(), keysToValue(),
    and valueToKeys() allow conversion between the integer
    representation of an enumeration or set value and its literal
    representation. The scope() function returns the class scope this
    enumerator was declared in.

    \sa QMetaObject, QMetaMethod, QMetaProperty
*/

/*!
    \fn bool QMetaEnum::isValid() const

    Returns \c true if this enum is valid (has a name); otherwise returns
    false.

    \sa name()
*/

/*!
    \fn const QMetaObject *QMetaEnum::enclosingMetaObject() const
    \internal
*/


/*!
    \fn QMetaEnum::QMetaEnum()
    \internal
*/

/*!
    Returns the name of the type (without the scope).

    For example, the Qt::Key enumeration has \c
    Key as the type name and \l Qt as the scope.

    For flags this returns the name of the flag type, not the
    name of the enum type.

    \sa isValid(), scope(), enumName()
*/
const char *QMetaEnum::name() const
{
    if (!mobj)
        return nullptr;
    return rawStringData(mobj, data.name());
}

/*!
    Returns the enum name of the flag (without the scope).

    For example, the Qt::AlignmentFlag flag has \c
    AlignmentFlag as the enum name, but \c Alignment as the type name.
    Non flag enums has the same type and enum names.

    Enum names have the same scope as the type name.

    \since 5.12
    \sa isValid(), name()
*/
const char *QMetaEnum::enumName() const
{
    if (!mobj)
        return nullptr;
    return rawStringData(mobj, data.alias());
}

/*!
    Returns the meta type of the enum.

    If the QMetaObject that this enum is part of was generated with Qt 6.5 or
    earlier, this will be an invalid meta type.

    \note This is the meta type of the enum itself, not of its underlying
    integral type. You can retrieve the meta type of the underlying type of the
    enum using \l{QMetaType::underlyingType()}.

    \since 6.6
*/
QMetaType QMetaEnum::metaType() const
{
    if (!mobj)
        return {};

    const QMetaObjectPrivate *p = priv(mobj->d.data);
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
    if (p->revision < 12)
        QMetaType();
#endif

    return QMetaType(mobj->d.metaTypes[data.index(mobj) + p->propertyCount]);
}

/*!
    Returns the number of keys.

    \sa key()
*/
int QMetaEnum::keyCount() const
{
    if (!mobj)
        return 0;
    return data.keyCount();
}

/*!
    Returns the key with the given \a index, or \nullptr if no such key exists.

    \sa keyCount(), value(), valueToKey()
*/
const char *QMetaEnum::key(int index) const
{
    if (!mobj)
        return nullptr;
    if (index >= 0  && index < int(data.keyCount()))
        return rawStringData(mobj, mobj->d.data[data.data() + 2*index]);
    return nullptr;
}

/*!
    Returns the value with the given \a index; or returns -1 if there
    is no such value.

    \sa keyCount(), key(), keyToValue()
*/
int QMetaEnum::value(int index) const
{
    if (!mobj)
        return 0;
    if (index >= 0 && index < int(data.keyCount()))
        return mobj->d.data[data.data() + 2 * index + 1];
    return -1;
}

/*!
    Returns \c true if this enumerator is used as a flag; otherwise returns
    false.

    When used as flags, enumerators can be combined using the OR
    operator.

    \sa keysToValue(), valueToKeys()
*/
bool QMetaEnum::isFlag() const
{
    if (!mobj)
        return false;
    return data.flags() & EnumIsFlag;
}

/*!
    \since 5.8

    Returns \c true if this enumerator is declared as a C++11 enum class;
    otherwise returns false.
*/
bool QMetaEnum::isScoped() const
{
    if (!mobj)
        return false;
    return data.flags() & EnumIsScoped;
}

/*!
    Returns the scope this enumerator was declared in.

    For example, the Qt::AlignmentFlag enumeration has \c Qt as
    the scope and \c AlignmentFlag as the name.

    \sa name()
*/
const char *QMetaEnum::scope() const
{
    return mobj ? objectClassName(mobj) : nullptr;
}

/*!
    Returns the integer value of the given enumeration \a key, or -1
    if \a key is not defined.

    If \a key is not defined, *\a{ok} is set to false; otherwise
    *\a{ok} is set to true.

    For flag types, use keysToValue().

    \sa valueToKey(), isFlag(), keysToValue()
*/
int QMetaEnum::keyToValue(const char *key, bool *ok) const
{
    if (ok != nullptr)
        *ok = false;
    if (!mobj || !key)
        return -1;
    uint scope = 0;
    const char *qualified_key = key;
    const char *s = key + qstrlen(key);
    while (s > key && *s != ':')
        --s;
    if (s > key && *(s - 1) == ':') {
        scope = s - key - 1;
        key += scope + 2;
    }
    for (int i = 0; i < int(data.keyCount()); ++i) {
        const QByteArray className = stringData(mobj, priv(mobj->d.data)->className);
        if ((!scope || (className.size() == int(scope) && strncmp(qualified_key, className.constData(), scope) == 0))
             && strcmp(key, rawStringData(mobj, mobj->d.data[data.data() + 2*i])) == 0) {
            if (ok != nullptr)
                *ok = true;
            return mobj->d.data[data.data() + 2 * i + 1];
        }
    }
    return -1;
}

/*!
    Returns the string that is used as the name of the given
    enumeration \a value, or \nullptr if \a value is not defined.

    For flag types, use valueToKeys().

    \sa isFlag(), valueToKeys()
*/
const char *QMetaEnum::valueToKey(int value) const
{
    if (!mobj)
        return nullptr;
    for (int i = 0; i < int(data.keyCount()); ++i)
        if (value == (int)mobj->d.data[data.data() + 2 * i + 1])
            return rawStringData(mobj, mobj->d.data[data.data() + 2 * i]);
    return nullptr;
}

static auto parse_scope(QLatin1StringView qualifiedKey) noexcept
{
    struct R {
        std::optional<QLatin1StringView> scope;
        QLatin1StringView key;
    };
    const auto scopePos = qualifiedKey.lastIndexOf("::"_L1);
    if (scopePos < 0)
        return R{std::nullopt, qualifiedKey};
    else
        return R{qualifiedKey.first(scopePos), qualifiedKey.sliced(scopePos + 2)};
}

/*!
    Returns the value derived from combining together the values of
    the \a keys using the OR operator, or -1 if \a keys is not
    defined. Note that the strings in \a keys must be '|'-separated.

    If \a keys is not defined, *\a{ok} is set to false; otherwise
    *\a{ok} is set to true.

    \sa isFlag(), valueToKey(), valueToKeys()
*/
int QMetaEnum::keysToValue(const char *keys, bool *ok) const
{
    if (ok != nullptr)
        *ok = false;
    if (!mobj || !keys)
        return -1;

    auto lookup = [&] (QLatin1StringView key) -> std::optional<int> {
        for (int i = data.keyCount() - 1; i >= 0; --i) {
            if (key == stringDataView(mobj, mobj->d.data[data.data() + 2*i]))
                return mobj->d.data[data.data() + 2*i + 1];
        }
        return std::nullopt;
    };
    auto className = [&] { return stringDataView(mobj, priv(mobj->d.data)->className); };

    int value = 0;
    for (const QLatin1StringView &untrimmed : qTokenize(QLatin1StringView{keys}, u'|')) {
        const auto parsed = parse_scope(untrimmed.trimmed());
        if (parsed.scope && *parsed.scope != className())
            return -1; // wrong type name in qualified name
        if (auto thisValue = lookup(parsed.key))
            value |= *thisValue;
        else
            return -1; // no such enumerator
    }
    if (ok != nullptr)
        *ok = true;
    return value;
}

namespace
{
template <typename String, typename Container, typename Separator>
void join_reversed(String &s, const Container &c, Separator sep)
{
    if (c.empty())
        return;
    qsizetype len = qsizetype(c.size()) - 1; // N - 1 separators
    for (auto &e : c)
        len += qsizetype(e.size()); // N parts
    s.reserve(len);
    bool first = true;
    for (auto rit = c.rbegin(), rend = c.rend(); rit != rend; ++rit) {
        const auto &e = *rit;
        if (!first)
            s.append(sep);
        first = false;
        s.append(e.data(), e.size());
    }
}
} // unnamed namespace

/*!
    Returns a byte array of '|'-separated keys that represents the
    given \a value.

    \sa isFlag(), valueToKey(), keysToValue()
*/
QByteArray QMetaEnum::valueToKeys(int value) const
{
    QByteArray keys;
    if (!mobj)
        return keys;
    QVarLengthArray<QLatin1StringView, sizeof(int) * CHAR_BIT> parts;
    int v = value;
    // reverse iterate to ensure values like Qt::Dialog=0x2|Qt::Window are processed first.
    for (int i = data.keyCount() - 1; i >= 0; --i) {
        int k = mobj->d.data[data.data() + 2 * i + 1];
        if ((k != 0 && (v & k) == k) || (k == value)) {
            v = v & ~k;
            parts.push_back(stringDataView(mobj, mobj->d.data[data.data() + 2 * i]));
        }
    }
    join_reversed(keys, parts, '|');
    return keys;
}

/*!
  \internal
 */
QMetaEnum::QMetaEnum(const QMetaObject *mobj, int index)
    : mobj(mobj), data({ mobj->d.data + priv(mobj->d.data)->enumeratorData + index * Data::Size })
{
    Q_ASSERT(index >= 0 && index < priv(mobj->d.data)->enumeratorCount);
}

int QMetaEnum::Data::index(const QMetaObject *mobj) const
{
    return (d - mobj->d.data - priv(mobj->d.data)->enumeratorData) / Size;
}

/*!
    \fn QMetaEnum QMetaEnum::fromType()
    \since 5.5

    Returns the QMetaEnum corresponding to the type in the template parameter.
    The enum needs to be declared with Q_ENUM.
*/

/*!
    \class QMetaProperty
    \inmodule QtCore
    \brief The QMetaProperty class provides meta-data about a property.

    \ingroup objectmodel

    Property meta-data is obtained from an object's meta-object. See
    QMetaObject::property() and QMetaObject::propertyCount() for
    details.

    \section1 Property Meta-Data

    A property has a name() and a type(), as well as various
    attributes that specify its behavior: isReadable(), isWritable(),
    isDesignable(), isScriptable(), revision(), and isStored().

    If the property is an enumeration, isEnumType() returns \c true; if the
    property is an enumeration that is also a flag (i.e. its values
    can be combined using the OR operator), isEnumType() and
    isFlagType() both return true. The enumerator for these types is
    available from enumerator().

    The property's values are set and retrieved with read(), write(),
    and reset(); they can also be changed through QObject's set and get
    functions. See QObject::setProperty() and QObject::property() for
    details.

    \section1 Copying and Assignment

    QMetaProperty objects can be copied by value. However, each copy will
    refer to the same underlying property meta-data.

    \sa QMetaObject, QMetaEnum, QMetaMethod, {Qt's Property System}
*/

/*!
    \fn bool QMetaProperty::isValid() const

    Returns \c true if this property is valid (readable); otherwise
    returns \c false.

    \sa isReadable()
*/

/*!
    \fn const QMetaObject *QMetaProperty::enclosingMetaObject() const
    \internal
*/

/*!
    \fn QMetaProperty::QMetaProperty()
    \internal
*/

/*!
    Returns this property's name.

    \sa type(), typeName()
*/
const char *QMetaProperty::name() const
{
    if (!mobj)
        return nullptr;
    return rawStringData(mobj, data.name());
}

/*!
    Returns the name of this property's type.

    \sa type(), name()
*/
const char *QMetaProperty::typeName() const
{
    if (!mobj)
        return nullptr;
    // TODO: can the metatype be invalid for dynamic metaobjects?
    if (const auto mt = metaType(); mt.isValid())
        return mt.name();
    return rawTypeNameFromTypeInfo(mobj, data.type());
}

/*! \fn QVariant::Type QMetaProperty::type() const
    \deprecated

    Returns this property's type. The return value is one
    of the values of the QVariant::Type enumeration.

    \sa typeName(), name(), metaType()
*/

/*! \fn int QMetaProperty::userType() const
    \since 4.2

    Returns this property's user type. The return value is one
    of the values that are registered with QMetaType.

    This is equivalent to metaType().id()

    \sa type(), QMetaType, typeName(), metaType()
 */

/*! \fn int QMetaProperty::typeId() const
    \since 6.0

    Returns the storage type of the property. This is
    the same as metaType().id().

    \sa QMetaType, typeName(), metaType()
 */

/*!
    \since 6.0

    Returns this property's QMetaType.

    \sa QMetaType
 */
QMetaType QMetaProperty::metaType() const
{
    if (!mobj)
        return {};
    return QMetaType(mobj->d.metaTypes[data.index(mobj)]);
}

int QMetaProperty::Data::index(const QMetaObject *mobj) const
{
    return (d - mobj->d.data - priv(mobj->d.data)->propertyData) / Size;
}

/*!
  \since 4.6

  Returns this property's index.
*/
int QMetaProperty::propertyIndex() const
{
    if (!mobj)
        return -1;
    return data.index(mobj) + mobj->propertyOffset();
}

/*!
  \since 5.14

  Returns this property's index relative within the enclosing meta object.
*/
int QMetaProperty::relativePropertyIndex() const
{
    if (!mobj)
        return -1;
    return data.index(mobj);
}

/*!
    Returns \c true if the property's type is an enumeration value that
    is used as a flag; otherwise returns \c false.

    Flags can be combined using the OR operator. A flag type is
    implicitly also an enum type.

    \sa isEnumType(), enumerator(), QMetaEnum::isFlag()
*/

bool QMetaProperty::isFlagType() const
{
    return isEnumType() && menum.isFlag();
}

/*!
    Returns \c true if the property's type is an enumeration value;
    otherwise returns \c false.

    \sa enumerator(), isFlagType()
*/
bool QMetaProperty::isEnumType() const
{
    if (!mobj)
        return false;
    return (data.flags() & EnumOrFlag) && menum.name();
}

/*!
    \internal

    Returns \c true if the property has a C++ setter function that
    follows Qt's standard "name" / "setName" pattern. Designer and uic
    query hasStdCppSet() in order to avoid expensive
    QObject::setProperty() calls. All properties in Qt [should] follow
    this pattern.
*/
bool QMetaProperty::hasStdCppSet() const
{
    if (!mobj)
        return false;
    return (data.flags() & StdCppSet);
}

/*!
    \internal

    Returns \c true if the property is an alias.
    This is for instance true for a property declared in QML
    as 'property alias'.
*/
bool QMetaProperty::isAlias() const
{
    if (!mobj)
        return false;
    return (data.flags() & Alias);
}

#if QT_DEPRECATED_SINCE(6, 4)
/*!
    \internal
    Historically:
    Executes metacall with QMetaObject::RegisterPropertyMetaType flag.
    Returns id of registered type or QMetaType::UnknownType if a type
    could not be registered for any reason.
    Obsolete since Qt 6
*/
int QMetaProperty::registerPropertyType() const
{
    return typeId();
}
#endif

QMetaProperty::QMetaProperty(const QMetaObject *mobj, int index)
    : mobj(mobj),
      data(getMetaPropertyData(mobj, index))
{
    Q_ASSERT(index >= 0 && index < priv(mobj->d.data)->propertyCount);

    if (!(data.flags() & EnumOrFlag))
        return;
    const char *type = rawTypeNameFromTypeInfo(mobj, data.type());
    menum = mobj->enumerator(mobj->indexOfEnumerator(type));
    if (menum.isValid())
        return;
    const char *enum_name = type;
    const char *scope_name = objectClassName(mobj);
    char *scope_buffer = nullptr;

    const char *colon = strrchr(enum_name, ':');
    // ':' will always appear in pairs
    Q_ASSERT(colon <= enum_name || *(colon - 1) == ':');
    if (colon > enum_name) {
        int len = colon - enum_name - 1;
        scope_buffer = (char *)malloc(len + 1);
        memcpy(scope_buffer, enum_name, len);
        scope_buffer[len] = '\0';
        scope_name = scope_buffer;
        enum_name = colon + 1;
    }

    const QMetaObject *scope = nullptr;
    if (qstrcmp(scope_name, "Qt") == 0)
        scope = &Qt::staticMetaObject;
    else
        scope = QMetaObject_findMetaObject(mobj, scope_name);
    if (scope)
        menum = scope->enumerator(scope->indexOfEnumerator(enum_name));
    if (scope_buffer)
        free(scope_buffer);
}

/*!
   \internal
   Constructs the \c QMetaProperty::Data for the \a index th property of \a mobj
 */
QMetaProperty::Data QMetaProperty::getMetaPropertyData(const QMetaObject *mobj, int index)
{
    return { mobj->d.data + priv(mobj->d.data)->propertyData + index * Data::Size };
}

/*!
    Returns the enumerator if this property's type is an enumerator
    type; otherwise the returned value is undefined.

    \sa isEnumType(), isFlagType()
*/
QMetaEnum QMetaProperty::enumerator() const
{
    return menum;
}

/*!
    Reads the property's value from the given \a object. Returns the value
    if it was able to read it; otherwise returns an invalid variant.

    \sa write(), reset(), isReadable()
*/
QVariant QMetaProperty::read(const QObject *object) const
{
    if (!object || !mobj)
        return QVariant();

    // the status variable is changed by qt_metacall to indicate what it did
    // this feature is currently only used by Qt D-Bus and should not be depended
    // upon. Don't change it without looking into QDBusAbstractInterface first
    // -1 (unchanged): normal qt_metacall, result stored in argv[0]
    // changed: result stored directly in value
    int status = -1;
    QVariant value;
    void *argv[] = { nullptr, &value, &status };
    QMetaType t(mobj->d.metaTypes[data.index(mobj)]);
    if (t == QMetaType::fromType<QVariant>()) {
        argv[0] = &value;
    } else {
        value = QVariant(t, nullptr);
        argv[0] = value.data();
    }
    if (priv(mobj->d.data)->flags & PropertyAccessInStaticMetaCall && mobj->d.static_metacall) {
        mobj->d.static_metacall(const_cast<QObject*>(object), QMetaObject::ReadProperty, data.index(mobj), argv);
    } else {
        QMetaObject::metacall(const_cast<QObject*>(object), QMetaObject::ReadProperty,
                              data.index(mobj) + mobj->propertyOffset(), argv);
    }

    if (status != -1)
        return value;
    if (t != QMetaType::fromType<QVariant>() && argv[0] != value.data())
        // pointer or reference
        return QVariant(t, argv[0]);
    return value;
}

/*!
    Writes \a value as the property's value to the given \a object. Returns
    true if the write succeeded; otherwise returns \c false.

    If \a value is not of the same type as the property, a conversion
    is attempted. An empty QVariant() is equivalent to a call to reset()
    if this property is resettable, or setting a default-constructed object
    otherwise.

    \note This function internally makes a copy of \a value. Prefer to use the
    rvalue overload when possible.

    \sa read(), reset(), isWritable()
*/
bool QMetaProperty::write(QObject *object, const QVariant &value) const
{
    if (!object || !isWritable())
        return false;
    return write(object, QVariant(value));
}

/*!
    \overload
    \since 6.6
*/
bool QMetaProperty::write(QObject *object, QVariant &&v) const
{
    if (!object || !isWritable())
        return false;
    QMetaType t(mobj->d.metaTypes[data.index(mobj)]);
    if (t != QMetaType::fromType<QVariant>() && t != v.metaType()) {
        if (isEnumType() && !t.metaObject() && v.metaType().id() == QMetaType::QString) {
            // Assigning a string to a property of type Q_ENUMS (instead of Q_ENUM)
            bool ok;
            if (isFlagType())
                v = QVariant(menum.keysToValue(v.toByteArray(), &ok));
            else
                v = QVariant(menum.keyToValue(v.toByteArray(), &ok));
            if (!ok)
                return false;
        } else if (!v.isValid()) {
            if (isResettable())
                return reset(object);
            v = QVariant(t, nullptr);
        } else if (!v.convert(t)) {
            return false;
        }
    }
    // the status variable is changed by qt_metacall to indicate what it did
    // this feature is currently only used by Qt D-Bus and should not be depended
    // upon. Don't change it without looking into QDBusAbstractInterface first
    // -1 (unchanged): normal qt_metacall, result stored in argv[0]
    // changed: result stored directly in value, return the value of status
    int status = -1;
    // the flags variable is used by the declarative module to implement
    // interception of property writes.
    int flags = 0;
    void *argv[] = { nullptr, &v, &status, &flags };
    if (t == QMetaType::fromType<QVariant>())
        argv[0] = &v;
    else
        argv[0] = v.data();
    if (priv(mobj->d.data)->flags & PropertyAccessInStaticMetaCall && mobj->d.static_metacall)
        mobj->d.static_metacall(object, QMetaObject::WriteProperty, data.index(mobj), argv);
    else
        QMetaObject::metacall(object, QMetaObject::WriteProperty, data.index(mobj) + mobj->propertyOffset(), argv);

    return status;
}

/*!
    Resets the property for the given \a object with a reset method.
    Returns \c true if the reset worked; otherwise returns \c false.

    Reset methods are optional; only a few properties support them.

    \sa read(), write()
*/
bool QMetaProperty::reset(QObject *object) const
{
    if (!object || !mobj || !isResettable())
        return false;
    void *argv[] = { nullptr };
    if (priv(mobj->d.data)->flags & PropertyAccessInStaticMetaCall && mobj->d.static_metacall)
        mobj->d.static_metacall(object, QMetaObject::ResetProperty, data.index(mobj), argv);
    else
        QMetaObject::metacall(object, QMetaObject::ResetProperty, data.index(mobj) + mobj->propertyOffset(), argv);
    return true;
}

/*!
    \since 6.0
    Returns the bindable interface for the property on a given \a object.

    If the property doesn't support bindings, the returned interface will be
    invalid.

    \sa QObjectBindableProperty, QProperty, isBindable()
*/
QUntypedBindable QMetaProperty::bindable(QObject *object) const
{
    QUntypedBindable bindable;
    void * argv[1] { &bindable };
    mobj->metacall(object, QMetaObject::BindableProperty, data.index(mobj) + mobj->propertyOffset(), argv);
    return bindable;
}
/*!
    \since 5.5

    Reads the property's value from the given \a gadget. Returns the value
    if it was able to read it; otherwise returns an invalid variant.

    This function should only be used if this is a property of a Q_GADGET
*/
QVariant QMetaProperty::readOnGadget(const void *gadget) const
{
    Q_ASSERT(priv(mobj->d.data)->flags & PropertyAccessInStaticMetaCall && mobj->d.static_metacall);
    return read(reinterpret_cast<const QObject*>(gadget));
}

/*!
    \since 5.5

    Writes \a value as the property's value to the given \a gadget. Returns
    true if the write succeeded; otherwise returns \c false.

    This function should only be used if this is a property of a Q_GADGET
*/
bool QMetaProperty::writeOnGadget(void *gadget, const QVariant &value) const
{
    Q_ASSERT(priv(mobj->d.data)->flags & PropertyAccessInStaticMetaCall && mobj->d.static_metacall);
    return write(reinterpret_cast<QObject*>(gadget), value);
}

/*!
    \overload
    \since 6.6
*/
bool QMetaProperty::writeOnGadget(void *gadget, QVariant &&value) const
{
    Q_ASSERT(priv(mobj->d.data)->flags & PropertyAccessInStaticMetaCall && mobj->d.static_metacall);
    return write(reinterpret_cast<QObject*>(gadget), std::move(value));
}

/*!
    \since 5.5

    Resets the property for the given \a gadget with a reset method.
    Returns \c true if the reset worked; otherwise returns \c false.

    Reset methods are optional; only a few properties support them.

    This function should only be used if this is a property of a Q_GADGET
*/
bool QMetaProperty::resetOnGadget(void *gadget) const
{
    Q_ASSERT(priv(mobj->d.data)->flags & PropertyAccessInStaticMetaCall && mobj->d.static_metacall);
    return reset(reinterpret_cast<QObject*>(gadget));
}

/*!
    Returns \c true if this property can be reset to a default value; otherwise
    returns \c false.

    \sa reset()
*/
bool QMetaProperty::isResettable() const
{
    if (!mobj)
        return false;
    return data.flags() & Resettable;
}

/*!
    Returns \c true if this property is readable; otherwise returns \c false.

    \sa isWritable(), read(), isValid()
 */
bool QMetaProperty::isReadable() const
{
    if (!mobj)
        return false;
    return data.flags() & Readable;
}

/*!
    Returns \c true if this property has a corresponding change notify signal;
    otherwise returns \c false.

    \sa notifySignal()
 */
bool QMetaProperty::hasNotifySignal() const
{
    if (!mobj)
        return false;
    return data.notifyIndex() != uint(-1);
}

/*!
    \since 4.5

    Returns the QMetaMethod instance of the property change notifying signal if
    one was specified, otherwise returns an invalid QMetaMethod.

    \sa hasNotifySignal()
 */
QMetaMethod QMetaProperty::notifySignal() const
{
    int id = notifySignalIndex();
    if (id != -1)
        return mobj->method(id);
    else
        return QMetaMethod();
}

/*!
    \since 4.6

    Returns the index of the property change notifying signal if one was
    specified, otherwise returns -1.

    \sa hasNotifySignal()
 */
int QMetaProperty::notifySignalIndex() const
{
    if (!mobj || data.notifyIndex() == std::numeric_limits<uint>::max())
        return -1;
    uint methodIndex = data.notifyIndex();
    if (methodIndex & IsUnresolvedSignal) {
        methodIndex &= ~IsUnresolvedSignal;
        const QByteArray signalName = stringData(mobj, methodIndex);
        const QMetaObject *m = mobj;
        const int idx = QMetaObjectPrivate::indexOfMethodRelative<MethodSignal>(&m, signalName, 0, nullptr);
        if (idx >= 0) {
            return idx + m->methodOffset();
        } else {
            qWarning("QMetaProperty::notifySignal: cannot find the NOTIFY signal %s in class %s for property '%s'",
                     signalName.constData(), objectClassName(mobj), name());
            return -1;
        }
    }
    return methodIndex + mobj->methodOffset();
}

// This method has been around for a while, but the documentation was marked \internal until 5.1
/*!
    \since 5.1

    Returns the property revision if one was
    specified by REVISION, otherwise returns 0.
 */
int QMetaProperty::revision() const
{
    if (!mobj)
        return 0;
    return data.revision();
}

/*!
    Returns \c true if this property is writable; otherwise returns
    false.

    \sa isReadable(), write()
 */
bool QMetaProperty::isWritable() const
{
    if (!mobj)
        return false;
    return data.flags() & Writable;
}

/*!
    Returns \c false if the \c{Q_PROPERTY()}'s \c DESIGNABLE attribute
    is false; otherwise returns \c true.

    \sa isScriptable(), isStored()
*/
bool QMetaProperty::isDesignable() const
{
    if (!mobj)
        return false;
    return data.flags() & Designable;
}

/*!
    Returns \c false if the \c{Q_PROPERTY()}'s \c SCRIPTABLE attribute
    is false; otherwise returns true.

    \sa isDesignable(), isStored()
*/
bool QMetaProperty::isScriptable() const
{
    if (!mobj)
        return false;
    return data.flags() & Scriptable;
}

/*!
    Returns \c true if the property is stored; otherwise returns
    false.

    The function returns \c false if the
    \c{Q_PROPERTY()}'s \c STORED attribute is false; otherwise returns
    true.

    \sa isDesignable(), isScriptable()
*/
bool QMetaProperty::isStored() const
{
    if (!mobj)
        return false;
    return data.flags() & Stored;
}

/*!
    Returns \c false if the \c {Q_PROPERTY()}'s \c USER attribute is false.
    Otherwise it returns true, indicating the property is designated as the
    \c USER property, i.e., the one that the user can edit or
    that is significant in some other way.

    \sa QMetaObject::userProperty(), isDesignable(), isScriptable()
*/
bool QMetaProperty::isUser() const
{
    if (!mobj)
        return false;
    return data.flags() & User;
}

/*!
    \since 4.6
    Returns \c true if the property is constant; otherwise returns \c false.

    A property is constant if the \c{Q_PROPERTY()}'s \c CONSTANT attribute
    is set.
*/
bool QMetaProperty::isConstant() const
{
    if (!mobj)
        return false;
    return data.flags() & Constant;
}

/*!
    \since 4.6
    Returns \c true if the property is final; otherwise returns \c false.

    A property is final if the \c{Q_PROPERTY()}'s \c FINAL attribute
    is set.
*/
bool QMetaProperty::isFinal() const
{
    if (!mobj)
        return false;
    return data.flags() & Final;
}

/*!
  \since 5.15
  Returns \c true if the property is required; otherwise returns \c false.

  A property is final if the \c{Q_PROPERTY()}'s \c REQUIRED attribute
  is set.
*/
bool QMetaProperty::isRequired() const
{
    if (!mobj)
        return false;
    return data.flags() & Required;
}

/*!
    \since 6.0
    Returns \c true if the \c{Q_PROPERTY()} exposes binding functionality; otherwise returns false.

    This implies that you can create bindings that use this property as a dependency or install QPropertyObserver
    objects on this property. Unless the property is readonly, you can also set a binding on this property.

    \sa QProperty, isWritable(), bindable()
*/
bool QMetaProperty::isBindable() const
{
    if (!mobj)
        return false;
    return (data.flags() & Bindable);
}

/*!
    \class QMetaClassInfo
    \inmodule QtCore

    \brief The QMetaClassInfo class provides additional information
    about a class.

    \ingroup objectmodel

    Class information items are simple \e{name}--\e{value} pairs that
    are specified using Q_CLASSINFO() in the source code. The
    information can be retrieved using name() and value(). For example:

    \snippet code/src_corelib_kernel_qmetaobject.cpp 5

    This mechanism is free for you to use in your Qt applications.

    \note It's also used by the \l[ActiveQt]{Active Qt},
    \l[QtDBus]{Qt D-Bus}, \l[QtQml]{Qt QML}, and \l{Qt Remote Objects}
    modules. Some keys might be set when using these modules.

    \sa QMetaObject
*/

/*!
    \fn QMetaClassInfo::QMetaClassInfo()
    \internal
*/

/*!
    \fn const QMetaObject *QMetaClassInfo::enclosingMetaObject() const
    \internal
*/

/*!
    Returns the name of this item.

    \sa value()
*/
const char *QMetaClassInfo::name() const
{
    if (!mobj)
        return nullptr;
    return rawStringData(mobj, data.name());
}

/*!
    Returns the value of this item.

    \sa name()
*/
const char *QMetaClassInfo::value() const
{
    if (!mobj)
        return nullptr;
    return rawStringData(mobj, data.value());
}

/*!
    \class QMethodRawArguments
    \internal

    A wrapper class for the void ** arguments array used by the meta
    object system. If a slot uses a single argument of this type,
    the meta object system will pass the raw arguments array directly
    to the slot and set the arguments count in the slot description to
    zero, so that any signal can connect to it.

    This is used internally to implement signal relay functionality in
    our state machine and dbus.
*/

/*!
    \macro QMetaMethodArgument Q_ARG(Type, const Type &value)
    \relates QMetaObject

    This macro takes a \a Type and a \a value of that type and
    returns a QMetaMethodArgument, which can be passed to the template
    QMetaObject::invokeMethod() with the \c {Args &&...} arguments.

    \sa Q_RETURN_ARG()
*/

/*!
    \macro QMetaMethodReturnArgument Q_RETURN_ARG(Type, Type &value)
    \relates QMetaObject

    This macro takes a \a Type and a non-const reference to a \a
    value of that type and returns a QMetaMethodReturnArgument, which can be
    passed to the template QMetaObject::invokeMethod() with the \c {Args &&...}
    arguments.

    \sa Q_ARG()
*/

/*!
    \class QGenericArgument
    \inmodule QtCore

    \brief The QGenericArgument class is an internal helper class for
    marshalling arguments.

    This class should never be used directly. Please use the \l Q_ARG()
    macro instead.

    \sa Q_ARG(), QMetaObject::invokeMethod(), QGenericReturnArgument
*/

/*!
    \fn QGenericArgument::QGenericArgument(const char *name, const void *data)

    Constructs a QGenericArgument object with the given \a name and \a data.
*/

/*!
    \fn QGenericArgument::data () const

    Returns the data set in the constructor.
*/

/*!
    \fn QGenericArgument::name () const

    Returns the name set in the constructor.
*/

/*!
    \class QGenericReturnArgument
    \inmodule QtCore

    \brief The QGenericReturnArgument class is an internal helper class for
    marshalling arguments.

    This class should never be used directly. Please use the
    Q_RETURN_ARG() macro instead.

    \sa Q_RETURN_ARG(), QMetaObject::invokeMethod(), QGenericArgument
*/

/*!
    \fn QGenericReturnArgument::QGenericReturnArgument(const char *name, void *data)

    Constructs a QGenericReturnArgument object with the given \a name
    and \a data.
*/

/*!
    \internal
    If the local_method_index is a cloned method, return the index of the original.

    Example: if the index of "destroyed()" is passed, the index of "destroyed(QObject*)" is returned
 */
int QMetaObjectPrivate::originalClone(const QMetaObject *mobj, int local_method_index)
{
    Q_ASSERT(local_method_index < get(mobj)->methodCount);
    while (QMetaMethod::fromRelativeMethodIndex(mobj, local_method_index).data.flags() & MethodCloned) {
        Q_ASSERT(local_method_index > 0);
        --local_method_index;
    }
    return local_method_index;
}

/*!
    \internal

    Returns the parameter type names extracted from the given \a signature.
*/
QList<QByteArray> QMetaObjectPrivate::parameterTypeNamesFromSignature(const char *signature)
{
    QList<QByteArray> list;
    while (*signature && *signature != '(')
        ++signature;
    while (*signature && *signature != ')' && *++signature != ')') {
        const char *begin = signature;
        int level = 0;
        while (*signature && (level > 0 || *signature != ',') && *signature != ')') {
            if (*signature == '<')
                ++level;
            else if (*signature == '>')
                --level;
            ++signature;
        }
        list += QByteArray(begin, signature - begin);
    }
    return list;
}

QT_END_NAMESPACE
