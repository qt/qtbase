/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "qproperty.h"
#include "qproperty_p.h"

#include <qscopedvaluerollback.h>

QT_BEGIN_NAMESPACE

using namespace QtPrivate;

void QPropertyBindingDataPointer::addObserver(QPropertyObserver *observer)
{
    if (auto *binding = bindingPtr()) {
        observer->prev = &binding->firstObserver.ptr;
        observer->next = binding->firstObserver.ptr;
        if (observer->next)
            observer->next->prev = &observer->next;
        binding->firstObserver.ptr = observer;
    } else {
        auto firstObserver = reinterpret_cast<QPropertyObserver*>(ptr->d_ptr & ~QPropertyBindingData::FlagMask);
        observer->prev = reinterpret_cast<QPropertyObserver**>(&ptr->d_ptr);
        observer->next = firstObserver;
        if (observer->next)
            observer->next->prev = &observer->next;
    }
    setFirstObserver(observer);
}

QPropertyBindingPrivate::~QPropertyBindingPrivate()
{
    if (firstObserver)
        firstObserver.unlink();
}

void QPropertyBindingPrivate::unlinkAndDeref()
{
    propertyDataPtr = nullptr;
    if (!ref.deref())
        delete this;
}

void QPropertyBindingPrivate::markDirtyAndNotifyObservers()
{
    if (dirty)
        return;
    dirty = true;
    if (firstObserver)
        firstObserver.notify(this, propertyDataPtr);
    if (hasStaticObserver)
        staticObserverCallback(propertyDataPtr);
}

bool QPropertyBindingPrivate::evaluateIfDirtyAndReturnTrueIfValueChanged()
{
    if (!dirty)
        return false;

    if (updating) {
        error = QPropertyBindingError(QPropertyBindingError::BindingLoop);
        return false;
    }

    /*
     * Evaluating the binding might lead to the binding being broken. This can
     * cause ref to reach zero at the end of the function.  However, the
     * updateGuard's destructor will then still trigger, trying to set the
     * updating bool to its old value
     * To prevent this, we create a QPropertyBindingPrivatePtr which ensures
     * that the object is still alive when updateGuard's dtor runs.
     */
    QPropertyBindingPrivatePtr keepAlive {this};
    QScopedValueRollback<bool> updateGuard(updating, true);

    QBindingEvaluationState evaluationFrame(this);

    bool changed = false;

    if (hasBindingWrapper) {
        changed = staticBindingWrapper(metaType, propertyDataPtr, evaluationFunction);
    } else {
        changed = evaluationFunction(metaType, propertyDataPtr);
    }

    dirty = false;
    return changed;
}

QUntypedPropertyBinding::QUntypedPropertyBinding() = default;

QUntypedPropertyBinding::QUntypedPropertyBinding(QMetaType metaType, QUntypedPropertyBinding::BindingEvaluationFunction function,
                                                 const QPropertyBindingSourceLocation &location)
{
    d = new QPropertyBindingPrivate(metaType, std::move(function), std::move(location));
}

QUntypedPropertyBinding::QUntypedPropertyBinding(QUntypedPropertyBinding &&other)
    : d(std::move(other.d))
{
}

QUntypedPropertyBinding::QUntypedPropertyBinding(const QUntypedPropertyBinding &other)
    : d(other.d)
{
}

QUntypedPropertyBinding &QUntypedPropertyBinding::operator=(const QUntypedPropertyBinding &other)
{
    d = other.d;
    return *this;
}

QUntypedPropertyBinding &QUntypedPropertyBinding::operator=(QUntypedPropertyBinding &&other)
{
    d = std::move(other.d);
    return *this;
}

QUntypedPropertyBinding::QUntypedPropertyBinding(QPropertyBindingPrivate *priv)
    : d(priv)
{
}

QUntypedPropertyBinding::~QUntypedPropertyBinding()
{
}

bool QUntypedPropertyBinding::isNull() const
{
    return !d;
}

QPropertyBindingError QUntypedPropertyBinding::error() const
{
    if (!d)
        return QPropertyBindingError();
    return d->bindingError();
}

QMetaType QUntypedPropertyBinding::valueMetaType() const
{
    if (!d)
        return QMetaType();
    return d->valueMetaType();
}

QPropertyBindingData::QPropertyBindingData(QPropertyBindingData &&other, QUntypedPropertyData *propertyDataPtr)
{
    std::swap(d_ptr, other.d_ptr);
    QPropertyBindingDataPointer d{this};
    d.setFirstObserver(nullptr);
    if (auto binding = d.bindingPtr())
        binding->setProperty(propertyDataPtr);
}

