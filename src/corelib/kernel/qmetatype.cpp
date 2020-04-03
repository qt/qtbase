/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qmetatype.h"
#include "qmetatype_p.h"
#include "qobjectdefs.h"
#include "qdatetime.h"
#include "qbytearray.h"
#include "qreadwritelock.h"
#include "qstring.h"
#include "qstringlist.h"
#include "qvector.h"
#include "qlocale.h"
#if QT_CONFIG(easingcurve)
#include "qeasingcurve.h"
#endif
#include "quuid.h"
#include "qvariant.h"
#include "qdatastream.h"
#include "qmetatypeswitcher_p.h"

#if QT_CONFIG(regularexpression)
#  include "qregularexpression.h"
#endif

#ifndef QT_BOOTSTRAPPED
#  include "qbitarray.h"
#  include "qurl.h"
#  include "qvariant.h"
#  include "qjsonvalue.h"
#  include "qjsonobject.h"
#  include "qjsonarray.h"
#  include "qjsondocument.h"
#  include "qcborvalue.h"
#  include "qcborarray.h"
#  include "qcbormap.h"
#  include "qbytearraylist.h"
#endif

#if QT_CONFIG(itemmodel)
#  include "qabstractitemmodel.h"
#endif

#ifndef QT_NO_GEOM_VARIANT
# include "qsize.h"
# include "qpoint.h"
# include "qrect.h"
# include "qline.h"
#endif

#include <bitset>
#include <new>

QT_BEGIN_NAMESPACE

#define NS(x) QT_PREPEND_NAMESPACE(x)


namespace {
struct DefinedTypesFilter {
    template<typename T>
    struct Acceptor {
        static const bool IsAccepted = QtMetaTypePrivate::TypeDefinition<T>::IsAvailable && QModulesPrivate::QTypeModuleInfo<T>::IsCore;
    };
};

struct QMetaTypeCustomRegistry
{
    QReadWriteLock lock;
    QVector<QtPrivate::QMetaTypeInterface *> registry;
    QHash<QByteArray, QtPrivate::QMetaTypeInterface *> aliases;
#ifndef QT_NO_DATASTREAM
    struct DataStreamOps
    {
        QMetaType::SaveOperator saveOp;
        QMetaType::LoadOperator loadOp;
    };
    QHash<int, DataStreamOps> dataStreamOp;
#endif
    // index of first empty (unregistered) type in registry, if any.
    int firstEmpty = 0;

    int registerCustomType(QtPrivate::QMetaTypeInterface *ti)
    {
        {
            QWriteLocker l(&lock);
            if (ti->typeId)
                return ti->typeId;
            QByteArray name =
#ifndef QT_NO_QOBJECT
                    QMetaObject::normalizedType
#endif
                    (ti->name);
            if (auto ti2 = aliases.value(name)) {
                ti->typeId.storeRelaxed(ti2->typeId.loadRelaxed());
                return ti2->typeId;
            }
            aliases[name] = ti;
            int size = registry.size();
            while (firstEmpty < size && registry[firstEmpty])
                ++firstEmpty;
            if (firstEmpty < size) {
                registry[firstEmpty] = ti;
                ++firstEmpty;
            } else {
                registry.append(ti);
                firstEmpty = registry.size();
            }
            ti->typeId = firstEmpty + QMetaType::User;
        }
        if (ti->legacyRegisterOp)
            ti->legacyRegisterOp();
        return ti->typeId;
    };

    void unregisterDynamicType(int id)
    {
        if (!id)
            return;
        Q_ASSERT(id > QMetaType::User);
        QWriteLocker l(&lock);
        int idx = id - QMetaType::User - 1;
        auto &ti = registry[idx];

        // We must unregister all names.
        auto it = aliases.begin();
        while (it != aliases.end()) {
            if (it.value() == ti)
                it = aliases.erase(it);
            else
                ++it;
        }

        ti = nullptr;

        firstEmpty = std::min(firstEmpty, idx);
    }

    QtPrivate::QMetaTypeInterface *getCustomType(int id)
    {
        QReadLocker l(&lock);
        return registry.value(id - QMetaType::User - 1);
    }
};

Q_GLOBAL_STATIC(QMetaTypeCustomRegistry, customTypeRegistry)

} // namespace

/*!
    \macro Q_DECLARE_OPAQUE_POINTER(PointerType)
    \relates QMetaType
    \since 5.0

    This macro enables pointers to forward-declared types (\a PointerType)
    to be registered with QMetaType using either Q_DECLARE_METATYPE()
    or qRegisterMetaType().

    \sa Q_DECLARE_METATYPE(), qRegisterMetaType()
*/

/*!
    \macro Q_DECLARE_METATYPE(Type)
    \relates QMetaType

    This macro makes the type \a Type known to QMetaType as long as it
    provides a public default constructor, a public copy constructor and
    a public destructor.
    It is needed to use the type \a Type as a custom type in QVariant.

    This macro requires that \a Type is a fully defined type at the point where
    it is used. For pointer types, it also requires that the pointed to type is
    fully defined. Use in conjunction with Q_DECLARE_OPAQUE_POINTER() to
    register pointers to forward declared types.

    Ideally, this macro should be placed below the declaration of
    the class or struct. If that is not possible, it can be put in
    a private header file which has to be included every time that
    type is used in a QVariant.

    Adding a Q_DECLARE_METATYPE() makes the type known to all template
    based functions, including QVariant. Note that if you intend to
    use the type in \e queued signal and slot connections or in
    QObject's property system, you also have to call
    qRegisterMetaType() since the names are resolved at runtime.

    This example shows a typical use case of Q_DECLARE_METATYPE():

    \snippet code/src_corelib_kernel_qmetatype.cpp 0

    If \c MyStruct is in a namespace, the Q_DECLARE_METATYPE() macro
    has to be outside the namespace:

    \snippet code/src_corelib_kernel_qmetatype.cpp 1

    Since \c{MyStruct} is now known to QMetaType, it can be used in QVariant:

    \snippet code/src_corelib_kernel_qmetatype.cpp 2

    Some types are registered automatically and do not need this macro:

    \list
    \li Pointers to classes derived from QObject
    \li QList<T>, QVector<T>, QQueue<T>, QStack<T> or QSet<T>
        where T is a registered meta type
    \li QHash<T1, T2>, QMap<T1, T2> or QPair<T1, T2> where T1 and T2 are
        registered meta types
    \li QPointer<T>, QSharedPointer<T>, QWeakPointer<T>, where T is a class that derives from QObject
    \li Enumerations registered with Q_ENUM or Q_FLAG
    \li Classes that have a Q_GADGET macro
    \endlist

    \sa qRegisterMetaType()
*/

/*!
    \macro Q_DECLARE_SEQUENTIAL_CONTAINER_METATYPE(Container)
    \relates QMetaType

    This macro makes the container \a Container known to QMetaType as a sequential
    container. This makes it possible to put an instance of Container<T> into
    a QVariant, if T itself is known to QMetaType.

    Note that all of the Qt sequential containers already have built-in
    support, and it is not necessary to use this macro with them. The
    std::vector and std::list containers also have built-in support.

    This example shows a typical use of Q_DECLARE_SEQUENTIAL_CONTAINER_METATYPE():

    \snippet code/src_corelib_kernel_qmetatype.cpp 10
*/

