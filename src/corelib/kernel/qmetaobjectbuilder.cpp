// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmetaobjectbuilder_p.h"

#include "qobject_p.h"
#include "qmetaobject_p.h"

#include <vector>
#include <stdlib.h>

QT_BEGIN_NAMESPACE

/*!
    \class QMetaObjectBuilder
    \inmodule QtCore
    \internal
    \brief The QMetaObjectBuilder class supports building QMetaObject objects at runtime.

*/

/*!
    \enum QMetaObjectBuilder::AddMember
    This enum defines which members of QMetaObject should be copied by QMetaObjectBuilder::addMetaObject()

    \value ClassName Add the class name.
    \value SuperClass Add the super class.
    \value Methods Add methods that aren't signals or slots.
    \value Signals Add signals.
    \value Slots Add slots.
    \value Constructors Add constructors.
    \value Properties Add properties.
    \value Enumerators Add enumerators.
    \value ClassInfos Add items of class information.
    \value RelatedMetaObjects Add related meta objects.
    \value StaticMetacall Add the static metacall function.
    \value PublicMethods Add public methods (ignored for signals).
    \value ProtectedMethods Add protected methods (ignored for signals).
    \value PrivateMethods All private methods (ignored for signals).
    \value AllMembers Add all members.
    \value AllPrimaryMembers Add everything except the class name, super class, and static metacall function.
*/

// copied from moc's generator.cpp
namespace QtPrivate {
Q_CORE_EXPORT bool isBuiltinType(const QByteArray &type)
{
    int id = QMetaType::fromName(type).id();
    if (!id && !type.isEmpty() && type != "void")
        return false;
    return (id < QMetaType::User);
}
} // namespace QtPrivate

// copied from qmetaobject.cpp
[[maybe_unused]] static inline const QMetaObjectPrivate *qmobPriv(const uint* data)
{ return reinterpret_cast<const QMetaObjectPrivate*>(data); }

class QMetaMethodBuilderPrivate
{
public:
    QMetaMethodBuilderPrivate
            (QMetaMethod::MethodType _methodType,
             const QByteArray& _signature,
             const QByteArray& _returnType = QByteArray("void"),
             QMetaMethod::Access _access = QMetaMethod::Public,
             int _revision = 0)
        : signature(QMetaObject::normalizedSignature(_signature.constData())),
          returnType(QMetaObject::normalizedType(_returnType)),
          attributes(((int)_access) | (((int)_methodType) << 2)),
          revision(_revision)
    {
        Q_ASSERT((_methodType == QMetaMethod::Constructor) == returnType.isNull());
    }

    QByteArray signature;
    QByteArray returnType;
    QList<QByteArray> parameterNames;
    QByteArray tag;
    int attributes;
    int revision;

    QMetaMethod::MethodType methodType() const
    {
        return (QMetaMethod::MethodType)((attributes & MethodTypeMask) >> 2);
    }

    QMetaMethod::Access access() const
    {
        return (QMetaMethod::Access)(attributes & AccessMask);
    }

    void setAccess(QMetaMethod::Access value)
    {
        attributes = ((attributes & ~AccessMask) | (int)value);
    }

    QList<QByteArray> parameterTypes() const
    {
        return QMetaObjectPrivate::parameterTypeNamesFromSignature(signature);
    }

    int parameterCount() const
    {
        return parameterTypes().size();
    }

    QByteArray name() const
    {
        return signature.left(qMax(signature.indexOf('('), 0));
    }
};
Q_DECLARE_TYPEINFO(QMetaMethodBuilderPrivate, Q_RELOCATABLE_TYPE);

class QMetaPropertyBuilderPrivate
{
public:
    QMetaPropertyBuilderPrivate
            (const QByteArray& _name, const QByteArray& _type, QMetaType _metaType, int notifierIdx=-1,
             int _revision = 0)
        : name(_name),
          type(QMetaObject::normalizedType(_type.constData())),
          metaType(_metaType),
          flags(Readable | Writable | Scriptable), notifySignal(notifierIdx),
          revision(_revision)
    {

    }

    QByteArray name;
    QByteArray type;
    QMetaType metaType;
    int flags;
    int notifySignal;
    int revision;

    bool flag(int f) const
    {
        return ((flags & f) != 0);
    }

    void setFlag(int f, bool value)
    {
        if (value)
            flags |= f;
        else
            flags &= ~f;
    }
};
Q_DECLARE_TYPEINFO(QMetaPropertyBuilderPrivate, Q_RELOCATABLE_TYPE);

class QMetaEnumBuilderPrivate
{
public:
    QMetaEnumBuilderPrivate(const QByteArray &_name)
        : name(_name), enumName(_name), isFlag(false), isScoped(false)
    {
    }

    QByteArray name;
    QByteArray enumName;
    QMetaType metaType;
    bool isFlag;
    bool isScoped;
    QList<QByteArray> keys;
    QList<int> values;
};
Q_DECLARE_TYPEINFO(QMetaEnumBuilderPrivate, Q_RELOCATABLE_TYPE);

class QMetaObjectBuilderPrivate
{
public:
    QMetaObjectBuilderPrivate()
        : flags(0)
    {
        superClass = &QObject::staticMetaObject;
        staticMetacallFunction = nullptr;
    }

    bool hasRevisionedMethods() const;

    QByteArray className;
    const QMetaObject *superClass;
    QMetaObjectBuilder::StaticMetacallFunction staticMetacallFunction;
    std::vector<QMetaMethodBuilderPrivate> methods;
    std::vector<QMetaMethodBuilderPrivate> constructors;
    std::vector<QMetaPropertyBuilderPrivate> properties;
    QList<QByteArray> classInfoNames;
    QList<QByteArray> classInfoValues;
    std::vector<QMetaEnumBuilderPrivate> enumerators;
    QList<const QMetaObject *> relatedMetaObjects;
    MetaObjectFlags flags;
};

bool QMetaObjectBuilderPrivate::hasRevisionedMethods() const
{
    for (const auto &method : methods) {
        if (method.revision)
            return true;
    }
    return false;
}

/*!
    Constructs a new QMetaObjectBuilder.
*/
QMetaObjectBuilder::QMetaObjectBuilder()
{
    d = new QMetaObjectBuilderPrivate();
}

/*!
    Constructs a new QMetaObjectBuilder which is a copy of the
    meta object information in \a prototype.  Note: the super class
    contents for \a prototype are not copied, only the immediate
    class that is defined by \a prototype.

    The \a members parameter indicates which members of \a prototype
    should be added.  The default is AllMembers.

    \sa addMetaObject()
*/
QMetaObjectBuilder::QMetaObjectBuilder(const QMetaObject *prototype,
                                       QMetaObjectBuilder::AddMembers members)
{
    d = new QMetaObjectBuilderPrivate();
    addMetaObject(prototype, members);
}

/*!
    Destroys this meta object builder.
*/
QMetaObjectBuilder::~QMetaObjectBuilder()
{
    delete d;
}

/*!
    Returns the name of the class being constructed by this
    meta object builder.  The default value is an empty QByteArray.

    \sa setClassName(), superClass()
*/
QByteArray QMetaObjectBuilder::className() const
{
    return d->className;
}

/*!
    Sets the \a name of the class being constructed by this
    meta object builder.

    \sa className(), setSuperClass()
*/
void QMetaObjectBuilder::setClassName(const QByteArray &name)
{
    d->className = name;
}

/*!
    Returns the superclass meta object of the class being constructed
    by this meta object builder.  The default value is the meta object
    for QObject.

    \sa setSuperClass(), className()
*/
const QMetaObject *QMetaObjectBuilder::superClass() const
{
    return d->superClass;
}

/*!
    Sets the superclass meta object of the class being constructed
    by this meta object builder to \a meta.  The \a meta parameter
    must not be null.

    \sa superClass(), setClassName()
*/
void QMetaObjectBuilder::setSuperClass(const QMetaObject *meta)
{
    Q_ASSERT(meta);
    d->superClass = meta;
}

/*!
    Returns the flags of the class being constructed by this meta object
    builder.

    \sa setFlags()
*/
MetaObjectFlags QMetaObjectBuilder::flags() const
{
    return d->flags;
}

/*!
    Sets the \a flags of the class being constructed by this meta object
    builder.

    \sa flags()
*/
void QMetaObjectBuilder::setFlags(MetaObjectFlags flags)
{
    d->flags = flags;
}

/*!
    Returns the number of methods in this class, excluding the number
    of methods in the base class.  These include signals and slots
    as well as normal member functions.

    \sa addMethod(), method(), removeMethod(), indexOfMethod()
*/
int QMetaObjectBuilder::methodCount() const
{
    return int(d->methods.size());
}

/*!
    Returns the number of constructors in this class.

    \sa addConstructor(), constructor(), removeConstructor(), indexOfConstructor()
*/
int QMetaObjectBuilder::constructorCount() const
{
    return int(d->constructors.size());
}