void QPropertyBindingData::moveAssign(QPropertyBindingData &&other, QUntypedPropertyData *propertyDataPtr)
{
    if (&other == this)
        return;

    QPropertyBindingDataPointer d{this};
    auto observer = d.firstObserver();
    d.setFirstObserver(nullptr);

    if (auto binding = d.bindingPtr()) {
        binding->unlinkAndDeref();
        d_ptr &= FlagMask;
    }

    std::swap(d_ptr, other.d_ptr);

    if (auto binding = d.bindingPtr())
        binding->setProperty(propertyDataPtr);

    d.setFirstObserver(observer.ptr);

    // The caller will have to notify observers.
}

QPropertyBindingData::~QPropertyBindingData()
{
    QPropertyBindingDataPointer d{this};
    for (auto observer = d.firstObserver(); observer;) {
        auto next = observer.nextObserver();
        observer.unlink();
        observer = next;
    }
    if (auto binding = d.bindingPtr())
        binding->unlinkAndDeref();
}

QUntypedPropertyBinding QPropertyBindingData::setBinding(const QUntypedPropertyBinding &binding,
                                                  QUntypedPropertyData *propertyDataPtr,
                                                  QPropertyObserverCallback staticObserverCallback,
                                                  QtPrivate::QPropertyBindingWrapper guardCallback)
{
    QPropertyBindingPrivatePtr oldBinding;
    QPropertyBindingPrivatePtr newBinding = binding.d;

    QPropertyBindingDataPointer d{this};
    QPropertyObserverPointer observer;

    if (auto *existingBinding = d.bindingPtr()) {
        if (existingBinding == newBinding.data())
            return QUntypedPropertyBinding(oldBinding.data());
        oldBinding = QPropertyBindingPrivatePtr(existingBinding);
        observer = oldBinding->takeObservers();
        oldBinding->unlinkAndDeref();
        d_ptr &= FlagMask;
    } else {
        observer = d.firstObserver();
    }

    if (newBinding) {
        newBinding.data()->ref.ref();
        d_ptr = (d_ptr & FlagMask) | reinterpret_cast<quintptr>(newBinding.data());
        d_ptr |= BindingBit;
        newBinding->setDirty(true);
        newBinding->setProperty(propertyDataPtr);
        if (observer)
            newBinding->prependObserver(observer);
        newBinding->setStaticObserver(staticObserverCallback, guardCallback);
    } else if (observer) {
        d.setObservers(observer.ptr);
    } else {
        d_ptr &= ~QPropertyBindingData::BindingBit;
    }

    if (oldBinding)
        oldBinding->detachFromProperty();

    return QUntypedPropertyBinding(oldBinding.data());
}

QPropertyBindingPrivate *QPropertyBindingData::binding() const
{
    QPropertyBindingDataPointer d{this};
    if (auto binding = d.bindingPtr())
        return binding;
    return nullptr;
}

static thread_local QBindingEvaluationState *currentBindingEvaluationState = nullptr;

QBindingEvaluationState::QBindingEvaluationState(QPropertyBindingPrivate *binding)
    : binding(binding)
{
    // store a pointer to the currentBindingEvaluationState to avoid a TLS lookup in
    // the destructor (as these come with a non zero cost)
    currentState = &currentBindingEvaluationState;
    previousState = *currentState;
    *currentState = this;
    binding->clearDependencyObservers();
}

QBindingEvaluationState::~QBindingEvaluationState()
{
    *currentState = previousState;
}

QPropertyBindingPrivate *QPropertyBindingPrivate::currentlyEvaluatingBinding()
{
    return currentBindingEvaluationState ? currentBindingEvaluationState->binding : nullptr;
}

void QPropertyBindingData::evaluateIfDirty() const
{
    QPropertyBindingDataPointer d{this};
    QPropertyBindingPrivate *binding = d.bindingPtr();
    if (!binding)
        return;
    binding->evaluateIfDirtyAndReturnTrueIfValueChanged();
}

void QPropertyBindingData::removeBinding()
{
    QPropertyBindingDataPointer d{this};

    if (auto *existingBinding = d.bindingPtr()) {
        auto observer = existingBinding->takeObservers();
        d_ptr &= ExtraBit;
        if (observer)
            d.setObservers(observer.ptr);
        existingBinding->unlinkAndDeref();
    }
}

void QPropertyBindingData::registerWithCurrentlyEvaluatingBinding() const
{
    auto currentState = currentBindingEvaluationState;
    if (!currentState)
        return;

    QPropertyBindingDataPointer d{this};

    QPropertyObserverPointer dependencyObserver = currentState->binding->allocateDependencyObserver();
    dependencyObserver.setBindingToMarkDirty(currentState->binding);
    dependencyObserver.observeProperty(d);
}

void QPropertyBindingData::notifyObservers(QUntypedPropertyData *propertyDataPtr) const
{
    QPropertyBindingDataPointer d{this};
    if (QPropertyObserverPointer observer = d.firstObserver())
        observer.notify(d.bindingPtr(), propertyDataPtr);
}

