// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2021 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
// Copyright (C) 2021 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmetatype.h"
#include "qmetatype_p.h"
#include "qobject.h"
#include "qobjectdefs.h"
#include "qdatetime.h"
#include "qbytearray.h"
#include "qreadwritelock.h"
#include "qhash.h"
#include "qmap.h"
#include "qstring.h"
#include "qstringlist.h"
#include "qlist.h"
#include "qlocale.h"
#include "qdebug.h"
#if QT_CONFIG(easingcurve)
#include "qeasingcurve.h"
#endif
#include "quuid.h"
#include "qvariant.h"
#include "qdatastream.h"

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
#  include "qmetaobject.h"
#  include "qsequentialiterable.h"
#  include "qassociativeiterable.h"
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
#include <cstring>

QT_BEGIN_NAMESPACE

#define NS(x) QT_PREPEND_NAMESPACE(x)

QT_IMPL_METATYPE_EXTERN_TAGGED(QtMetaTypePrivate::QPairVariantInterfaceImpl, QPairVariantInterfaceImpl)

using QtMetaTypePrivate::isInterfaceFor;

namespace {
struct QMetaTypeDeleter
{
    const QtPrivate::QMetaTypeInterface *iface;
    void operator()(void *data)
    {
        if (iface->alignment > __STDCPP_DEFAULT_NEW_ALIGNMENT__) {
            operator delete(data, std::align_val_t(iface->alignment));
        } else {
            operator delete(data);
        }
    }
};

struct QMetaTypeCustomRegistry
{

#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0) && !defined(QT_BOOTSTRAPPED)
    QMetaTypeCustomRegistry()
    {
        /* qfloat16 was neither a builtin, nor unconditionally registered
          in QtCore in Qt <= 6.2.
          Inserting it as an alias ensures that a QMetaType::id call
          will get the correct built-in type-id (the interface pointers
          might still not match, but we already deal with that case.
        */
        aliases.insert("qfloat16", QtPrivate::qMetaTypeInterfaceForType<qfloat16>());
    }
#endif

    QReadWriteLock lock;
    QList<const QtPrivate::QMetaTypeInterface *> registry;
    QHash<QByteArray, const QtPrivate::QMetaTypeInterface *> aliases;
    // index of first empty (unregistered) type in registry, if any.
    int firstEmpty = 0;

    int registerCustomType(const QtPrivate::QMetaTypeInterface *cti)
    {
        // we got here because cti->typeId is 0, so this is a custom meta type
        // (not read-only)
        auto ti = const_cast<QtPrivate::QMetaTypeInterface *>(cti);
        {
            QWriteLocker l(&lock);
            if (int id = ti->typeId.loadRelaxed())
                return id;
            QByteArray name =
#ifndef QT_NO_QOBJECT
                    QMetaObject::normalizedType
#endif
                    (ti->name);
            if (auto ti2 = aliases.value(name)) {
                const auto id = ti2->typeId.loadRelaxed();
                ti->typeId.storeRelaxed(id);
                return id;
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
            ti->typeId.storeRelaxed(firstEmpty + QMetaType::User);
        }
        if (ti->legacyRegisterOp)
            ti->legacyRegisterOp();
        return ti->typeId.loadRelaxed();
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

    const QtPrivate::QMetaTypeInterface *getCustomType(int id)
    {
        QReadLocker l(&lock);
        return registry.value(id - QMetaType::User - 1);
    }
};

Q_GLOBAL_STATIC(QMetaTypeCustomRegistry, customTypeRegistry)

} // namespace

// used by QVariant::save(): returns the name used in the Q_DECLARE_METATYPE
// macro (one of them, indetermine which one)
const char *QtMetaTypePrivate::typedefNameForType(const QtPrivate::QMetaTypeInterface *type_d)
{
    const char *name = nullptr;
    if (!customTypeRegistry.exists())
        return name;
    QMetaTypeCustomRegistry *r = &*customTypeRegistry;

    QByteArrayView officialName(type_d->name);
    QReadLocker l(&r->lock);
    auto it = r->aliases.constBegin();
    auto end = r->aliases.constEnd();
    for ( ; it != end; ++it) {
        if (it.value() != type_d)
            continue;
        if (it.key() == officialName)
            continue;               // skip the official name
        name = it.key().constData();
        ++it;
        break;
    }

#ifndef QT_NO_DEBUG
    QByteArrayList otherNames;
    for ( ; it != end; ++it) {
        if (it.value() == type_d && it.key() != officialName)
            otherNames << it.key();
    }
    l.unlock();
    if (!otherNames.isEmpty())
        qWarning("QMetaType: type %s has more than one typedef alias: %s, %s",
                 type_d->name, name, otherNames.join(", ").constData());
#endif

    return name;
}

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
    \li QList<T>, QQueue<T>, QStack<T> or QSet<T>
        where T is a registered meta type
    \li QHash<T1, T2>, QMap<T1, T2> or QPair<T1, T2> where T1 and T2 are
        registered meta types
    \li QPointer<T>, QSharedPointer<T>, QWeakPointer<T>, where T is a class that derives from QObject
    \li Enumerations registered with Q_ENUM or Q_FLAG
    \li Classes that have a Q_GADGET macro
    \endlist

    \note This method also registers the stream and debug operators for the type if they
    are visible at registration time. As this is done automatically in some places,
    it is strongly recommended to declare the stream operators for a type directly
    after the type itself. Because of the argument dependent lookup rules of C++, it is
    also strongly recommended to declare the operators in the same namespace as the type itself.

    The stream operators should have the following signatures:

    \snippet code/src_corelib_kernel_qmetatype.cpp 6

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
    \value Char16 \c{char16_t}
    \value Char32 \c{char32_t}
    \value ULong \c{unsigned long}
    \value ULongLong ULongLong
    \value UShort \c{unsigned short}
    \value SChar \c{signed char}
    \value UChar \c{unsigned char}
    \value Float \c float
    \value Float16 qfloat16
    \omitvalue Float128
    \omitvalue BFloat16
    \omitvalue Int128
    \omitvalue UInt128
    \value QObjectStar QObject *

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
    \value QVariantPair QVariantPair
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
    \value QVariant QVariant

    \value User  Base value for user types
    \value UnknownType This is an invalid type id. It is returned from QMetaType for types that are not registered

    Additional types can be registered using qRegisterMetaType() or by calling
    registerType().