/*!
    Returns the number of properties in this class, excluding the number
    of properties in the base class.

    \sa addProperty(), property(), removeProperty(), indexOfProperty()
*/
int QMetaObjectBuilder::propertyCount() const
{
    return int(d->properties.size());
}

/*!
    Returns the number of enumerators in this class, excluding the
    number of enumerators in the base class.

    \sa addEnumerator(), enumerator(), removeEnumerator()
    \sa indexOfEnumerator()
*/
int QMetaObjectBuilder::enumeratorCount() const
{
    return int(d->enumerators.size());
}

/*!
    Returns the number of items of class information in this class,
    exclusing the number of items of class information in the base class.

    \sa addClassInfo(), classInfoName(), classInfoValue(), removeClassInfo()
    \sa indexOfClassInfo()
*/
int QMetaObjectBuilder::classInfoCount() const
{
    return d->classInfoNames.size();
}

/*!
    Returns the number of related meta objects that are associated
    with this class.

    Related meta objects are used when resolving the enumerated type
    associated with a property, where the enumerated type is in a
    different class from the property.

    \sa addRelatedMetaObject(), relatedMetaObject()
    \sa removeRelatedMetaObject()
*/
int QMetaObjectBuilder::relatedMetaObjectCount() const
{
    return d->relatedMetaObjects.size();
}

/*!
    Adds a new public method to this class with the specified \a signature.
    Returns an object that can be used to adjust the other attributes
    of the method.  The \a signature will be normalized before it is
    added to the class.

    \sa method(), methodCount(), removeMethod(), indexOfMethod()
*/
QMetaMethodBuilder QMetaObjectBuilder::addMethod(const QByteArray &signature)
{
    int index = int(d->methods.size());
    d->methods.push_back(QMetaMethodBuilderPrivate(QMetaMethod::Method, signature));
    return QMetaMethodBuilder(this, index);
}

/*!
    Adds a new public method to this class with the specified
    \a signature and \a returnType.  Returns an object that can be
    used to adjust the other attributes of the method.  The \a signature
    and \a returnType will be normalized before they are added to
    the class.

    \sa method(), methodCount(), removeMethod(), indexOfMethod()
*/
QMetaMethodBuilder QMetaObjectBuilder::addMethod(const QByteArray &signature,
                                                 const QByteArray &returnType)
{
    int index = int(d->methods.size());
    d->methods.push_back(QMetaMethodBuilderPrivate(QMetaMethod::Method, signature, returnType));
    return QMetaMethodBuilder(this, index);
}

/*!
    Adds a new public method to this class that has the same information as
    \a prototype.  This is used to clone the methods of an existing
    QMetaObject.  Returns an object that can be used to adjust the
    attributes of the method.

    This function will detect if \a prototype is an ordinary method,
    signal, slot, or constructor and act accordingly.

    \sa method(), methodCount(), removeMethod(), indexOfMethod()
*/
QMetaMethodBuilder QMetaObjectBuilder::addMethod(const QMetaMethod &prototype)
{
    QMetaMethodBuilder method;
    if (prototype.methodType() == QMetaMethod::Method)
        method = addMethod(prototype.methodSignature());
    else if (prototype.methodType() == QMetaMethod::Signal)
        method = addSignal(prototype.methodSignature());
    else if (prototype.methodType() == QMetaMethod::Slot)
        method = addSlot(prototype.methodSignature());
    else if (prototype.methodType() == QMetaMethod::Constructor)
        method = addConstructor(prototype.methodSignature());
    method.setReturnType(prototype.typeName());
    method.setParameterNames(prototype.parameterNames());
    method.setTag(prototype.tag());
    method.setAccess(prototype.access());
    method.setAttributes(prototype.attributes());
    method.setRevision(prototype.revision());
    return method;
}

/*!
    Adds a new public slot to this class with the specified \a signature.
    Returns an object that can be used to adjust the other attributes
    of the slot.  The \a signature will be normalized before it is
    added to the class.

    \sa addMethod(), addSignal(), indexOfSlot()
*/
QMetaMethodBuilder QMetaObjectBuilder::addSlot(const QByteArray &signature)
{
    int index = int(d->methods.size());
    d->methods.push_back(QMetaMethodBuilderPrivate(QMetaMethod::Slot, signature));
    return QMetaMethodBuilder(this, index);
}

/*!
    Adds a new signal to this class with the specified \a signature.
    Returns an object that can be used to adjust the other attributes
    of the signal.  The \a signature will be normalized before it is
    added to the class.

    \sa addMethod(), addSlot(), indexOfSignal()
*/
QMetaMethodBuilder QMetaObjectBuilder::addSignal(const QByteArray &signature)
{
    int index = int(d->methods.size());
    d->methods.push_back(QMetaMethodBuilderPrivate(QMetaMethod::Signal, signature,
                                                   QByteArray("void"), QMetaMethod::Public));
    return QMetaMethodBuilder(this, index);
}

/*!
    Adds a new constructor to this class with the specified \a signature.
    Returns an object that can be used to adjust the other attributes
    of the constructor.  The \a signature will be normalized before it is
    added to the class.

    \sa constructor(), constructorCount(), removeConstructor()
    \sa indexOfConstructor()
*/
QMetaMethodBuilder QMetaObjectBuilder::addConstructor(const QByteArray &signature)
{
    int index = int(d->constructors.size());
    d->constructors.push_back(QMetaMethodBuilderPrivate(QMetaMethod::Constructor, signature,
                                                        /*returnType=*/QByteArray()));
    return QMetaMethodBuilder(this, -(index + 1));
}

/*!
    Adds a new constructor to this class that has the same information as
    \a prototype.  This is used to clone the constructors of an existing
    QMetaObject.  Returns an object that can be used to adjust the
    attributes of the constructor.

    This function requires that \a prototype be a constructor.

    \sa constructor(), constructorCount(), removeConstructor()
    \sa indexOfConstructor()
*/
QMetaMethodBuilder QMetaObjectBuilder::addConstructor(const QMetaMethod &prototype)
{
    Q_ASSERT(prototype.methodType() == QMetaMethod::Constructor);
    QMetaMethodBuilder ctor = addConstructor(prototype.methodSignature());
    ctor.setReturnType(prototype.typeName());
    ctor.setParameterNames(prototype.parameterNames());
    ctor.setTag(prototype.tag());
    ctor.setAccess(prototype.access());
    ctor.setAttributes(prototype.attributes());
    return ctor;
}

/*!
    Adds a new readable/writable property to this class with the
    specified \a name and \a type.  Returns an object that can be used
    to adjust the other attributes of the property.  The \a type will
    be normalized before it is added to the class. \a notifierId will
    be registered as the property's \e notify signal.

    \sa property(), propertyCount(), removeProperty(), indexOfProperty()
*/
QMetaPropertyBuilder QMetaObjectBuilder::addProperty(const QByteArray &name, const QByteArray &type,
                                                     int notifierId)
{
    return addProperty(name, type, QMetaType::fromName(type), notifierId);
}

/*!
    \overload
 */
QMetaPropertyBuilder QMetaObjectBuilder::addProperty(const QByteArray &name, const QByteArray &type, QMetaType metaType, int notifierId)
{
    int index = int(d->properties.size());
    d->properties.push_back(QMetaPropertyBuilderPrivate(name, type, metaType, notifierId));
    return QMetaPropertyBuilder(this, index);
}

/*!
    Adds a new property to this class that has the same information as
    \a prototype.  This is used to clone the properties of an existing
    QMetaObject.  Returns an object that can be used to adjust the
    attributes of the property.

    \sa property(), propertyCount(), removeProperty(), indexOfProperty()
*/
QMetaPropertyBuilder QMetaObjectBuilder::addProperty(const QMetaProperty &prototype)
{
    QMetaPropertyBuilder property = addProperty(prototype.name(), prototype.typeName(), prototype.metaType());
    property.setReadable(prototype.isReadable());
    property.setWritable(prototype.isWritable());
    property.setResettable(prototype.isResettable());
    property.setDesignable(prototype.isDesignable());
    property.setScriptable(prototype.isScriptable());
    property.setStored(prototype.isStored());
    property.setUser(prototype.isUser());
    property.setStdCppSet(prototype.hasStdCppSet());
    property.setEnumOrFlag(prototype.isEnumType());
    property.setConstant(prototype.isConstant());
    property.setFinal(prototype.isFinal());
    property.setRevision(prototype.revision());
    if (prototype.hasNotifySignal()) {
        // Find an existing method for the notify signal, or add a new one.
        QMetaMethod method = prototype.notifySignal();
        int index = indexOfMethod(method.methodSignature());
        if (index == -1)
            index = addMethod(method).index();
        d->properties[property._index].notifySignal = index;
    }
    return property;
}