int QPropertyBindingDataPointer::observerCount() const
{
    int count = 0;
    for (auto observer = firstObserver(); observer; observer = observer.nextObserver())
        ++count;
    return count;
}

QPropertyObserver::QPropertyObserver(ChangeHandler changeHandler)
{
    QPropertyObserverPointer d{this};
    d.setChangeHandler(changeHandler);
}

QPropertyObserver::QPropertyObserver(QUntypedPropertyData *aliasedPropertyPtr)
{
    QPropertyObserverPointer d{this};
    d.setAliasedProperty(aliasedPropertyPtr);
}

/*! \internal
 */
void QPropertyObserver::setSource(const QPropertyBindingData &property)
{
    QPropertyObserverPointer d{this};
    QPropertyBindingDataPointer propPrivate{&property};
    d.observeProperty(propPrivate);
}


QPropertyObserver::~QPropertyObserver()
{
    QPropertyObserverPointer d{this};
    d.unlink();
}

QPropertyObserver::QPropertyObserver(QPropertyObserver &&other)
{
    std::swap(bindingToMarkDirty, other.bindingToMarkDirty);
    std::swap(next, other.next);
    std::swap(prev, other.prev);
    if (next)
        next->prev = &next;
    if (prev)
        prev.setPointer(this);
}

QPropertyObserver &QPropertyObserver::operator=(QPropertyObserver &&other)
{
    if (this == &other)
        return *this;

    QPropertyObserverPointer d{this};
    d.unlink();
    bindingToMarkDirty = nullptr;

    std::swap(bindingToMarkDirty, other.bindingToMarkDirty);
    std::swap(next, other.next);
    std::swap(prev, other.prev);
    if (next)
        next->prev = &next;
    if (prev)
        prev.setPointer(this);

    return *this;
}

void QPropertyObserverPointer::unlink()
{
    if (ptr->next.tag() & QPropertyObserver::ObserverNotifiesAlias)
        ptr->aliasedPropertyData = nullptr;
    if (ptr->next)
        ptr->next->prev = ptr->prev;
    if (ptr->prev)
        ptr->prev.setPointer(ptr->next.data());
    ptr->next = nullptr;
    ptr->prev.clear();
}

void QPropertyObserverPointer::setChangeHandler(QPropertyObserver::ChangeHandler changeHandler)
{
    ptr->changeHandler = changeHandler;
    ptr->next.setTag(QPropertyObserver::ObserverNotifiesChangeHandler);
}

void QPropertyObserverPointer::setAliasedProperty(QUntypedPropertyData *property)
{
    ptr->aliasedPropertyData = property;
    ptr->next.setTag(QPropertyObserver::ObserverNotifiesAlias);
}

void QPropertyObserverPointer::setBindingToMarkDirty(QPropertyBindingPrivate *binding)
{
    ptr->bindingToMarkDirty = binding;
    ptr->next.setTag(QPropertyObserver::ObserverNotifiesBinding);
}

void QPropertyObserverPointer::notify(QPropertyBindingPrivate *triggeringBinding, QUntypedPropertyData *propertyDataPtr)
{
    bool knownIfPropertyChanged = false;
    bool propertyChanged = true;

    auto observer = const_cast<QPropertyObserver*>(ptr);
    while (observer) {
        auto * const next = observer->next.data();
        switch (observer->next.tag()) {
        case QPropertyObserver::ObserverNotifiesChangeHandler:
            if (!knownIfPropertyChanged && triggeringBinding) {
                knownIfPropertyChanged = true;

                propertyChanged = triggeringBinding->evaluateIfDirtyAndReturnTrueIfValueChanged();
            }
            if (!propertyChanged)
                return;

            if (auto handlerToCall = std::exchange(observer->changeHandler, nullptr)) {
                handlerToCall(observer, propertyDataPtr);
                observer->changeHandler = handlerToCall;
            }
            break;
        case QPropertyObserver::ObserverNotifiesBinding:
            if (observer->bindingToMarkDirty)
                observer->bindingToMarkDirty->markDirtyAndNotifyObservers();
            break;
        case QPropertyObserver::ObserverNotifiesAlias:
            break;
        }
        observer = next;
    }
}

void QPropertyObserverPointer::observeProperty(QPropertyBindingDataPointer property)
{
    if (ptr->prev)
        unlink();
    property.addObserver(ptr);
}

QPropertyBindingError::QPropertyBindingError()
{
}

QPropertyBindingError::QPropertyBindingError(Type type, const QString &description)
{
    if (type != NoError) {
        d = new QPropertyBindingErrorPrivate;
        d->type = type;
        d->description = description;
    }
}

QPropertyBindingError::QPropertyBindingError(const QPropertyBindingError &other)
    : d(other.d)
{
}