/*!
    \macro Q_DECLARE_ASSOCIATIVE_CONTAINER_METATYPE(Container)
    \relates QMetaType

    This macro makes the container \a Container known to QMetaType as an associative
    container. This makes it possible to put an instance of Container<T, U> into
    a QVariant, if T and U are themselves known to QMetaType.

    Note that all of the Qt associative containers already have built-in
    support, and it is not necessary to use this macro with them. The
    std::map container also has built-in support.

    This example shows a typical use of Q_DECLARE_ASSOCIATIVE_CONTAINER_METATYPE():

    \snippet code/src_corelib_kernel_qmetatype.cpp 11
*/

/*!
    \macro Q_DECLARE_SMART_POINTER_METATYPE(SmartPointer)
    \relates QMetaType

    This macro makes the smart pointer \a SmartPointer known to QMetaType as a
    smart pointer. This makes it possible to put an instance of SmartPointer<T> into
    a QVariant, if T is a type which inherits QObject.

    Note that the QWeakPointer, QSharedPointer and QPointer already have built-in
    support, and it is not necessary to use this macro with them.

    This example shows a typical use of Q_DECLARE_SMART_POINTER_METATYPE():

    \snippet code/src_corelib_kernel_qmetatype.cpp 13
*/

/*!
    \enum QMetaType::Type

    These are the built-in types supported by QMetaType:

    \value Void \c void
    \value Bool \c bool
    \value Int \c int
    \value UInt \c{unsigned int}
    \value Double \c double
    \value QChar QChar
    \value QString QString
    \value QByteArray QByteArray
    \value Nullptr \c{std::nullptr_t}

    \value VoidStar \c{void *}
    \value Long \c{long}
    \value LongLong LongLong
    \value Short \c{short}
    \value Char \c{char}
    \value ULong \c{unsigned long}
    \value ULongLong ULongLong
    \value UShort \c{unsigned short}
    \value SChar \c{signed char}
    \value UChar \c{unsigned char}
    \value Float \c float
    \value QObjectStar QObject *
    \value QVariant QVariant

    \value QCursor QCursor
    \value QDate QDate
    \value QSize QSize
    \value QTime QTime
    \value QVariantList QVariantList
    \value QPolygon QPolygon
    \value QPolygonF QPolygonF
    \value QColor QColor
    \value QColorSpace QColorSpace (introduced in Qt 5.15)
    \value QSizeF QSizeF
    \value QRectF QRectF
    \value QLine QLine
    \value QTextLength QTextLength
    \value QStringList QStringList
    \value QVariantMap QVariantMap
    \value QVariantHash QVariantHash
    \value QIcon QIcon
    \value QPen QPen
    \value QLineF QLineF
    \value QTextFormat QTextFormat
    \value QRect QRect
    \value QPoint QPoint
    \value QUrl QUrl
    \value QRegularExpression QRegularExpression
    \value QDateTime QDateTime
    \value QPointF QPointF
    \value QPalette QPalette
    \value QFont QFont
    \value QBrush QBrush
    \value QRegion QRegion
    \value QBitArray QBitArray
    \value QImage QImage
    \value QKeySequence QKeySequence
    \value QSizePolicy QSizePolicy
    \value QPixmap QPixmap
    \value QLocale QLocale
    \value QBitmap QBitmap
    \value QTransform QTransform
    \value QMatrix4x4 QMatrix4x4
    \value QVector2D QVector2D
    \value QVector3D QVector3D
    \value QVector4D QVector4D
    \value QQuaternion QQuaternion
    \value QEasingCurve QEasingCurve
    \value QJsonValue QJsonValue
    \value QJsonObject QJsonObject
    \value QJsonArray QJsonArray
    \value QJsonDocument QJsonDocument
    \value QCborValue QCborValue
    \value QCborArray QCborArray
    \value QCborMap QCborMap
    \value QCborSimpleType QCborSimpleType
    \value QModelIndex QModelIndex
    \value QPersistentModelIndex QPersistentModelIndex (introduced in Qt 5.5)
    \value QUuid QUuid
    \value QByteArrayList QByteArrayList

    \value User  Base value for user types
    \value UnknownType This is an invalid type id. It is returned from QMetaType for types that are not registered
    \omitvalue LastCoreType
    \omitvalue LastGuiType

    Additional types can be registered using Q_DECLARE_METATYPE().

    \sa type(), typeName()
*/

/*!
    \enum QMetaType::TypeFlag

    The enum describes attributes of a type supported by QMetaType.

    \value NeedsConstruction This type has non-trivial constructors. If the flag is not set instances can be safely initialized with memset to 0.
    \value NeedsDestruction This type has a non-trivial destructor. If the flag is not set calls to the destructor are not necessary before discarding objects.
    \value MovableType An instance of a type having this attribute can be safely moved by memcpy.
    \omitvalue SharedPointerToQObject
    \value IsEnumeration This type is an enumeration
    \value PointerToQObject This type is a pointer to a derived of QObject
    \omitvalue WeakPointerToQObject
    \omitvalue TrackingPointerToQObject
    \omitvalue WasDeclaredAsMetaType
    \omitvalue IsGadget \omit This type is a Q_GADGET and it's corresponding QMetaObject can be accessed with QMetaType::metaObject Since 5.5. \endomit
    \omitvalue PointerToGadget
*/

/*!
    \class QMetaType
    \inmodule QtCore
    \brief The QMetaType class manages named types in the meta-object system.

    \ingroup objectmodel
    \threadsafe

    The class is used as a helper to marshall types in QVariant and
    in queued signals and slots connections. It associates a type
    name to a type so that it can be created and destructed
    dynamically at run-time. Declare new types with Q_DECLARE_METATYPE()
    to make them available to QVariant and other template-based functions.
    Call qRegisterMetaType() to make types available to non-template based
    functions, such as the queued signal and slot connections.

    Any class or struct that has a public default
    constructor, a public copy constructor, and a public destructor
    can be registered.

    The following code allocates and destructs an instance of
    \c{MyClass}:

    \snippet code/src_corelib_kernel_qmetatype.cpp 3

    If we want the stream operators \c operator<<() and \c
    operator>>() to work on QVariant objects that store custom types,
    the custom type must provide \c operator<<() and \c operator>>()
    operators.

    \sa Q_DECLARE_METATYPE(), QVariant::setValue(), QVariant::value(), QVariant::fromValue()
*/

/*!
    \fn bool QMetaType::isValid() const
    \since 5.0

    Returns \c true if this QMetaType object contains valid
    information about a type, false otherwise.
*/
bool QMetaType::isValid() const
{
    return d_ptr;
}

/*!
    \fn bool QMetaType::isRegistered() const
    \since 5.0

    Returns \c true if this QMetaType object contains valid
    information about a type, false otherwise.
*/
bool QMetaType::isRegistered() const
{
    return d_ptr;
}