/*!
    Adds a new enumerator to this class with the specified
    \a name.  Returns an object that can be used to adjust
    the other attributes of the enumerator.

    \sa enumerator(), enumeratorCount(), removeEnumerator()
    \sa indexOfEnumerator()
*/
QMetaEnumBuilder QMetaObjectBuilder::addEnumerator(const QByteArray &name)
{
    int index = int(d->enumerators.size());
    d->enumerators.push_back(QMetaEnumBuilderPrivate(name));
    return QMetaEnumBuilder(this, index);
}

/*!
    Adds a new enumerator to this class that has the same information as
    \a prototype.  This is used to clone the enumerators of an existing
    QMetaObject.  Returns an object that can be used to adjust the
    attributes of the enumerator.

    \sa enumerator(), enumeratorCount(), removeEnumerator()
    \sa indexOfEnumerator()
*/
QMetaEnumBuilder QMetaObjectBuilder::addEnumerator(const QMetaEnum &prototype)
{
    QMetaEnumBuilder en = addEnumerator(prototype.name());
    en.setEnumName(prototype.enumName());
    en.setMetaType(prototype.metaType());
    en.setIsFlag(prototype.isFlag());
    en.setIsScoped(prototype.isScoped());
    int count = prototype.keyCount();
    for (int index = 0; index < count; ++index)
        en.addKey(prototype.key(index), prototype.value(index));
    return en;
}

/*!
    Adds \a name and \a value as an item of class information to this class.
    Returns the index of the new item of class information.

    \sa classInfoCount(), classInfoName(), classInfoValue(), removeClassInfo()
    \sa indexOfClassInfo()
*/
int QMetaObjectBuilder::addClassInfo(const QByteArray &name, const QByteArray &value)
{
    int index = d->classInfoNames.size();
    d->classInfoNames += name;
    d->classInfoValues += value;
    return index;
}

/*!
    Adds \a meta to this class as a related meta object.  Returns
    the index of the new related meta object entry.

    Related meta objects are used when resolving the enumerated type
    associated with a property, where the enumerated type is in a
    different class from the property.

    \sa relatedMetaObjectCount(), relatedMetaObject()
    \sa removeRelatedMetaObject()
*/
int QMetaObjectBuilder::addRelatedMetaObject(const QMetaObject *meta)
{
    Q_ASSERT(meta);
    int index = d->relatedMetaObjects.size();
    d->relatedMetaObjects.append(meta);
    return index;
}

/*!
    Adds the contents of \a prototype to this meta object builder.
    This function is useful for cloning the contents of an existing QMetaObject.

    The \a members parameter indicates which members of \a prototype
    should be added.  The default is AllMembers.
*/
void QMetaObjectBuilder::addMetaObject(const QMetaObject *prototype,
                                       QMetaObjectBuilder::AddMembers members)
{
    Q_ASSERT(prototype);
    int index;

    if ((members & ClassName) != 0)
        d->className = prototype->className();

    if ((members & SuperClass) != 0)
        d->superClass = prototype->superClass();

    if ((members & (Methods | Signals | Slots)) != 0) {
        for (index = prototype->methodOffset(); index < prototype->methodCount(); ++index) {
            QMetaMethod method = prototype->method(index);
            if (method.methodType() != QMetaMethod::Signal) {
                if (method.access() == QMetaMethod::Public && (members & PublicMethods) == 0)
                    continue;
                if (method.access() == QMetaMethod::Private && (members & PrivateMethods) == 0)
                    continue;
                if (method.access() == QMetaMethod::Protected && (members & ProtectedMethods) == 0)
                    continue;
            }
            if (method.methodType() == QMetaMethod::Method && (members & Methods) != 0) {
                addMethod(method);
            } else if (method.methodType() == QMetaMethod::Signal &&
                       (members & Signals) != 0) {
                addMethod(method);
            } else if (method.methodType() == QMetaMethod::Slot &&
                       (members & Slots) != 0) {
                addMethod(method);
            }
        }
    }

    if ((members & Constructors) != 0) {
        for (index = 0; index < prototype->constructorCount(); ++index)
            addConstructor(prototype->constructor(index));
    }

    if ((members & Properties) != 0) {
        for (index = prototype->propertyOffset(); index < prototype->propertyCount(); ++index)
            addProperty(prototype->property(index));
    }

    if ((members & Enumerators) != 0) {
        for (index = prototype->enumeratorOffset(); index < prototype->enumeratorCount(); ++index)
            addEnumerator(prototype->enumerator(index));
    }

    if ((members & ClassInfos) != 0) {
        for (index = prototype->classInfoOffset(); index < prototype->classInfoCount(); ++index) {
            QMetaClassInfo ci = prototype->classInfo(index);
            addClassInfo(ci.name(), ci.value());
        }
    }

    if ((members & RelatedMetaObjects) != 0) {
        Q_ASSERT(qmobPriv(prototype->d.data)->revision >= 2);
        const auto *objects = prototype->d.relatedMetaObjects;
        if (objects) {
            while (*objects != nullptr) {
                addRelatedMetaObject(*objects);
                ++objects;
            }
        }
    }

    if ((members & StaticMetacall) != 0) {
        Q_ASSERT(qmobPriv(prototype->d.data)->revision >= 6);
        if (prototype->d.static_metacall)
            setStaticMetacallFunction(prototype->d.static_metacall);
    }
}

/*!
    Returns the method at \a index in this class.

    \sa methodCount(), addMethod(), removeMethod(), indexOfMethod()
*/
QMetaMethodBuilder QMetaObjectBuilder::method(int index) const
{
    if (uint(index) < d->methods.size())
        return QMetaMethodBuilder(this, index);
    else
        return QMetaMethodBuilder();
}

/*!
    Returns the constructor at \a index in this class.

    \sa methodCount(), addMethod(), removeMethod(), indexOfConstructor()
*/
QMetaMethodBuilder QMetaObjectBuilder::constructor(int index) const
{
    if (uint(index) < d->constructors.size())
        return QMetaMethodBuilder(this, -(index + 1));
    else
        return QMetaMethodBuilder();
}

/*!
    Returns the property at \a index in this class.

    \sa methodCount(), addMethod(), removeMethod(), indexOfProperty()
*/
QMetaPropertyBuilder QMetaObjectBuilder::property(int index) const
{
    if (uint(index) < d->properties.size())
        return QMetaPropertyBuilder(this, index);
    else
        return QMetaPropertyBuilder();
}

/*!
    Returns the enumerator at \a index in this class.

    \sa enumeratorCount(), addEnumerator(), removeEnumerator()
    \sa indexOfEnumerator()
*/
QMetaEnumBuilder QMetaObjectBuilder::enumerator(int index) const
{
    if (uint(index) < d->enumerators.size())
        return QMetaEnumBuilder(this, index);
    else
        return QMetaEnumBuilder();
}

/*!
    Returns the related meta object at \a index in this class.

    Related meta objects are used when resolving the enumerated type
    associated with a property, where the enumerated type is in a
    different class from the property.

    \sa relatedMetaObjectCount(), addRelatedMetaObject()
    \sa removeRelatedMetaObject()
*/
const QMetaObject *QMetaObjectBuilder::relatedMetaObject(int index) const
{
    if (index >= 0 && index < d->relatedMetaObjects.size())
        return d->relatedMetaObjects[index];
    else
        return nullptr;
}

/*!
    Returns the name of the item of class information at \a index
    in this class.

    \sa classInfoCount(), addClassInfo(), classInfoValue(), removeClassInfo()
    \sa indexOfClassInfo()
*/
QByteArray QMetaObjectBuilder::classInfoName(int index) const
{
    if (index >= 0 && index < d->classInfoNames.size())
        return d->classInfoNames[index];
    else
        return QByteArray();
}

/*!
    Returns the value of the item of class information at \a index
    in this class.

    \sa classInfoCount(), addClassInfo(), classInfoName(), removeClassInfo()
    \sa indexOfClassInfo()
*/
QByteArray QMetaObjectBuilder::classInfoValue(int index) const
{
    if (index >= 0 && index < d->classInfoValues.size())
        return d->classInfoValues[index];
    else
        return QByteArray();
}

/*!
    Removes the method at \a index from this class.  The indices of
    all following methods will be adjusted downwards by 1.  If the
    method is registered as a notify signal on a property, then the
    notify signal will be removed from the property.

    \sa methodCount(), addMethod(), method(), indexOfMethod()
*/
void QMetaObjectBuilder::removeMethod(int index)
{
    if (uint(index) < d->methods.size()) {
        d->methods.erase(d->methods.begin() + index);
        for (auto &property : d->properties) {
            // Adjust the indices of property notify signal references.
            if (property.notifySignal == index) {
                property.notifySignal = -1;
            } else if (property.notifySignal > index)
                property.notifySignal--;
        }
    }
}