QPropertyBindingError &QPropertyBindingError::operator=(const QPropertyBindingError &other)
{
    d = other.d;
    return *this;
}

QPropertyBindingError::QPropertyBindingError(QPropertyBindingError &&other)
    : d(std::move(other.d))
{
}

QPropertyBindingError &QPropertyBindingError::operator=(QPropertyBindingError &&other)
{
    d = std::move(other.d);
    return *this;
}

QPropertyBindingError::~QPropertyBindingError()
{
}

QPropertyBindingError::Type QPropertyBindingError::type() const
{
    if (!d)
        return QPropertyBindingError::NoError;
    return d->type;
}

QString QPropertyBindingError::description() const
{
    if (!d)
        return QString();
    return d->description;
}

/*!
  \class QPropertyData
  \inmodule QtCore
  \brief The QPropertyData class is a helper class for properties with automatic property bindings.
  \since 6.0

  \ingroup tools

  QPropertyData\<T\> is a common base class for classes that can hold properties with automatic
  data bindings. It mainly wraps the stored data, and offers low level access to that data.

  The low level access to the data provided by this class bypasses the binding mechanism, and should be
  used with care, as updates to the values will not get propagated to any bindings that depend on this
  property.

  You should usually call value() and setValue() on QProperty<T> or QBindablePropertyData<T>, not use
  the low level mechanisms provided in this class.
*/

/*! \fn QPropertyData<T>::parameter_type QPropertyData<T>::valueBypassingBindings() const

    \returns the data stored in this property.

    \note As this will bypass any binding evaluation it might return an outdated value if a
    binding is set on this property. Using this method will also not register the property
    access with any currently executing binding.
*/

/*! \fn void QPropertyData<T>::setValueBypassingBindings(parameter_type v)

    Sets the data value stored in this property to \a v.

    \note Using this method will bypass any potential binding registered for this property.
*/

/*! \fn void QPropertyData<T>::setValueBypassingBindings(rvalue_ref v)
    \overload

    Sets the data value stored in this property to \a v.

    \note Using this method will bypass any potential binding registered for this property.
*/


/*!
  \class QProperty
  \inmodule QtCore
  \brief The QProperty class is a template class that enables automatic property bindings.
  \since 6.0

  \ingroup tools

  QProperty\<T\> is a generic container that holds an instance of T. You can assign
  a value to it and you can read it via the value() function or the T conversion
  operator. You can also tie the property to an expression that computes the value
  dynamically, the binding expression. It is represented as a C++ lambda and
  can be used to express relationships between different properties in your
  application.

  The binding expression computes the value by reading other QProperty values.
  Behind the scenes this dependency is tracked. Whenever a change in any property's
  dependency is detected, the binding expression is re-evaluated and the new
  result is applied to the property. This happens lazily, by marking the binding
  as dirty and evaluating it only when the property's value is requested. For example:

  \code
    QProperty<QString> firstname("John");
    QProperty<QString> lastname("Smith");
    QProperty<int> age(41);

    QProperty<QString> fullname;
    fullname.setBinding([&]() { return firstname.value() + " " + lastname.value() + " age:" + QString::number(age.value()); });

    qDebug() << fullname.value(); // Prints "John Smith age: 41"

    firstname = "Emma"; // Marks binding expression as dirty

    qDebug() << fullname.value(); // Re-evaluates the binding expression and prints "Emma Smith age: 41"

    // Birthday is coming up
    age.setValue(age.value() + 1);

    qDebug() << fullname.value(); // Re-evaluates the binding expression and prints "Emma Smith age: 42"
  \endcode

  When a new value is assigned to the \c firstname property, the binding
  expression for \c fullname is marked as dirty. So when the last \c qDebug() statement
  tries to read the name value of the \c fullname property, the expression is
  evaluated again, \c firstname() will be called again and return the new value.

  Since bindings are C++ lambda expressions, they may do anything that's possible
  in C++. This includes calling other functions. If those functions access values
  held by QProperty, they automatically become dependencies to the binding.

  Binding expressions may use properties of any type, so in the above example the age
  is an integer and folded into the string value using conversion to integer, but
  the dependency is fully tracked.

  \section1 Tracking properties

  Sometimes the relationships between properties cannot be expressed using
  bindings. Instead you may need to run custom code whenever the value of a property
  changes and instead of assigning the value to another property, pass it to
  other parts of your application. For example writing data into a network socket
  or printing debug output. QProperty provides two mechanisms for tracking.

  You can register for a callback function to be called whenever the value of
  a property changes, by using onValueChanged(). If you want the callback to also
  be called for the current value of the property, register your callback using
  subscribe() instead.
*/

/*!
  \fn template <typename T> QProperty<T>::QProperty()

  Constructs a property with a default constructed instance of T.
*/

/*!
  \fn template <typename T> explicit QProperty<T>::QProperty(const T &initialValue)

  Constructs a property with the provided \a initialValue.
*/

