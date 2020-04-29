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
#include "qpropertybinding_p.h"

#include <qscopedvaluerollback.h>

QT_BEGIN_NAMESPACE

using namespace QtPrivate;

QPropertyBase::QPropertyBase(QPropertyBase &&other, void *propertyDataPtr)
{
    std::swap(d_ptr, other.d_ptr);
    QPropertyBasePointer d{this};
    d.setFirstObserver(nullptr);
    if (auto binding = d.bindingPtr())
        binding->setProperty(propertyDataPtr);
}

void QPropertyBase::moveAssign(QPropertyBase &&other, void *propertyDataPtr)
{
    if (&other == this)
        return;

    QPropertyBasePointer d{this};
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

QPropertyBase::~QPropertyBase()
{
    QPropertyBasePointer d{this};
    for (auto observer = d.firstObserver(); observer;) {
        auto next = observer.nextObserver();
        observer.unlink();
        observer = next;
    }
    if (auto binding = d.bindingPtr())
        binding->unlinkAndDeref();
}

QUntypedPropertyBinding QPropertyBase::setBinding(const QUntypedPropertyBinding &binding,
                                                  void *propertyDataPtr,
                                                  void *staticObserver,
                                                  void (*staticObserverCallback)(void*))
{
    QPropertyBindingPrivatePtr oldBinding;
    QPropertyBindingPrivatePtr newBinding = binding.d;

    QPropertyBasePointer d{this};
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
        newBinding->setStaticObserver(staticObserver, staticObserverCallback);
    } else if (observer) {
        d.setObservers(observer.ptr);
    } else {
        d_ptr &= ~QPropertyBase::BindingBit;
    }

    return QUntypedPropertyBinding(oldBinding.data());
}

QPropertyBindingPrivate *QPropertyBase::binding()
{
    QPropertyBasePointer d{this};
    if (auto binding = d.bindingPtr())
        return binding;
    return nullptr;
}

QPropertyBindingPrivate *QPropertyBasePointer::bindingPtr() const
{
    if (ptr->d_ptr & QPropertyBase::BindingBit)
        return reinterpret_cast<QPropertyBindingPrivate*>(ptr->d_ptr & ~QPropertyBase::FlagMask);
    return nullptr;
}

void QPropertyBasePointer::setObservers(QPropertyObserver *observer)
{
    observer->prev = reinterpret_cast<QPropertyObserver**>(&(ptr->d_ptr));
    ptr->d_ptr = (reinterpret_cast<quintptr>(observer) & ~QPropertyBase::FlagMask);
}

void QPropertyBasePointer::addObserver(QPropertyObserver *observer)
{
    if (auto *binding = bindingPtr()) {
        observer->prev = &binding->firstObserver.ptr;
        observer->next = binding->firstObserver.ptr;
        if (observer->next)
            observer->next->prev = &observer->next;
        binding->firstObserver.ptr = observer;
    } else {
        auto firstObserver = reinterpret_cast<QPropertyObserver*>(ptr->d_ptr & ~QPropertyBase::FlagMask);
        observer->prev = reinterpret_cast<QPropertyObserver**>(&ptr->d_ptr);
        observer->next = firstObserver;
        if (observer->next)
            observer->next->prev = &observer->next;
    }
    setFirstObserver(observer);
}

void QPropertyBasePointer::setFirstObserver(QPropertyObserver *observer)
{
    if (auto *binding = bindingPtr()) {
        binding->firstObserver.ptr = observer;
        return;
    }
    ptr->d_ptr = reinterpret_cast<quintptr>(observer) | (ptr->d_ptr & QPropertyBase::FlagMask);
}

QPropertyObserverPointer QPropertyBasePointer::firstObserver() const
{
    if (auto *binding = bindingPtr())
        return binding->firstObserver;
    return {reinterpret_cast<QPropertyObserver*>(ptr->d_ptr & ~QPropertyBase::FlagMask)};
}

static thread_local BindingEvaluationState *currentBindingEvaluationState = nullptr;

BindingEvaluationState::BindingEvaluationState(QPropertyBindingPrivate *binding)
    : binding(binding)
{
    previousState = currentBindingEvaluationState;
    currentBindingEvaluationState = this;
    binding->clearDependencyObservers();
}

BindingEvaluationState::~BindingEvaluationState()
{
    currentBindingEvaluationState = previousState;
}