/*!
    Removes the constructor at \a index from this class.  The indices of
    all following constructors will be adjusted downwards by 1.

    \sa constructorCount(), addConstructor(), constructor()
    \sa indexOfConstructor()
*/
void QMetaObjectBuilder::removeConstructor(int index)
{
    if (uint(index) < d->constructors.size())
        d->constructors.erase(d->constructors.begin() + index);
}

/*!
    Removes the property at \a index from this class.  The indices of
    all following properties will be adjusted downwards by 1.

    \sa propertyCount(), addProperty(), property(), indexOfProperty()
*/
void QMetaObjectBuilder::removeProperty(int index)
{
    if (uint(index) < d->properties.size())
        d->properties.erase(d->properties.begin() + index);
}

/*!
    Removes the enumerator at \a index from this class.  The indices of
    all following enumerators will be adjusted downwards by 1.

    \sa enumertorCount(), addEnumerator(), enumerator()
    \sa indexOfEnumerator()
*/
void QMetaObjectBuilder::removeEnumerator(int index)
{
    if (uint(index) < d->enumerators.size())
        d->enumerators.erase(d->enumerators.begin() + index);
}

/*!
    Removes the item of class information at \a index from this class.
    The indices of all following items will be adjusted downwards by 1.

    \sa classInfoCount(), addClassInfo(), classInfoName(), classInfoValue()
    \sa indexOfClassInfo()
*/
void QMetaObjectBuilder::removeClassInfo(int index)
{
    if (index >= 0 && index < d->classInfoNames.size()) {
        d->classInfoNames.removeAt(index);
        d->classInfoValues.removeAt(index);
    }
}

/*!
    Removes the related meta object at \a index from this class.
    The indices of all following related meta objects will be adjusted
    downwards by 1.

    Related meta objects are used when resolving the enumerated type
    associated with a property, where the enumerated type is in a
    different class from the property.

    \sa relatedMetaObjectCount(), addRelatedMetaObject()
    \sa relatedMetaObject()
*/
void QMetaObjectBuilder::removeRelatedMetaObject(int index)
{
    if (index >= 0 && index < d->relatedMetaObjects.size())
        d->relatedMetaObjects.removeAt(index);
}

/*!
    Finds a method with the specified \a signature and returns its index;
    otherwise returns -1.  The \a signature will be normalized by this method.

    \sa method(), methodCount(), addMethod(), removeMethod()
*/
int QMetaObjectBuilder::indexOfMethod(const QByteArray &signature)
{
    QByteArray sig = QMetaObject::normalizedSignature(signature);
    for (const auto &method : d->methods) {
        if (sig == method.signature)
            return int(&method - &d->methods.front());
    }
    return -1;
}

/*!
    Finds a signal with the specified \a signature and returns its index;
    otherwise returns -1.  The \a signature will be normalized by this method.

    \sa indexOfMethod(), indexOfSlot()
*/
int QMetaObjectBuilder::indexOfSignal(const QByteArray &signature)
{
    QByteArray sig = QMetaObject::normalizedSignature(signature);
    for (const auto &method : d->methods) {
        if (method.methodType() == QMetaMethod::Signal && sig == method.signature)
            return int(&method - &d->methods.front());
    }
    return -1;
}

/*!
    Finds a slot with the specified \a signature and returns its index;
    otherwise returns -1.  The \a signature will be normalized by this method.

    \sa indexOfMethod(), indexOfSignal()
*/
int QMetaObjectBuilder::indexOfSlot(const QByteArray &signature)
{
    QByteArray sig = QMetaObject::normalizedSignature(signature);
    for (const auto &method : d->methods) {
        if (method.methodType() == QMetaMethod::Slot && sig == method.signature)
            return int(&method - &d->methods.front());
    }
    return -1;
}

/*!
    Finds a constructor with the specified \a signature and returns its index;
    otherwise returns -1.  The \a signature will be normalized by this method.

    \sa constructor(), constructorCount(), addConstructor(), removeConstructor()
*/
int QMetaObjectBuilder::indexOfConstructor(const QByteArray &signature)
{
    QByteArray sig = QMetaObject::normalizedSignature(signature);
    for (const auto &constructor : d->constructors) {
        if (sig == constructor.signature)
            return int(&constructor - &d->constructors.front());
    }
    return -1;
}

/*!
    Finds a property with the specified \a name and returns its index;
    otherwise returns -1.

    \sa property(), propertyCount(), addProperty(), removeProperty()
*/
int QMetaObjectBuilder::indexOfProperty(const QByteArray &name)
{
    for (const auto &property : d->properties) {
        if (name == property.name)
            return int(&property - &d->properties.front());
    }
    return -1;
}

/*!
    Finds an enumerator with the specified \a name and returns its index;
    otherwise returns -1.

    \sa enumertor(), enumeratorCount(), addEnumerator(), removeEnumerator()
*/
int QMetaObjectBuilder::indexOfEnumerator(const QByteArray &name)
{
    for (const auto &enumerator : d->enumerators) {
        if (name == enumerator.name)
            return int(&enumerator - &d->enumerators.front());
    }
    return -1;
}

/*!
    Finds an item of class information with the specified \a name and
    returns its index; otherwise returns -1.

    \sa classInfoName(), classInfoValue(), classInfoCount(), addClassInfo()
    \sa removeClassInfo()
*/
int QMetaObjectBuilder::indexOfClassInfo(const QByteArray &name)
{
    for (int index = 0; index < d->classInfoNames.size(); ++index) {
        if (name == d->classInfoNames[index])
            return index;
    }
    return -1;
}

// Align on a specific type boundary.
#ifdef ALIGN
#  undef ALIGN
#endif
#define ALIGN(size,type)    \
    (size) = ((size) + sizeof(type) - 1) & ~(sizeof(type) - 1)

/*!
    \class QMetaStringTable
    \inmodule QtCore
    \internal
    \brief The QMetaStringTable class can generate a meta-object string table at runtime.
*/

QMetaStringTable::QMetaStringTable(const QByteArray &className)
    : m_index(0)
    , m_className(className)
{
    const int index = enter(m_className);
    Q_ASSERT(index == 0);
    Q_UNUSED(index);
}

// Enters the given value into the string table (if it hasn't already been
// entered). Returns the index of the string.
int QMetaStringTable::enter(const QByteArray &value)
{
    Entries::iterator it = m_entries.find(value);
    if (it != m_entries.end())
        return it.value();
    int pos = m_index;
    m_entries.insert(value, pos);
    ++m_index;
    return pos;
}

int QMetaStringTable::preferredAlignment()
{
    return alignof(uint);
}

// Returns the size (in bytes) required for serializing this string table.
int QMetaStringTable::blobSize() const
{
    int size = int(m_entries.size() * 2 * sizeof(uint));
    Entries::const_iterator it;
    for (it = m_entries.constBegin(); it != m_entries.constEnd(); ++it)
        size += it.key().size() + 1;
    return size;
}

static void writeString(char *out, int i, const QByteArray &str,
                        const int offsetOfStringdataMember, int &stringdataOffset)
{
    int size = str.size();
    int offset = offsetOfStringdataMember + stringdataOffset;
    uint offsetLen[2] = { uint(offset), uint(size) };

    memcpy(out + 2 * i * sizeof(uint), &offsetLen, 2 * sizeof(uint));

    memcpy(out + offset, str.constData(), size);
    out[offset + size] = '\0';

    stringdataOffset += size + 1;
}

// Writes strings to string data struct.
// The struct consists of an array of QByteArrayData, followed by a char array
// containing the actual strings. This format must match the one produced by
// moc (see generator.cpp).
void QMetaStringTable::writeBlob(char *out) const
{
    Q_ASSERT(!(reinterpret_cast<quintptr>(out) & (preferredAlignment() - 1)));

    int offsetOfStringdataMember = int(m_entries.size() * 2 * sizeof(uint));
    int stringdataOffset = 0;

    // qt_metacast expects the first string in the string table to be the class name.
    writeString(out, /*index*/ 0, m_className, offsetOfStringdataMember, stringdataOffset);

    for (Entries::ConstIterator it = m_entries.constBegin(), end = m_entries.constEnd();
         it != end; ++it) {
        const int i = it.value();
        if (i == 0)
            continue;
        const QByteArray &str = it.key();

        writeString(out, i, str, offsetOfStringdataMember, stringdataOffset);
    }
}

// Returns the sum of all parameters (including return type) for the given
// \a methods. This is needed for calculating the size of the methods'
// parameter type/name meta-data.
static int aggregateParameterCount(const std::vector<QMetaMethodBuilderPrivate> &methods)
{
    int sum = 0;
    for (const auto &method : methods)
        sum += method.parameterCount() + 1; // +1 for return type
    return sum;
}