/*!
    \fn int QMetaType::id() const
    \since 5.13

    Returns id type hold by this QMetatype instance.
*/
int QMetaType::id() const
{
    if (d_ptr) {
        if (d_ptr->typeId)
            return d_ptr->typeId;
        auto reg = customTypeRegistry();
        if (reg) {
            return reg->registerCustomType(d_ptr);
        }
    }
    return 0;
}

/*!
    \fn bool QMetaType::sizeOf() const
    \since 5.0

    Returns the size of the type in bytes (i.e. sizeof(T),
    where T is the actual type for which this QMetaType instance
    was constructed for).

    This function is typically used together with construct()
    to perform low-level management of the memory used by a type.

    \sa QMetaType::construct(), QMetaType::sizeOf()
*/
int QMetaType::sizeOf() const
{
    if (d_ptr)
        return d_ptr->size;
    return 0;
}

/*!
    \fn TypeFlags QMetaType::flags() const
    \since 5.0

    Returns flags of the type for which this QMetaType instance was constructed.

    \sa QMetaType::TypeFlags, QMetaType::typeFlags()
*/
QMetaType::TypeFlags QMetaType::flags() const
{
    if (d_ptr)
        return TypeFlags(d_ptr->flags);
    return {};
}

/*!
    \fn const QMetaObject *QMetaType::metaObject() const
    \since 5.5

    return a QMetaObject relative to this type.

    If the type is a pointer type to a subclass of QObject, flags() contains
    QMetaType::PointerToQObject and this function returns the corresponding QMetaObject. This can
    be used to in combinaison with QMetaObject::construct to create QObject of this type.

    If the type is a Q_GADGET, flags() contains QMetaType::IsGadget, and this function returns its
    QMetaObject.  This can be used to retrieve QMetaMethod and QMetaProperty and use them on a
    pointer of this type. (given by QVariant::data for example)

    If the type is an enumeration, flags() contains QMetaType::IsEnumeration, and this function
    returns the QMetaObject of the enclosing object if the enum was registered as a Q_ENUM or
    \nullptr otherwise

    \sa QMetaType::metaObjectForType(), QMetaType::flags()
*/
const QMetaObject *QMetaType::metaObject() const
{
    return d_ptr ? d_ptr->metaObject : nullptr;
}

/*!
    \fn void *QMetaType::create(const void *copy = 0) const
    \since 5.0

    Returns a copy of \a copy, assuming it is of the type that this
    QMetaType instance was created for. If \a copy is \nullptr, creates
    a default constructed instance.

    \sa QMetaType::destroy()
*/
void *QMetaType::create(const void *copy) const
{
    if (d_ptr) {
        void *where =
#ifdef __STDCPP_DEFAULT_NEW_ALIGNMENT__
            d_ptr->alignment > __STDCPP_DEFAULT_NEW_ALIGNMENT__ ?
                operator new(d_ptr->size, std::align_val_t(d_ptr->alignment)) :
#endif
                operator new(d_ptr->size);
        return construct(where, copy);
    }
    return nullptr;
}

/*!
    \fn void QMetaType::destroy(void *data) const
    \since 5.0

    Destroys the \a data, assuming it is of the type that this
    QMetaType instance was created for.

    \sa QMetaType::create()
*/
void QMetaType::destroy(void *data) const
{
    if (d_ptr && d_ptr->dtor) {
        d_ptr->dtor(d_ptr, data);
        if (d_ptr->alignment > __STDCPP_DEFAULT_NEW_ALIGNMENT__) {
            operator delete(data, std::align_val_t(d_ptr->alignment));
        } else {
            operator delete(data);
        }
    }
}

/*!
    \fn void *QMetaType::construct(void *where, const void *copy = 0) const
    \since 5.0

    Constructs a value of the type that this QMetaType instance
    was constructed for in the existing memory addressed by \a where,
    that is a copy of \a copy, and returns \a where. If \a copy is
    zero, the value is default constructed.

    This is a low-level function for explicitly managing the memory
    used to store the type. Consider calling create() if you don't
    need this level of control (that is, use "new" rather than
    "placement new").

    You must ensure that \a where points to a location where the new
    value can be stored and that \a where is suitably aligned.
    The type's size can be queried by calling sizeOf().

    The rule of thumb for alignment is that a type is aligned to its
    natural boundary, which is the smallest power of 2 that is bigger
    than the type, unless that alignment is larger than the maximum
    useful alignment for the platform. For practical purposes,
    alignment larger than 2 * sizeof(void*) is only necessary for
    special hardware instructions (e.g., aligned SSE loads and stores
    on x86).
*/
void *QMetaType::construct(void *where, const void *copy) const
{
    if (!where)
        return nullptr;
    if (d_ptr) {
        if (copy && d_ptr->copyCtr) {
            d_ptr->copyCtr(d_ptr, where, copy);
            return where;
        } else if (!copy && d_ptr->defaultCtr) {
            d_ptr->defaultCtr(d_ptr, where);
            return where;
        }
    }
    return nullptr;
}

/*!
    \fn void QMetaType::destruct(void *data) const
    \since 5.0

    Destructs the value, located at \a data, assuming that it is
    of the type for which this QMetaType instance was constructed for.

    Unlike destroy(), this function only invokes the type's
    destructor, it doesn't invoke the delete operator.
    \sa QMetaType::construct()
*/
void QMetaType::destruct(void *data) const
{
    if (!data)
        return;
    if (d_ptr && d_ptr->dtor) {
        d_ptr->dtor(d_ptr, data);
        return;
    }
}

void QtMetaTypePrivate::derefAndDestroy(NS(QtPrivate::QMetaTypeInterface) *d_ptr)
{
    if (d_ptr && !d_ptr->ref.deref()) {
        if (auto reg = customTypeRegistry())
            reg->unregisterDynamicType(d_ptr->typeId.loadRelaxed());
        Q_ASSERT(d_ptr->deleteSelf);
        d_ptr->deleteSelf(d_ptr);
    }
}

/*!
    \fn QMetaType::~QMetaType()

    Destructs this object.
*/
QMetaType::~QMetaType()
{
    QtMetaTypePrivate::derefAndDestroy(d_ptr);
}

QMetaType::QMetaType(QtPrivate::QMetaTypeInterface *d) : d_ptr(d)
{
    if (d_ptr)
        d_ptr->ref.ref();
}

QMetaType::QMetaType() : d_ptr(nullptr) {}

QMetaType::QMetaType(const QMetaType &other) : QMetaType(other.d_ptr) {}
QMetaType &QMetaType::operator=(const QMetaType &other)
{
    if (d_ptr != other.d_ptr) {
        this->~QMetaType();
        new (this) QMetaType(other.d_ptr);
    }
    return *this;
}

/*!
    \fn template<typename T> QMetaType QMetaType::fromType()
    \since 5.15

    Returns the QMetaType corresponding to the type in the template parameter.
*/

/*! \fn bool operator==(const QMetaType &a, const QMetaType &b)
    \since 5.15
    \relates QMetaType
    \overload

    Returns \c true if the QMetaType \a a represents the same type
    as the QMetaType \a b, otherwise returns \c false.
*/