void QPropertyBase::evaluateIfDirty()
{
    QPropertyBasePointer d{this};
    QPropertyBindingPrivate *binding = d.bindingPtr();
    if (!binding)
        return;
    binding->evaluateIfDirtyAndReturnTrueIfValueChanged();
}

void QPropertyBase::removeBinding()
{
    QPropertyBasePointer d{this};

    if (auto *existingBinding = d.bindingPtr()) {
        auto observer = existingBinding->takeObservers();
        existingBinding->unlinkAndDeref();
        d_ptr &= ExtraBit;
        if (observer)
            d.setObservers(observer.ptr);
    }
}

void QPropertyBase::registerWithCurrentlyEvaluatingBinding() const
{
    auto currentState = currentBindingEvaluationState;
    if (!currentState)
        return;

    QPropertyBasePointer d{this};

    QPropertyObserverPointer dependencyObserver = currentState->binding->allocateDependencyObserver();
    dependencyObserver.setBindingToMarkDirty(currentState->binding);
    dependencyObserver.observeProperty(d);
}

void QPropertyBase::notifyObservers(void *propertyDataPtr)
{
    QPropertyBasePointer d{this};
    if (QPropertyObserverPointer observer = d.firstObserver())
        observer.notify(d.bindingPtr(), propertyDataPtr);
}

int QPropertyBasePointer::observerCount() const
{
    int count = 0;
    for (auto observer = firstObserver(); observer; observer = observer.nextObserver())
        ++count;
    return count;
}

QPropertyObserver::QPropertyObserver(void (*callback)(QPropertyObserver *, void *))
{
    QPropertyObserverPointer d{this};
    d.setChangeHandler(callback);
}

QPropertyObserver::QPropertyObserver(void *aliasedPropertyPtr)
{
    QPropertyObserverPointer d{this};
    d.setAliasedProperty(aliasedPropertyPtr);
}

void QPropertyObserver::setSource(QPropertyBase &property)
{
    QPropertyObserverPointer d{this};
    QPropertyBasePointer propPrivate{&property};
    d.observeProperty(propPrivate);
}


QPropertyObserver::~QPropertyObserver()
{
    QPropertyObserverPointer d{this};
    d.unlink();
}

QPropertyObserver::QPropertyObserver() = default;

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
        ptr->aliasedPropertyPtr = 0;
    if (ptr->next)
        ptr->next->prev = ptr->prev;
    if (ptr->prev)
        ptr->prev.setPointer(ptr->next.data());
    ptr->next = nullptr;
    ptr->prev.clear();
}

void QPropertyObserverPointer::setChangeHandler(void (*changeHandler)(QPropertyObserver *, void *))
{
    ptr->changeHandler = changeHandler;
    ptr->next.setTag(QPropertyObserver::ObserverNotifiesChangeHandler);
}

void QPropertyObserverPointer::setAliasedProperty(void *propertyPtr)
{
    ptr->aliasedPropertyPtr = quintptr(propertyPtr);
    ptr->next.setTag(QPropertyObserver::ObserverNotifiesAlias);
}

void QPropertyObserverPointer::setBindingToMarkDirty(QPropertyBindingPrivate *binding)
{
    ptr->bindingToMarkDirty = binding;
    ptr->next.setTag(QPropertyObserver::ObserverNotifiesBinding);
}

void QPropertyObserverPointer::notify(QPropertyBindingPrivate *triggeringBinding, void *propertyDataPtr)
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

void QPropertyObserverPointer::observeProperty(QPropertyBasePointer property)
{
    if (ptr->prev)
        unlink();
    property.addObserver(ptr);
}