    \sa type(), typeName()
*/

/*!
    \enum QMetaType::TypeFlag

    The enum describes attributes of a type supported by QMetaType.

    \value NeedsConstruction This type has a default constructor. If the flag is not set, instances can be safely initialized with memset to 0.
    \value NeedsCopyConstruction (since 6.5) This type has a non-trivial copy constructor. If the flag is not set, instances can be copied with memcpy.
    \value NeedsMoveConstruction (since 6.5) This type has a non-trivial move constructor. If the flag is not set, instances can be moved with memcpy.
    \value NeedsDestruction This type has a non-trivial destructor. If the flag is not set, calls to the destructor are not necessary before discarding objects.
    \value RelocatableType An instance of a type having this attribute can be safely moved to a different memory location using memcpy.
    \omitvalue MovableType
    \omitvalue SharedPointerToQObject
    \value IsEnumeration This type is an enumeration.
    \value IsUnsignedEnumeration If the type is an Enumeration, its underlying type is unsigned.
    \value PointerToQObject This type is a pointer to a class derived from QObject.
    \value IsPointer This type is a pointer to another type.
    \omitvalue WeakPointerToQObject
    \omitvalue TrackingPointerToQObject
    \omitvalue IsGadget \omit (since Qt 5.5) This type is a Q_GADGET and its corresponding QMetaObject can be accessed with QMetaType::metaObject. \endomit
    \omitvalue PointerToGadget
    \omitvalue IsQmlList
    \value IsConst Indicates that values of this type are immutable; for instance, because they are pointers to const objects.

    \note Before Qt 6.5, both the NeedsConstruction and NeedsDestruction flags
    were incorrectly set if the either copy construtor or destructor were
    non-trivial (that is, if the type was not trivial).

    Note that the Needs flags may be set but the meta type may not have a
    publicly-accessible constructor of the relevant type or a
    publicly-accessible destructor.
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
    dynamically at run-time.

    Type names can be registered with QMetaType by using either
    qRegisterMetaType() or registerType(). Registration is not required for
    most operations; it's only required for operations that attempt to resolve
    a type name in string form back to a QMetaType object or the type's ID.
    Those include some old-style signal-slot connections using
    QObject::connect(), reading user-types from \l QDataStream to \l QVariant,
    or binding to other languages and IPC mechanisms, like QML, D-Bus,
    JavaScript, etc.

    The following code allocates and destructs an instance of \c{MyClass} by
    its name, which requires that \c{MyClass} have been previously registered:

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

    \sa isRegistered()
*/
bool QMetaType::isValid() const
{
    return d_ptr;
}

/*!
    \fn bool QMetaType::isRegistered() const
    \since 5.0

    Returns \c true if this QMetaType object has been registered with the Qt
    global metatype registry. Registration allows the type to be found by its
    name (using QMetaType::fromName()) or by its ID (using the constructor).

    \sa qRegisterMetaType(), isValid()
*/
bool QMetaType::isRegistered() const
{
    return d_ptr && d_ptr->typeId.loadRelaxed();
}

/*!
    \fn int QMetaType::id() const
    \since 5.13

    Returns id type held by this QMetatype instance.
*/

/*!
    \fn void QMetaType::registerType() const
    \since 6.5

    Registers this QMetaType with the type registry so it can be found by name,
    using QMetaType::fromName().

    \sa qRegisterMetaType()
 */
/*!
    \internal
    Out-of-line path for registerType() and slow path id().
 */
int QMetaType::registerHelper(const QtPrivate::QMetaTypeInterface *iface)
{
    Q_ASSERT(iface);
    auto reg = customTypeRegistry();
    if (reg) {
        return reg->registerCustomType(iface);
    }
    return 0;
}

/*!
    \fn constexpr qsizetype QMetaType::sizeOf() const
    \since 5.0

    Returns the size of the type in bytes (i.e. sizeof(T),
    where T is the actual type for which this QMetaType instance
    was constructed for).

    This function is typically used together with construct()
    to perform low-level management of the memory used by a type.

    \sa QMetaType::construct(), QMetaType::sizeOf(), QMetaType::alignOf()
*/

/*!
  \fn constexpr int QMetaType::alignOf() const
  \since 6.0

  Returns the alignment of the type in bytes (i.e. alignof(T),
  where T is the actual type for which this QMetaType instance
  was constructed for).

  This function is typically used together with construct()
  to perform low-level management of the memory used by a type.

  \sa QMetaType::construct(), QMetaType::sizeOf()

 */

/*!
    \fn constexpr TypeFlags QMetaType::flags() const
    \since 5.0

    Returns flags of the type for which this QMetaType instance was
    constructed. To inspect specific type traits, prefer using one of the "is-"
    functions rather than the flags directly.

    \sa QMetaType::TypeFlags, QMetaType::flags(), isDefaultConstructible(),
        isCopyConstructible(), isMoveConstructible(), isDestructible(),
        isEqualityComparable(), isOrdered()
*/

/*!
    \fn constexpr const QMetaObject *QMetaType::metaObject() const
    \since 5.5

    Returns a QMetaObject relative to this type.

    If the type is a pointer type to a subclass of QObject, flags() contains
    QMetaType::PointerToQObject and this function returns the corresponding QMetaObject.
    This can be used in combination with QMetaObject::newInstance() to create QObjects of this type.

    If the type is a Q_GADGET, flags() contains QMetaType::IsGadget.
    If the type is a pointer to a Q_GADGET, flags() contains QMetaType::PointerToGadget.
    In both cases, this function returns its QMetaObject.
    This can be used to retrieve QMetaMethod and QMetaProperty and use them on a
    pointer of this type for example, as given by QVariant::data().

    If the type is an enumeration, flags() contains QMetaType::IsEnumeration.
    In this case, this function returns the QMetaObject of the enclosing
    object if the enum was registered as a Q_ENUM or \nullptr otherwise.

    \sa QMetaType::flags()
*/

/*!
    \fn void *QMetaType::create(const void *copy = nullptr) const
    \since 5.0

    Returns a copy of \a copy, assuming it is of the type that this
    QMetaType instance was created for. If \a copy is \nullptr, creates
    a default constructed instance.

    \sa QMetaType::destroy()
*/
void *QMetaType::create(const void *copy) const
{
    if (copy ? !isCopyConstructible() : !isDefaultConstructible())
        return nullptr;

    std::unique_ptr<void, QMetaTypeDeleter> where(nullptr, {d_ptr});
    if (d_ptr->alignment > __STDCPP_DEFAULT_NEW_ALIGNMENT__)
        where.reset(operator new(d_ptr->size, std::align_val_t(d_ptr->alignment)));
    else
        where.reset(operator new(d_ptr->size));

    QtMetaTypePrivate::construct(d_ptr, where.get(), copy);
    return where.release();
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
    if (data && isDestructible()) {
        QtMetaTypePrivate::destruct(d_ptr, data);
        QMetaTypeDeleter{d_ptr}(data);
    }
}

/*!
    \fn void *QMetaType::construct(void *where, const void *copy = nullptr) const
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
    if (copy ? !isCopyConstructible() : !isDefaultConstructible())
        return nullptr;

    QtMetaTypePrivate::construct(d_ptr, where, copy);
    return where;
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
    if (data && isDestructible())
        QtMetaTypePrivate::destruct(d_ptr, data);
}

static QPartialOrdering threeWayCompare(const void *ptr1, const void *ptr2)
{
    std::less<const void *> less;
    if (less(ptr1, ptr2))
        return QPartialOrdering::Less;
    if (less(ptr2, ptr1))
        return QPartialOrdering::Greater;
    return QPartialOrdering::Equivalent;
}

/*!
    Compares the objects at \a lhs and \a rhs for ordering.

    Returns QPartialOrdering::Unordered if comparison is not supported
    or the values are unordered. Otherwise, returns
    QPartialOrdering::Less, QPartialOrdering::Equivalent or
    QPartialOrdering::Greater if \a lhs is less than, equivalent
    to or greater than \a rhs, respectively.

    Both objects must be of the type described by this metatype. If either \a lhs
    or \a rhs is \nullptr, the values are unordered. Comparison is only supported
    if the type's less than operator was visible to the metatype declaration.

    If the type's equality operator was also visible, values will only compare equal if the
    equality operator says they are. In the absence of an equality operator, when neither
    value is less than the other, values are considered equal; if equality is also available
    and two such values are not equal, they are considered unordered, just as NaN (not a
    number) values of a floating point type lie outside its ordering.

    \note If no less than operator was visible to the metatype declaration, values are
    unordered even if an equality operator visible to the declaration considers them equal:
    \c{compare() == 0} only agrees with equals() if the less than operator was visible.

    \since 6.0
    \sa equals(), isOrdered()
*/
QPartialOrdering QMetaType::compare(const void *lhs, const void *rhs) const
{
    if (!lhs || !rhs)
        return QPartialOrdering::Unordered;
    if (d_ptr && d_ptr->flags & QMetaType::IsPointer)
        return threeWayCompare(*reinterpret_cast<const void * const *>(lhs),
                               *reinterpret_cast<const void * const *>(rhs));
    if (d_ptr && d_ptr->lessThan) {
        if (d_ptr->equals && d_ptr->equals(d_ptr, lhs, rhs))
            return QPartialOrdering::Equivalent;
        if (d_ptr->lessThan(d_ptr, lhs, rhs))
            return QPartialOrdering::Less;
        if (d_ptr->lessThan(d_ptr, rhs, lhs))
            return QPartialOrdering::Greater;
        if (!d_ptr->equals)
            return QPartialOrdering::Equivalent;
    }
    return QPartialOrdering::Unordered;
}

/*!
    Compares the objects at \a lhs and \a rhs for equality.

    Both objects must be of the type described by this metatype.  Can only compare the
    two objects if a less than or equality operator for the type was visible to the
    metatype declaration.  Otherwise, the metatype never considers values equal.  When
    an equality operator was visible to the metatype declaration, it is authoritative;
    otherwise, if less than is visible, when neither value is less than the other, the
    two are considered equal.  If values are unordered (see compare() for details) they
    are not equal.

    Returns true if the two objects compare equal, otherwise false.

    \since 6.0
    \sa isEqualityComparable(), compare()
*/
bool QMetaType::equals(const void *lhs, const void *rhs) const
{
    if (!lhs || !rhs)
        return false;
    if (d_ptr) {
        if (d_ptr->flags & QMetaType::IsPointer)
            return *reinterpret_cast<const void * const *>(lhs) == *reinterpret_cast<const void * const *>(rhs);

        if (d_ptr->equals)
            return d_ptr->equals(d_ptr, lhs, rhs);
        if (d_ptr->lessThan && !d_ptr->lessThan(d_ptr, lhs, rhs) && !d_ptr->lessThan(d_ptr, rhs, lhs))
            return true;
    }
    return false;
}

/*!
    \fn bool QMetaType::isDefaultConstructible() const noexcept
    \since 6.5

    Returns true if this type can be default-constructed. If it can be, then
    construct() and create() can be used with a \c{copy} parameter that is
    null.

    \sa flags(), isCopyConstructible(), isMoveConstructible(), isDestructible()
 */

/*!
    \fn bool QMetaType::isCopyConstructible() const noexcept
    \since 6.5

    Returns true if this type can be copy-constructed. If it can be, then
    construct() and create() can be used with a \c{copy} parameter that is
    not null.

    \sa flags(), isDefaultConstructible(), isMoveConstructible(), isDestructible()
 */

/*!
    \fn bool QMetaType::isMoveConstructible() const noexcept
    \since 6.5

    Returns true if this type can be move-constructed. QMetaType currently does
    not have an API to make use of this trait.

    \sa flags(), isDefaultConstructible(), isCopyConstructible(), isDestructible()
 */

/*!
    \fn bool QMetaType::isDestructible() const noexcept
    \since 6.5

    Returns true if this type can be destroyed. If it can be, then destroy()
    and destruct() can be called.

    \sa flags(), isDefaultConstructible(), isCopyConstructible(), isMoveConstructible()
 */

bool QMetaType::isDefaultConstructible(const QtPrivate::QMetaTypeInterface *iface) noexcept
{
    return !isInterfaceFor<void>(iface) && QtMetaTypePrivate::isDefaultConstructible(iface);
}

bool QMetaType::isCopyConstructible(const QtPrivate::QMetaTypeInterface *iface) noexcept
{
    return !isInterfaceFor<void>(iface) && QtMetaTypePrivate::isCopyConstructible(iface);
}

bool QMetaType::isMoveConstructible(const QtPrivate::QMetaTypeInterface *iface) noexcept
{
    return !isInterfaceFor<void>(iface) && QtMetaTypePrivate::isMoveConstructible(iface);
}

bool QMetaType::isDestructible(const QtPrivate::QMetaTypeInterface *iface) noexcept
{
    return !isInterfaceFor<void>(iface) && QtMetaTypePrivate::isDestructible(iface);
}

/*!
    Returns \c true if a less than or equality operator for the type described by
    this metatype was visible to the metatype declaration, otherwise \c false.

    \sa equals(), isOrdered()
*/
bool QMetaType::isEqualityComparable() const
{
    return d_ptr && (d_ptr->flags & QMetaType::IsPointer || d_ptr->equals != nullptr || d_ptr->lessThan != nullptr);
}

/*!
    Returns \c true if a less than operator for the type described by this metatype
    was visible to the metatype declaration, otherwise \c false.

    \sa compare(), isEqualityComparable()
*/
bool QMetaType::isOrdered() const
{
    return d_ptr && (d_ptr->flags & QMetaType::IsPointer || d_ptr->lessThan != nullptr);
}


/*!
   \internal
*/
void QMetaType::unregisterMetaType(QMetaType type)
{
    if (type.d_ptr && type.d_ptr->typeId.loadRelaxed() >= QMetaType::User) {
        // this is a custom meta type (not read-only)
        auto d = const_cast<QtPrivate::QMetaTypeInterface *>(type.d_ptr);
        if (auto reg = customTypeRegistry())
            reg->unregisterDynamicType(d->typeId.loadRelaxed());
        d->typeId.storeRelease(0);
    }
}

/*!
    \fn template<typename T> QMetaType QMetaType::fromType()
    \since 5.15

    Returns the QMetaType corresponding to the type in the template parameter.
*/

/*! \fn bool QMetaType::operator==(QMetaType a, QMetaType b)
    \since 5.15
    \overload

    Returns \c true if the QMetaType \a a represents the same type
    as the QMetaType \a b, otherwise returns \c false.
*/

/*! \fn bool QMetaType::operator!=(QMetaType a, QMetaType b)
    \since 5.15
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

static const struct : QMetaTypeModuleHelper
{
    template<typename T, typename LiteralWrapper =
             std::conditional_t<std::is_same_v<T, QString>, QLatin1StringView, const char *>>
    static inline bool convertToBool(const T &source)
    {
        T str = source.toLower();
        return !(str.isEmpty() || str == LiteralWrapper("0") || str == LiteralWrapper("false"));
    }

    const QtPrivate::QMetaTypeInterface *interfaceForType(int type) const override {
        switch (type) {
            QT_FOR_EACH_STATIC_PRIMITIVE_TYPE(QT_METATYPE_CONVERT_ID_TO_TYPE)
            QT_FOR_EACH_STATIC_PRIMITIVE_POINTER(QT_METATYPE_CONVERT_ID_TO_TYPE)
            QT_FOR_EACH_STATIC_CORE_CLASS(QT_METATYPE_CONVERT_ID_TO_TYPE)
            QT_FOR_EACH_STATIC_CORE_POINTER(QT_METATYPE_CONVERT_ID_TO_TYPE)
            QT_FOR_EACH_STATIC_CORE_TEMPLATE(QT_METATYPE_CONVERT_ID_TO_TYPE)
        default:
            return nullptr;
        }
    }

    bool convert(const void *from, int fromTypeId, void *to, int toTypeId) const override
    {
        Q_ASSERT(fromTypeId != toTypeId);

        // canConvert calls with two nullptr
        bool onlyCheck = (from == nullptr && to == nullptr);

        // other callers must provide two valid pointers
        Q_ASSERT(onlyCheck || (bool(from) && bool(to)));

        using Char = char;
        using SChar = signed char;
        using UChar = unsigned char;
        using Short = short;
        using UShort = unsigned short;
        using Int = int;
        using UInt = unsigned int;
        using Long = long;
        using LongLong = qlonglong;
        using ULong = unsigned long;
        using ULongLong = qulonglong;
        using Float = float;
        using Double = double;
        using Bool = bool;
        using Nullptr = std::nullptr_t;

#define QMETATYPE_CONVERTER_ASSIGN_DOUBLE(To, From) \
    QMETATYPE_CONVERTER(To, From, result = double(source); return true;)
#define QMETATYPE_CONVERTER_ASSIGN_NUMBER(To, From) \
    QMETATYPE_CONVERTER(To, From, result = To::number(source); return true;)
#ifndef QT_BOOTSTRAPPED
#define CONVERT_CBOR_AND_JSON(To) \
    QMETATYPE_CONVERTER(To, QCborValue, \
        if constexpr(std::is_same_v<To, Bool>) { \
            if (!source.isBool()) \
                return false; \
            result = source.toBool(); \
        } else { \
            if (!source.isInteger() && !source.isDouble()) \
                return false; \
            if constexpr(std::is_integral_v<To>) \
                result = source.toInteger(); \
            else \
                result = source.toDouble(); \
        } \
        return true; \
    ); \
    QMETATYPE_CONVERTER(To, QJsonValue, \
        if constexpr(std::is_same_v<To, Bool>) { \
            if (!source.isBool()) \
                return false; \
            result = source.toBool(); \
        } else { \
            if (!source.isDouble()) \
                return false; \
            if constexpr(std::is_integral_v<To>) \
                result = source.toInteger(); \
            else \
                result = source.toDouble(); \
        } \
        return true; \
    )
#else
#define CONVERT_CBOR_AND_JSON(To)
#endif

#define INTEGRAL_CONVERTER(To) \
    QMETATYPE_CONVERTER_ASSIGN(To, Bool); \
    QMETATYPE_CONVERTER_ASSIGN(To, Char); \
    QMETATYPE_CONVERTER_ASSIGN(To, UChar); \
    QMETATYPE_CONVERTER_ASSIGN(To, SChar); \
    QMETATYPE_CONVERTER_ASSIGN(To, Short); \
    QMETATYPE_CONVERTER_ASSIGN(To, UShort); \
    QMETATYPE_CONVERTER_ASSIGN(To, Int); \
    QMETATYPE_CONVERTER_ASSIGN(To, UInt); \
    QMETATYPE_CONVERTER_ASSIGN(To, Long); \
    QMETATYPE_CONVERTER_ASSIGN(To, ULong); \
    QMETATYPE_CONVERTER_ASSIGN(To, LongLong); \
    QMETATYPE_CONVERTER_ASSIGN(To, ULongLong); \
    QMETATYPE_CONVERTER(To, Float, result = qRound64(source); return true;); \
    QMETATYPE_CONVERTER(To, Double, result = qRound64(source); return true;); \
    QMETATYPE_CONVERTER(To, QChar, result = source.unicode(); return true;); \
    QMETATYPE_CONVERTER(To, QString, \
        bool ok = false; \
        if constexpr(std::is_same_v<To, bool>) \
            result = (ok = true, convertToBool(source)); \
        else if constexpr(std::is_signed_v<To>) \
            result = To(source.toLongLong(&ok)); \
        else \
            result = To(source.toULongLong(&ok)); \
        return ok; \
    ); \
    QMETATYPE_CONVERTER(To, QByteArray, \
        bool ok = false; \
        if constexpr(std::is_same_v<To, bool>) \
            result = (ok = true, convertToBool(source)); \
        else if constexpr(std::is_signed_v<To>) \
            result = To(source.toLongLong(&ok)); \
        else \
            result = To(source.toULongLong(&ok)); \
        return ok; \
    ); \
    CONVERT_CBOR_AND_JSON(To)

#define FLOAT_CONVERTER(To) \
    QMETATYPE_CONVERTER_ASSIGN(To, Bool); \
    QMETATYPE_CONVERTER_ASSIGN(To, Char); \
    QMETATYPE_CONVERTER_ASSIGN(To, UChar); \
    QMETATYPE_CONVERTER_ASSIGN(To, SChar); \
    QMETATYPE_CONVERTER_ASSIGN(To, Short); \
    QMETATYPE_CONVERTER_ASSIGN(To, UShort); \
    QMETATYPE_CONVERTER_ASSIGN(To, Int); \
    QMETATYPE_CONVERTER_ASSIGN(To, UInt); \
    QMETATYPE_CONVERTER_ASSIGN(To, Long); \
    QMETATYPE_CONVERTER_ASSIGN(To, ULong); \
    QMETATYPE_CONVERTER_ASSIGN(To, LongLong); \
    QMETATYPE_CONVERTER_ASSIGN(To, ULongLong); \
    QMETATYPE_CONVERTER_ASSIGN(To, Float); \
    QMETATYPE_CONVERTER_ASSIGN(To, Double); \
    QMETATYPE_CONVERTER(To, QString, \
        bool ok = false; \
        result = source.toDouble(&ok); \
        return ok; \
    ); \
    QMETATYPE_CONVERTER(To, QByteArray, \
        bool ok = false; \
        result = source.toDouble(&ok); \
        return ok; \
    ); \
    CONVERT_CBOR_AND_JSON(To)

        switch (makePair(toTypeId, fromTypeId)) {

        // integral conversions
        INTEGRAL_CONVERTER(Bool);
        INTEGRAL_CONVERTER(Char);
        INTEGRAL_CONVERTER(UChar);
        INTEGRAL_CONVERTER(SChar);
        INTEGRAL_CONVERTER(Short);
        INTEGRAL_CONVERTER(UShort);
        INTEGRAL_CONVERTER(Int);
        INTEGRAL_CONVERTER(UInt);
        INTEGRAL_CONVERTER(Long);
        INTEGRAL_CONVERTER(ULong);
        INTEGRAL_CONVERTER(LongLong);
        INTEGRAL_CONVERTER(ULongLong);
        FLOAT_CONVERTER(Float);
        FLOAT_CONVERTER(Double);

#ifndef QT_BOOTSTRAPPED
        QMETATYPE_CONVERTER_ASSIGN(QUrl, QString);
        QMETATYPE_CONVERTER(QUrl, QCborValue,
            if (source.isUrl()) {
                result = source.toUrl();
                return true;
             }
            return false;
        );
#endif
#if QT_CONFIG(itemmodel)
        QMETATYPE_CONVERTER_ASSIGN(QModelIndex, QPersistentModelIndex);
        QMETATYPE_CONVERTER_ASSIGN(QPersistentModelIndex, QModelIndex);
#endif // QT_CONFIG(itemmodel)

        // QChar methods
#define QMETATYPE_CONVERTER_ASSIGN_QCHAR(From) \
        QMETATYPE_CONVERTER(QChar, From, result = QChar::fromUcs2(source); return true;)
        QMETATYPE_CONVERTER_ASSIGN_QCHAR(Char);
        QMETATYPE_CONVERTER_ASSIGN_QCHAR(SChar);
        QMETATYPE_CONVERTER_ASSIGN_QCHAR(Short);
        QMETATYPE_CONVERTER_ASSIGN_QCHAR(Long);
        QMETATYPE_CONVERTER_ASSIGN_QCHAR(Int);
        QMETATYPE_CONVERTER_ASSIGN_QCHAR(LongLong);
        QMETATYPE_CONVERTER_ASSIGN_QCHAR(Float);
        QMETATYPE_CONVERTER_ASSIGN_QCHAR(UChar);
        QMETATYPE_CONVERTER_ASSIGN_QCHAR(UShort);
        QMETATYPE_CONVERTER_ASSIGN_QCHAR(ULong);
        QMETATYPE_CONVERTER_ASSIGN_QCHAR(UInt);
        QMETATYPE_CONVERTER_ASSIGN_QCHAR(ULongLong);

        // conversions to QString
        QMETATYPE_CONVERTER_ASSIGN(QString, QChar);
        QMETATYPE_CONVERTER(QString, Bool,
            result = source ? QStringLiteral("true") : QStringLiteral("false");
            return true;
        );
        QMETATYPE_CONVERTER_ASSIGN_NUMBER(QString, Short);
        QMETATYPE_CONVERTER_ASSIGN_NUMBER(QString, Long);
        QMETATYPE_CONVERTER_ASSIGN_NUMBER(QString, Int);
        QMETATYPE_CONVERTER_ASSIGN_NUMBER(QString, LongLong);
        QMETATYPE_CONVERTER_ASSIGN_NUMBER(QString, UShort);
        QMETATYPE_CONVERTER_ASSIGN_NUMBER(QString, ULong);
        QMETATYPE_CONVERTER_ASSIGN_NUMBER(QString, UInt);
        QMETATYPE_CONVERTER_ASSIGN_NUMBER(QString, ULongLong);
        QMETATYPE_CONVERTER(QString, Float,
            result = QString::number(source, 'g', QLocale::FloatingPointShortest);
            return true;
        );
        QMETATYPE_CONVERTER(QString, Double,
            result = QString::number(source, 'g', QLocale::FloatingPointShortest);
            return true;
        );
        QMETATYPE_CONVERTER(QString, Char,
            result = QString::fromLatin1(&source, 1);
            return true;
        );
        QMETATYPE_CONVERTER(QString, SChar,
            char s = source;
            result = QString::fromLatin1(&s, 1);
            return true;
        );
        QMETATYPE_CONVERTER(QString, UChar,
            char s = source;
            result = QString::fromLatin1(&s, 1);
            return true;
        );
#if QT_CONFIG(datestring)
        QMETATYPE_CONVERTER(QString, QDate, result = source.toString(Qt::ISODate); return true;);
        QMETATYPE_CONVERTER(QString, QTime, result = source.toString(Qt::ISODateWithMs); return true;);
        QMETATYPE_CONVERTER(QString, QDateTime, result = source.toString(Qt::ISODateWithMs); return true;);
#endif
        QMETATYPE_CONVERTER(QString, QByteArray, result = QString::fromUtf8(source); return true;);
        QMETATYPE_CONVERTER(QString, QStringList,
            return (source.size() == 1) ? (result = source.at(0), true) : false;
        );
#ifndef QT_BOOTSTRAPPED
        QMETATYPE_CONVERTER(QString, QUrl, result = source.toString(); return true;);
        QMETATYPE_CONVERTER(QString, QJsonValue,
            if (source.isString() || source.isNull()) {
                result = source.toString();
                return true;
            }
            return false;
        );
#endif
        QMETATYPE_CONVERTER(QString, Nullptr, Q_UNUSED(source); result = QString(); return true;);

        // QByteArray
        QMETATYPE_CONVERTER(QByteArray, QString, result = source.toUtf8(); return true;);
        QMETATYPE_CONVERTER(QByteArray, Bool,
            result = source ? "true" : "false";
            return true;
        );
        QMETATYPE_CONVERTER(QByteArray, Char, result = QByteArray(source, 1); return true;);
        QMETATYPE_CONVERTER(QByteArray, SChar, result = QByteArray(source, 1); return true;);
        QMETATYPE_CONVERTER(QByteArray, UChar, result = QByteArray(source, 1); return true;);
        QMETATYPE_CONVERTER_ASSIGN_NUMBER(QByteArray, Short);
        QMETATYPE_CONVERTER_ASSIGN_NUMBER(QByteArray, Long);
        QMETATYPE_CONVERTER_ASSIGN_NUMBER(QByteArray, Int);
        QMETATYPE_CONVERTER_ASSIGN_NUMBER(QByteArray, LongLong);
        QMETATYPE_CONVERTER_ASSIGN_NUMBER(QByteArray, UShort);
        QMETATYPE_CONVERTER_ASSIGN_NUMBER(QByteArray, ULong);
        QMETATYPE_CONVERTER_ASSIGN_NUMBER(QByteArray, UInt);
        QMETATYPE_CONVERTER_ASSIGN_NUMBER(QByteArray, ULongLong);
        QMETATYPE_CONVERTER(QByteArray, Float,
            result = QByteArray::number(source, 'g', QLocale::FloatingPointShortest);
            return true;
        );
        QMETATYPE_CONVERTER(QByteArray, Double,
            result = QByteArray::number(source, 'g', QLocale::FloatingPointShortest);
            return true;
        );
        QMETATYPE_CONVERTER(QByteArray, Nullptr, Q_UNUSED(source); result = QByteArray(); return true;);

        QMETATYPE_CONVERTER(QString, QUuid, result = source.toString(); return true;);
        QMETATYPE_CONVERTER(QUuid, QString, result = QUuid(source); return true;);
        QMETATYPE_CONVERTER(QByteArray, QUuid, result = source.toByteArray(); return true;);
        QMETATYPE_CONVERTER(QUuid, QByteArray, result = QUuid(source); return true;);

#ifndef QT_NO_GEOM_VARIANT
        QMETATYPE_CONVERTER(QSize, QSizeF, result = source.toSize(); return true;);
        QMETATYPE_CONVERTER_ASSIGN(QSizeF, QSize);
        QMETATYPE_CONVERTER(QLine, QLineF, result = source.toLine(); return true;);
        QMETATYPE_CONVERTER_ASSIGN(QLineF, QLine);
        QMETATYPE_CONVERTER(QRect, QRectF, result = source.toRect(); return true;);
        QMETATYPE_CONVERTER_ASSIGN(QRectF, QRect);
        QMETATYPE_CONVERTER(QPoint, QPointF, result = source.toPoint(); return true;);
        QMETATYPE_CONVERTER_ASSIGN(QPointF, QPoint);
 #endif

        QMETATYPE_CONVERTER(QByteArrayList, QVariantList,
            result.reserve(source.size());
            for (const auto &v: source)
                result.append(v.toByteArray());
            return true;
        );
        QMETATYPE_CONVERTER(QVariantList, QByteArrayList,
            result.reserve(source.size());
            for (const auto &v: source)
                result.append(QVariant(v));
            return true;
        );

        QMETATYPE_CONVERTER(QStringList, QVariantList,
            result.reserve(source.size());
            for (const auto &v: source)
                result.append(v.toString());
            return true;
        );
        QMETATYPE_CONVERTER(QVariantList, QStringList,
            result.reserve(source.size());
            for (const auto &v: source)
                result.append(QVariant(v));
            return true;
        );
        QMETATYPE_CONVERTER(QStringList, QString, result = QStringList() << source; return true;);

        QMETATYPE_CONVERTER(QVariantHash, QVariantMap,
            for (auto it = source.begin(); it != source.end(); ++it)
                result.insert(it.key(), it.value());
            return true;
        );
        QMETATYPE_CONVERTER(QVariantMap, QVariantHash,
            for (auto it = source.begin(); it != source.end(); ++it)
                result.insert(it.key(), it.value());
            return true;
        );

#ifndef QT_BOOTSTRAPPED
        QMETATYPE_CONVERTER_ASSIGN(QCborValue, QString);
        QMETATYPE_CONVERTER(QString, QCborValue,
            if (source.isContainer() || source.isTag())
                 return false;
            result = source.toVariant().toString();
            return true;
        );
        QMETATYPE_CONVERTER_ASSIGN(QCborValue, QByteArray);
        QMETATYPE_CONVERTER(QByteArray, QCborValue,
            if (source.isByteArray()) {
                result = source.toByteArray();
                return true;
            }
            return false;
        );
        QMETATYPE_CONVERTER_ASSIGN(QCborValue, QUuid);
        QMETATYPE_CONVERTER(QUuid, QCborValue,
            if (!source.isUuid())
                return false;
            result = source.toUuid();
            return true;
        );
        QMETATYPE_CONVERTER(QCborValue, QVariantList, result = QCborArray::fromVariantList(source); return true;);
        QMETATYPE_CONVERTER(QVariantList, QCborValue,
            if (!source.isArray())
                return false;
            result = source.toArray().toVariantList();
            return true;
        );
        QMETATYPE_CONVERTER(QCborValue, QVariantMap, result = QCborMap::fromVariantMap(source); return true;);
        QMETATYPE_CONVERTER(QVariantMap, QCborValue,
            if (!source.isMap())
                return false;
                result = source.toMap().toVariantMap();
            return true;
        );
        QMETATYPE_CONVERTER(QCborValue, QVariantHash, result = QCborMap::fromVariantHash(source); return true;);
        QMETATYPE_CONVERTER(QVariantHash, QCborValue,
            if (!source.isMap())
                return false;
            result = source.toMap().toVariantHash();
            return true;
        );
#if QT_CONFIG(regularexpression)
        QMETATYPE_CONVERTER(QCborValue, QRegularExpression, result = QCborValue(source); return true;);
        QMETATYPE_CONVERTER(QRegularExpression, QCborValue,
            if (!source.isRegularExpression())
                return false;
            result = source.toRegularExpression();
            return true;
        );
#endif

        QMETATYPE_CONVERTER(QCborValue, Nullptr,
            Q_UNUSED(source);
            result = QCborValue(QCborValue::Null);
            return true;
        );
        QMETATYPE_CONVERTER(Nullptr, QCborValue,
            result = nullptr;
            return source.isNull();
        );
        QMETATYPE_CONVERTER_ASSIGN(QCborValue, Bool);
        QMETATYPE_CONVERTER_ASSIGN(QCborValue, Int);
        QMETATYPE_CONVERTER_ASSIGN(QCborValue, UInt);
        QMETATYPE_CONVERTER(QCborValue, ULong, result = qlonglong(source); return true;);
        QMETATYPE_CONVERTER(QCborValue, Long, result = qlonglong(source); return true;);
        QMETATYPE_CONVERTER_ASSIGN(QCborValue, LongLong);
        QMETATYPE_CONVERTER(QCborValue, ULongLong, result = qlonglong(source); return true;);
        QMETATYPE_CONVERTER_ASSIGN(QCborValue, UShort);
        QMETATYPE_CONVERTER_ASSIGN(QCborValue, UChar);
        QMETATYPE_CONVERTER_ASSIGN(QCborValue, Char);
        QMETATYPE_CONVERTER_ASSIGN(QCborValue, SChar);
        QMETATYPE_CONVERTER_ASSIGN(QCborValue, Short);
        QMETATYPE_CONVERTER_ASSIGN(QCborValue, Double);
        QMETATYPE_CONVERTER_ASSIGN(QCborValue, Float);
        QMETATYPE_CONVERTER(QCborValue, QStringList,
            result = QCborArray::fromStringList(source);
            return true;
        );
        QMETATYPE_CONVERTER(QCborValue, QDate,
            result = QCborValue(source.startOfDay());
            return true;
        );
        QMETATYPE_CONVERTER_ASSIGN(QCborValue, QUrl);
        QMETATYPE_CONVERTER(QCborValue, QJsonValue,
            result = QCborValue::fromJsonValue(source);
            return true;
        );
        QMETATYPE_CONVERTER(QCborValue, QJsonObject,
            result = QCborMap::fromJsonObject(source);
            return true;
        );
        QMETATYPE_CONVERTER(QCborValue, QJsonArray,
            result = QCborArray::fromJsonArray(source);
            return true;
        );
        QMETATYPE_CONVERTER(QCborValue, QJsonDocument,
            QJsonDocument doc = source;
            if (doc.isArray())
                result = QCborArray::fromJsonArray(doc.array());
            else
                result = QCborMap::fromJsonObject(doc.object());
            return true;
        );
        QMETATYPE_CONVERTER_ASSIGN(QCborValue, QCborMap);
        QMETATYPE_CONVERTER_ASSIGN(QCborValue, QCborArray);

        QMETATYPE_CONVERTER_ASSIGN(QCborValue, QDateTime);
        QMETATYPE_CONVERTER(QDateTime, QCborValue,
            if (source.isDateTime()) {
                result = source.toDateTime();
                return true;
            }
            return false;
        );

        QMETATYPE_CONVERTER_ASSIGN(QCborValue, QCborSimpleType);
        QMETATYPE_CONVERTER(QCborSimpleType, QCborValue,
            if (source.isSimpleType()) {
                 result = source.toSimpleType();
                 return true;
             }
             return false;
        );

        QMETATYPE_CONVERTER(QCborArray, QVariantList, result = QCborArray::fromVariantList(source); return true;);
        QMETATYPE_CONVERTER(QVariantList, QCborArray, result = source.toVariantList(); return true;);
        QMETATYPE_CONVERTER(QCborArray, QStringList, result = QCborArray::fromStringList(source); return true;);
        QMETATYPE_CONVERTER(QCborMap, QVariantMap, result = QCborMap::fromVariantMap(source); return true;);
        QMETATYPE_CONVERTER(QVariantMap, QCborMap, result = source.toVariantMap(); return true;);
        QMETATYPE_CONVERTER(QCborMap, QVariantHash, result = QCborMap::fromVariantHash(source); return true;);
        QMETATYPE_CONVERTER(QVariantHash, QCborMap, result = source.toVariantHash(); return true;);

        QMETATYPE_CONVERTER(QCborArray, QCborValue,
            if (!source.isArray())
                return false;
            result = source.toArray();
            return true;
        );
        QMETATYPE_CONVERTER(QCborArray, QJsonDocument,
            if (!source.isArray())
                return false;
            result = QCborArray::fromJsonArray(source.array());
            return true;
        );
        QMETATYPE_CONVERTER(QCborArray, QJsonValue,
            if (!source.isArray())
                return false;
            result = QCborArray::fromJsonArray(source.toArray());
            return true;
        );
        QMETATYPE_CONVERTER(QCborArray, QJsonArray,
            result = QCborArray::fromJsonArray(source);
            return true;
        );
        QMETATYPE_CONVERTER(QCborMap, QCborValue,
            if (!source.isMap())
                return false;
            result = source.toMap();
            return true;
        );
        QMETATYPE_CONVERTER(QCborMap, QJsonDocument,
            if (source.isArray())
                return false;
            result = QCborMap::fromJsonObject(source.object());
            return true;
        );
        QMETATYPE_CONVERTER(QCborMap, QJsonValue,
            if (!source.isObject())
                return false;
            result = QCborMap::fromJsonObject(source.toObject());
            return true;
        );
        QMETATYPE_CONVERTER(QCborMap, QJsonObject,
            result = QCborMap::fromJsonObject(source);
            return true;
        );


        QMETATYPE_CONVERTER(QVariantList, QJsonValue,
            if (!source.isArray())
                return false;
            result = source.toArray().toVariantList();
            return true;
        );
        QMETATYPE_CONVERTER(QVariantList, QJsonArray, result = source.toVariantList(); return true;);
        QMETATYPE_CONVERTER(QVariantMap, QJsonValue,
            if (!source.isObject())
                return false;
            result = source.toObject().toVariantMap();
            return true;
        );
        QMETATYPE_CONVERTER(QVariantMap, QJsonObject, result = source.toVariantMap(); return true;);
        QMETATYPE_CONVERTER(QVariantHash, QJsonValue,
            if (!source.isObject())
                return false;
            result = source.toObject().toVariantHash();
            return true;
        );
        QMETATYPE_CONVERTER(QVariantHash, QJsonObject, result = source.toVariantHash(); return true;);


        QMETATYPE_CONVERTER(QJsonArray, QStringList, result = QJsonArray::fromStringList(source); return true;);
        QMETATYPE_CONVERTER(QJsonArray, QVariantList, result = QJsonArray::fromVariantList(source); return true;);
        QMETATYPE_CONVERTER(QJsonArray, QJsonValue,
            if (!source.isArray())
                return false;
            result = source.toArray();
            return true;
        );
        QMETATYPE_CONVERTER(QJsonArray, QJsonDocument,
            if (!source.isArray())
                return false;
            result = source.array();
            return true;
        );
        QMETATYPE_CONVERTER(QJsonArray, QCborValue,
            if (!source.isArray())
                return false;
            result = source.toArray().toJsonArray();
            return true;
        );
        QMETATYPE_CONVERTER(QJsonArray, QCborArray, result = source.toJsonArray(); return true;);
        QMETATYPE_CONVERTER(QJsonObject, QVariantMap, result = QJsonObject::fromVariantMap(source); return true;);
        QMETATYPE_CONVERTER(QJsonObject, QVariantHash, result = QJsonObject::fromVariantHash(source); return true;);
        QMETATYPE_CONVERTER(QJsonObject, QJsonValue,
            if (!source.isObject())
                return false;
            result = source.toObject();
            return true;
        );
        QMETATYPE_CONVERTER(QJsonObject, QJsonDocument,
            if (source.isArray())
                return false;
            result = source.object();
            return true;
        );
        QMETATYPE_CONVERTER(QJsonObject, QCborValue,
            if (!source.isMap())
                return false;
            result = source.toMap().toJsonObject();
            return true;
        );
        QMETATYPE_CONVERTER(QJsonObject, QCborMap, result = source.toJsonObject(); return true; );

        QMETATYPE_CONVERTER(QJsonValue, Nullptr,
            Q_UNUSED(source);
            result = QJsonValue(QJsonValue::Null);
            return true;
        );
        QMETATYPE_CONVERTER(Nullptr, QJsonValue,
            result = nullptr;
            return source.isNull();
        );
        QMETATYPE_CONVERTER(QJsonValue, Bool,
            result = QJsonValue(source);
            return true;);
        QMETATYPE_CONVERTER_ASSIGN_DOUBLE(QJsonValue, Int);
        QMETATYPE_CONVERTER_ASSIGN_DOUBLE(QJsonValue, UInt);
        QMETATYPE_CONVERTER_ASSIGN_DOUBLE(QJsonValue, Double);
        QMETATYPE_CONVERTER_ASSIGN_DOUBLE(QJsonValue, Float);
        QMETATYPE_CONVERTER_ASSIGN_DOUBLE(QJsonValue, ULong);
        QMETATYPE_CONVERTER_ASSIGN_DOUBLE(QJsonValue, Long);
        QMETATYPE_CONVERTER_ASSIGN_DOUBLE(QJsonValue, LongLong);
        QMETATYPE_CONVERTER_ASSIGN_DOUBLE(QJsonValue, ULongLong);
        QMETATYPE_CONVERTER_ASSIGN_DOUBLE(QJsonValue, UShort);
        QMETATYPE_CONVERTER_ASSIGN_DOUBLE(QJsonValue, UChar);
        QMETATYPE_CONVERTER_ASSIGN_DOUBLE(QJsonValue, Char);
        QMETATYPE_CONVERTER_ASSIGN_DOUBLE(QJsonValue, SChar);
        QMETATYPE_CONVERTER_ASSIGN_DOUBLE(QJsonValue, Short);
        QMETATYPE_CONVERTER_ASSIGN(QJsonValue, QString);
        QMETATYPE_CONVERTER(QJsonValue, QStringList,
            result = QJsonValue(QJsonArray::fromStringList(source));
            return true;
        );
        QMETATYPE_CONVERTER(QJsonValue, QVariantList,
            result = QJsonValue(QJsonArray::fromVariantList(source));
            return true;
        );
        QMETATYPE_CONVERTER(QJsonValue, QVariantMap,
            result = QJsonValue(QJsonObject::fromVariantMap(source));
            return true;
        );
        QMETATYPE_CONVERTER(QJsonValue, QVariantHash,
            result = QJsonValue(QJsonObject::fromVariantHash(source));
            return true;
        );
        QMETATYPE_CONVERTER(QJsonValue, QJsonObject,
            result = source;
            return true;
        );
        QMETATYPE_CONVERTER(QJsonValue, QJsonArray,
            result = source;
            return true;
        );
        QMETATYPE_CONVERTER(QJsonValue, QJsonDocument,
            QJsonDocument doc = source;
            result = doc.isArray() ? QJsonValue(doc.array()) : QJsonValue(doc.object());
            return true;
        );
        QMETATYPE_CONVERTER(QJsonValue, QCborValue,
            result = source.toJsonValue();
            return true;
        );
        QMETATYPE_CONVERTER(QJsonValue, QCborMap,
            result = source.toJsonObject();
            return true;
        );
        QMETATYPE_CONVERTER(QJsonValue, QCborArray,
            result = source.toJsonArray();
            return true;
        );

#endif

        QMETATYPE_CONVERTER(QDate, QDateTime, result = source.date(); return true;);
        QMETATYPE_CONVERTER(QTime, QDateTime, result = source.time(); return true;);
        QMETATYPE_CONVERTER(QDateTime, QDate, result = source.startOfDay(); return true;);
#if QT_CONFIG(datestring)
        QMETATYPE_CONVERTER(QDate, QString,
            result = QDate::fromString(source, Qt::ISODate);
            return result.isValid();
        );
        QMETATYPE_CONVERTER(QTime, QString,
            result = QTime::fromString(source, Qt::ISODate);
            return result.isValid();
        );
        QMETATYPE_CONVERTER(QDateTime, QString,
            result = QDateTime::fromString(source, Qt::ISODate);
            return result.isValid();
        );
#endif

        }
        return false;
    }
} metatypeHelper = {};

Q_CONSTINIT Q_CORE_EXPORT const QMetaTypeModuleHelper *qMetaTypeGuiHelper = nullptr;
Q_CONSTINIT Q_CORE_EXPORT const QMetaTypeModuleHelper *qMetaTypeWidgetsHelper = nullptr;

static const QMetaTypeModuleHelper *qModuleHelperForType(int type)
{
    if (type <= QMetaType::LastCoreType)
        return &metatypeHelper;
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

    bool insertIfNotContains(Key k, const T &f)
    {
        const QWriteLocker locker(&lock);
        const qsizetype oldSize = map.size();
        auto &e = map[k];
        if (map.size() == oldSize) // already present
            return false;
        e = f;
        return true;
    }

    const T *function(Key k) const
    {
        const QReadLocker locker(&lock);
        auto it = map.find(k);
        return it == map.end() ? nullptr : std::addressof(*it);
    }

    void remove(int from, int to)
    {
        const Key k(from, to);
        const QWriteLocker locker(&lock);
        map.remove(k);
    }
private:
    mutable QReadWriteLock lock;
    QHash<Key, T> map;
};

typedef QMetaTypeFunctionRegistry<QMetaType::ConverterFunction,QPair<int,int> >
QMetaTypeConverterRegistry;

Q_GLOBAL_STATIC(QMetaTypeConverterRegistry, customTypesConversionRegistry)

using QMetaTypeMutableViewRegistry
        = QMetaTypeFunctionRegistry<QMetaType::MutableViewFunction, QPair<int,int>>;
Q_GLOBAL_STATIC(QMetaTypeMutableViewRegistry, customTypesMutableViewRegistry)

/*!
    \fn bool QMetaType::registerConverter()
    \since 5.2
    Registers the possibility of an implicit conversion from type From to type To in the meta
    type system. Returns \c true if the registration succeeded, otherwise false.

    \snippet qmetatype/registerConverters.cpp implicit
*/

/*!
    \fn template<typename From, typename To> static bool QMetaType::registerConverter(To(From::*function)() const)
    \since 5.2
    \overload
    Registers a method \a function like To From::function() const as converter from type From
    to type To in the meta type system. Returns \c true if the registration succeeded, otherwise false.

    \snippet qmetatype/registerConverters.cpp member
*/

/*!
    \fn template<typename From, typename To> static bool QMetaType::registerConverter(To(From::*function)(bool*) const)
    \since 5.2
    \overload
    Registers a method \a function like To From::function(bool *ok) const as converter from type From
    to type To in the meta type system. Returns \c true if the registration succeeded, otherwise false.

    The \c ok pointer can be used by the function to indicate whether the conversion succeeded.
    \snippet qmetatype/registerConverters.cpp memberOk

*/

/*!
    \fn template<typename From, typename To, typename UnaryFunction> static bool QMetaType::registerConverter(UnaryFunction function)
    \since 5.2
    \overload
    Registers a unary function object \a function as converter from type From
    to type To in the meta type system. Returns \c true if the registration succeeded, otherwise false.

    \a function must take an instance of type \c From and return an instance of \c To. It can be a function
    pointer, a lambda or a functor object. Since Qt 6.5, the \a function can also return an instance of
    \c std::optional<To> to be able to indicate failed conversions.
    \snippet qmetatype/registerConverters.cpp unaryfunc
*/

/*!
    Registers function \a f as converter function from type id \a from to \a to.
    If there's already a conversion registered, this does nothing but deleting \a f.
    Returns \c true if the registration succeeded, otherwise false.
    \since 5.2
    \internal
*/
bool QMetaType::registerConverterFunction(const ConverterFunction &f, QMetaType from, QMetaType to)
{
    if (!customTypesConversionRegistry()->insertIfNotContains(qMakePair(from.id(), to.id()), f)) {
        qWarning("Type conversion already registered from type %s to type %s",
                 from.name(), to.name());
        return false;
    }
    return true;
}

/*!
    \fn template<typename From, typename To> static bool QMetaType::registerMutableView(To(From::*function)())
    \since 6.0
    \overload
    Registers a method \a function like \c {To From::function()} as mutable view of type \c {To} on
    type \c {From} in the meta type system. Returns \c true if the registration succeeded, otherwise
    \c false.
*/

/*!
    \fn template<typename From, typename To, typename UnaryFunction> static bool QMetaType::registerMutableView(UnaryFunction function)
    \since 6.0
    \overload
    Registers a unary function object \a function as mutable view of type To on type From
    in the meta type system. Returns \c true if the registration succeeded, otherwise \c false.
*/

/*!
    Registers function \a f as mutable view of type id \a to on type id \a from.
    Returns \c true if the registration succeeded, otherwise \c false.
    \since 6.0
    \internal
*/
bool QMetaType::registerMutableViewFunction(const MutableViewFunction &f, QMetaType from, QMetaType to)
{
    if (!customTypesMutableViewRegistry()->insertIfNotContains(qMakePair(from.id(), to.id()), f)) {
        qWarning("Mutable view on type already registered from type %s to type %s",
                 from.name(), to.name());
        return false;
    }
    return true;
}

/*!
    \internal
 */
void QMetaType::unregisterMutableViewFunction(QMetaType from, QMetaType to)
{
    if (customTypesMutableViewRegistry.isDestroyed())
        return;
    customTypesMutableViewRegistry()->remove(from.id(), to.id());
}

/*!
    \internal

    Invoked automatically when a converter function object is destroyed.
 */
void QMetaType::unregisterConverterFunction(QMetaType from, QMetaType to)
{
    if (customTypesConversionRegistry.isDestroyed())
        return;
    customTypesConversionRegistry()->remove(from.id(), to.id());
}

#ifndef QT_NO_DEBUG_STREAM

/*!
    \fn QDebug QMetaType::operator<<(QDebug d, QMetaType m)
    \since 6.5
    Writes the QMetaType \a m to the stream \a d, and returns the stream.
*/
QDebug operator<<(QDebug d, QMetaType m)
{
    const QDebugStateSaver saver(d);
    return d.nospace() << "QMetaType(" << m.name() << ")";
}

/*!
    Streams the object at \a rhs to the debug stream \a dbg. Returns \c true
    on success, otherwise false.
    \since 5.2
*/
bool QMetaType::debugStream(QDebug& dbg, const void *rhs)
{
    if (d_ptr && d_ptr->flags & QMetaType::IsPointer) {
        dbg << *reinterpret_cast<const void * const *>(rhs);
        return true;
    }
    if (d_ptr && d_ptr->debugStream) {
        d_ptr->debugStream(d_ptr, dbg, rhs);
        return true;
    }
    return false;
}

/*!
    \fn bool QMetaType::debugStream(QDebug& dbg, const void *rhs, int typeId)
    \overload
    \deprecated
*/

/*!
    \fn bool QMetaType::hasRegisteredDebugStreamOperator()
    \deprecated
    \since 5.2

    Returns \c true, if the meta type system has a registered debug stream operator for type T.
 */

/*!
    \fn bool QMetaType::hasRegisteredDebugStreamOperator(int typeId)
    \deprecated Use QMetaType::hasRegisteredDebugStreamOperator() instead.

    Returns \c true, if the meta type system has a registered debug stream operator for type
    id \a typeId.
    \since 5.2
*/

/*!
    \since 6.0

    Returns \c true, if the meta type system has a registered debug stream operator for this
    meta type.
*/
bool QMetaType::hasRegisteredDebugStreamOperator() const
{
    return d_ptr && d_ptr->debugStream != nullptr;
}
#endif

#ifndef QT_NO_QOBJECT
/*!
  \internal
  returns a QMetaEnum for a given meta tape type id if possible
*/
static QMetaEnum metaEnumFromType(QMetaType t)
{
    if (t.flags() & QMetaType::IsEnumeration) {
        if (const QMetaObject *metaObject = t.metaObject()) {
            QByteArrayView qflagsNamePrefix = "QFlags<";
            QByteArray enumName = t.name();
            if (enumName.endsWith('>') && enumName.startsWith(qflagsNamePrefix)) {
                // extract the template argument
                enumName.chop(1);
                enumName = enumName.sliced(qflagsNamePrefix.size());
            }
            if (qsizetype lastColon = enumName.lastIndexOf(':'); lastColon != -1)
                enumName = enumName.sliced(lastColon + 1);
            return metaObject->enumerator(metaObject->indexOfEnumerator(enumName));
        }
    }
    return QMetaEnum();
}
#endif

static bool convertFromEnum(QMetaType fromType, const void *from, QMetaType toType, void *to)
{
    qlonglong ll;
    if (fromType.flags() & QMetaType::IsUnsignedEnumeration) {
        qulonglong ull;
        switch (fromType.sizeOf()) {
        case 1:
            ull = *static_cast<const unsigned char *>(from);
            break;
        case 2:
            ull = *static_cast<const unsigned short *>(from);
            break;
        case 4:
            ull = *static_cast<const unsigned int *>(from);
            break;
        case 8:
            ull = *static_cast<const quint64 *>(from);
            break;
        default:
            Q_UNREACHABLE();
        }
        if (toType.id() == QMetaType::ULongLong) {
            *static_cast<qulonglong *>(to) = ull;
            return true;
        }
        if (toType.id() != QMetaType::QString && toType.id() != QMetaType::QByteArray)
            return QMetaType::convert(QMetaType::fromType<qulonglong>(), &ull, toType, to);
        ll = qlonglong(ull);
    } else {
        switch (fromType.sizeOf()) {
        case 1:
            ll = *static_cast<const signed char *>(from);
            break;
        case 2:
            ll = *static_cast<const short *>(from);
            break;
        case 4:
            ll = *static_cast<const int *>(from);
            break;
        case 8:
            ll = *static_cast<const qint64 *>(from);
            break;
        default:
            Q_UNREACHABLE();
        }
        if (toType.id() == QMetaType::LongLong) {
            *static_cast<qlonglong *>(to) = ll;
            return true;
        }
        if (toType.id() != QMetaType::QString && toType.id() != QMetaType::QByteArray)
            return QMetaType::convert(QMetaType::fromType<qlonglong>(), &ll, toType, to);
    }
#ifndef QT_NO_QOBJECT
    QMetaEnum en = metaEnumFromType(fromType);
    if (en.isValid()) {
        if (en.isFlag()) {
            const QByteArray keys = en.valueToKeys(static_cast<int>(ll));
            if (toType.id() == QMetaType::QString)
                *static_cast<QString *>(to) = QString::fromUtf8(keys);
            else
                *static_cast<QByteArray *>(to) = keys;
        } else {
            const char *key = en.valueToKey(static_cast<int>(ll));
            if (toType.id() == QMetaType::QString)
                *static_cast<QString *>(to) = QString::fromUtf8(key);
            else
                *static_cast<QByteArray *>(to) = key;
        }
        return true;
    }
#endif
    if (toType.id() == QMetaType::QString || toType.id() == QMetaType::QByteArray)
        return QMetaType::convert(QMetaType::fromType<qlonglong>(), &ll, toType, to);
    return false;
}

static bool convertToEnum(QMetaType fromType, const void *from, QMetaType toType, void *to)
{
    int fromTypeId = fromType.id();
    qlonglong value = -1;
    bool ok = false;
#ifndef QT_NO_QOBJECT
    if (fromTypeId == QMetaType::QString || fromTypeId == QMetaType::QByteArray) {
        QMetaEnum en = metaEnumFromType(toType);
        if (en.isValid()) {
            QByteArray keys = (fromTypeId == QMetaType::QString)
                    ? static_cast<const QString *>(from)->toUtf8()
                    : *static_cast<const QByteArray *>(from);
            value = en.keysToValue(keys.constData(), &ok);
        }
    }
#endif
    if (!ok) {
        if (fromTypeId == QMetaType::LongLong) {
            value = *static_cast<const qlonglong *>(from);
            ok = true;
        } else {
            ok = QMetaType::convert(fromType, from, QMetaType::fromType<qlonglong>(), &value);
        }
    }

    if (!ok)
        return false;

    switch (toType.sizeOf()) {
    case 1:
        *static_cast<signed char *>(to) = value;
        return true;
    case 2:
        *static_cast<qint16 *>(to) = value;
        return true;
    case 4:
        *static_cast<qint32 *>(to) = value;
        return true;
    case 8:
        *static_cast<qint64 *>(to) = value;
        return true;
    default:
        Q_UNREACHABLE_RETURN(false);
    }
}

#ifndef QT_BOOTSTRAPPED
static bool convertIterableToVariantList(QMetaType fromType, const void *from, void *to)
{
    QSequentialIterable list;
    if (!QMetaType::convert(fromType, from, QMetaType::fromType<QSequentialIterable>(), &list))
        return false;

    QVariantList &l = *static_cast<QVariantList *>(to);
    l.clear();
    l.reserve(list.size());
    auto end = list.end();
    for (auto it = list.begin(); it != end; ++it)
        l << *it;
    return true;
}

static bool convertIterableToVariantMap(QMetaType fromType, const void *from, void *to)
{
    QAssociativeIterable map;
    if (!QMetaType::convert(fromType, from, QMetaType::fromType<QAssociativeIterable>(), &map))
        return false;

    QVariantMap &h = *static_cast<QVariantMap *>(to);
    h.clear();
    auto end = map.end();
    for (auto it = map.begin(); it != end; ++it)
        h.insert(it.key().toString(), it.value());
    return true;
}

static bool convertIterableToVariantHash(QMetaType fromType, const void *from, void *to)
{
    QAssociativeIterable map;
    if (!QMetaType::convert(fromType, from, QMetaType::fromType<QAssociativeIterable>(), &map))
        return false;

    QVariantHash &h = *static_cast<QVariantHash *>(to);
    h.clear();
    h.reserve(map.size());
    auto end = map.end();
    for (auto it = map.begin(); it != end; ++it)
        h.insert(it.key().toString(), it.value());
    return true;
}
#endif

static bool convertIterableToVariantPair(QMetaType fromType, const void *from, void *to)
{
    const QMetaType::ConverterFunction * const f =
        customTypesConversionRegistry()->function(qMakePair(fromType.id(),
                                                            qMetaTypeId<QtMetaTypePrivate::QPairVariantInterfaceImpl>()));
    if (!f)
        return false;

    QtMetaTypePrivate::QPairVariantInterfaceImpl pi;
    (*f)(from, &pi);

    QVariant v1(pi._metaType_first);
    void *dataPtr;
    if (pi._metaType_first == QMetaType::fromType<QVariant>())
        dataPtr = &v1;
    else
        dataPtr = v1.data();
    pi.first(dataPtr);

    QVariant v2(pi._metaType_second);
    if (pi._metaType_second == QMetaType::fromType<QVariant>())
        dataPtr = &v2;
    else
        dataPtr = v2.data();
    pi.second(dataPtr);

    *static_cast<QVariantPair *>(to) = QVariantPair(v1, v2);
    return true;
}

#ifndef QT_BOOTSTRAPPED
static bool convertToSequentialIterable(QMetaType fromType, const void *from, void *to)
{
    using namespace QtMetaTypePrivate;
    const int fromTypeId = fromType.id();

    QSequentialIterable &i = *static_cast<QSequentialIterable *>(to);
    switch (fromTypeId) {
    case QMetaType::QVariantList:
        i = QSequentialIterable(reinterpret_cast<const QVariantList *>(from));
        return true;
    case QMetaType::QStringList:
        i = QSequentialIterable(reinterpret_cast<const QStringList *>(from));
        return true;
    case QMetaType::QByteArrayList:
        i = QSequentialIterable(reinterpret_cast<const QByteArrayList *>(from));
        return true;
    case QMetaType::QString:
        i = QSequentialIterable(reinterpret_cast<const QString *>(from));
        return true;
    case QMetaType::QByteArray:
        i = QSequentialIterable(reinterpret_cast<const QByteArray *>(from));
        return true;
    default: {
        QSequentialIterable impl;
        if (QMetaType::convert(
                    fromType, from, QMetaType::fromType<QIterable<QMetaSequence>>(), &impl)) {
            i = std::move(impl);
            return true;
        }
    }
    }

    return false;
}

static bool canConvertToSequentialIterable(QMetaType fromType)
{
    switch (fromType.id()) {
    case QMetaType::QVariantList:
    case QMetaType::QStringList:
    case QMetaType::QByteArrayList:
    case QMetaType::QString:
    case QMetaType::QByteArray:
        return true;
    default:
        return QMetaType::canConvert(fromType, QMetaType::fromType<QIterable<QMetaSequence>>());
    }
}

static bool canImplicitlyViewAsSequentialIterable(QMetaType fromType)
{
    switch (fromType.id()) {
    case QMetaType::QVariantList:
    case QMetaType::QStringList:
    case QMetaType::QByteArrayList:
    case QMetaType::QString:
    case QMetaType::QByteArray:
        return true;
    default:
        return QMetaType::canView(
                    fromType, QMetaType::fromType<QIterable<QMetaSequence>>());
    }
}

static bool viewAsSequentialIterable(QMetaType fromType, void *from, void *to)
{
    using namespace QtMetaTypePrivate;
    const int fromTypeId = fromType.id();

    QSequentialIterable &i = *static_cast<QSequentialIterable *>(to);
    switch (fromTypeId) {
    case QMetaType::QVariantList:
        i = QSequentialIterable(reinterpret_cast<QVariantList *>(from));
        return true;
    case QMetaType::QStringList:
        i = QSequentialIterable(reinterpret_cast<QStringList *>(from));
        return true;
    case QMetaType::QByteArrayList:
        i = QSequentialIterable(reinterpret_cast<QByteArrayList *>(from));
        return true;
    case QMetaType::QString:
        i = QSequentialIterable(reinterpret_cast<QString *>(from));
        return true;
    case QMetaType::QByteArray:
        i = QSequentialIterable(reinterpret_cast<QByteArray *>(from));
        return true;
    default: {
        QIterable<QMetaSequence> j(QMetaSequence(), nullptr);
        if (QMetaType::view(
                    fromType, from, QMetaType::fromType<QIterable<QMetaSequence>>(), &j)) {
            i = std::move(j);
            return true;
        }
    }
    }

    return false;
}

static bool convertToAssociativeIterable(QMetaType fromType, const void *from, void *to)
{
    using namespace QtMetaTypePrivate;

    QAssociativeIterable &i = *static_cast<QAssociativeIterable *>(to);
    if (fromType.id() == QMetaType::QVariantMap) {
        i = QAssociativeIterable(reinterpret_cast<const QVariantMap *>(from));
        return true;
    }
    if (fromType.id() == QMetaType::QVariantHash) {
        i = QAssociativeIterable(reinterpret_cast<const QVariantHash *>(from));
        return true;
    }

    QAssociativeIterable impl;
    if (QMetaType::convert(
                fromType, from, QMetaType::fromType<QIterable<QMetaAssociation>>(), &impl)) {
        i = std::move(impl);
        return true;
    }

    return false;
}

static bool canConvertMetaObject(QMetaType fromType, QMetaType toType)
{
    if ((fromType.flags() & QMetaType::IsPointer) != (toType.flags() & QMetaType::IsPointer))
        return false; // Can not convert between pointer and value

    const QMetaObject *f = fromType.metaObject();
    const QMetaObject *t = toType.metaObject();
    if (f && t) {
        return f->inherits(t) || (t->inherits(f));
    }
    return false;
}

static bool canConvertToAssociativeIterable(QMetaType fromType)
{
    switch (fromType.id()) {
    case QMetaType::QVariantMap:
    case QMetaType::QVariantHash:
        return true;
    default:
        return QMetaType::canConvert(fromType, QMetaType::fromType<QIterable<QMetaAssociation>>());
    }
}

static bool canImplicitlyViewAsAssociativeIterable(QMetaType fromType)
{
    switch (fromType.id()) {
    case QMetaType::QVariantMap:
    case QMetaType::QVariantHash:
        return true;
    default:
        return QMetaType::canView(
                    fromType, QMetaType::fromType<QIterable<QMetaAssociation>>());
    }
}

static bool viewAsAssociativeIterable(QMetaType fromType, void *from, void *to)
{
    using namespace QtMetaTypePrivate;
    int fromTypeId = fromType.id();

    QAssociativeIterable &i = *static_cast<QAssociativeIterable *>(to);
    if (fromTypeId == QMetaType::QVariantMap) {
        i = QAssociativeIterable(reinterpret_cast<QVariantMap *>(from));
        return true;
    }
    if (fromTypeId == QMetaType::QVariantHash) {
        i = QAssociativeIterable(reinterpret_cast<QVariantHash *>(from));
        return true;
    }

    QIterable<QMetaAssociation> j(QMetaAssociation(), nullptr);
    if (QMetaType::view(
                fromType, from, QMetaType::fromType<QIterable<QMetaAssociation>>(), &j)) {
        i = std::move(j);
        return true;
    }

    return false;
}

static bool convertMetaObject(QMetaType fromType, const void *from, QMetaType toType, void *to)
{
    // handle QObject conversion
    if ((fromType.flags() & QMetaType::PointerToQObject) && (toType.flags() & QMetaType::PointerToQObject)) {
        QObject *fromObject = *static_cast<QObject * const *>(from);
        // use dynamic metatype of from if possible
        if (fromObject && fromObject->metaObject()->inherits(toType.metaObject()))  {
            *static_cast<QObject **>(to) = toType.metaObject()->cast(fromObject);
            return true;
        } else if (!fromObject && fromType.metaObject()) {
            // if fromObject is null, use static fromType to check if conversion works
            *static_cast<void **>(to) = nullptr;
            return fromType.metaObject()->inherits(toType.metaObject());
        }
    } else if ((fromType.flags() & QMetaType::IsPointer) == (toType.flags() & QMetaType::IsPointer)) {
        // fromType and toType are of same 'pointedness'
        const QMetaObject *f = fromType.metaObject();
        const QMetaObject *t = toType.metaObject();
        if (f && t && f->inherits(t)) {
            toType.destruct(to);
            toType.construct(to, from);
            return true;
        }
    }
    return false;
}
#endif

/*!
    \fn bool QMetaType::convert(const void *from, int fromTypeId, void *to, int toTypeId)
    \deprecated

    Converts the object at \a from from \a fromTypeId to the preallocated space at \a to
    typed \a toTypeId. Returns \c true, if the conversion succeeded, otherwise false.

    Both \a from and \a to have to be valid pointers.

    \since 5.2
*/

/*!
    Converts the object at \a from from \a fromType to the preallocated space at \a to
    typed \a toType. Returns \c true, if the conversion succeeded, otherwise false.

    Both \a from and \a to have to be valid pointers.

    \since 5.2
*/
bool QMetaType::convert(QMetaType fromType, const void *from, QMetaType toType, void *to)
{
    if (!fromType.isValid() || !toType.isValid())
        return false;

    if (fromType == toType) {
        // just make a copy
        fromType.destruct(to);
        fromType.construct(to, from);
        return true;
    }

    int fromTypeId = fromType.id();
    int toTypeId = toType.id();

    if (auto moduleHelper = qModuleHelperForType(qMax(fromTypeId, toTypeId))) {
        if (moduleHelper->convert(from, fromTypeId, to, toTypeId))
            return true;
    }
    const QMetaType::ConverterFunction * const f =
        customTypesConversionRegistry()->function(qMakePair(fromTypeId, toTypeId));
    if (f)
        return (*f)(from, to);

    if (fromType.flags() & QMetaType::IsEnumeration)
        return convertFromEnum(fromType, from, toType, to);
    if (toType.flags() & QMetaType::IsEnumeration)
        return convertToEnum(fromType, from, toType, to);
    if (toTypeId == Nullptr) {
        *static_cast<std::nullptr_t *>(to) = nullptr;
        if (fromType.flags() & QMetaType::IsPointer) {
            if (*static_cast<const void * const *>(from) == nullptr)
                return true;
        }
    }

    if (toTypeId == QVariantPair && convertIterableToVariantPair(fromType, from, to))
        return true;

#ifndef QT_BOOTSTRAPPED
    // handle iterables
    if (toTypeId == QVariantList && convertIterableToVariantList(fromType, from, to))
        return true;

    if (toTypeId == QVariantMap && convertIterableToVariantMap(fromType, from, to))
        return true;

    if (toTypeId == QVariantHash && convertIterableToVariantHash(fromType, from, to))
        return true;

    if (toTypeId == qMetaTypeId<QSequentialIterable>())
        return convertToSequentialIterable(fromType, from, to);

    if (toTypeId == qMetaTypeId<QAssociativeIterable>())
        return convertToAssociativeIterable(fromType, from, to);

    return convertMetaObject(fromType, from, toType, to);
#else
    return false;
#endif
}

/*!
    Creates a mutable view on the object at \a from of \a fromType in the preallocated space at
    \a to typed \a toType. Returns \c true if the conversion succeeded, otherwise false.
    \since 6.0
*/
bool QMetaType::view(QMetaType fromType, void *from, QMetaType toType, void *to)
{
    if (!fromType.isValid() || !toType.isValid())
        return false;

    int fromTypeId = fromType.id();
    int toTypeId = toType.id();

    const QMetaType::MutableViewFunction * const f =
        customTypesMutableViewRegistry()->function(qMakePair(fromTypeId, toTypeId));
    if (f)
        return (*f)(from, to);

#ifndef QT_BOOTSTRAPPED
    if (toTypeId == qMetaTypeId<QSequentialIterable>())
        return viewAsSequentialIterable(fromType, from, to);

    if (toTypeId == qMetaTypeId<QAssociativeIterable>())
        return viewAsAssociativeIterable(fromType, from, to);

    return convertMetaObject(fromType, from, toType, to);
#else
    return false;
#endif
}

/*!
    Returns \c true if QMetaType::view can create a mutable view of type \a toType
    on type \a fromType.

    Converting between pointers of types derived from QObject will return true for this
    function if a qobject_cast from the type described by \a fromType to the type described
    by \a toType would succeed.

    You can create a mutable view of type QSequentialIterable on any container registered with
    Q_DECLARE_SEQUENTIAL_CONTAINER_METATYPE().

    Similarly you can create a mutable view of type QAssociativeIterable on any container
    registered with Q_DECLARE_ASSOCIATIVE_CONTAINER_METATYPE().

    \sa convert(), QSequentialIterable, Q_DECLARE_SEQUENTIAL_CONTAINER_METATYPE(),
        QAssociativeIterable, Q_DECLARE_ASSOCIATIVE_CONTAINER_METATYPE()
*/
bool QMetaType::canView(QMetaType fromType, QMetaType toType)
{
    int fromTypeId = fromType.id();
    int toTypeId = toType.id();

    if (fromTypeId == UnknownType || toTypeId == UnknownType)
        return false;

    const MutableViewFunction * const f =
        customTypesMutableViewRegistry()->function(qMakePair(fromTypeId, toTypeId));
    if (f)
        return true;

#ifndef QT_BOOTSTRAPPED
    if (toTypeId == qMetaTypeId<QSequentialIterable>())
        return canImplicitlyViewAsSequentialIterable(fromType);

    if (toTypeId == qMetaTypeId<QAssociativeIterable>())
        return canImplicitlyViewAsAssociativeIterable(fromType);

    if (canConvertMetaObject(fromType, toType))
        return true;
#endif

    return false;
}

/*!
    Returns \c true if QMetaType::convert can convert from \a fromType to
    \a toType.

    The following conversions are supported by Qt:

    \table
    \header \li Type \li Automatically Cast To
    \row \li \l QMetaType::Bool \li \l QMetaType::QChar, \l QMetaType::Double,
        \l QMetaType::Int, \l QMetaType::LongLong, \l QMetaType::QString,
        \l QMetaType::UInt, \l QMetaType::ULongLong
    \row \li \l QMetaType::QByteArray \li \l QMetaType::Double,
        \l QMetaType::Int, \l QMetaType::LongLong, \l QMetaType::QString,
        \l QMetaType::UInt, \l QMetaType::ULongLong, \l QMetaType::QUuid
    \row \li \l QMetaType::QChar \li \l QMetaType::Bool, \l QMetaType::Int,
        \l QMetaType::UInt, \l QMetaType::LongLong, \l QMetaType::ULongLong
    \row \li \l QMetaType::QColor \li \l QMetaType::QString
    \row \li \l QMetaType::QDate \li \l QMetaType::QDateTime,
        \l QMetaType::QString
    \row \li \l QMetaType::QDateTime \li \l QMetaType::QDate,
        \l QMetaType::QString, \l QMetaType::QTime
    \row \li \l QMetaType::Double \li \l QMetaType::Bool, \l QMetaType::Int,
        \l QMetaType::LongLong, \l QMetaType::QString, \l QMetaType::UInt,
        \l QMetaType::ULongLong
    \row \li \l QMetaType::QFont \li \l QMetaType::QString
    \row \li \l QMetaType::Int \li \l QMetaType::Bool, \l QMetaType::QChar,
        \l QMetaType::Double, \l QMetaType::LongLong, \l QMetaType::QString,
        \l QMetaType::UInt, \l QMetaType::ULongLong
    \row \li \l QMetaType::QKeySequence \li \l QMetaType::Int,
        \l QMetaType::QString
    \row \li \l QMetaType::QVariantList \li \l QMetaType::QStringList (if the
        list's items can be converted to QStrings)
    \row \li \l QMetaType::LongLong \li \l QMetaType::Bool,
        \l QMetaType::QByteArray, \l QMetaType::QChar, \l QMetaType::Double,
        \l QMetaType::Int, \l QMetaType::QString, \l QMetaType::UInt,
        \l QMetaType::ULongLong
    \row \li \l QMetaType::QPoint \li QMetaType::QPointF
    \row \li \l QMetaType::QRect \li QMetaType::QRectF
    \row \li \l QMetaType::QString \li \l QMetaType::Bool,
        \l QMetaType::QByteArray, \l QMetaType::QChar, \l QMetaType::QColor,
        \l QMetaType::QDate, \l QMetaType::QDateTime, \l QMetaType::Double,
        \l QMetaType::QFont, \l QMetaType::Int, \l QMetaType::QKeySequence,
        \l QMetaType::LongLong, \l QMetaType::QStringList, \l QMetaType::QTime,
        \l QMetaType::UInt, \l QMetaType::ULongLong, \l QMetaType::QUuid
    \row \li \l QMetaType::QStringList \li \l QMetaType::QVariantList,
        \l QMetaType::QString (if the list contains exactly one item)
    \row \li \l QMetaType::QTime \li \l QMetaType::QString
    \row \li \l QMetaType::UInt \li \l QMetaType::Bool, \l QMetaType::QChar,
        \l QMetaType::Double, \l QMetaType::Int, \l QMetaType::LongLong,
        \l QMetaType::QString, \l QMetaType::ULongLong
    \row \li \l QMetaType::ULongLong \li \l QMetaType::Bool,
        \l QMetaType::QChar, \l QMetaType::Double, \l QMetaType::Int,
        \l QMetaType::LongLong, \l QMetaType::QString, \l QMetaType::UInt
    \row \li \l QMetaType::QUuid \li \l QMetaType::QByteArray, \l QMetaType::QString
    \endtable

    Casting between primitive type (int, float, bool etc.) is supported.

    Converting between pointers of types derived from QObject will also return true for this
    function if a qobject_cast from the type described by \a fromType to the type described
    by \a toType would succeed.

    A cast from a sequential container will also return true for this
    function if the \a toType is QVariantList.

    Similarly, a cast from an associative container will also return true for this
    function the \a toType is QVariantHash or QVariantMap.

    \sa convert(), QSequentialIterable, Q_DECLARE_SEQUENTIAL_CONTAINER_METATYPE(), QAssociativeIterable,
        Q_DECLARE_ASSOCIATIVE_CONTAINER_METATYPE()
*/
bool QMetaType::canConvert(QMetaType fromType, QMetaType toType)
{
    int fromTypeId = fromType.id();
    int toTypeId = toType.id();

    if (fromTypeId == UnknownType || toTypeId == UnknownType)
        return false;

    if (fromTypeId == toTypeId)
        return true;

    if (auto moduleHelper = qModuleHelperForType(qMax(fromTypeId, toTypeId))) {
        if (moduleHelper->convert(nullptr, fromTypeId, nullptr, toTypeId))
            return true;
    }
    const ConverterFunction * const f =
        customTypesConversionRegistry()->function(std::make_pair(fromTypeId, toTypeId));
    if (f)
        return true;

#ifndef QT_BOOTSTRAPPED
    if (toTypeId == qMetaTypeId<QSequentialIterable>())
        return canConvertToSequentialIterable(fromType);

    if (toTypeId == qMetaTypeId<QAssociativeIterable>())
        return canConvertToAssociativeIterable(fromType);

    if (toTypeId == QVariantList
            && canConvert(fromType, QMetaType::fromType<QSequentialIterable>())) {
        return true;
    }

    if ((toTypeId == QVariantHash || toTypeId == QVariantMap)
            && canConvert(fromType, QMetaType::fromType<QAssociativeIterable>())) {
        return true;
    }
#endif

    if (toTypeId == QVariantPair && hasRegisteredConverterFunction(
                    fromType, QMetaType::fromType<QtMetaTypePrivate::QPairVariantInterfaceImpl>()))
        return true;

    if (fromType.flags() & IsEnumeration) {
        if (toTypeId == QString || toTypeId == QByteArray)
            return true;
        return canConvert(QMetaType(LongLong), toType);
    }
    if (toType.flags() & IsEnumeration) {
        if (fromTypeId == QString || fromTypeId == QByteArray)
            return true;
        return canConvert(fromType, QMetaType(LongLong));
    }
    if (toTypeId == Nullptr && fromType.flags() & IsPointer)
        return true;
#ifndef QT_BOOTSTRAPPED
    if (canConvertMetaObject(fromType, toType))
        return true;
#endif

    return false;
}

/*!
    \fn bool QMetaType::compare(const void *lhs, const void *rhs, int typeId, int* result)
    \deprecated Use the non-static compare method instead

    Compares the objects at \a lhs and \a rhs. Both objects need to be of type \a typeId.
    \a result is set to less than, equal to or greater than zero, if \a lhs is less than, equal to
    or greater than \a rhs. Returns \c true, if the comparison succeeded, otherwise \c false.
*/

/*!
    \fn bool QMetaType::hasRegisteredConverterFunction()
    Returns \c true, if the meta type system has a registered conversion from type From to type To.
    \since 5.2
    \overload
    */

/*!
    Returns \c true, if the meta type system has a registered conversion from meta type id \a fromType
    to \a toType
    \since 5.2
*/
bool QMetaType::hasRegisteredConverterFunction(QMetaType fromType, QMetaType toType)
{
    return customTypesConversionRegistry()->contains(qMakePair(fromType.id(), toType.id()));
}

/*!
    \fn bool QMetaType::hasRegisteredMutableViewFunction()
    Returns \c true, if the meta type system has a registered mutable view on type From of type To.
    \since 6.0
    \overload
*/

/*!
    Returns \c true, if the meta type system has a registered mutable view on meta type id
    \a fromType of meta type id \a toType.
    \since 5.2
*/
bool QMetaType::hasRegisteredMutableViewFunction(QMetaType fromType, QMetaType toType)
{
    return customTypesMutableViewRegistry()->contains(qMakePair(fromType.id(), toType.id()));
}

/*!
    \fn const char *QMetaType::typeName(int typeId)
    \deprecated

    Returns the type name associated with the given \a typeId, or a null
    pointer if no matching type was found. The returned pointer must not be
    deleted.

    \sa type(), isRegistered(), Type, name()
*/

/*!
    \fn constexpr const char *QMetaType::name() const
    \since 5.15

    Returns the type name associated with this QMetaType, or a null
    pointer if no matching type was found. The returned pointer must not be
    deleted.

    \sa typeName()
*/

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
    if (customTypeRegistry.exists()) {
        auto reg = &*customTypeRegistry;
#if QT_CONFIG(thread)
        Q_ASSERT(!reg->lock.tryLockForWrite());
#endif
        if (auto ti = reg->aliases.value(QByteArray::fromRawData(typeName, length), nullptr)) {
            return ti->typeId.loadRelaxed();
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


static const QtPrivate::QMetaTypeInterface *interfaceForTypeNoWarning(int typeId)
{
    const QtPrivate::QMetaTypeInterface *iface = nullptr;
    if (typeId >= QMetaType::User) {
        if (customTypeRegistry.exists())
            iface = customTypeRegistry->getCustomType(typeId);
    } else {
        if (auto moduleHelper = qModuleHelperForType(typeId))
            iface = moduleHelper->interfaceForType(typeId);
    }
    return iface;
}

/*!
    Returns \c true if the datatype with ID \a type is registered;
    otherwise returns \c false.

    \sa type(), typeName(), Type
*/
bool QMetaType::isRegistered(int type)
{
    return interfaceForTypeNoWarning(type) != nullptr;
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
    \fn int QMetaType::type(const char *typeName)
    \deprecated

    Returns a handle to the type called \a typeName, or QMetaType::UnknownType if there is
    no such type.

    \sa isRegistered(), typeName(), Type
*/

/*!
    \internal

    Similar to QMetaType::type(); the only difference is that this function
    doesn't attempt to normalize the type name (i.e., the lookup will fail
    for type names in non-normalized form).
*/
Q_CORE_EXPORT int qMetaTypeTypeInternal(const char *typeName)
{
    return qMetaTypeTypeImpl</*tryNormalizedType=*/false>(typeName, int(qstrlen(typeName)));
}

/*!
    \fn int QMetaType::type(const QT_PREPEND_NAMESPACE(QByteArray) &typeName)

    \since 5.5
    \overload
    \deprecated

    Returns a handle to the type called \a typeName, or 0 if there is
    no such type.

    \sa isRegistered(), typeName()
*/

#ifndef QT_NO_DATASTREAM
/*!
    Writes the object pointed to by \a data to the given \a stream.
    Returns \c true if the object is saved successfully; otherwise
    returns \c false.

    Normally, you should not need to call this function directly.
    Instead, use QVariant's \c operator<<(), which relies on save()
    to stream custom types.

    \sa load()
*/
bool QMetaType::save(QDataStream &stream, const void *data) const
{
    if (!data || !isValid())
        return false;

    // keep compatibility for long/ulong
    if (id() == QMetaType::Long) {
        stream << qlonglong(*(long *)data);
        return true;
    } else if (id() == QMetaType::ULong) {
        stream << qlonglong(*(unsigned long *)data);
        return true;
    }

    if (!d_ptr->dataStreamOut)
        return false;

    d_ptr->dataStreamOut(d_ptr, stream, data);
    return true;
}

/*!
   \fn bool QMetaType::save(QDataStream &stream, int type, const void *data)
   \overload
   \deprecated
*/

/*!
    Reads the object of this type from the given \a stream into \a data.
    Returns \c true if the object is loaded successfully; otherwise
    returns \c false.

    Normally, you should not need to call this function directly.
    Instead, use QVariant's \c operator>>(), which relies on load()
    to stream custom types.

    \sa save()
*/
bool QMetaType::load(QDataStream &stream, void *data) const
{
    if (!data || !isValid())
        return false;

    // keep compatibility for long/ulong
    if (id() == QMetaType::Long) {
        qlonglong ll;
        stream >> ll;
        *(long *)data = long(ll);
        return true;
    } else if (id() == QMetaType::ULong) {
        qulonglong ull;
        stream >> ull;
        *(unsigned long *)data = (unsigned long)(ull);
        return true;
    }
    if (!d_ptr->dataStreamIn)
        return false;

    d_ptr->dataStreamIn(d_ptr, stream, data);
    return true;
}

/*!
    \since 6.1

    Returns \c true, if the meta type system has registered data stream operators for this
    meta type.
*/
bool QMetaType::hasRegisteredDataStreamOperators() const
{
    int type = id();
    if (type == QMetaType::Long || type == QMetaType::ULong)
        return true;
    return d_ptr && d_ptr->dataStreamIn != nullptr && d_ptr->dataStreamOut != nullptr;
}

/*!
   \since 6.6

   If this metatype represents an enumeration, this method returns a
   metatype of a numeric class of the same signedness and size as the
   enums underlying type.
   If it represents a QFlags type, it returns QMetaType::Int.
   In all other cases an invalid QMetaType is returned.
 */
QMetaType QMetaType::underlyingType() const
{
    if (!d_ptr || !(flags() & IsEnumeration))
        return {};
    /* QFlags has enumeration set so that's handled here (qint32
       case), as QFlags uses int as the underlying type
       Note that we do some approximation here, as we cannot
       differentiate between different underlying types of the
       same size and signedness (consider char <-> (un)signed char,
       int <-> long <-> long long).

       ### TODO PENDING: QTBUG-111926 - QFlags supporting >32 bit int
    */
    if (flags() & IsUnsignedEnumeration) {
        switch (sizeOf()) {
        case 1:
            return QMetaType::fromType<quint8>();
        case 2:
            return QMetaType::fromType<quint16>();
        case 4:
            return QMetaType::fromType<quint32>();
        case 8:
            return QMetaType::fromType<quint64>();
        default:
            break;
        }
    } else {
        switch (sizeOf()) {
        case 1:
            return QMetaType::fromType<qint8>();
        case 2:
            return QMetaType::fromType<qint16>();
        case 4:
            return QMetaType::fromType<qint32>();
        case 8:
            return QMetaType::fromType<qint64>();
        default:
            break;
        }
    }
    // int128 can be handled above once we have qint128
    return QMetaType();
}

/*!
   \fn bool QMetaType::load(QDataStream &stream, int type, void *data)
   \overload
   \deprecated
*/
#endif // QT_NO_DATASTREAM

/*!
    Returns a QMetaType matching \a typeName. The returned object is
    not valid if the typeName is not known to QMetaType
 */
QMetaType QMetaType::fromName(QByteArrayView typeName)
{
    return QMetaType(qMetaTypeTypeImpl</*tryNormalizedType=*/true>(typeName.data(), typeName.size()));
}

/*!
    \fn void *QMetaType::create(int type, const void *copy)
    \deprecated

    Returns a copy of \a copy, assuming it is of type \a type. If \a
    copy is zero, creates a default constructed instance.

    \sa destroy(), isRegistered(), Type
*/

/*!
    \fn void QMetaType::destroy(int type, void *data)
    \deprecated
    Destroys the \a data, assuming it is of the \a type given.

    \sa create(), isRegistered(), Type
*/

/*!
    \fn void *QMetaType::construct(int type, void *where, const void *copy)
    \since 5.0
    \deprecated

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


/*!
    \fn void QMetaType::destruct(int type, void *where)
    \since 5.0
    \deprecated

    Destructs the value of the given \a type, located at \a where.

    Unlike destroy(), this function only invokes the type's
    destructor, it doesn't invoke the delete operator.

    \sa construct()
*/

/*!
    \fn int QMetaType::sizeOf(int type)
    \since 5.0
    \deprecated

    Returns the size of the given \a type in bytes (i.e. sizeof(T),
    where T is the actual type identified by the \a type argument).

    This function is typically used together with construct()
    to perform low-level management of the memory used by a type.

    \sa construct(), QMetaType::alignOf()
*/

/*!
    \fn QMetaType::TypeFlags QMetaType::typeFlags(int type)
    \since 5.0
    \deprecated

    Returns flags of the given \a type.

    \sa QMetaType::TypeFlags
*/

/*!
    \fn const QMetaObject *QMetaType::metaObjectForType(int type)
    \since 5.0
    \deprecated

    returns QMetaType::metaObject for \a type

    \sa metaObject()
*/

/*!
    \fn int qRegisterMetaType(const char *typeName)
    \relates QMetaType
    \obsolete
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

    \sa {QMetaType::}{isRegistered()}, Q_DECLARE_METATYPE()
*/

/*!
    \fn int qRegisterMetaType()
    \relates QMetaType
    \threadsafe
    \since 4.2

    Call this function to register the type \c T. Returns the meta type Id.

    Example:

    \snippet code/src_corelib_kernel_qmetatype.cpp 7

    This function requires that \c{T} is a fully defined type at the point
    where the function is called. For pointer types, it also requires that the
    pointed to type is fully defined. Use Q_DECLARE_OPAQUE_POINTER() to be able
    to register pointers to forward declared types.

    To use the type \c T in QMetaType, QVariant, or with the
    QObject::property() API, registration is not necessary.

    To use the type \c T in queued signal and slot connections,
    \c{qRegisterMetaType<T>()} must be called before the first connection is
    established. That is typically done in the constructor of the class that
    uses \c T, or in the \c{main()} function.

    After a type has been registered, it can be found by its name using
    QMetaType::fromName().

    \sa Q_DECLARE_METATYPE()
 */

/*!
    \fn int qRegisterMetaType(QMetaType meta)
    \relates QMetaType
    \threadsafe
    \since 6.5

    Registers the meta type \a meta and returns its type Id.

    This function requires that \c{T} is a fully defined type at the point
    where the function is called. For pointer types, it also requires that the
    pointed to type is fully defined. Use Q_DECLARE_OPAQUE_POINTER() to be able
    to register pointers to forward declared types.

    To use the type \c T in QMetaType, QVariant, or with the
    QObject::property() API, registration is not necessary.

    To use the type \c T in queued signal and slot connections,
    \c{qRegisterMetaType<T>()} must be called before the first connection is
    established. That is typically done in the constructor of the class that
    uses \c T, or in the \c{main()} function.

    After a type has been registered, it can be found by its name using
    QMetaType::fromName().
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

static const QtPrivate::QMetaTypeInterface *interfaceForType(int typeId)
{
    const QtPrivate::QMetaTypeInterface *iface = interfaceForTypeNoWarning(typeId);
    if (!iface && typeId != QMetaType::UnknownType)
        qWarning("Trying to construct an instance of an invalid type, type id: %i", typeId);

    return iface;
}

/*!
     \fn QMetaType::QMetaType()
     \since 6.0

     Constructs a default, invalid, QMetaType object.
*/

/*!
     \fn QMetaType::QMetaType(int typeId)
     \since 5.0

     Constructs a QMetaType object that contains all information about type \a typeId.
*/
QMetaType::QMetaType(int typeId) : QMetaType(interfaceForType(typeId)) {}


/*! \fn size_t qHash(QMetaType type, size_t seed = 0)
    \relates QMetaType
    \since 6.4

    Returns the hash value for the \a type, using \a seed to seed the calculation.
*/

namespace QtPrivate {
#if !defined(QT_BOOTSTRAPPED) && !defined(Q_CC_MSVC) && !defined(Q_OS_INTEGRITY)

// Explicit instantiation definition
#define QT_METATYPE_DECLARE_TEMPLATE_ITER(TypeName, Id, Name)   \
    template class QMetaTypeForType<Name>;                      \
    template struct QMetaTypeInterfaceWrapper<Name>;
QT_FOR_EACH_STATIC_PRIMITIVE_NON_VOID_TYPE(QT_METATYPE_DECLARE_TEMPLATE_ITER)
QT_FOR_EACH_STATIC_PRIMITIVE_POINTER(QT_METATYPE_DECLARE_TEMPLATE_ITER)
QT_FOR_EACH_STATIC_CORE_CLASS(QT_METATYPE_DECLARE_TEMPLATE_ITER)
QT_FOR_EACH_STATIC_CORE_POINTER(QT_METATYPE_DECLARE_TEMPLATE_ITER)
QT_FOR_EACH_STATIC_CORE_TEMPLATE(QT_METATYPE_DECLARE_TEMPLATE_ITER)

#undef QT_METATYPE_DECLARE_TEMPLATE_ITER
#endif
}

QT_END_NAMESPACE