/*!
  \fn template <typename T> explicit QProperty<T>::QProperty(T &&initialValue)

  Move-Constructs a property with the provided \a initialValue.
*/

/*!
  \fn template <typename T> QProperty<T>::QProperty(QProperty<T> &&other)

  Move-constructs a QProperty instance, making it point at the same object that
  \a other was pointing to.
*/

/*!
  \fn template <typename T> QProperty<T> &QProperty<T>::operator=(QProperty &&other)

  Move-assigns \a other to this QProperty instance.
*/

/*!
  \fn template <typename T> QProperty<T>::QProperty(const QPropertyBinding<T> &binding)

  Constructs a property that is tied to the provided \a binding expression. The
  first time the property value is read, the binding is evaluated. Whenever a
  dependency of the binding changes, the binding will be re-evaluated the next
  time the value of this property is read.
*/

/*!
  \fn template <typename T> template <typename Functor> QProperty<T>::QProperty(Functor &&f)

  Constructs a property that is tied to the provided binding expression \a f. The
  first time the property value is read, the binding is evaluated. Whenever a
  dependency of the binding changes, the binding will be re-evaluated the next
  time the value of this property is read.
*/

/*!
  \fn template <typename T> QProperty<T>::~QProperty()

  Destroys the property.
*/

/*!
  \fn template <typename T> T QProperty<T>::value() const

  Returns the value of the property. This may evaluate a binding expression that
  is tied to this property, before returning the value.
*/

/*!
  \fn template <typename T> QProperty<T>::operator T() const

  Returns the value of the property. This may evaluate a binding expression that
  is tied to this property, before returning the value.
*/

/*!
  \fn template <typename T> void QProperty<T>::setValue(const T &newValue)

  Assigns \a newValue to this property and removes the property's associated
  binding, if present.
*/

/*!
  \fn template <typename T> void QProperty<T>::setValue(T &&newValue)
  \overload

  Assigns \a newValue to this property and removes the property's associated
  binding, if present.
*/

/*!
  \fn template <typename T> QProperty<T> &QProperty<T>::operator=(const T &newValue)

  Assigns \a newValue to this property and returns a reference to this QProperty.
*/

/*!
  \fn template <typename T> QProperty<T> &QProperty<T>::operator=(T &&newValue)
  \overload

  Assigns \a newValue to this property and returns a reference to this QProperty.
*/

/*!
  \fn template <typename T> QProperty<T> &QProperty<T>::operator=(const QPropertyBinding<T> &newBinding)

  Associates the value of this property with the provided \a newBinding
  expression and returns a reference to this property. The first time the
  property value is read, the binding is evaluated. Whenever a dependency of the
  binding changes, the binding will be re-evaluated the next time the value of
  this property is read.
*/

/*!
  \fn template <typename T> QPropertyBinding<T> QProperty<T>::setBinding(const QPropertyBinding<T> &newBinding)

  Associates the value of this property with the provided \a newBinding
  expression and returns the previously associated binding. The first time the
  property value is read, the binding is evaluated. Whenever a dependency of the
  binding changes, the binding will be re-evaluated the next time the value of
  this property is read.
*/

/*!
  \fn template <typename T> template <typename Functor> QPropertyBinding<T> QProperty<T>::setBinding(Functor f)
  \overload

  Associates the value of this property with the provided functor \a f and
  returns the previously associated binding. The first time the property value
  is read, the binding is evaluated by invoking the call operator () of \a f.
  Whenever a dependency of the binding changes, the binding will be re-evaluated
  the next time the value of this property is read.
*/

/*!
  \fn template <typename T> QPropertyBinding<T> QProperty<T>::setBinding(QPropertyBinding<T> &&newBinding)
  \overload

  Associates the value of this property with the provided \a newBinding
  expression and returns the previously associated binding. The first time the
  property value is read, the binding is evaluated. Whenever a dependency of the
  binding changes, the binding will be re-evaluated the next time the value of
  this property is read.
*/

/*!
  \fn template <typename T> QPropertyBinding<T> bool QProperty<T>::setBinding(const QUntypedPropertyBinding &newBinding)
  \overload

  Associates the value of this property with the provided \a newBinding
  expression. The first time the property value is read, the binding is evaluated.
  Whenever a dependency of the binding changes, the binding will be re-evaluated
  the next time the value of this property is read.

  Returns true if the type of this property is the same as the type the binding
  function returns; false otherwise.
*/

/*!
  \fn template <typename T> QPropertyBinding<T> QProperty<T>::binding() const

  Returns the binding expression that is associated with this property. A
  default constructed QPropertyBinding<T> will be returned if no such
  association exists.
*/

/*!
  \fn template <typename T> QPropertyBinding<T> QProperty<T>::takeBinding()

  Disassociates the binding expression from this property and returns it. After
  calling this function, the value of the property will only change if you
  assign a new value to it, or when a new binding is set.
*/