QPropertyBindingError::QPropertyBindingError(Type type)
{
    if (type != NoError) {
        d = new QPropertyBindingErrorPrivate;
        d->type = type;
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

void QPropertyBindingError::setDescription(const QString &description)
{
    if (!d)
        d = new QPropertyBindingErrorPrivate;
    d->description = description;
}

QString QPropertyBindingError::description() const
{
    if (!d)
        return QString();
    return d->description;
}

QPropertyBindingSourceLocation QPropertyBindingError::location() const
{
    if (!d)
        return QPropertyBindingSourceLocation();
    return d->location;
}

/*!
  \class QProperty
  \inmodule QtCore
  \brief The QProperty class is a template class that enables automatic property bindings.

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
  \class QNotifiedProperty
  \inmodule QtCore
  \brief The QNotifiedProperty class is a template class that enables automatic property bindings
         and invokes a callback function on the surrounding class when the value changes.

  \ingroup tools

  QNotifiedProperty\<T, Callback\> is a generic container that holds an
  instance of T and behaves mostly like \l QProperty. The extra template
  parameter is used to identify the surrounding class and a member function of
  that class. The member function will be called whenever the value held by the
  property changes.

  You can use QNotifiedProperty to port code that uses Q_PROPERTY. The getter
  and setter are trivial to adapt for accessing a \l QProperty rather than the
  plain value. In order to invoke the change signal on property changes, use
  QNotifiedProperty and pass the change signal as callback.

  \code
    class MyClass : public QObject
    {
        \Q_OBJECT
        // Replacing: Q_PROPERTY(int x READ x WRITE setX NOTIFY xChanged)
    public:
        int x() const { return xProp; }
        void setX(int x) { xProp = x; }

    signals:
        void xChanged();

    private:
        // Now you can set bindings on xProp and use it in other bindings.
        QNotifiedProperty<int, &MyClass::xChanged> xProp;
    };
  \endcode
*/

/*!
  \fn template <typename T, typename Class, void(Class::*Callback)()> QNotifiedProperty<T, Callback>::QNotifiedProperty()

  Constructs a property with a default constructed instance of T.
*/

/*!
  \fn template <typename T, typename Class, void(Class::*Callback)()> explicit QNotifiedProperty<T, Callback>::QNotifiedProperty(const T &initialValue)

  Constructs a property with the provided \a initialValue.
*/

/*!
  \fn template <typename T, typename Class, void(Class::*Callback)()> explicit QNotifiedProperty<T, Callback>::QNotifiedProperty(T &&initialValue)

  Move-Constructs a property with the provided \a initialValue.
*/

/*!
  \fn template <typename T, typename Class, void(Class::*Callback)()> QNotifiedProperty<T, Callback>::QNotifiedProperty(Class *owner, const QPropertyBinding<T> &binding)

  Constructs a property that is tied to the provided \a binding expression. The
  first time the property value is read, the binding is evaluated. Whenever a
  dependency of the binding changes, the binding will be re-evaluated the next
  time the value of this property is read. When the property value changes \a
  owner is notified via the Callback function.
*/

/*!
  \fn template <typename T, typename Class, void(Class::*Callback)()> QNotifiedProperty<T, Callback>::QNotifiedProperty(Class *owner, QPropertyBinding<T> &&binding)

  Constructs a property that is tied to the provided \a binding expression. The
  first time the property value is read, the binding is evaluated. Whenever a
  dependency of the binding changes, the binding will be re-evaluated the next
  time the value of this property is read. When the property value changes \a
  owner is notified via the Callback function.
*/


/*!
  \fn template <typename T, typename Class, void(Class::*Callback)()> template <typename Functor> QNotifiedProperty<T, Callback>::QNotifiedProperty(Class *owner, Functor &&f)

  Constructs a property that is tied to the provided binding expression \a f. The
  first time the property value is read, the binding is evaluated. Whenever a
  dependency of the binding changes, the binding will be re-evaluated the next
  time the value of this property is read. When the property value changes \a
  owner is notified via the Callback function.
*/

/*!
  \fn template <typename T, typename Class, void(Class::*Callback)()> QNotifiedProperty<T, Callback>::~QNotifiedProperty()

  Destroys the property.
*/

/*!
  \fn template <typename T, typename Class, void(Class::*Callback)()> T QNotifiedProperty<T, Callback>::value() const

  Returns the value of the property. This may evaluate a binding expression that
  is tied to this property, before returning the value.
*/

/*!
  \fn template <typename T, typename Class, void(Class::*Callback)()> QNotifiedProperty<T, Callback>::operator T() const

  Returns the value of the property. This may evaluate a binding expression that
  is tied to this property, before returning the value.
*/

/*!
  \fn template <typename T, typename Class, void(Class::*Callback)()> void QNotifiedProperty<T, Callback>::setValue(Class *owner, const T &newValue)

  Assigns \a newValue to this property and removes the property's associated
  binding, if present. If the property value changes as a result, calls the
  Callback function on \a owner.
*/

/*!
  \fn template <typename T, typename Class, void(Class::*Callback)()> void QNotifiedProperty<T, Callback>::setValue(Class *owner, T &&newValue)
  \overload

  Assigns \a newValue to this property and removes the property's associated
  binding, if present. If the property value changes as a result, calls the
  Callback function on \a owner.
*/

/*!
  \fn template <typename T, typename Class, void(Class::*Callback)()> QPropertyBinding<T> QNotifiedProperty<T, Callback>::setBinding(Class *owner, const QPropertyBinding<T> &newBinding)

  Associates the value of this property with the provided \a newBinding
  expression and returns the previously associated binding. The first time the
  property value is read, the binding is evaluated. Whenever a dependency of the
  binding changes, the binding will be re-evaluated the next time the value of
  this property is read. When the property value changes \a owner is notified
  via the Callback function.
*/

/*!
  \fn template <typename T, typename Class, void(Class::*Callback)()> template <typename Functor> QPropertyBinding<T> QNotifiedProperty<T, Callback>::setBinding(Class *owner, Functor f)
  \overload

  Associates the value of this property with the provided functor \a f and
  returns the previously associated binding. The first time the property value
  is read, the binding is evaluated by invoking the call operator () of \a f.
  Whenever a dependency of the binding changes, the binding will be re-evaluated
  the next time the value of this property is read. When the property value
  changes \a owner is notified via the Callback function.
*/

/*!
  \fn template <typename T, typename Class, void(Class::*Callback)()> QPropertyBinding<T> QNotifiedProperty<T, Callback>::setBinding(Class *owner, QPropertyBinding<T> &&newBinding)
  \overload

  Associates the value of this property with the provided \a newBinding
  expression and returns the previously associated binding. The first time the
  property value is read, the binding is evaluated. Whenever a dependency of the
  binding changes, the binding will be re-evaluated the next time the value of
  this property is read. When the property value changes \a owner is notified
  via the Callback function.
*/

/*!
  \fn template <typename T, typename Class, void(Class::*Callback)()> QPropertyBinding<T> bool QNotifiedProperty<T, Callback>::setBinding(Class *owner, const QUntypedPropertyBinding &newBinding)
  \overload

  Associates the value of this property with the provided \a newBinding
  expression. The first time the property value is read, the binding is evaluated.
  Whenever a dependency of the binding changes, the binding will be re-evaluated
  the next time the value of this property is read. When the property value
  changes \a owner is notified via the Callback function.

  Returns true if the type of this property is the same as the type the binding
  function returns; false otherwise.
*/

/*!
  \fn template <typename T, typename Class, void(Class::*Callback)()> bool QNotifiedProperty<T, Callback>::hasBinding() const

  Returns true if the property is associated with a binding; false otherwise.
*/


/*!
  \fn template <typename T, typename Class, void(Class::*Callback)()> QPropertyBinding<T> QNotifiedProperty<T, Callback>::binding() const

  Returns the binding expression that is associated with this property. A
  default constructed QPropertyBinding<T> will be returned if no such
  association exists.
*/

/*!
  \fn template <typename T, typename Class, void(Class::*Callback)()> QPropertyBinding<T> QNotifiedProperty<T, Callback>::takeBinding()

  Disassociates the binding expression from this property and returns it. After
  calling this function, the value of the property will only change if you
  assign a new value to it, or when a new binding is set.
*/

/*!
  \fn template <typename T, typename Class, void(Class::*Callback)()> template <typename Functor> QPropertyChangeHandler<T, Functor> QNotifiedProperty<T, Callback>::onValueChanged(Functor f)

  Registers the given functor \a f as a callback that shall be called whenever
  the value of the property changes.

  The callback \a f is expected to be a type that has a plain call operator () without any
  parameters. This means that you can provide a C++ lambda expression, an std::function
  or even a custom struct with a call operator.

  The returned property change handler object keeps track of the registration. When it
  goes out of scope, the callback is de-registered.
*/

/*!
  \fn template <typename T, typename Class, void(Class::*Callback)()> template <typename Functor> QPropertyChangeHandler<T, Functor> QNotifiedProperty<T, Callback>::subscribe(Functor f)

  Subscribes the given functor \a f as a callback that is called immediately and whenever
  the value of the property changes in the future.

  The callback \a f is expected to be a type that has a plain call operator () without any
  parameters. This means that you can provide a C++ lambda expression, an std::function
  or even a custom struct with a call operator.

  The returned property change handler object keeps track of the subscription. When it
  goes out of scope, the callback is unsubscribed.
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

QT_END_NAMESPACE