enum Mode {
    Prepare, // compute the size of the metaobject
    Construct // construct metaobject in pre-allocated buffer
};
// Build a QMetaObject in "buf" based on the information in "d".
// If the mode is prepare, then return the number of bytes needed to
// build the QMetaObject.
template<Mode mode>
static int buildMetaObject(QMetaObjectBuilderPrivate *d, char *buf,
                           int expectedSize)
{
    Q_UNUSED(expectedSize); // Avoid warning in release mode
    Q_UNUSED(buf);
    qsizetype size = 0;
    int dataIndex;
    int paramsIndex;
    int enumIndex;
    int index;
    bool hasRevisionedMethods = d->hasRevisionedMethods();

    // Create the main QMetaObject structure at the start of the buffer.
    QMetaObject *meta = reinterpret_cast<QMetaObject *>(buf);
    size += sizeof(QMetaObject);
    ALIGN(size, int);
    if constexpr (mode == Construct) {
        meta->d.superdata = d->superClass;
        meta->d.relatedMetaObjects = nullptr;
        meta->d.extradata = nullptr;
        meta->d.metaTypes = nullptr;
        meta->d.static_metacall = d->staticMetacallFunction;
    }

    // Populate the QMetaObjectPrivate structure.
    QMetaObjectPrivate *pmeta
        = reinterpret_cast<QMetaObjectPrivate *>(buf + size);
    //int pmetaSize = size;
    dataIndex = MetaObjectPrivateFieldCount;
    int methodParametersDataSize =
            ((aggregateParameterCount(d->methods)
             + aggregateParameterCount(d->constructors)) * 2) // types and parameter names
            - int(d->methods.size())       // return "parameters" don't have names
            - int(d->constructors.size()); // "this" parameters don't have names
    if constexpr (mode == Construct) {
        static_assert(QMetaObjectPrivate::OutputRevision == 12, "QMetaObjectBuilder should generate the same version as moc");
        pmeta->revision = QMetaObjectPrivate::OutputRevision;
        pmeta->flags = d->flags.toInt();
        pmeta->className = 0;   // Class name is always the first string.
        //pmeta->signalCount is handled in the "output method loop" as an optimization.

        pmeta->classInfoCount = d->classInfoNames.size();
        pmeta->classInfoData = dataIndex;
        dataIndex += 2 * d->classInfoNames.size();

        pmeta->methodCount = int(d->methods.size());
        pmeta->methodData = dataIndex;
        dataIndex += QMetaObjectPrivate::IntsPerMethod * int(d->methods.size());
        if (hasRevisionedMethods)
            dataIndex += int(d->methods.size());
        paramsIndex = dataIndex;
        dataIndex += methodParametersDataSize;

        pmeta->propertyCount = int(d->properties.size());
        pmeta->propertyData = dataIndex;
        dataIndex += QMetaObjectPrivate::IntsPerProperty * int(d->properties.size());

        pmeta->enumeratorCount = int(d->enumerators.size());
        pmeta->enumeratorData = dataIndex;
        dataIndex += QMetaObjectPrivate::IntsPerEnum * int(d->enumerators.size());

        pmeta->constructorCount = int(d->constructors.size());
        pmeta->constructorData = dataIndex;
        dataIndex += QMetaObjectPrivate::IntsPerMethod * int(d->constructors.size());
    } else {
        dataIndex += 2 * int(d->classInfoNames.size());
        dataIndex += QMetaObjectPrivate::IntsPerMethod * int(d->methods.size());
        if (hasRevisionedMethods)
            dataIndex += int(d->methods.size());
        paramsIndex = dataIndex;
        dataIndex += methodParametersDataSize;
        dataIndex += QMetaObjectPrivate::IntsPerProperty * int(d->properties.size());
        dataIndex += QMetaObjectPrivate::IntsPerEnum * int(d->enumerators.size());
        dataIndex += QMetaObjectPrivate::IntsPerMethod * int(d->constructors.size());
    }

    // Allocate space for the enumerator key names and values.
    enumIndex = dataIndex;
    for (const auto &enumerator : d->enumerators)
        dataIndex += 2 * enumerator.keys.size();

    // Zero terminator at the end of the data offset table.
    ++dataIndex;

    // Find the start of the data and string tables.
    int *data = reinterpret_cast<int *>(pmeta);
    size += dataIndex * sizeof(int);
    ALIGN(size, void *);
    [[maybe_unused]] char *str = reinterpret_cast<char *>(buf + size);
    if constexpr (mode == Construct) {
        meta->d.stringdata = reinterpret_cast<const uint *>(str);
        meta->d.data = reinterpret_cast<uint *>(data);
    }

    // Reset the current data position to just past the QMetaObjectPrivate.
    dataIndex = MetaObjectPrivateFieldCount;

    QMetaStringTable strings(d->className);

    // Output the class infos,
    Q_ASSERT(!buf || dataIndex == pmeta->classInfoData);
    for (index = 0; index < d->classInfoNames.size(); ++index) {
        [[maybe_unused]] int name = strings.enter(d->classInfoNames[index]);
        [[maybe_unused]] int value = strings.enter(d->classInfoValues[index]);
        if constexpr (mode == Construct) {
            data[dataIndex] = name;
            data[dataIndex + 1] = value;
        }
        dataIndex += 2;
    }

    // Output the methods in the class.
    Q_ASSERT(!buf || dataIndex == pmeta->methodData);
    // + 1 for metatype of this metaobject
    int parameterMetaTypesIndex = int(d->properties.size()) + 1;
    for (const auto &method : d->methods) {
        [[maybe_unused]] int name = strings.enter(method.name());
        int argc = method.parameterCount();
        [[maybe_unused]] int tag = strings.enter(method.tag);
        [[maybe_unused]] int attrs = method.attributes;
        if constexpr (mode == Construct) {
            data[dataIndex]     = name;
            data[dataIndex + 1] = argc;
            data[dataIndex + 2] = paramsIndex;
            data[dataIndex + 3] = tag;
            data[dataIndex + 4] = attrs;
            data[dataIndex + 5] = parameterMetaTypesIndex;
            if (method.methodType() == QMetaMethod::Signal)
                pmeta->signalCount++;
        }
        dataIndex += QMetaObjectPrivate::IntsPerMethod;
        paramsIndex += 1 + argc * 2;
        parameterMetaTypesIndex += 1 + argc;
    }
    if (hasRevisionedMethods) {
        for (const auto &method : d->methods) {
            if constexpr (mode == Construct)
                data[dataIndex] = method.revision;
            ++dataIndex;
        }
    }

    // Output the method parameters in the class.
    Q_ASSERT(!buf || dataIndex == pmeta->methodData + int(d->methods.size()) * QMetaObjectPrivate::IntsPerMethod
             + (hasRevisionedMethods ? int(d->methods.size()) : 0));
    for (int x = 0; x < 2; ++x) {
        const std::vector<QMetaMethodBuilderPrivate> &methods = (x == 0) ? d->methods : d->constructors;
        for (const auto &method : methods) {
            const QList<QByteArray> paramTypeNames = method.parameterTypes();
            int paramCount = paramTypeNames.size();
            for (int i = -1; i < paramCount; ++i) {
                const QByteArray &typeName = (i < 0) ? method.returnType : paramTypeNames.at(i);
                [[maybe_unused]] int typeInfo;
                if (QtPrivate::isBuiltinType(typeName))
                    typeInfo = QMetaType::fromName(typeName).id();
                else
                    typeInfo = IsUnresolvedType | strings.enter(typeName);
                if constexpr (mode == Construct)
                    data[dataIndex] = typeInfo;
                ++dataIndex;
            }

            QList<QByteArray> paramNames = method.parameterNames;
            while (paramNames.size() < paramCount)
                paramNames.append(QByteArray());
            for (int i = 0; i < paramCount; ++i) {
                [[maybe_unused]] int stringIndex = strings.enter(paramNames.at(i));
                if constexpr (mode == Construct)
                    data[dataIndex] = stringIndex;
                ++dataIndex;
            }
        }
    }

    // Output the properties in the class.
    Q_ASSERT(!buf || dataIndex == pmeta->propertyData);
    for (QMetaPropertyBuilderPrivate &prop : d->properties) {
        [[maybe_unused]] int name = strings.enter(prop.name);

        // try to resolve the metatype again if it was unknown
        if (!prop.metaType.isValid())
            prop.metaType = QMetaType::fromName(prop.type);
        [[maybe_unused]] const int typeInfo = prop.metaType.isValid()
                       ? prop.metaType.id()
                       : IsUnresolvedType | strings.enter(prop.type);

        [[maybe_unused]] int flags = prop.flags;

        if (!QtPrivate::isBuiltinType(prop.type))
            flags |= EnumOrFlag;

        if constexpr (mode == Construct) {
            data[dataIndex]     = name;
            data[dataIndex + 1] = typeInfo;
            data[dataIndex + 2] = flags;
            data[dataIndex + 3] = prop.notifySignal;
            data[dataIndex + 4] = prop.revision;
        }
        dataIndex += QMetaObjectPrivate::IntsPerProperty;
    }

    // Output the enumerators in the class.
    Q_ASSERT(!buf || dataIndex == pmeta->enumeratorData);
    for (const auto &enumerator : d->enumerators) {
        [[maybe_unused]] int name = strings.enter(enumerator.name);
        [[maybe_unused]] int enumName = strings.enter(enumerator.enumName);
        [[maybe_unused]] int isFlag = enumerator.isFlag ? EnumIsFlag : 0;
        [[maybe_unused]] int isScoped = enumerator.isScoped ? EnumIsScoped : 0;
        int count = enumerator.keys.size();
        int enumOffset = enumIndex;
        if constexpr (mode == Construct) {
            data[dataIndex]     = name;
            data[dataIndex + 1] = enumName;
            data[dataIndex + 2] = isFlag | isScoped;
            data[dataIndex + 3] = count;
            data[dataIndex + 4] = enumOffset;
        }
        for (int key = 0; key < count; ++key) {
            [[maybe_unused]] int keyIndex = strings.enter(enumerator.keys[key]);
            if constexpr (mode == Construct) {
                data[enumOffset++] = keyIndex;
                data[enumOffset++] = enumerator.values[key];
            }
        }
        dataIndex += QMetaObjectPrivate::IntsPerEnum;
        enumIndex += 2 * count;
    }

    // Output the constructors in the class.
    Q_ASSERT(!buf || dataIndex == pmeta->constructorData);
    for (const auto &ctor : d->constructors) {
        [[maybe_unused]] int name = strings.enter(ctor.name());
        int argc = ctor.parameterCount();
        [[maybe_unused]] int tag = strings.enter(ctor.tag);
        [[maybe_unused]] int attrs = ctor.attributes;
        if constexpr (mode == Construct) {
            data[dataIndex]     = name;
            data[dataIndex + 1] = argc;
            data[dataIndex + 2] = paramsIndex;
            data[dataIndex + 3] = tag;
            data[dataIndex + 4] = attrs;
            data[dataIndex + 5] = parameterMetaTypesIndex;
        }
        dataIndex += QMetaObjectPrivate::IntsPerMethod;
        paramsIndex += 1 + argc * 2;
        parameterMetaTypesIndex += argc;
    }

    size += strings.blobSize();

    if constexpr (mode == Construct)
        strings.writeBlob(str);

    // Output the zero terminator in the data array.
    if constexpr (mode == Construct)
        data[enumIndex] = 0;

    // Create the relatedMetaObjects block if we need one.
    if (d->relatedMetaObjects.size() > 0) {
        using SuperData = QMetaObject::SuperData;
        ALIGN(size, SuperData);
        auto objects = reinterpret_cast<SuperData *>(buf + size);
        if constexpr (mode == Construct) {
            meta->d.relatedMetaObjects = objects;
            for (index = 0; index < d->relatedMetaObjects.size(); ++index)
                objects[index] = d->relatedMetaObjects[index];
            objects[index] = nullptr;
        }
        size += sizeof(SuperData) * (d->relatedMetaObjects.size() + 1);
    }

    ALIGN(size, QtPrivate::QMetaTypeInterface *);
    auto types = reinterpret_cast<QtPrivate::QMetaTypeInterface **>(buf + size);
    if constexpr (mode == Construct) {
        meta->d.metaTypes = types;
        for (const auto &prop : d->properties) {
            QMetaType mt = prop.metaType;
            *types = reinterpret_cast<QtPrivate::QMetaTypeInterface *&>(mt);
            types++;
        }
        // add metatype interface for this metaobject - must be null
        // as we can't know our metatype
        *types = nullptr;
        types++;
        for (const auto &method: d->methods) {
            QMetaType mt(QMetaType::fromName(method.returnType).id());
            *types = reinterpret_cast<QtPrivate::QMetaTypeInterface *&>(mt);
            types++;
            for (const auto &parameterType: method.parameterTypes()) {
                QMetaType mt = QMetaType::fromName(parameterType);
                *types = reinterpret_cast<QtPrivate::QMetaTypeInterface *&>(mt);
                types++;
            }
        }
        for (const auto &constructor : d->constructors) {
            for (const auto &parameterType : constructor.parameterTypes()) {
                QMetaType mt = QMetaType::fromName(parameterType);
                *types = reinterpret_cast<QtPrivate::QMetaTypeInterface *&>(mt);
                types++;
            }
        }
    }
    // parameterMetaTypesIndex is equal to the total number of metatypes
    size += sizeof(QMetaType) * parameterMetaTypesIndex;

    // Align the final size and return it.
    ALIGN(size, void *);
    Q_ASSERT(!buf || size == expectedSize);
    return size;
}