/*!
  \fn template <typename T> template <typename Functor> QPropertyChangeHandler<T, Functor> QProperty<T>::onValueChanged(Functor f)

  Registers the given functor \a f as a callback that shall be called whenever
  the value of the property changes.

  The callback \a f is expected to be a type that has a plain call operator () without any
  parameters. This means that you can provide a C++ lambda expression, an std::function
  or even a custom struct with a call operator.

  The returned property change handler object keeps track of the registration. When it
  goes out of scope, the callback is de-registered.
*/

/*!
  \fn template <typename T> template <typename Functor> QPropertyChangeHandler<T, Functor> QProperty<T>::subscribe(Functor f)

  Subscribes the given functor \a f as a callback that is called immediately and whenever
  the value of the property changes in the future.

  The callback \a f is expected to be a type that has a plain call operator () without any
  parameters. This means that you can provide a C++ lambda expression, an std::function
  or even a custom struct with a call operator.

  The returned property change handler object keeps track of the subscription. When it
  goes out of scope, the callback is unsubscribed.
*/

/*!
  \fn template <typename T> QtPrivate::QPropertyBindingData &QProperty<T>::bindingData() const
  \internal
*/


/*!
  \class QPropertyChangeHandler
  \inmodule QtCore
  \brief The QPropertyChangeHandler class controls the lifecycle of change callback installed on a QProperty.

  \ingroup tools

  QPropertyChangeHandler\<PropertyType, Functor\> is created when registering a
  callback on a QProperty to listen to changes to the property's value, using QProperty::onValueChanged
  and QProperty::subscribe. As long as the change handler is alive, the callback remains installed.

  A handler instance can be transferred between C++ scopes using move semantics.
*/

/*!
  \class QPropertyAlias
  \inmodule QtCore
  \brief The QPropertyAlias class is a safe alias for a QProperty with same template parameter.

  \ingroup tools

  QPropertyAlias\<T\> wraps a pointer to a QProperty\<T\> and automatically
  invalidates itself when the QProperty\<T\> is destroyed. It forwards all
  method invocations to the wrapped property. For example:

  \code
    QProperty<QString> *name = new QProperty<QString>("John");
    QProperty<int> age(41);

    QPropertyAlias<QString> nameAlias(name);
    QPropertyAlias<int> ageAlias(&age);

    QPropertyAlias<QString> fullname;
    fullname.setBinding([&]() { return nameAlias.value() + " age:" + QString::number(ageAlias.value()); });

    qDebug() << fullname.value(); // Prints "Smith age: 41"

    *name = "Emma"; // Marks binding expression as dirty

    qDebug() << fullname.value(); // Re-evaluates the binding expression and prints "Emma age: 41"

    // Birthday is coming up
    ageAlias.setValue(age.value() + 1); // Writes the age property through the alias

    qDebug() << fullname.value(); // Re-evaluates the binding expression and prints "Emma age: 42"

    delete name; // Leaves the alias in an invalid, but accessible state
    nameAlias.setValue("Eve"); // Ignored: nameAlias carries a default-constructed QString now

    ageAlias.setValue(92);
    qDebug() << fullname.value(); // Re-evaluates the binding expression and prints " age: 92"
  \endcode
*/

/*!
  \fn template <typename T> QPropertyAlias<T>::QPropertyAlias(QProperty<T> *property)

  Constructs a property alias for the given \a property.
*/

/*!
  \fn template <typename T> explicit QPropertyAlias<T>::QPropertyAlias(QPropertyAlias<T> *alias)

  Constructs a property alias for the property aliased by \a alias.
*/

/*!
  \fn template <typename T> T QPropertyAlias<T>::value() const

  Returns the value of the aliased property. This may evaluate a binding
  expression that is tied to the property, before returning the value.
*/

/*!
  \fn template <typename T> QPropertyAlias<T>::operator T() const

  Returns the value of the aliased property. This may evaluate a binding
  expression that is tied to the property, before returning the value.
*/

/*!
  \fn template <typename T> void QPropertyAlias<T>::setValue(const T &newValue)

  Assigns \a newValue to the aliased property and removes the property's
  associated binding, if present.
*/

/*!
  \fn template <typename T> void QPropertyAlias<T>::setValue(T &&newValue)
  \overload

  Assigns \a newValue to the aliased property and removes the property's
  associated binding, if present.
*/

/*!
  \fn template <typename T> QPropertyAlias<T> &QPropertyAlias<T>::operator=(const T &newValue)

  Assigns \a newValue to the aliased property and returns a reference to this
  QPropertyAlias.
*/

/*!
  \fn template <typename T> QPropertyAlias<T> &QPropertyAlias<T>::operator=(T &&newValue)
  \overload

  Assigns \a newValue to the aliased property and returns a reference to this
  QPropertyAlias.
*/