/*! \fn bool operator!=(const QMetaType &a, const QMetaType &b)
    \since 5.15
    \relates QMetaType
    \overload

    Returns \c true if the QMetaType \a a represents a different type
    than the QMetaType \a b, otherwise returns \c false.
*/

#define QT_ADD_STATIC_METATYPE(MetaTypeName, MetaTypeId, RealName) \
    { #RealName, sizeof(#RealName) - 1, MetaTypeId },

#define QT_ADD_STATIC_METATYPE_ALIASES_ITER(MetaTypeName, MetaTypeId, AliasingName, RealNameStr) \
    { RealNameStr, sizeof(RealNameStr) - 1, QMetaType::MetaTypeName },



static const struct { const char * typeName; int typeNameLength; int type; } types[] = {
    QT_FOR_EACH_STATIC_TYPE(QT_ADD_STATIC_METATYPE)
    QT_FOR_EACH_STATIC_ALIAS_TYPE(QT_ADD_STATIC_METATYPE_ALIASES_ITER)
    QT_ADD_STATIC_METATYPE(_, QMetaTypeId2<qreal>::MetaType, qreal)
    {nullptr, 0, QMetaType::UnknownType}
};

Q_CORE_EXPORT const QMetaTypeModuleHelper *qMetaTypeGuiHelper = nullptr;
Q_CORE_EXPORT const QMetaTypeModuleHelper *qMetaTypeWidgetsHelper = nullptr;

static const QMetaTypeModuleHelper *qModuleHelperForType(int type)
{
    if (type >= QMetaType::FirstGuiType && type <= QMetaType::LastGuiType)
        return qMetaTypeGuiHelper;
    else if (type >= QMetaType::FirstWidgetsType && type <= QMetaType::LastWidgetsType)
        return qMetaTypeWidgetsHelper;
    return nullptr;
}

template<typename T, typename Key>
class QMetaTypeFunctionRegistry
{
public:
    ~QMetaTypeFunctionRegistry()
    {
        const QWriteLocker locker(&lock);
        map.clear();
    }

    bool contains(Key k) const
    {
        const QReadLocker locker(&lock);
        return map.contains(k);
    }

    bool insertIfNotContains(Key k, const T *f)
    {
        const QWriteLocker locker(&lock);
        const T* &fun = map[k];
        if (fun)
            return false;
        fun = f;
        return true;
    }

    const T *function(Key k) const
    {
        const QReadLocker locker(&lock);
        return map.value(k, nullptr);
    }

    void remove(int from, int to)
    {
        const Key k(from, to);
        const QWriteLocker locker(&lock);
        map.remove(k);
    }
private:
    mutable QReadWriteLock lock;
    QHash<Key, const T *> map;
};

typedef QMetaTypeFunctionRegistry<QtPrivate::AbstractConverterFunction,QPair<int,int> >
QMetaTypeConverterRegistry;
typedef QMetaTypeFunctionRegistry<QtPrivate::AbstractComparatorFunction,int>
QMetaTypeComparatorRegistry;
typedef QMetaTypeFunctionRegistry<QtPrivate::AbstractDebugStreamFunction,int>
QMetaTypeDebugStreamRegistry;

Q_GLOBAL_STATIC(QMetaTypeConverterRegistry, customTypesConversionRegistry)
Q_GLOBAL_STATIC(QMetaTypeComparatorRegistry, customTypesComparatorRegistry)
Q_GLOBAL_STATIC(QMetaTypeDebugStreamRegistry, customTypesDebugStreamRegistry)

/*!
    \fn bool QMetaType::registerConverter()
    \since 5.2
    Registers the possibility of an implicit conversion from type From to type To in the meta
    type system. Returns \c true if the registration succeeded, otherwise false.
*/

/*!
    \fn  template<typename MemberFunction, int> bool QMetaType::registerConverter(MemberFunction function)
    \since 5.2
    \overload
    Registers a method \a function like To From::function() const as converter from type From
    to type To in the meta type system. Returns \c true if the registration succeeded, otherwise false.
*/

/*!
    \fn template<typename MemberFunctionOk, char> bool QMetaType::registerConverter(MemberFunctionOk function)
    \since 5.2
    \overload
    Registers a method \a function like To From::function(bool *ok) const as converter from type From
    to type To in the meta type system. Returns \c true if the registration succeeded, otherwise false.
*/

/*!
    \fn template<typename UnaryFunction> bool QMetaType::registerConverter(UnaryFunction function)
    \since 5.2
    \overload
    Registers a unary function object \a function as converter from type From
    to type To in the meta type system. Returns \c true if the registration succeeded, otherwise false.
*/

/*!
    \fn bool QMetaType::registerComparators()
    \since 5.2
    Registers comparison operators for the user-registered type T. This requires T to have
    both an operator== and an operator<.
    Returns \c true if the registration succeeded, otherwise false.
*/

/*!
    \fn bool QMetaType::registerEqualsComparator()
    \since 5.5
    Registers equals operator for the user-registered type T. This requires T to have
    an operator==.
    Returns \c true if the registration succeeded, otherwise false.
*/

#ifndef QT_NO_DEBUG_STREAM
/*!
    \fn bool QMetaType::registerDebugStreamOperator()
    Registers the debug stream operator for the user-registered type T. This requires T to have
    an operator<<(QDebug dbg, T).
    Returns \c true if the registration succeeded, otherwise false.
*/
#endif

/*!
    Registers function \a f as converter function from type id \a from to \a to.
    If there's already a conversion registered, this does nothing but deleting \a f.
    Returns \c true if the registration succeeded, otherwise false.
    \since 5.2
    \internal
*/
bool QMetaType::registerConverterFunction(const QtPrivate::AbstractConverterFunction *f, int from, int to)
{
    if (!customTypesConversionRegistry()->insertIfNotContains(qMakePair(from, to), f)) {
        qWarning("Type conversion already registered from type %s to type %s",
                 QMetaType::typeName(from), QMetaType::typeName(to));
        return false;
    }
    return true;
}

/*!
    \internal

    Invoked automatically when a converter function object is destroyed.
 */
void QMetaType::unregisterConverterFunction(int from, int to)
{
    if (customTypesConversionRegistry.isDestroyed())
        return;
    customTypesConversionRegistry()->remove(from, to);
}

bool QMetaType::registerComparatorFunction(const QtPrivate::AbstractComparatorFunction *f, int type)
{
    if (!customTypesComparatorRegistry()->insertIfNotContains(type, f)) {
        qWarning("Comparators already registered for type %s", QMetaType::typeName(type));
        return false;
    }
    return true;
}

/*!
    \fn bool QMetaType::hasRegisteredComparators()
    Returns \c true, if the meta type system has registered comparators for type T.
    \since 5.2
 */

/*!
    Returns \c true, if the meta type system has registered comparators for type id \a typeId.
    \since 5.2
 */
bool QMetaType::hasRegisteredComparators(int typeId)
{
    return customTypesComparatorRegistry()->contains(typeId);
}

#ifndef QT_NO_DEBUG_STREAM
bool QMetaType::registerDebugStreamOperatorFunction(const QtPrivate::AbstractDebugStreamFunction *f,
                                                    int type)
{
    if (!customTypesDebugStreamRegistry()->insertIfNotContains(type, f)) {
        qWarning("Debug stream operator already registered for type %s", QMetaType::typeName(type));
        return false;
    }
    return true;
}

/*!
    \fn bool QMetaType::hasRegisteredDebugStreamOperator()
    Returns \c true, if the meta type system has a registered debug stream operator for type T.
    \since 5.2
 */

/*!
    Returns \c true, if the meta type system has a registered debug stream operator for type
    id \a typeId.
    \since 5.2
*/
bool QMetaType::hasRegisteredDebugStreamOperator(int typeId)
{
    return customTypesDebugStreamRegistry()->contains(typeId);
}
#endif

/*!
    Converts the object at \a from from \a fromTypeId to the preallocated space at \a to
    typed \a toTypeId. Returns \c true, if the conversion succeeded, otherwise false.
    \since 5.2
*/
bool QMetaType::convert(const void *from, int fromTypeId, void *to, int toTypeId)
{
    const QtPrivate::AbstractConverterFunction * const f =
        customTypesConversionRegistry()->function(qMakePair(fromTypeId, toTypeId));
    return f && f->convert(f, from, to);
}

/*!
    Compares the objects at \a lhs and \a rhs. Both objects need to be of type \a typeId.
    \a result is set to less than, equal to or greater than zero, if \a lhs is less than, equal to
    or greater than \a rhs. Returns \c true, if the comparison succeeded, otherwise \c false.
    \since 5.2
*/
bool QMetaType::compare(const void *lhs, const void *rhs, int typeId, int* result)
{
    const QtPrivate::AbstractComparatorFunction * const f =
        customTypesComparatorRegistry()->function(typeId);
    if (!f)
        return false;
    if (f->equals(f, lhs, rhs))
        *result = 0;
    else if (f->lessThan)
        *result = f->lessThan(f, lhs, rhs) ? -1 : 1;
    else
        return false;
    return true;
}

/*!
    Compares the objects at \a lhs and \a rhs. Both objects need to be of type \a typeId.
    \a result is set to zero, if \a lhs equals to rhs. Returns \c true, if the comparison
    succeeded, otherwise \c false.
    \since 5.5
*/
bool QMetaType::equals(const void *lhs, const void *rhs, int typeId, int *result)
{
    const QtPrivate::AbstractComparatorFunction * const f
        = customTypesComparatorRegistry()->function(typeId);
    if (!f)
        return false;
    if (f->equals(f, lhs, rhs))
        *result = 0;
    else
        *result = -1;
    return true;
}

/*!
    Streams the object at \a rhs of type \a typeId to the debug stream \a dbg. Returns \c true
    on success, otherwise false.
    \since 5.2
*/
bool QMetaType::debugStream(QDebug& dbg, const void *rhs, int typeId)
{
    const QtPrivate::AbstractDebugStreamFunction * const f = customTypesDebugStreamRegistry()->function(typeId);
    if (!f)
        return false;
    f->stream(f, dbg, rhs);
    return true;
}

/*!
    \fn bool QMetaType::hasRegisteredConverterFunction()
    Returns \c true, if the meta type system has a registered conversion from type From to type To.
    \since 5.2
    \overload
    */

/*!
    Returns \c true, if the meta type system has a registered conversion from meta type id \a fromTypeId
    to \a toTypeId
    \since 5.2
*/
bool QMetaType::hasRegisteredConverterFunction(int fromTypeId, int toTypeId)
{
    return customTypesConversionRegistry()->contains(qMakePair(fromTypeId, toTypeId));
}

#ifndef QT_NO_DATASTREAM
/*!
    \internal
*/
void QMetaType::registerStreamOperators(const char *typeName, SaveOperator saveOp,
                                        LoadOperator loadOp)
{
    registerStreamOperators(type(typeName), saveOp, loadOp);
}

/*!
    \internal
*/
void QMetaType::registerStreamOperators(int idx, SaveOperator saveOp,
                                        LoadOperator loadOp)
{
    if (idx < User)
        return; //builtin types should not be registered;

    if (auto reg = customTypeRegistry()) {
        QWriteLocker locker(&reg->lock);
        reg->dataStreamOp[idx] = { saveOp, loadOp };
    }
}
#endif // QT_NO_DATASTREAM

// We don't officially support constexpr in MSVC 2015, but the limited support it
// has is enough for the code below.

#define STRINGIFY_TYPE_NAME(MetaTypeName, TypeId, RealName) \
    #RealName "\0"
#define CALCULATE_TYPE_LEN(MetaTypeName, TypeId, RealName) \
    short(sizeof(#RealName)),
#define MAP_TYPE_ID_TO_IDX(MetaTypeName, TypeId, RealName) \
    TypeId,

namespace {
// All type names in one long string.
constexpr char metaTypeStrings[] = QT_FOR_EACH_STATIC_TYPE(STRINGIFY_TYPE_NAME);

// The sizes of the strings in the metaTypeStrings string (including terminating null)
constexpr short metaTypeNameSizes[] = {
    QT_FOR_EACH_STATIC_TYPE(CALCULATE_TYPE_LEN)
};

// The type IDs, in the order of the metaTypeStrings data
constexpr short metaTypeIds[] = {
    QT_FOR_EACH_STATIC_TYPE(MAP_TYPE_ID_TO_IDX)
};

constexpr int MetaTypeNameCount = sizeof(metaTypeNameSizes) / sizeof(metaTypeNameSizes[0]);

template <typename IntegerSequence> struct MetaTypeOffsets;
template <int... TypeIds> struct MetaTypeOffsets<QtPrivate::IndexesList<TypeIds...>>
{
    // This would have been a lot easier if the meta types that the macro
    // QT_FOR_EACH_STATIC_TYPE declared were in sorted, ascending order, but
    // they're not (i.e., the first one declared is QMetaType::Void == 43,
    // followed by QMetaType::Bool == 1)... As a consequence, we need to use
    // the C++11 constexpr function calculateOffsetForTypeId below in order to
    // create the offset array.

    static constexpr int findTypeId(int typeId, int i = 0)
    {
        return i >= MetaTypeNameCount ? -1 :
                metaTypeIds[i] == typeId ? i : findTypeId(typeId, i + 1);
    }

    static constexpr short calculateOffsetForIdx(int i)
    {
        return i < 0 ? -1 :
               i == 0 ? 0 : metaTypeNameSizes[i - 1] + calculateOffsetForIdx(i - 1);
    }

    static constexpr short calculateOffsetForTypeId(int typeId)
    {
        return calculateOffsetForIdx(findTypeId(typeId));
#if 0
        // same as, but this is only valid in C++14:
        short offset = 0;
        for (int i = 0; i < MetaTypeNameCount; ++i) {
            if (metaTypeIds[i] == typeId)
                return offset;
            offset += metaTypeNameSizes[i];
        }
        return -1;
#endif
    }

    short offsets[sizeof...(TypeIds)];
    constexpr MetaTypeOffsets() : offsets{calculateOffsetForTypeId(TypeIds)...} {}

    const char *operator[](int typeId) const noexcept
    {
        short o = offsets[typeId];
        return o < 0 ? nullptr : metaTypeStrings + o;
    }
};
} // anonymous namespace

constexpr MetaTypeOffsets<QtPrivate::Indexes<QMetaType::HighestInternalId + 1>::Value> metaTypeNames {};
#undef STRINGIFY_TYPE_NAME
#undef CALCULATE_TYPE_LEN
#undef MAP_TYPE_ID_TO_IDX

/*!
    Returns the type name associated with the given \a typeId, or a null
    pointer if no matching type was found. The returned pointer must not be
    deleted.

    \sa type(), isRegistered(), Type, name()
*/
const char *QMetaType::typeName(int typeId)
{
    const uint type = typeId;
    if (Q_LIKELY(type <= QMetaType::HighestInternalId)) {
        return metaTypeNames[typeId];
    } else if (Q_UNLIKELY(type < QMetaType::User)) {
        return nullptr; // It can happen when someone cast int to QVariant::Type, we should not crash...
    }

    if (auto reg = customTypeRegistry()) {
        if (auto ti = reg->getCustomType(typeId))
            return ti->name;
    }
    return nullptr;
}

/*!
    \since 5.15

    Returns the type name associated with this QMetaType, or a null
    pointer if no matching type was found. The returned pointer must not be
    deleted.

    \sa typeName()
*/
QByteArray QMetaType::name() const
{
    return d_ptr ? d_ptr->name : nullptr;
}

/*
    Similar to QMetaType::type(), but only looks in the static set of types.
*/
static inline int qMetaTypeStaticType(const char *typeName, int length)
{
    int i = 0;
    while (types[i].typeName && ((length != types[i].typeNameLength)
                                 || memcmp(typeName, types[i].typeName, length))) {
        ++i;
    }
    return types[i].type;
}

/*
    Similar to QMetaType::type(), but only looks in the custom set of
    types, and doesn't lock the mutex.

*/
static int qMetaTypeCustomType_unlocked(const char *typeName, int length)
{
    if (auto reg = customTypeRegistry()) {
#if QT_CONFIG(thread)
        Q_ASSERT(!reg->lock.tryLockForWrite());
#endif
        if (auto ti = reg->aliases.value(QByteArray(typeName, length), nullptr)) {
            return ti->typeId;
        }
    }
    return QMetaType::UnknownType;
}

/*!
    \internal

    Registers a user type for marshalling, as an alias of another type (typedef).
    Note that normalizedTypeName is not checked for conformance with Qt's normalized format,
    so it must already conform.
*/
void QMetaType::registerNormalizedTypedef(const NS(QByteArray) & normalizedTypeName,
                                          QMetaType metaType)
{
    if (!metaType.isValid())
        return;
    if (auto reg = customTypeRegistry()) {
        QWriteLocker lock(&reg->lock);
        auto &al = reg->aliases[normalizedTypeName];
        if (al)
            return;
        al = metaType.d_ptr;
    }
}

/*!
    Returns \c true if the datatype with ID \a type is registered;
    otherwise returns \c false.

    \sa type(), typeName(), Type
*/
bool QMetaType::isRegistered(int type)
{
    return QMetaType(type).isRegistered();
}

template <bool tryNormalizedType>
static inline int qMetaTypeTypeImpl(const char *typeName, int length)
{
    if (!length)
        return QMetaType::UnknownType;
    int type = qMetaTypeStaticType(typeName, length);
    if (type == QMetaType::UnknownType) {
        QReadLocker locker(&customTypeRegistry()->lock);
        type = qMetaTypeCustomType_unlocked(typeName, length);
#ifndef QT_NO_QOBJECT
        if ((type == QMetaType::UnknownType) && tryNormalizedType) {
            const NS(QByteArray) normalizedTypeName = QMetaObject::normalizedType(typeName);
            type = qMetaTypeStaticType(normalizedTypeName.constData(),
                                       normalizedTypeName.size());
            if (type == QMetaType::UnknownType) {
                type = qMetaTypeCustomType_unlocked(normalizedTypeName.constData(),
                                                    normalizedTypeName.size());
            }
        }
#endif
    }
    return type;
}

/*!
    Returns a handle to the type called \a typeName, or QMetaType::UnknownType if there is
    no such type.

    \sa isRegistered(), typeName(), Type
*/
int QMetaType::type(const char *typeName)
{
    return qMetaTypeTypeImpl</*tryNormalizedType=*/true>(typeName, qstrlen(typeName));
}

/*!
    \a internal

    Similar to QMetaType::type(); the only difference is that this function
    doesn't attempt to normalize the type name (i.e., the lookup will fail
    for type names in non-normalized form).
*/
int qMetaTypeTypeInternal(const char *typeName)
{
    return qMetaTypeTypeImpl</*tryNormalizedType=*/false>(typeName, qstrlen(typeName));
}

/*!
    \since 5.5
    \overload

    Returns a handle to the type called \a typeName, or 0 if there is
    no such type.

    \sa isRegistered(), typeName()
*/
int QMetaType::type(const QT_PREPEND_NAMESPACE(QByteArray) &typeName)
{
    return qMetaTypeTypeImpl</*tryNormalizedType=*/true>(typeName.constData(), typeName.size());
}

#ifndef QT_NO_DATASTREAM
namespace
{

template<typename T>
class HasStreamOperator
{
    struct Yes { char unused[1]; };
    struct No { char unused[2]; };
    Q_STATIC_ASSERT(sizeof(Yes) != sizeof(No));

    template<class C> static decltype(std::declval<QDataStream&>().operator>>(std::declval<C&>()), Yes()) load(int);
    template<class C> static decltype(operator>>(std::declval<QDataStream&>(), std::declval<C&>()), Yes()) load(int);
    template<class C> static No load(...);
    template<class C> static decltype(operator<<(std::declval<QDataStream&>(), std::declval<const C&>()), Yes()) saveFunction(int);
    template<class C> static decltype(std::declval<QDataStream&>().operator<<(std::declval<const C&>()), Yes()) saveMethod(int);
    template<class C> static No saveMethod(...);
    template<class C> static No saveFunction(...);
    static constexpr bool LoadValue = QtMetaTypePrivate::TypeDefinition<T>::IsAvailable && (sizeof(load<T>(0)) == sizeof(Yes));
    static constexpr bool SaveValue = QtMetaTypePrivate::TypeDefinition<T>::IsAvailable &&
        ((sizeof(saveMethod<T>(0)) == sizeof(Yes)) || (sizeof(saveFunction<T>(0)) == sizeof(Yes)));
public:
    static constexpr bool Value = LoadValue && SaveValue;
};

// Quick sanity checks
Q_STATIC_ASSERT(HasStreamOperator<NS(QJsonDocument)>::Value);
Q_STATIC_ASSERT(!HasStreamOperator<void*>::Value);
Q_STATIC_ASSERT(HasStreamOperator<qint8>::Value);

template<typename T, bool IsAcceptedType = DefinedTypesFilter::Acceptor<T>::IsAccepted && HasStreamOperator<T>::Value>
struct FilteredOperatorSwitch
{
    static bool load(QDataStream &stream, T *data, int)
    {
        stream >> *data;
        return true;
    }
    static bool save(QDataStream &stream, const T *data, int)
    {
        stream << *data;
        return true;
    }
};

template<typename T>
struct FilteredOperatorSwitch<T, /* IsAcceptedType = */ false>
{
    static const QMetaTypeModuleHelper *getMetaTypeInterface()
    {
        if (QModulesPrivate::QTypeModuleInfo<T>::IsGui)
            return qMetaTypeGuiHelper;
        else if (QModulesPrivate::QTypeModuleInfo<T>::IsWidget)
            return qMetaTypeWidgetsHelper;
        return nullptr;
    }
    static bool save(QDataStream &stream, const T *data, int type)
    {
        if (auto interface = getMetaTypeInterface()) {
            return interface->save(stream, type, data);
        }
        return false;
    }
    static bool load(QDataStream &stream, T *data, int type)
    {
        if (auto interface = getMetaTypeInterface()) {
            return interface->load(stream, type, data);
        }
        return false;
    }
};

class SaveOperatorSwitch
{
public:
    QDataStream &stream;
    int m_type;

    template<typename T>
    bool delegate(const T *data)
    {
        return FilteredOperatorSwitch<T>::save(stream, data, m_type);
    }
    bool delegate(const char *data)
    {
        // force a char to be signed
        stream << qint8(*data);
        return true;
    }
    bool delegate(const long *data)
    {
        stream << qlonglong(*data);
        return true;
    }
    bool delegate(const unsigned long *data)
    {
        stream << qulonglong(*data);
        return true;
    }
    bool delegate(const QMetaTypeSwitcher::NotBuiltinType *data)
    {
        auto ct = customTypeRegistry();
        if (!ct)
            return false;
        QMetaType::SaveOperator op = nullptr;
        {
            QReadLocker lock(&ct->lock);
            op = ct->dataStreamOp.value(m_type).saveOp;
        }
        if (!op)
            return false;
        op(stream, data);
        return true;
    }
    bool delegate(const void*) { return false; }
    bool delegate(const QMetaTypeSwitcher::UnknownType*) { return false; }
};
class LoadOperatorSwitch
{
public:
    QDataStream &stream;
    int m_type;

    template<typename T>
    bool delegate(const T *data)
    {
        return FilteredOperatorSwitch<T>::load(stream, const_cast<T*>(data), m_type);
    }
    bool delegate(const char *data)
    {
        // force a char to be signed
        qint8 c;
        stream >> c;
        *const_cast<char*>(data) = c;
        return true;
    }
    bool delegate(const long *data)
    {
        qlonglong l;
        stream >> l;
        *const_cast<long*>(data) = l;
        return true;
    }
    bool delegate(const unsigned long *data)
    {
        qlonglong l;
        stream >> l;
        *const_cast<unsigned long*>(data) = l;
        return true;
    }
    bool delegate(const QMetaTypeSwitcher::NotBuiltinType *data)
    {
        auto ct = customTypeRegistry();
        if (!ct)
            return false;
        QMetaType::LoadOperator op = nullptr;
        {
            QReadLocker lock(&ct->lock);
            op = ct->dataStreamOp.value(m_type).loadOp;
        }
        if (!op)
            return false;
        op(stream, const_cast<QMetaTypeSwitcher::NotBuiltinType *>(data));
        return true;
    }
    bool delegate(const void*) { return false; }
    bool delegate(const QMetaTypeSwitcher::UnknownType*) { return false; }
};
}  // namespace

/*!
    Writes the object pointed to by \a data with the ID \a type to
    the given \a stream. Returns \c true if the object is saved
    successfully; otherwise returns \c false.

    The type must have been registered with qRegisterMetaType() and
    qRegisterMetaTypeStreamOperators() beforehand.

    Normally, you should not need to call this function directly.
    Instead, use QVariant's \c operator<<(), which relies on save()
    to stream custom types.

    \sa load(), qRegisterMetaTypeStreamOperators()
*/
bool QMetaType::save(QDataStream &stream, int type, const void *data)
{
    if (!data)
        return false;
    SaveOperatorSwitch saveOp{stream, type};
    return QMetaTypeSwitcher::switcher<bool>(saveOp, type, data);
}

/*!
    Reads the object of the specified \a type from the given \a
    stream into \a data. Returns \c true if the object is loaded
    successfully; otherwise returns \c false.

    The type must have been registered with qRegisterMetaType() and
    qRegisterMetaTypeStreamOperators() beforehand.

    Normally, you should not need to call this function directly.
    Instead, use QVariant's \c operator>>(), which relies on load()
    to stream custom types.

    \sa save(), qRegisterMetaTypeStreamOperators()
*/
bool QMetaType::load(QDataStream &stream, int type, void *data)
{
   if (!data)
        return false;
    LoadOperatorSwitch loadOp{stream, type};
    return QMetaTypeSwitcher::switcher<bool>(loadOp, type, data);
}
#endif // QT_NO_DATASTREAM

/*!
    Returns a copy of \a copy, assuming it is of type \a type. If \a
    copy is zero, creates a default constructed instance.

    \sa destroy(), isRegistered(), Type
*/
void *QMetaType::create(int type, const void *copy)
{
    return QMetaType(type).create(copy);
}

/*!
    Destroys the \a data, assuming it is of the \a type given.

    \sa create(), isRegistered(), Type
*/
void QMetaType::destroy(int type, void *data)
{
    QMetaType(type).destroy(data);
}

/*!
    \since 5.0

    Constructs a value of the given \a type in the existing memory
    addressed by \a where, that is a copy of \a copy, and returns
    \a where. If \a copy is zero, the value is default constructed.

    This is a low-level function for explicitly managing the memory
    used to store the type. Consider calling create() if you don't
    need this level of control (that is, use "new" rather than
    "placement new").

    You must ensure that \a where points to a location that can store
    a value of type \a type, and that \a where is suitably aligned.
    The type's size can be queried by calling sizeOf().

    The rule of thumb for alignment is that a type is aligned to its
    natural boundary, which is the smallest power of 2 that is bigger
    than the type, unless that alignment is larger than the maximum
    useful alignment for the platform. For practical purposes,
    alignment larger than 2 * sizeof(void*) is only necessary for
    special hardware instructions (e.g., aligned SSE loads and stores
    on x86).

    \sa destruct(), sizeOf()
*/
void *QMetaType::construct(int type, void *where, const void *copy)
{
    return QMetaType(type).construct(where, copy);
}


/*!
    \since 5.0

    Destructs the value of the given \a type, located at \a where.

    Unlike destroy(), this function only invokes the type's
    destructor, it doesn't invoke the delete operator.

    \sa construct()
*/
void QMetaType::destruct(int type, void *where)
{
    return QMetaType(type).destruct(where);
}

/*!
    \since 5.0

    Returns the size of the given \a type in bytes (i.e. sizeof(T),
    where T is the actual type identified by the \a type argument).

    This function is typically used together with construct()
    to perform low-level management of the memory used by a type.

    \sa construct()
*/
int QMetaType::sizeOf(int type)
{
    return QMetaType(type).sizeOf();
}

/*!
    \since 5.0

    Returns flags of the given \a type.

    \sa QMetaType::TypeFlags
*/
QMetaType::TypeFlags QMetaType::typeFlags(int type)
{
    return QMetaType(type).flags();
}


/*!
    \since 5.0

    returns QMetaType::metaObject for \a type

    \sa metaObject()
*/
const QMetaObject *QMetaType::metaObjectForType(int type)
{
    return QMetaType(type).metaObject();
}

/*!
    \fn int qRegisterMetaType(const char *typeName)
    \relates QMetaType
    \threadsafe

    Registers the type name \a typeName for the type \c{T}. Returns
    the internal ID used by QMetaType. Any class or struct that has a
    public default constructor, a public copy constructor and a public
    destructor can be registered.

    This function requires that \c{T} is a fully defined type at the point
    where the function is called. For pointer types, it also requires that the
    pointed to type is fully defined. Use Q_DECLARE_OPAQUE_POINTER() to be able
    to register pointers to forward declared types.

    After a type has been registered, you can create and destroy
    objects of that type dynamically at run-time.

    This example registers the class \c{MyClass}:

    \snippet code/src_corelib_kernel_qmetatype.cpp 4

    This function is useful to register typedefs so they can be used
    by QMetaProperty, or in QueuedConnections

    \snippet code/src_corelib_kernel_qmetatype.cpp 9

    \warning This function is useful only for registering an alias (typedef)
    for every other use case Q_DECLARE_METATYPE and qMetaTypeId() should be used instead.

    \sa {QMetaType::}{qRegisterMetaTypeStreamOperators()}, {QMetaType::}{isRegistered()},
        Q_DECLARE_METATYPE()
*/

/*!
    \fn void qRegisterMetaTypeStreamOperators(const char *typeName)
    \relates QMetaType
    \threadsafe

    Registers the stream operators for the type \c{T} called \a
    typeName.

    Afterward, the type can be streamed using QMetaType::load() and
    QMetaType::save(). These functions are used when streaming a
    QVariant.

    \snippet code/src_corelib_kernel_qmetatype.cpp 5

    The stream operators should have the following signatures:

    \snippet code/src_corelib_kernel_qmetatype.cpp 6

    \sa qRegisterMetaType(), QMetaType::isRegistered(), Q_DECLARE_METATYPE()
*/

/*!
    \fn int qRegisterMetaType()
    \relates QMetaType
    \threadsafe
    \since 4.2

    Call this function to register the type \c T. \c T must be declared with
    Q_DECLARE_METATYPE(). Returns the meta type Id.

    Example:

    \snippet code/src_corelib_kernel_qmetatype.cpp 7

    This function requires that \c{T} is a fully defined type at the point
    where the function is called. For pointer types, it also requires that the
    pointed to type is fully defined. Use Q_DECLARE_OPAQUE_POINTER() to be able
    to register pointers to forward declared types.

    After a type has been registered, you can create and destroy
    objects of that type dynamically at run-time.

    To use the type \c T in QVariant, using Q_DECLARE_METATYPE() is
    sufficient. To use the type \c T in queued signal and slot connections,
    \c{qRegisterMetaType<T>()} must be called before the first connection
    is established.

    Also, to use type \c T with the QObject::property() API,
    \c{qRegisterMetaType<T>()} must be called before it is used, typically
    in the constructor of the class that uses \c T, or in the \c{main()}
    function.

    \sa Q_DECLARE_METATYPE()
 */

/*!
    \fn int qMetaTypeId()
    \relates QMetaType
    \threadsafe
    \since 4.1

    Returns the meta type id of type \c T at compile time. If the
    type was not declared with Q_DECLARE_METATYPE(), compilation will
    fail.

    Typical usage:

    \snippet code/src_corelib_kernel_qmetatype.cpp 8

    QMetaType::type() returns the same ID as qMetaTypeId(), but does
    a lookup at runtime based on the name of the type.
    QMetaType::type() is a bit slower, but compilation succeeds if a
    type is not registered.

    \sa Q_DECLARE_METATYPE(), QMetaType::type()
*/

static QtPrivate::QMetaTypeInterface *interfaceForType(int typeId)
{
    if (typeId >= QMetaType::User) {
        if (auto reg = customTypeRegistry())
            return reg->getCustomType(typeId);
    }
    if (auto moduleHelper = qModuleHelperForType(typeId))
        return moduleHelper->interfaceForType(typeId);

    switch (typeId) {
        QT_FOR_EACH_STATIC_PRIMITIVE_TYPE(QT_METATYPE_CONVERT_ID_TO_TYPE)
        QT_FOR_EACH_STATIC_PRIMITIVE_POINTER(QT_METATYPE_CONVERT_ID_TO_TYPE)
        QT_FOR_EACH_STATIC_CORE_CLASS(QT_METATYPE_CONVERT_ID_TO_TYPE)
        QT_FOR_EACH_STATIC_CORE_POINTER(QT_METATYPE_CONVERT_ID_TO_TYPE)
        QT_FOR_EACH_STATIC_CORE_TEMPLATE(QT_METATYPE_CONVERT_ID_TO_TYPE)
    default:
        return nullptr;
    }
}

/*!
     \fn QMetaType::QMetaType(const int typeId)
     \since 5.0

     Constructs a QMetaType object that contains all information about type \a typeId.

     \note The default parameter was added in Qt 5.15.
*/
QMetaType::QMetaType(int typeId) : QMetaType(interfaceForType(typeId)) {}

namespace QtMetaTypePrivate {
const bool VectorBoolElements::true_element = true;
const bool VectorBoolElements::false_element = false;
}

namespace QtPrivate {
#ifndef QT_BOOTSTRAPPED
// Explicit instantiation definition
#define QT_METATYPE_DECLARE_TEMPLATE_ITER(TypeName, Id, Name) template class QMetaTypeForType<Name>;
QT_FOR_EACH_STATIC_PRIMITIVE_TYPE(QT_METATYPE_DECLARE_TEMPLATE_ITER)
QT_FOR_EACH_STATIC_PRIMITIVE_POINTER(QT_METATYPE_DECLARE_TEMPLATE_ITER)
QT_FOR_EACH_STATIC_CORE_CLASS(QT_METATYPE_DECLARE_TEMPLATE_ITER)
QT_FOR_EACH_STATIC_CORE_POINTER(QT_METATYPE_DECLARE_TEMPLATE_ITER)
QT_FOR_EACH_STATIC_CORE_TEMPLATE(QT_METATYPE_DECLARE_TEMPLATE_ITER)
#undef QT_METATYPE_DECLARE_TEMPLATE_ITER
#endif
}

QT_END_NAMESPACE