/*!
    Converts this meta object builder into a concrete QMetaObject.
    The return value should be deallocated using free() once it
    is no longer needed.

    The returned meta object is a snapshot of the state of the
    QMetaObjectBuilder.  Any further modifications to the QMetaObjectBuilder
    will not be reflected in previous meta objects returned by
    this method.
*/
QMetaObject *QMetaObjectBuilder::toMetaObject() const
{
    int size = buildMetaObject<Prepare>(d, nullptr, 0);
    char *buf = reinterpret_cast<char *>(malloc(size));
    memset(buf, 0, size);
    buildMetaObject<Construct>(d, buf, size);
    return reinterpret_cast<QMetaObject *>(buf);
}

/*!
    \typedef QMetaObjectBuilder::StaticMetacallFunction

    Typedef for static metacall functions.  The three parameters are
    the call type value, the constructor index, and the
    array of parameters.
*/

/*!
    Returns the static metacall function to use to construct objects
    of this class.  The default value is null.

    \sa setStaticMetacallFunction()
*/
QMetaObjectBuilder::StaticMetacallFunction QMetaObjectBuilder::staticMetacallFunction() const
{
    return d->staticMetacallFunction;
}

/*!
    Sets the static metacall function to use to construct objects
    of this class to \a value.  The default value is null.

    \sa staticMetacallFunction()
*/
void QMetaObjectBuilder::setStaticMetacallFunction
        (QMetaObjectBuilder::StaticMetacallFunction value)
{
    d->staticMetacallFunction = value;
}

/*!
    \class QMetaMethodBuilder
    \inmodule QtCore
    \internal
    \brief The QMetaMethodBuilder class enables modifications to a method definition on a meta object builder.
*/

QMetaMethodBuilderPrivate *QMetaMethodBuilder::d_func() const
{
    // Positive indices indicate methods, negative indices indicate constructors.
    if (_mobj && _index >= 0 && _index < int(_mobj->d->methods.size()))
        return &(_mobj->d->methods[_index]);
    else if (_mobj && -_index >= 1 && -_index <= int(_mobj->d->constructors.size()))
        return &(_mobj->d->constructors[(-_index) - 1]);
    else
        return nullptr;
}

/*!
    \fn QMetaMethodBuilder::QMetaMethodBuilder()
    \internal
*/

/*!
    Returns the index of this method within its QMetaObjectBuilder.
*/
int QMetaMethodBuilder::index() const
{
    if (_index >= 0)
        return _index;          // Method, signal, or slot
    else
        return (-_index) - 1;   // Constructor
}

/*!
    Returns the type of this method (signal, slot, method, or constructor).
*/
QMetaMethod::MethodType QMetaMethodBuilder::methodType() const
{
    QMetaMethodBuilderPrivate *d = d_func();
    if (d)
        return d->methodType();
    else
        return QMetaMethod::Method;
}

/*!
    Returns the signature of this method.

    \sa parameterNames(), returnType()
*/
QByteArray QMetaMethodBuilder::signature() const
{
    QMetaMethodBuilderPrivate *d = d_func();
    if (d)
        return d->signature;
    else
        return QByteArray();
}

/*!
    Returns the return type for this method; empty if the method's
    return type is \c{void}.

    \sa setReturnType(), signature()
*/
QByteArray QMetaMethodBuilder::returnType() const
{
    QMetaMethodBuilderPrivate *d = d_func();
    if (d)
        return d->returnType;
    else
        return QByteArray();
}

/*!
    Sets the return type for this method to \a value.  If \a value
    is empty, then the method's return type is \c{void}.  The \a value
    will be normalized before it is added to the method.

    \sa returnType(), parameterTypes(), signature()
*/
void QMetaMethodBuilder::setReturnType(const QByteArray &value)
{
    QMetaMethodBuilderPrivate *d = d_func();
    if (d)
        d->returnType = QMetaObject::normalizedType(value);
}

/*!
    Returns the list of parameter types for this method.

    \sa returnType(), parameterNames()
*/
QList<QByteArray> QMetaMethodBuilder::parameterTypes() const
{
    QMetaMethodBuilderPrivate *d = d_func();
    if (d)
        return d->parameterTypes();
    else
        return QList<QByteArray>();
}