/*!
  \fn template <typename T> QPropertyAlias<T> &QPropertyAlias<T>::operator=(const QPropertyBinding<T> &newBinding)
  \overload

  Associates the value of the aliased property with the provided \a newBinding
  expression and returns a reference to this alias. The first time the
  property value is read, either from the property itself or from any alias, the
  binding is evaluated. Whenever a dependency of the binding changes, the
  binding will be re-evaluated the next time the value of this property is read.
*/

/*!
  \fn template <typename T> QPropertyBinding<T> QPropertyAlias<T>::setBinding(const QPropertyBinding<T> &newBinding)

  Associates the value of the aliased property with the provided \a newBinding
  expression and returns any previous binding the associated with the aliased
  property. The first time the property value is read, either from the property
  itself or from any alias, the binding is evaluated. Whenever a dependency of
  the binding changes, the binding will be re-evaluated the next time the value
  of this property is read.

  Returns any previous binding associated with the property, or a
  default-constructed QPropertyBinding<T>.
*/

/*!
  \fn template <typename T> QPropertyBinding<T> QPropertyAlias<T>::setBinding(QPropertyBinding<T> &&newBinding)
  \overload

  Associates the value of the aliased property with the provided \a newBinding
  expression and returns any previous binding the associated with the aliased
  property. The first time the property value is read, either from the property
  itself or from any alias, the binding is evaluated. Whenever a dependency of
  the binding changes, the binding will be re-evaluated the next time the value
  of this property is read.

  Returns any previous binding associated with the property, or a
  default-constructed QPropertyBinding<T>.
*/

/*!
  \fn template <typename T> QPropertyBinding<T> bool QPropertyAlias<T>::setBinding(const QUntypedPropertyBinding &newBinding)
  \overload

  Associates the value of the aliased property with the provided \a newBinding
  expression. The first time the property value is read, either from the
  property itself or from any alias, the binding is evaluated. Whenever a
  dependency of the binding changes, the binding will be re-evaluated the next
  time the value of this property is read.

  Returns true if the type of this property is the same as the type the binding
  function returns; false otherwise.
*/

/*!
  \fn template <typename T> template <typename Functor> QPropertyBinding<T> setBinding(Functor f)
  \overload

  Associates the value of the aliased property with the provided functor \a f
  expression. The first time the property value is read, either from the
  property itself or from any alias, the binding is evaluated. Whenever a
  dependency of the binding changes, the binding will be re-evaluated the next
  time the value of this property is read.

  Returns any previous binding associated with the property, or a
  default-constructed QPropertyBinding<T>.
*/

/*!
  \fn template <typename T> bool QPropertyAlias<T>::hasBinding() const

  Returns true if the aliased property is associated with a binding; false
  otherwise.
*/

/*!
  \fn template <typename T> QPropertyBinding<T> QPropertyAlias<T>::binding() const

  Returns the binding expression that is associated with the aliased property. A
  default constructed QPropertyBinding<T> will be returned if no such
  association exists.
*/

/*!
  \fn template <typename T> QPropertyBinding<T> QPropertyAlias<T>::takeBinding()

  Disassociates the binding expression from the aliased property and returns it.
  After calling this function, the value of the property will only change if
  you assign a new value to it, or when a new binding is set.
*/

/*!
  \fn template <typename T> template <typename Functor> QPropertyChangeHandler<T, Functor> QPropertyAlias<T>::onValueChanged(Functor f)

  Registers the given functor \a f as a callback that shall be called whenever
  the value of the aliased property changes.

  The callback \a f is expected to be a type that has a plain call operator () without any
  parameters. This means that you can provide a C++ lambda expression, an std::function
  or even a custom struct with a call operator.

  The returned property change handler object keeps track of the registration. When it
  goes out of scope, the callback is de-registered.
*/

/*!
  \fn template <typename T> template <typename Functor> QPropertyChangeHandler<T, Functor> QPropertyAlias<T>::subscribe(Functor f)

  Subscribes the given functor \a f as a callback that is called immediately and whenever
  the value of the aliased property changes in the future.

  The callback \a f is expected to be a type that has a plain call operator () without any
  parameters. This means that you can provide a C++ lambda expression, an std::function
  or even a custom struct with a call operator.

  The returned property change handler object keeps track of the subscription. When it
  goes out of scope, the callback is unsubscribed.
*/

/*!
  \fn template <typename T> bool QPropertyAlias<T>::isValid() const

  Returns true if the aliased property still exists; false otherwise.

  If the aliased property doesn't exist, all other method calls are ignored.
*/

struct QBindingStorageData
{
    size_t size = 0;
    size_t used = 0;
    // Pair[] pairs;
};