/*!
    Returns the list of parameter names for this method.

    \sa setParameterNames()
*/
QList<QByteArray> QMetaMethodBuilder::parameterNames() const
{
    QMetaMethodBuilderPrivate *d = d_func();
    if (d)
        return d->parameterNames;
    else
        return QList<QByteArray>();
}

/*!
    Sets the list of parameter names for this method to \a value.

    \sa parameterNames()
*/
void QMetaMethodBuilder::setParameterNames(const QList<QByteArray> &value)
{
    QMetaMethodBuilderPrivate *d = d_func();
    if (d)
        d->parameterNames = value;
}

/*!
    Returns the tag associated with this method.

    \sa setTag()
*/
QByteArray QMetaMethodBuilder::tag() const
{
    QMetaMethodBuilderPrivate *d = d_func();
    if (d)
        return d->tag;
    else
        return QByteArray();
}

/*!
    Sets the tag associated with this method to \a value.

    \sa setTag()
*/
void QMetaMethodBuilder::setTag(const QByteArray &value)
{
    QMetaMethodBuilderPrivate *d = d_func();
    if (d)
        d->tag = value;
}

/*!
    Returns the access specification of this method (private, protected,
    or public).  The default value is QMetaMethod::Public for methods,
    slots, signals and constructors.

    \sa setAccess()
*/
QMetaMethod::Access QMetaMethodBuilder::access() const
{
    QMetaMethodBuilderPrivate *d = d_func();
    if (d)
        return d->access();
    else
        return QMetaMethod::Public;
}

/*!
    Sets the access specification of this method (private, protected,
    or public) to \a value.  If the method is a signal, this function
    will be ignored.

    \sa access()
*/
void QMetaMethodBuilder::setAccess(QMetaMethod::Access value)
{
    QMetaMethodBuilderPrivate *d = d_func();
    if (d && d->methodType() != QMetaMethod::Signal)
        d->setAccess(value);
}

/*!
    Returns the additional attributes for this method.

    \sa setAttributes()
*/
int QMetaMethodBuilder::attributes() const
{
    QMetaMethodBuilderPrivate *d = d_func();
    if (d)
        return (d->attributes >> 4);
    else
        return 0;
}

/*!
    Sets the additional attributes for this method to \a value.

    \sa attributes()
*/
void QMetaMethodBuilder::setAttributes(int value)
{
    QMetaMethodBuilderPrivate *d = d_func();
    if (d)
        d->attributes = ((d->attributes & 0x0f) | (value << 4));
}

/*!
    Returns true if the method is const qualified.
 */
int QMetaMethodBuilder::isConst() const
{
    QMetaMethodBuilderPrivate *d = d_func();
    if (!d)
        return false;
    return (d->attributes & MethodIsConst);
}

void QMetaMethodBuilder::setConst(bool methodIsConst)
{
    QMetaMethodBuilderPrivate *d = d_func();
    if (!d)
        return;
    if (methodIsConst)
        d->attributes |= MethodIsConst;
    else
        d->attributes &= ~MethodIsConst;
}

/*!
    Returns the revision of this method.

    \sa setRevision()
*/
int QMetaMethodBuilder::revision() const
{
    QMetaMethodBuilderPrivate *d = d_func();
    if (d)
        return d->revision;
    return 0;
}

/*!
    Sets the \a revision of this method.

    \sa revision()
*/
void QMetaMethodBuilder::setRevision(int revision)
{
    QMetaMethodBuilderPrivate *d = d_func();
    if (d) {
        d->revision = revision;
        if (revision)
            d->attributes |= MethodRevisioned;
        else
            d->attributes &= ~MethodRevisioned;
    }
}

/*!
    \class QMetaPropertyBuilder
    \inmodule QtCore
    \internal
    \brief The QMetaPropertyBuilder class enables modifications to a property definition on a meta object builder.
*/

QMetaPropertyBuilderPrivate *QMetaPropertyBuilder::d_func() const
{
    if (_mobj && _index >= 0 && _index < int(_mobj->d->properties.size()))
        return &(_mobj->d->properties[_index]);
    else
        return nullptr;
}

/*!
    \fn QMetaPropertyBuilder::QMetaPropertyBuilder()
    \internal
*/

/*!
    \fn int QMetaPropertyBuilder::index() const

    Returns the index of this property within its QMetaObjectBuilder.
*/

/*!
    Returns the name associated with this property.

    \sa type()
*/
QByteArray QMetaPropertyBuilder::name() const
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        return d->name;
    else
        return QByteArray();
}

/*!
    Returns the type associated with this property.

    \sa name()
*/
QByteArray QMetaPropertyBuilder::type() const
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        return d->type;
    else
        return QByteArray();
}

/*!
    Returns \c true if this property has a notify signal; false otherwise.

    \sa notifySignal(), setNotifySignal(), removeNotifySignal()
*/
bool QMetaPropertyBuilder::hasNotifySignal() const
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        return d->notifySignal != -1;
    else
        return false;
}

/*!
    Returns the notify signal associated with this property.

    \sa hasNotifySignal(), setNotifySignal(), removeNotifySignal()
*/
QMetaMethodBuilder QMetaPropertyBuilder::notifySignal() const
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d && d->notifySignal >= 0)
        return QMetaMethodBuilder(_mobj, d->notifySignal);
    else
        return QMetaMethodBuilder();
}

/*!
    Sets the notify signal associated with this property to \a value.

    \sa hasNotifySignal(), notifySignal(), removeNotifySignal()
*/
void QMetaPropertyBuilder::setNotifySignal(const QMetaMethodBuilder &value)
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d) {
        if (value._mobj) {
            d->notifySignal = value._index;
        } else {
            d->notifySignal = -1;
        }
    }
}

/*!
    Removes the notify signal from this property.

    \sa hasNotifySignal(), notifySignal(), setNotifySignal()
*/
void QMetaPropertyBuilder::removeNotifySignal()
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        d->notifySignal = -1;
}

/*!
    Returns \c true if this property is readable; otherwise returns \c false.
    The default value is true.

    \sa setReadable(), isWritable()
*/
bool QMetaPropertyBuilder::isReadable() const
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        return d->flag(Readable);
    else
        return false;
}

/*!
    Returns \c true if this property is writable; otherwise returns \c false.
    The default value is true.

    \sa setWritable(), isReadable()
*/
bool QMetaPropertyBuilder::isWritable() const
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        return d->flag(Writable);
    else
        return false;
}

/*!
    Returns \c true if this property can be reset to a default value; otherwise
    returns \c false.  The default value is false.

    \sa setResettable()
*/
bool QMetaPropertyBuilder::isResettable() const
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        return d->flag(Resettable);
    else
        return false;
}

/*!
    Returns \c true if this property is designable; otherwise returns \c false.
    This default value is false.

    \sa setDesignable(), isScriptable(), isStored()
*/
bool QMetaPropertyBuilder::isDesignable() const
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        return d->flag(Designable);
    else
        return false;
}

/*!
    Returns \c true if the property is scriptable; otherwise returns \c false.
    This default value is true.

    \sa setScriptable(), isDesignable(), isStored()
*/
bool QMetaPropertyBuilder::isScriptable() const
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        return d->flag(Scriptable);
    else
        return false;
}

/*!
    Returns \c true if the property is stored; otherwise returns \c false.
    This default value is false.

    \sa setStored(), isDesignable(), isScriptable()
*/
bool QMetaPropertyBuilder::isStored() const
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        return d->flag(Stored);
    else
        return false;
}

/*!
    Returns \c true if this property is designated as the \c USER
    property, i.e., the one that the user can edit or that is
    significant in some other way.  Otherwise it returns
    false.  This default value is false.

    \sa setUser(), isDesignable(), isScriptable()
*/
bool QMetaPropertyBuilder::isUser() const
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        return d->flag(User);
    else
        return false;
}

/*!
    Returns \c true if the property has a C++ setter function that
    follows Qt's standard "name" / "setName" pattern. Designer and uic
    query hasStdCppSet() in order to avoid expensive
    QObject::setProperty() calls. All properties in Qt [should] follow
    this pattern.  The default value is false.

    \sa setStdCppSet()
*/
bool QMetaPropertyBuilder::hasStdCppSet() const
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        return d->flag(StdCppSet);
    else
        return false;
}

/*!
    Returns \c true if the property is an enumerator or flag type;
    otherwise returns \c false.  This default value is false.

    \sa setEnumOrFlag()
*/
bool QMetaPropertyBuilder::isEnumOrFlag() const
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        return d->flag(EnumOrFlag);
    else
        return false;
}

/*!
    Returns \c true if the property is constant; otherwise returns \c false.
    The default value is false.
*/
bool QMetaPropertyBuilder::isConstant() const
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        return d->flag(Constant);
    else
        return false;
}

/*!
    Returns \c true if the property is final; otherwise returns \c false.
    The default value is false.
*/
bool QMetaPropertyBuilder::isFinal() const
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        return d->flag(Final);
    else
        return false;
}

/*!
 * Returns \c true if the property is an alias.
 * The default value is false
 */
bool QMetaPropertyBuilder::isAlias() const
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        return d->flag(Alias);
    else
        return false;
}

/*!
  Returns \c true if the property is bindable
  The default value is false
 */
bool QMetaPropertyBuilder::isBindable() const
{
    if (auto d = d_func())
        return d->flag(Bindable);
    else
        return false;
}

/*!
    Sets this property to readable if \a value is true.

    \sa isReadable(), setWritable()
*/
void QMetaPropertyBuilder::setReadable(bool value)
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        d->setFlag(Readable, value);
}

/*!
    Sets this property to writable if \a value is true.

    \sa isWritable(), setReadable()
*/
void QMetaPropertyBuilder::setWritable(bool value)
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        d->setFlag(Writable, value);
}

/*!
    Sets this property to resettable if \a value is true.

    \sa isResettable()
*/
void QMetaPropertyBuilder::setResettable(bool value)
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        d->setFlag(Resettable, value);
}

/*!
    Sets this property to designable if \a value is true.

    \sa isDesignable(), setScriptable(), setStored()
*/
void QMetaPropertyBuilder::setDesignable(bool value)
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        d->setFlag(Designable, value);
}

/*!
    Sets this property to scriptable if \a value is true.

    \sa isScriptable(), setDesignable(), setStored()
*/
void QMetaPropertyBuilder::setScriptable(bool value)
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        d->setFlag(Scriptable, value);
}

/*!
    Sets this property to storable if \a value is true.

    \sa isStored(), setDesignable(), setScriptable()
*/
void QMetaPropertyBuilder::setStored(bool value)
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        d->setFlag(Stored, value);
}

/*!
    Sets the \c USER flag on this property to \a value.

    \sa isUser(), setDesignable(), setScriptable()
*/
void QMetaPropertyBuilder::setUser(bool value)
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        d->setFlag(User, value);
}

/*!
    Sets the C++ setter flag on this property to \a value, which is
    true if the property has a C++ setter function that follows Qt's
    standard "name" / "setName" pattern.

    \sa hasStdCppSet()
*/
void QMetaPropertyBuilder::setStdCppSet(bool value)
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        d->setFlag(StdCppSet, value);
}

/*!
    Sets this property to be of an enumerator or flag type if
    \a value is true.

    \sa isEnumOrFlag()
*/
void QMetaPropertyBuilder::setEnumOrFlag(bool value)
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        d->setFlag(EnumOrFlag, value);
}

/*!
    Sets the \c CONSTANT flag on this property to \a value.

    \sa isConstant()
*/
void QMetaPropertyBuilder::setConstant(bool value)
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        d->setFlag(Constant, value);
}

/*!
    Sets the \c FINAL flag on this property to \a value.

    \sa isFinal()
*/
void QMetaPropertyBuilder::setFinal(bool value)
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        d->setFlag(Final, value);
}

/*!
   Sets the \c ALIAS flag on this property to \a value
 */
void QMetaPropertyBuilder::setAlias(bool value)
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        d->setFlag(Alias, value);
}

/*!
   Sets the\c BINDABLE flag on this property to \a value
 */
void QMetaPropertyBuilder::setBindable(bool value)
{
    if (auto d = d_func())
        d->setFlag(Bindable, value);
}

/*!
    Returns the revision of this property.

    \sa setRevision()
*/
int QMetaPropertyBuilder::revision() const
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        return d->revision;
    return 0;
}

/*!
    Sets the \a revision of this property.

    \sa revision()
*/
void QMetaPropertyBuilder::setRevision(int revision)
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        d->revision = revision;
}

/*!
    \class QMetaEnumBuilder
    \inmodule QtCore
    \internal
    \brief The QMetaEnumBuilder class enables modifications to an enumerator definition on a meta object builder.
*/

QMetaEnumBuilderPrivate *QMetaEnumBuilder::d_func() const
{
    if (_mobj && _index >= 0 && _index < int(_mobj->d->enumerators.size()))
        return &(_mobj->d->enumerators[_index]);
    else
        return nullptr;
}

/*!
    \fn QMetaEnumBuilder::QMetaEnumBuilder()
    \internal
*/

/*!
    \fn int QMetaEnumBuilder::index() const

    Returns the index of this enumerator within its QMetaObjectBuilder.
*/

/*!
    Returns the type name of the enumerator (without the scope).
*/
QByteArray QMetaEnumBuilder::name() const
{
    QMetaEnumBuilderPrivate *d = d_func();
    if (d)
        return d->name;
    else
        return QByteArray();
}

/*!
    Returns the enum name of the enumerator (without the scope).

    \since 5.12
*/
QByteArray QMetaEnumBuilder::enumName() const
{
    QMetaEnumBuilderPrivate *d = d_func();
    if (d)
        return d->enumName;
    else
        return QByteArray();
}

/*!
    Sets this enumerator to have the enum name \c alias.

    \since 5.12
    \sa isFlag(), enumName()
*/
void QMetaEnumBuilder::setEnumName(const QByteArray &alias)
{
    QMetaEnumBuilderPrivate *d = d_func();
    if (d)
        d->enumName = alias;
}

/*!
    Returns the meta type of the enumerator.

    \since 6.6
*/
QMetaType QMetaEnumBuilder::metaType() const
{
    if (QMetaEnumBuilderPrivate *d = d_func())
        return d->metaType;
    return QMetaType();
}

/*!
    Sets this enumerator to have the given \c metaType.

    \since 6.6
    \sa metaType()
*/
void QMetaEnumBuilder::setMetaType(QMetaType metaType)
{
    QMetaEnumBuilderPrivate *d = d_func();
    if (d)
        d->metaType = metaType;
}

/*!
    Returns \c true if this enumerator is used as a flag; otherwise returns
    false.

    \sa setIsFlag()
*/
bool QMetaEnumBuilder::isFlag() const
{
    QMetaEnumBuilderPrivate *d = d_func();
    if (d)
        return d->isFlag;
    else
        return false;
}

/*!
    Sets this enumerator to be used as a flag if \a value is true.

    \sa isFlag()
*/
void QMetaEnumBuilder::setIsFlag(bool value)
{
    QMetaEnumBuilderPrivate *d = d_func();
    if (d)
        d->isFlag = value;
}

/*!
    Return \c true if this enumerator should be considered scoped (C++11 enum class).

    \sa setIsScoped()
*/
bool QMetaEnumBuilder::isScoped() const
{
    QMetaEnumBuilderPrivate *d = d_func();
    if (d)
        return d->isScoped;
    return false;
}

/*!
    Sets this enumerator to be a scoped enum if \value is true

    \sa isScoped()
*/
void QMetaEnumBuilder::setIsScoped(bool value)
{
    QMetaEnumBuilderPrivate *d = d_func();
    if (d)
        d->isScoped = value;
}

/*!
    Returns the number of keys.

    \sa key(), addKey()
*/
int QMetaEnumBuilder::keyCount() const
{
    QMetaEnumBuilderPrivate *d = d_func();
    if (d)
        return d->keys.size();
    else
        return 0;
}

/*!
    Returns the key with the given \a index, or an empty QByteArray
    if no such key exists.

    \sa keyCount(), addKey(), value()
*/
QByteArray QMetaEnumBuilder::key(int index) const
{
    QMetaEnumBuilderPrivate *d = d_func();
    if (d && index >= 0 && index < d->keys.size())
        return d->keys[index];
    else
        return QByteArray();
}

/*!
    Returns the value with the given \a index; or returns -1 if there
    is no such value.

    \sa keyCount(), addKey(), key()
*/
int QMetaEnumBuilder::value(int index) const
{
    QMetaEnumBuilderPrivate *d = d_func();
    if (d && index >= 0 && index < d->keys.size())
        return d->values[index];
    else
        return -1;
}

/*!
    Adds a new key called \a name to this enumerator, associated
    with \a value.  Returns the index of the new key.

    \sa keyCount(), key(), value(), removeKey()
*/
int QMetaEnumBuilder::addKey(const QByteArray &name, int value)
{
    QMetaEnumBuilderPrivate *d = d_func();
    if (d) {
        int index = d->keys.size();
        d->keys += name;
        d->values += value;
        return index;
    } else {
        return -1;
    }
}

/*!
    Removes the key at \a index from this enumerator.

    \sa addKey()
*/
void QMetaEnumBuilder::removeKey(int index)
{
    QMetaEnumBuilderPrivate *d = d_func();
    if (d && index >= 0 && index < d->keys.size()) {
        d->keys.removeAt(index);
        d->values.removeAt(index);
    }
}

QT_END_NAMESPACE