struct QBindingStoragePrivate
{
    // This class basically implements a simple and fast hash map to store bindings for a QObject
    // The reason that we're not using QHash is that QPropertyBindingData can not be copied, only
    // moved. That doesn't work well together with an implicitly shared class.
    struct Pair
    {
        QUntypedPropertyData *data;
        QPropertyBindingData bindingData;
    };
    static_assert(alignof(Pair) == alignof(void *));
    static_assert(alignof(size_t) == alignof(void *));

    QBindingStorageData *&d;

    static inline Pair *pairs(QBindingStorageData *dd)
    {
        Q_ASSERT(dd);
        return reinterpret_cast<Pair *>(dd + 1);
    }
    void reallocate(size_t newSize)
    {
        Q_ASSERT(!d || newSize > d->size);
        size_t allocSize = sizeof(QBindingStorageData) + newSize*sizeof(Pair);
        void *nd = malloc(allocSize);
        memset(nd, 0, allocSize);
        QBindingStorageData *newData = new (nd) QBindingStorageData;
        newData->size = newSize;
        if (!d) {
            d = newData;
            return;
        }
        newData->used = d->used;
        Pair *p = pairs(d);
        for (size_t i = 0; i < d->size; ++i, ++p) {
            if (p->data) {
                Pair *pp = pairs(newData);
                size_t index = qHash(p->data);
                while (pp[index].data) {
                    ++index;
                    if (index == newData->size)
                        index = 0;
                }
                new (pp + index) Pair{p->data, QPropertyBindingData(std::move(p->bindingData), p->data)};
            }
        }
        // data has been moved, no need to call destructors on old Pairs
        free(d);
        d = newData;
    }

    QBindingStoragePrivate(QBindingStorageData *&_d) : d(_d) {}

    QPropertyBindingData *get(const QUntypedPropertyData *data)
    {
        if (!d)
            return nullptr;
        Q_ASSERT(d->size && (d->size & (d->size - 1)) == 0); // size is a power of two
        size_t index = qHash(data) & (d->size - 1);
        Pair *p = pairs(d);
        while (p[index].data) {
            if (p[index].data == data)
                return &p[index].bindingData;
            ++index;
            if (index == d->size)
                index = 0;
        }
        return nullptr;
    }
    QPropertyBindingData *getAndCreate(QUntypedPropertyData *data)
    {
        if (!d)
            reallocate(8);
        else if (d->used*2 >= d->size)
            reallocate(d->size*2);
        Q_ASSERT(d->size && (d->size & (d->size - 1)) == 0); // size is a power of two
        size_t index = qHash(data) & (d->size - 1);
        Pair *p = pairs(d);
        while (p[index].data) {
            if (p[index].data == data)
                return &p[index].bindingData;
            ++index;
            if (index == d->size)
                index = 0;
        }
        ++d->used;
        new (p + index) Pair{data, QPropertyBindingData()};
        return &p[index].bindingData;
    }

    void destroy()
    {
        if (!d)
            return;
        Pair *p = pairs(d);
        for (size_t i = 0; i < d->size; ++i) {
            if (p->data)
                p->~Pair();
            ++p;
        }
        free(d);
    }
};

/*!
    \class QBindingStorage
    \internal

    QBindingStorage acts as a storage for property binding related data in QObject.
    Any property in a QObject can be made bindable, by using the Q_BINDABLE_PROPERTY_DATA
    macro to declare the data storage. Then implement a setter and getter for the property
    and declare it as a Q_PROPERTY as usual. Finally make it bindable, but using
    the Q_BINDABLE_PROPERTY macro after the declaration of the setter and getter.
*/

QBindingStorage::QBindingStorage()
{
    currentlyEvaluatingBinding = &currentBindingEvaluationState;
}

QBindingStorage::~QBindingStorage()
{
    QBindingStoragePrivate(d).destroy();
}

void QBindingStorage::maybeUpdateBindingAndRegister(const QUntypedPropertyData *data) const
{
    QUntypedPropertyData *dd = const_cast<QUntypedPropertyData *>(data);
    auto storage = *currentlyEvaluatingBinding ?
                QBindingStoragePrivate(d).getAndCreate(dd) :
                QBindingStoragePrivate(d).get(dd);
    if (!storage)
        return;
    if (auto *binding = storage->binding())
        binding->evaluateIfDirtyAndReturnTrueIfValueChanged();
    storage->registerWithCurrentlyEvaluatingBinding();
}

QPropertyBindingData *QBindingStorage::bindingData(const QUntypedPropertyData *data) const
{
    return QBindingStoragePrivate(d).get(data);
}

QPropertyBindingData *QBindingStorage::bindingData(QUntypedPropertyData *data, bool create)
{
    auto storage = create ?
                QBindingStoragePrivate(d).getAndCreate(data) :
                QBindingStoragePrivate(d).get(data);
    return storage;
}


QT_END_NAMESPACE
