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

#ifndef QPROPERTY_H
#define QPROPERTY_H

#include <QtCore/qglobal.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qstring.h>
#include <functional>
#include <type_traits>
#include <variant>

#include <QtCore/qpropertyprivate.h>

#if __has_include(<source_location>) && __cplusplus >= 202002L && !defined(Q_CLANG_QDOC)
#include <experimental/source_location>
#define QT_PROPERTY_COLLECT_BINDING_LOCATION
#define QT_PROPERTY_DEFAULT_BINDING_LOCATION QPropertyBindingSourceLocation(std::source_location::current())
#elif __has_include(<experimental/source_location>) && __cplusplus >= 201703L && !defined(Q_CLANG_QDOC)
#include <experimental/source_location>
#define QT_PROPERTY_COLLECT_BINDING_LOCATION
#define QT_PROPERTY_DEFAULT_BINDING_LOCATION QPropertyBindingSourceLocation(std::experimental::source_location::current())
#else
#define QT_PROPERTY_DEFAULT_BINDING_LOCATION QPropertyBindingSourceLocation()
#endif

QT_BEGIN_NAMESPACE

template <typename T>
class QPropertyData : public QUntypedPropertyData
{
protected:
    mutable T val = T();
private:
    class DisableRValueRefs {};
protected:
    static constexpr bool UseReferences = !(std::is_arithmetic_v<T> || std::is_enum_v<T> || std::is_pointer_v<T>);
public:
    using value_type = T;
    using parameter_type = std::conditional_t<UseReferences, const T &, T>;
    using rvalue_ref = typename std::conditional_t<UseReferences, T &&, DisableRValueRefs>;
    using arrow_operator_result = std::conditional_t<std::is_pointer_v<T>, const T &,
                                        std::conditional_t<QTypeTraits::is_dereferenceable_v<T>, const T &, void>>;

    QPropertyData() = default;
    QPropertyData(parameter_type t) : val(t) {}
    QPropertyData(rvalue_ref t) : val(std::move(t)) {}
    ~QPropertyData() = default;

    parameter_type valueBypassingBindings() const { return val; }
    void setValueBypassingBindings(parameter_type v) { val = v; }
    void setValueBypassingBindings(rvalue_ref v) { val = std::move(v); }
};

struct Q_CORE_EXPORT QPropertyBindingSourceLocation
{
    const char *fileName = nullptr;
    const char *functionName = nullptr;
    quint32 line = 0;
    quint32 column = 0;
    QPropertyBindingSourceLocation() = default;
#ifdef QT_PROPERTY_COLLECT_BINDING_LOCATION
    QPropertyBindingSourceLocation(const std::experimental::source_location &cppLocation)
    {
        fileName = cppLocation.file_name();
        functionName = cppLocation.function_name();
        line = cppLocation.line();
        column = cppLocation.column();
    }
#endif
};

template <typename Functor> class QPropertyChangeHandler;
class QPropertyBindingErrorPrivate;

class Q_CORE_EXPORT QPropertyBindingError
{
public:
    enum Type {
        NoError,
        BindingLoop,
        EvaluationError,
        UnknownError
    };

    QPropertyBindingError();
    QPropertyBindingError(Type type, const QString &description = QString());

    QPropertyBindingError(const QPropertyBindingError &other);
    QPropertyBindingError &operator=(const QPropertyBindingError &other);
    QPropertyBindingError(QPropertyBindingError &&other);
    QPropertyBindingError &operator=(QPropertyBindingError &&other);
    ~QPropertyBindingError();

    bool hasError() const { return d.get() != nullptr; }
    Type type() const;
    QString description() const;

private:
    QSharedDataPointer<QPropertyBindingErrorPrivate> d;
};

class Q_CORE_EXPORT QUntypedPropertyBinding
{
public:
    // writes binding result into dataPtr
    using BindingFunctionVTable = QtPrivate::BindingFunctionVTable;

    QUntypedPropertyBinding();
    QUntypedPropertyBinding(QMetaType metaType, const BindingFunctionVTable *vtable, void *function, const QPropertyBindingSourceLocation &location);

    template<typename Functor>
    QUntypedPropertyBinding(QMetaType metaType, Functor &&f, const QPropertyBindingSourceLocation &location)
        : QUntypedPropertyBinding(metaType, &QtPrivate::bindingFunctionVTable<std::remove_reference_t<Functor>>, &f, location)
    {}

    QUntypedPropertyBinding(QUntypedPropertyBinding &&other);
    QUntypedPropertyBinding(const QUntypedPropertyBinding &other);
    QUntypedPropertyBinding &operator=(const QUntypedPropertyBinding &other);
    QUntypedPropertyBinding &operator=(QUntypedPropertyBinding &&other);
    ~QUntypedPropertyBinding();

    bool isNull() const;

    QPropertyBindingError error() const;

    QMetaType valueMetaType() const;

    explicit QUntypedPropertyBinding(QPropertyBindingPrivate *priv);
private:
    friend class QtPrivate::QPropertyBindingData;
    friend class QPropertyBindingPrivate;
    template <typename> friend class QPropertyBinding;
    QPropertyBindingPrivatePtr d;
};

template <typename PropertyType>
class QPropertyBinding : public QUntypedPropertyBinding
{

public:
    QPropertyBinding() = default;

    template<typename Functor>
    QPropertyBinding(Functor &&f, const QPropertyBindingSourceLocation &location)
        : QUntypedPropertyBinding(QMetaType::fromType<PropertyType>(), &QtPrivate::bindingFunctionVTable<std::remove_reference_t<Functor>, PropertyType>, &f, location)
    {}


    // Internal
    explicit QPropertyBinding(const QUntypedPropertyBinding &binding)
        : QUntypedPropertyBinding(binding)
    {}
};

namespace Qt {
    template <typename Functor>
    auto makePropertyBinding(Functor &&f, const QPropertyBindingSourceLocation &location = QT_PROPERTY_DEFAULT_BINDING_LOCATION,
                             std::enable_if_t<std::is_invocable_v<Functor>> * = nullptr)
    {
        return QPropertyBinding<std::invoke_result_t<Functor>>(std::forward<Functor>(f), location);
    }
}

struct QPropertyObserverPrivate;
struct QPropertyObserverPointer;
class QPropertyObserver;

class QPropertyObserverBase
{
public:
    // Internal
    enum ObserverTag {
        ObserverNotifiesBinding, // observer was installed to notify bindings that obsverved property changed
        ObserverNotifiesChangeHandler, // observer is a change handler, which runs on every change
        ObserverNotifiesAlias, // used for QPropertyAlias
        ObserverIsPlaceholder  // the observer before this one is currently evaluated in QPropertyObserver::notifyObservers.
    };
protected:
    using ChangeHandler = void (*)(QPropertyObserver*, QUntypedPropertyData *);

private:
    friend struct QPropertyObserverNodeProtector;
    friend class QPropertyObserver;
    friend struct QPropertyObserverPointer;
    friend struct QPropertyBindingDataPointer;
    friend class QPropertyBindingPrivate;

    QTaggedPointer<QPropertyObserver, ObserverTag> next;
    // prev is a pointer to the "next" element within the previous node, or to the "firstObserverPtr" if it is the
    // first node.
    QtPrivate::QTagPreservingPointerToPointer<QPropertyObserver, ObserverTag> prev;

    union {
        QPropertyBindingPrivate *bindingToMarkDirty = nullptr;
        ChangeHandler changeHandler;
        QUntypedPropertyData *aliasedPropertyData;
    };
};

class Q_CORE_EXPORT QPropertyObserver : public QPropertyObserverBase
{
public:
    constexpr QPropertyObserver() = default;
    QPropertyObserver(QPropertyObserver &&other) noexcept;
    QPropertyObserver &operator=(QPropertyObserver &&other) noexcept;
    ~QPropertyObserver();

    template<typename Property, typename = typename Property::InheritsQUntypedPropertyData>
    void setSource(const Property &property)
    { setSource(property.bindingData()); }
    void setSource(const QtPrivate::QPropertyBindingData &property);

protected:
    QPropertyObserver(ChangeHandler changeHandler);
    QPropertyObserver(QUntypedPropertyData *aliasedPropertyPtr);

    QUntypedPropertyData *aliasedProperty() const
    {
        return aliasedPropertyData;
    }

private:

    QPropertyObserver(const QPropertyObserver &) = delete;
    QPropertyObserver &operator=(const QPropertyObserver &) = delete;

};

template <typename Functor>
class [[nodiscard]] QPropertyChangeHandler : public QPropertyObserver
{
    Functor m_handler;
public:
    QPropertyChangeHandler(Functor handler)
        : QPropertyObserver([](QPropertyObserver *self, QUntypedPropertyData *) {
              auto This = static_cast<QPropertyChangeHandler<Functor>*>(self);
              This->m_handler();
          })
        , m_handler(handler)
    {
    }

    template<typename Property, typename = typename Property::InheritsQUntypedPropertyData>
    QPropertyChangeHandler(const Property &property, Functor handler)
        : QPropertyObserver([](QPropertyObserver *self, QUntypedPropertyData *) {
              auto This = static_cast<QPropertyChangeHandler<Functor>*>(self);
              This->m_handler();
          })
        , m_handler(handler)
    {
        setSource(property);
    }
};

template <typename T>
class QProperty : public QPropertyData<T>
{
    QtPrivate::QPropertyBindingData d;
    bool is_equal(const T &v)
    {
        if constexpr (QTypeTraits::has_operator_equal_v<T>) {
            if (v == this->val)
                return true;
        }
        return false;
    }

public:
    using value_type = typename QPropertyData<T>::value_type;
    using parameter_type = typename QPropertyData<T>::parameter_type;
    using rvalue_ref = typename QPropertyData<T>::rvalue_ref;
    using arrow_operator_result = typename QPropertyData<T>::arrow_operator_result;

    QProperty() = default;
    explicit QProperty(parameter_type initialValue) : QPropertyData<T>(initialValue) {}
    explicit QProperty(rvalue_ref initialValue) : QPropertyData<T>(std::move(initialValue)) {}
    explicit QProperty(const QPropertyBinding<T> &binding)
        : QProperty()
    { setBinding(binding); }
#ifndef Q_CLANG_QDOC
    template <typename Functor>
    explicit QProperty(Functor &&f, const QPropertyBindingSourceLocation &location = QT_PROPERTY_DEFAULT_BINDING_LOCATION,
                       typename std::enable_if_t<std::is_invocable_r_v<T, Functor&>> * = nullptr)
        : QProperty(QPropertyBinding<T>(std::forward<Functor>(f), location))
    {}
#else
    template <typename Functor>
    explicit QProperty(Functor &&f);
#endif
    ~QProperty() = default;

    parameter_type value() const
    {
        if (d.hasBinding())
            d.evaluateIfDirty(this);
        d.registerWithCurrentlyEvaluatingBinding();
        return this->val;
    }

    arrow_operator_result operator->() const
    {
        if constexpr (QTypeTraits::is_dereferenceable_v<T>) {
            return value();
        } else if constexpr (std::is_pointer_v<T>) {
            value();
            return this->val;
        } else {
            return;
        }
    }

    parameter_type operator*() const
    {
        return value();
    }

    operator parameter_type() const
    {
        return value();
    }

    void setValue(rvalue_ref newValue)
    {
        d.removeBinding();
        if (is_equal(newValue))
            return;
        this->val = std::move(newValue);
        notify();
    }

    void setValue(parameter_type newValue)
    {
        d.removeBinding();
        if (is_equal(newValue))
            return;
        this->val = newValue;
        notify();
    }

    QProperty<T> &operator=(rvalue_ref newValue)
    {
        setValue(std::move(newValue));
        return *this;
    }

    QProperty<T> &operator=(parameter_type newValue)
    {
        setValue(newValue);
        return *this;
    }

    QPropertyBinding<T> setBinding(const QPropertyBinding<T> &newBinding)
    {
        QPropertyBinding<T> oldBinding(d.setBinding(newBinding, this));
        notify();
        return oldBinding;
    }

    bool setBinding(const QUntypedPropertyBinding &newBinding)
    {
        if (!newBinding.isNull() && newBinding.valueMetaType().id() != qMetaTypeId<T>())
            return false;
        setBinding(static_cast<const QPropertyBinding<T> &>(newBinding));
        return true;
    }

    void markDirty() {
        d.markDirty();
        notify();
    }

#ifndef Q_CLANG_QDOC
    template <typename Functor>
    QPropertyBinding<T> setBinding(Functor &&f,
                                   const QPropertyBindingSourceLocation &location = QT_PROPERTY_DEFAULT_BINDING_LOCATION,
                                   std::enable_if_t<std::is_invocable_v<Functor>> * = nullptr)
    {
        return setBinding(Qt::makePropertyBinding(std::forward<Functor>(f), location));
    }
#else
    template <typename Functor>
    QPropertyBinding<T> setBinding(Functor f);
#endif

    bool hasBinding() const { return d.hasBinding(); }

    QPropertyBinding<T> binding() const
    {
        return QPropertyBinding<T>(QUntypedPropertyBinding(d.binding()));
    }

    QPropertyBinding<T> takeBinding()
    {
        return QPropertyBinding<T>(d.setBinding(QUntypedPropertyBinding(), this));
    }

    template<typename Functor>
    QPropertyChangeHandler<Functor> onValueChanged(Functor f)
    {
        static_assert(std::is_invocable_v<Functor>, "Functor callback must be callable without any parameters");
        return QPropertyChangeHandler<Functor>(*this, f);
    }

    template<typename Functor>
    QPropertyChangeHandler<Functor> subscribe(Functor f)
    {
        static_assert(std::is_invocable_v<Functor>, "Functor callback must be callable without any parameters");
        f();
        return onValueChanged(f);
    }

    const QtPrivate::QPropertyBindingData &bindingData() const { return d; }
private:
    void notify()
    {
        d.notifyObservers(this);
    }

    Q_DISABLE_COPY_MOVE(QProperty)
};

namespace Qt {
    template <typename PropertyType>
    QPropertyBinding<PropertyType> makePropertyBinding(const QProperty<PropertyType> &otherProperty,
                                                       const QPropertyBindingSourceLocation &location =
                                                       QT_PROPERTY_DEFAULT_BINDING_LOCATION)
    {
        return Qt::makePropertyBinding([&otherProperty]() -> PropertyType { return otherProperty; }, location);
    }
}


namespace QtPrivate
{

struct QBindableInterface
{
    using Getter = void (*)(const QUntypedPropertyData *d, void *value);
    using Setter = void (*)(QUntypedPropertyData *d, const void *value);
    using BindingGetter = QUntypedPropertyBinding (*)(const QUntypedPropertyData *d);
    using BindingSetter = QUntypedPropertyBinding (*)(QUntypedPropertyData *d, const QUntypedPropertyBinding &binding);
    using MakeBinding = QUntypedPropertyBinding (*)(const QUntypedPropertyData *d, const QPropertyBindingSourceLocation &location);
    using SetObserver = void (*)(const QUntypedPropertyData *d, QPropertyObserver *observer);
    using GetMetaType = QMetaType (*)();
    Getter getter;
    Setter setter;
    BindingGetter getBinding;
    BindingSetter setBinding;
    MakeBinding makeBinding;
    SetObserver setObserver;
    GetMetaType metaType;
};

template<typename Property, typename = void>
class QBindableInterfaceForProperty
{
    using T = typename Property::value_type;
public:
    // interface for computed properties. Those do not have a binding()/setBinding() method, but one can
    // install observers on them.
    static constexpr QBindableInterface iface = {
        [](const QUntypedPropertyData *d, void *value) -> void
        { *static_cast<T*>(value) = static_cast<const Property *>(d)->value(); },
        nullptr,
        nullptr,
        nullptr,
        [](const QUntypedPropertyData *d, const QPropertyBindingSourceLocation &location) -> QUntypedPropertyBinding
        { return Qt::makePropertyBinding([d]() -> T { return static_cast<const Property *>(d)->value(); }, location); },
        [](const QUntypedPropertyData *d, QPropertyObserver *observer) -> void
        { observer->setSource(static_cast<const Property *>(d)->bindingData()); },
        []() { return QMetaType::fromType<T>(); }
    };
};

template<typename Property>
class QBindableInterfaceForProperty<const Property, std::void_t<decltype(std::declval<Property>().binding())>>
{
    using T = typename Property::value_type;
public:
    // A bindable created from a const property results in a read-only interface, too.
    static constexpr QBindableInterface iface = {

        [](const QUntypedPropertyData *d, void *value) -> void
        { *static_cast<T*>(value) = static_cast<const Property *>(d)->value(); },
        /*setter=*/nullptr,
        [](const QUntypedPropertyData *d) -> QUntypedPropertyBinding
        { return static_cast<const Property *>(d)->binding(); },
        /*setBinding=*/nullptr,
        [](const QUntypedPropertyData *d, const QPropertyBindingSourceLocation &location) -> QUntypedPropertyBinding
        { return Qt::makePropertyBinding([d]() -> T { return static_cast<const Property *>(d)->value(); }, location); },
        [](const QUntypedPropertyData *d, QPropertyObserver *observer) -> void
        { observer->setSource(static_cast<const Property *>(d)->bindingData()); },
        []() { return QMetaType::fromType<T>(); }
    };
};

template<typename Property>
class QBindableInterfaceForProperty<Property, std::void_t<decltype(std::declval<Property>().binding())>>
{
    using T = typename Property::value_type;
public:
    static constexpr QBindableInterface iface = {
        [](const QUntypedPropertyData *d, void *value) -> void
        { *static_cast<T*>(value) = static_cast<const Property *>(d)->value(); },
        [](QUntypedPropertyData *d, const void *value) -> void
        { static_cast<Property *>(d)->setValue(*static_cast<const T*>(value)); },
        [](const QUntypedPropertyData *d) -> QUntypedPropertyBinding
        { return static_cast<const Property *>(d)->binding(); },
        [](QUntypedPropertyData *d, const QUntypedPropertyBinding &binding) -> QUntypedPropertyBinding
        { return static_cast<Property *>(d)->setBinding(static_cast<const QPropertyBinding<T> &>(binding)); },
        [](const QUntypedPropertyData *d, const QPropertyBindingSourceLocation &location) -> QUntypedPropertyBinding
        { return Qt::makePropertyBinding([d]() -> T { return static_cast<const Property *>(d)->value(); }, location); },
        [](const QUntypedPropertyData *d, QPropertyObserver *observer) -> void
        { observer->setSource(static_cast<const Property *>(d)->bindingData()); },
        []() { return QMetaType::fromType<T>(); }
    };
};

}

class QUntypedBindable
{
protected:
    QUntypedPropertyData *data = nullptr;
    const QtPrivate::QBindableInterface *iface = nullptr;
    constexpr QUntypedBindable(QUntypedPropertyData *d, const QtPrivate::QBindableInterface *i)
        : data(d), iface(i)
    {}

public:
    constexpr QUntypedBindable() = default;
    template<typename Property>
    QUntypedBindable(Property *p)
        : data(const_cast<std::remove_cv_t<Property> *>(p)),
          iface(&QtPrivate::QBindableInterfaceForProperty<Property>::iface)
    { Q_ASSERT(data && iface); }

    bool isValid() const { return data != nullptr; }
    bool isBindable() const { return iface && iface->getBinding; }
    bool isReadOnly() const { return !(iface && iface->setBinding && iface->setObserver); }

    QUntypedPropertyBinding makeBinding(const QPropertyBindingSourceLocation &location = QT_PROPERTY_DEFAULT_BINDING_LOCATION)
    {
        return iface ? iface->makeBinding(data, location) : QUntypedPropertyBinding();
    }

    QUntypedPropertyBinding takeBinding()
    {
        if (!iface)
            return QUntypedPropertyBinding {};
        // We do not have a dedicated takeBinding function pointer in the interface
        // therefore we synthesize takeBinding by retrieving the binding with binding
        // and calling setBinding with a default constructed QUntypedPropertyBinding
        // afterwards.
        if (!(iface->getBinding && iface->setBinding))
            return QUntypedPropertyBinding {};
        QUntypedPropertyBinding binding = iface->getBinding(data);
        iface->setBinding(data, QUntypedPropertyBinding{});
        return binding;
    }

    void observe(QPropertyObserver *observer)
    {
        if (iface)
            iface->setObserver(data, observer);
    }

    template<typename Functor>
    QPropertyChangeHandler<Functor> onValueChanged(Functor f)
    {
        QPropertyChangeHandler<Functor> handler(f);
        observe(&handler);
        return handler;
    }

    template<typename Functor>
    QPropertyChangeHandler<Functor> subscribe(Functor f)
    {
        f();
        return onValueChanged(f);
    }

    QUntypedPropertyBinding binding() const
    {
        if (!iface->getBinding)
            return QUntypedPropertyBinding();
        return iface->getBinding(data);
    }
    bool setBinding(const QUntypedPropertyBinding &binding)
    {
        if (!iface->setBinding)
            return false;
        if (!binding.isNull() && binding.valueMetaType() != iface->metaType())
            return false;
        iface->setBinding(data, binding);
        return true;
    }
    bool hasBinding() const
    {
        return !binding().isNull();
    }

};

template<typename T>
class QBindable : public QUntypedBindable
{
    template<typename U>
    friend class QPropertyAlias;
    constexpr QBindable(QUntypedPropertyData *d, const QtPrivate::QBindableInterface *i)
        : QUntypedBindable(d, i)
    {}
public:
    using QUntypedBindable::QUntypedBindable;
    explicit QBindable(const QUntypedBindable &b) : QUntypedBindable(b)
    {
        if (iface && iface->metaType() != QMetaType::fromType<T>()) {
            data = nullptr;
            iface = nullptr;
        }
    }

    QPropertyBinding<T> makeBinding(const QPropertyBindingSourceLocation &location = QT_PROPERTY_DEFAULT_BINDING_LOCATION)
    {
        return static_cast<QPropertyBinding<T> &&>(QUntypedBindable::makeBinding(location));
    }
    QPropertyBinding<T> binding() const
    {
        return static_cast<QPropertyBinding<T> &&>(QUntypedBindable::binding());
    }

    QPropertyBinding<T> takeBinding()
    {
        return static_cast<QPropertyBinding<T> &&>(QUntypedBindable::takeBinding());
    }

    using QUntypedBindable::setBinding;
    QPropertyBinding<T> setBinding(const QPropertyBinding<T> &binding)
    {
        Q_ASSERT(!iface || binding.isNull() || binding.valueMetaType() == iface->metaType());
        return (iface && iface->setBinding) ? static_cast<QPropertyBinding<T> &&>(iface->setBinding(data, binding)) : QPropertyBinding<T>();
    }
#ifndef Q_CLANG_QDOC
    template <typename Functor>
    QPropertyBinding<T> setBinding(Functor &&f,
                                   const QPropertyBindingSourceLocation &location = QT_PROPERTY_DEFAULT_BINDING_LOCATION,
                                   std::enable_if_t<std::is_invocable_v<Functor>> * = nullptr)
    {
        return setBinding(Qt::makePropertyBinding(std::forward<Functor>(f), location));
    }
#else
    template <typename Functor>
    QPropertyBinding<T> setBinding(Functor f);
#endif

    T value() const
    {
        if (iface) {
            T result;
            iface->getter(data, &result);
            return result;
        }
        return T{};
    }

    void setValue(const T &value)
    {
        if (iface && iface->setter)
            iface->setter(data, &value);
    }
};

template<typename T>
class QPropertyAlias : public QPropertyObserver
{
    Q_DISABLE_COPY_MOVE(QPropertyAlias)
    const QtPrivate::QBindableInterface *iface = nullptr;

public:
    QPropertyAlias(QProperty<T> *property)
        : QPropertyObserver(property),
          iface(&QtPrivate::QBindableInterfaceForProperty<QProperty<T>>::iface)
    {
        if (iface)
            iface->setObserver(aliasedProperty(), this);
    }

    template<typename Property, typename = typename Property::InheritsQUntypedPropertyData>
    QPropertyAlias(Property *property)
        : QPropertyObserver(property),
          iface(&QtPrivate::QBindableInterfaceForProperty<Property>::iface)
    {
        if (iface)
            iface->setObserver(aliasedProperty(), this);
    }

    QPropertyAlias(QPropertyAlias<T> *alias)
        : QPropertyObserver(alias->aliasedProperty()),
          iface(alias->iface)
    {
        if (iface)
            iface->setObserver(aliasedProperty(), this);
    }

    QPropertyAlias(const QBindable<T> &property)
        : QPropertyObserver(property.data),
          iface(property.iface)
    {
        if (iface)
            iface->setObserver(aliasedProperty(), this);
    }

    T value() const
    {
        T t = T();
        if (auto *p = aliasedProperty())
            iface->getter(p, &t);
        return t;
    }

    operator T() const { return value(); }

    void setValue(const T &newValue)
    {
        if (auto *p = aliasedProperty())
            iface->setter(p, &newValue);
    }

    QPropertyAlias<T> &operator=(const T &newValue)
    {
        setValue(newValue);
        return *this;
    }

    QPropertyBinding<T> setBinding(const QPropertyBinding<T> &newBinding)
    {
        return QBindable<T>(aliasedProperty(), iface).setBinding(newBinding);
    }

    bool setBinding(const QUntypedPropertyBinding &newBinding)
    {
        return QBindable<T>(aliasedProperty(), iface).setBinding(newBinding);
    }

#ifndef Q_CLANG_QDOC
    template <typename Functor>
    QPropertyBinding<T> setBinding(Functor &&f,
                                   const QPropertyBindingSourceLocation &location = QT_PROPERTY_DEFAULT_BINDING_LOCATION,
                                   std::enable_if_t<std::is_invocable_v<Functor>> * = nullptr)
    {
        return setBinding(Qt::makePropertyBinding(std::forward<Functor>(f), location));
    }
#else
    template <typename Functor>
    QPropertyBinding<T> setBinding(Functor f);
#endif

    bool hasBinding() const
    {
        return QBindable<T>(aliasedProperty(), iface).hasBinding();
    }

    QPropertyBinding<T> binding() const
    {
        return QBindable<T>(aliasedProperty(), iface).binding();
    }

    QPropertyBinding<T> takeBinding()
    {
        return QBindable<T>(aliasedProperty(), iface).takeBinding();
    }

    template<typename Functor>
    QPropertyChangeHandler<Functor> onValueChanged(Functor f)
    {
        return QBindable<T>(aliasedProperty(), iface).onValueChanged(f);
    }

    template<typename Functor>
    QPropertyChangeHandler<Functor> subscribe(Functor f)
    {
        return QBindable<T>(aliasedProperty(), iface).subscribe(f);
    }

    bool isValid() const
    {
        return aliasedProperty() != nullptr;
    }
};

namespace QtPrivate {

struct BindingEvaluationState;
struct CompatPropertySafePoint;
}

struct QBindingStatus
{
    QtPrivate::BindingEvaluationState *currentlyEvaluatingBinding = nullptr;
    QtPrivate::CompatPropertySafePoint *currentCompatProperty = nullptr;
};

struct QBindingStorageData;
class Q_CORE_EXPORT QBindingStorage
{
    mutable QBindingStorageData *d = nullptr;
    QBindingStatus *bindingStatus = nullptr;

    template<typename Class, typename T, auto Offset, auto Setter>
    friend class QObjectCompatProperty;
public:
    QBindingStorage();
    ~QBindingStorage();

    bool isEmpty() { return !d; }

    void maybeUpdateBindingAndRegister(const QUntypedPropertyData *data) const
    {
        if (!d && !bindingStatus->currentlyEvaluatingBinding)
            return;
        maybeUpdateBindingAndRegister_helper(data);
    }
    QtPrivate::QPropertyBindingData *bindingData(const QUntypedPropertyData *data) const
    {
        if (!d)
            return nullptr;
        return bindingData_helper(data);
    }
    QtPrivate::QPropertyBindingData *bindingData(QUntypedPropertyData *data, bool create)
    {
        if (!d && !create)
            return nullptr;
        return bindingData_helper(data, create);
    }
private:
    void maybeUpdateBindingAndRegister_helper(const QUntypedPropertyData *data) const;
    QtPrivate::QPropertyBindingData *bindingData_helper(const QUntypedPropertyData *data) const;
    QtPrivate::QPropertyBindingData *bindingData_helper(QUntypedPropertyData *data, bool create);
};


template<typename Class, typename T, auto Offset, auto Signal = nullptr>
class QObjectBindableProperty : public QPropertyData<T>
{
    using ThisType = QObjectBindableProperty<Class, T, Offset, Signal>;
    static bool constexpr HasSignal = !std::is_same_v<decltype(Signal), std::nullptr_t>;
    Class *owner()
    {
        char *that = reinterpret_cast<char *>(this);
        return reinterpret_cast<Class *>(that - QtPrivate::detail::getOffset(Offset));
    }
    const Class *owner() const
    {
        char *that = const_cast<char *>(reinterpret_cast<const char *>(this));
        return reinterpret_cast<Class *>(that - QtPrivate::detail::getOffset(Offset));
    }
    static void signalCallBack(QUntypedPropertyData *o)
    {
        QObjectBindableProperty *that = static_cast<QObjectBindableProperty *>(o);
        if constexpr (HasSignal)
            (that->owner()->*Signal)();
    }
public:
    using value_type = typename QPropertyData<T>::value_type;
    using parameter_type = typename QPropertyData<T>::parameter_type;
    using rvalue_ref = typename QPropertyData<T>::rvalue_ref;
    using arrow_operator_result = typename QPropertyData<T>::arrow_operator_result;

    QObjectBindableProperty() = default;
    explicit QObjectBindableProperty(const T &initialValue) : QPropertyData<T>(initialValue) {}
    explicit QObjectBindableProperty(T &&initialValue) : QPropertyData<T>(std::move(initialValue)) {}
    explicit QObjectBindableProperty(const QPropertyBinding<T> &binding)
        : QObjectBindableProperty()
    { setBinding(binding); }
#ifndef Q_CLANG_QDOC
    template <typename Functor>
    explicit QObjectBindableProperty(Functor &&f, const QPropertyBindingSourceLocation &location = QT_PROPERTY_DEFAULT_BINDING_LOCATION,
                       typename std::enable_if_t<std::is_invocable_r_v<T, Functor&>> * = nullptr)
        : QObjectBindableProperty(QPropertyBinding<T>(std::forward<Functor>(f), location))
    {}
#else
    template <typename Functor>
    explicit QObjectBindableProperty(Functor &&f);
#endif

    parameter_type value() const
    {
        qGetBindingStorage(owner())->maybeUpdateBindingAndRegister(this);
        return this->val;
    }

    arrow_operator_result operator->() const
    {
        if constexpr (QTypeTraits::is_dereferenceable_v<T>) {
            return value();
        } else if constexpr (std::is_pointer_v<T>) {
            value();
            return this->val;
        } else {
            return;
        }
    }

    parameter_type operator*() const
    {
        return value();
    }

    operator parameter_type() const
    {
        return value();
    }

    void setValue(parameter_type t)
    {
        auto *bd = qGetBindingStorage(owner())->bindingData(this);
        if (bd)
            bd->removeBinding();
        if (this->val == t)
            return;
        this->val = t;
        notify(bd);
    }

    void setValue(rvalue_ref t)
    {
        auto *bd = qGetBindingStorage(owner())->bindingData(this);
        if (bd)
            bd->removeBinding();
        if (this->val == t)
            return;
        this->val = std::move(t);
        notify(bd);
    }

    QObjectBindableProperty &operator=(rvalue_ref newValue)
    {
        setValue(std::move(newValue));
        return *this;
    }

    QObjectBindableProperty &operator=(parameter_type newValue)
    {
        setValue(newValue);
        return *this;
    }

    QPropertyBinding<T> setBinding(const QPropertyBinding<T> &newBinding)
    {
        QtPrivate::QPropertyBindingData *bd = qGetBindingStorage(owner())->bindingData(this, true);
        QUntypedPropertyBinding oldBinding(bd->setBinding(newBinding, this, HasSignal ? &signalCallBack : nullptr));
        notify(bd);
        return static_cast<QPropertyBinding<T> &>(oldBinding);
    }

    bool setBinding(const QUntypedPropertyBinding &newBinding)
    {
        if (!newBinding.isNull() && newBinding.valueMetaType().id() != qMetaTypeId<T>())
            return false;
        setBinding(static_cast<const QPropertyBinding<T> &>(newBinding));
        return true;
    }

#ifndef Q_CLANG_QDOC
    template <typename Functor>
    QPropertyBinding<T> setBinding(Functor &&f,
                                   const QPropertyBindingSourceLocation &location = QT_PROPERTY_DEFAULT_BINDING_LOCATION,
                                   std::enable_if_t<std::is_invocable_v<Functor>> * = nullptr)
    {
        return setBinding(Qt::makePropertyBinding(std::forward<Functor>(f), location));
    }
#else
    template <typename Functor>
    QPropertyBinding<T> setBinding(Functor f);
#endif

    bool hasBinding() const
    {
        auto *bd = qGetBindingStorage(owner())->bindingData(this);
        return bd && bd->binding() != nullptr;
    }

    void markDirty() {
        QBindingStorage *storage = qGetBindingStorage(owner());
        auto bd = storage->bindingData(this, /*create=*/false);
        if (bd) { // if we have no BindingData, nobody can listen anyway
            bd->markDirty();
            notify(bd);
        }
    }

    QPropertyBinding<T> binding() const
    {
        auto *bd = qGetBindingStorage(owner())->bindingData(this);
        return static_cast<QPropertyBinding<T> &&>(QUntypedPropertyBinding(bd ? bd->binding() : nullptr));
    }

    QPropertyBinding<T> takeBinding()
    {
        return setBinding(QPropertyBinding<T>());
    }

    template<typename Functor>
    QPropertyChangeHandler<Functor> onValueChanged(Functor f)
    {
        static_assert(std::is_invocable_v<Functor>, "Functor callback must be callable without any parameters");
        return QPropertyChangeHandler<Functor>(*this, f);
    }

    template<typename Functor>
    QPropertyChangeHandler<Functor> subscribe(Functor f)
    {
        static_assert(std::is_invocable_v<Functor>, "Functor callback must be callable without any parameters");
        f();
        return onValueChanged(f);
    }

    const QtPrivate::QPropertyBindingData &bindingData() const
    {
        auto *storage = const_cast<QBindingStorage *>(qGetBindingStorage(owner()));
        return *storage->bindingData(const_cast<ThisType *>(this), true);
    }
private:
    void notify(const QtPrivate::QPropertyBindingData *binding)
    {
        if (binding)
            binding->notifyObservers(this);
        if constexpr (HasSignal)
            (owner()->*Signal)();
    }
};

#define Q_OBJECT_BINDABLE_PROPERTY3(Class, Type, name) \
    static constexpr size_t _qt_property_##name##_offset() { \
        return offsetof(Class, name); \
    } \
    QObjectBindableProperty<Class, Type, Class::_qt_property_##name##_offset, nullptr> name;

#define Q_OBJECT_BINDABLE_PROPERTY4(Class, Type, name, Signal) \
    static constexpr size_t _qt_property_##name##_offset() { \
        return offsetof(Class, name); \
    } \
    QObjectBindableProperty<Class, Type, Class::_qt_property_##name##_offset, Signal> name;

#define Q_OBJECT_BINDABLE_PROPERTY(...) \
    QT_WARNING_PUSH QT_WARNING_DISABLE_INVALID_OFFSETOF \
    QT_OVERLOADED_MACRO(Q_OBJECT_BINDABLE_PROPERTY, __VA_ARGS__) \
    QT_WARNING_POP

#define Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS4(Class, Type, name, value)                            \
    static constexpr size_t _qt_property_##name##_offset()                                         \
    {                                                                                              \
        return offsetof(Class, name);                                                              \
    }                                                                                              \
    QObjectBindableProperty<Class, Type, Class::_qt_property_##name##_offset, nullptr> name =      \
            QObjectBindableProperty<Class, Type, Class::_qt_property_##name##_offset, nullptr>(    \
                    value);

#define Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS5(Class, Type, name, value, Signal)                    \
    static constexpr size_t _qt_property_##name##_offset()                                         \
    {                                                                                              \
        return offsetof(Class, name);                                                              \
    }                                                                                              \
    QObjectBindableProperty<Class, Type, Class::_qt_property_##name##_offset, Signal> name =       \
            QObjectBindableProperty<Class, Type, Class::_qt_property_##name##_offset, Signal>(     \
                    value);

#define Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(...)                                                  \
    QT_WARNING_PUSH QT_WARNING_DISABLE_INVALID_OFFSETOF \
    QT_OVERLOADED_MACRO(Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS, __VA_ARGS__) \
    QT_WARNING_POP

template<typename Class, typename T, auto Offset, auto Getter>
class QObjectComputedProperty : public QUntypedPropertyData
{
    Class *owner()
    {
        char *that = reinterpret_cast<char *>(this);
        return reinterpret_cast<Class *>(that - QtPrivate::detail::getOffset(Offset));
    }
    const Class *owner() const
    {
        char *that = const_cast<char *>(reinterpret_cast<const char *>(this));
        return reinterpret_cast<Class *>(that - QtPrivate::detail::getOffset(Offset));
    }

public:
    using value_type = T;
    using parameter_type = T;

    QObjectComputedProperty() = default;

    parameter_type value() const
    {
        qGetBindingStorage(owner())->maybeUpdateBindingAndRegister(this);
        return (owner()->*Getter)();
    }

    std::conditional_t<QTypeTraits::is_dereferenceable_v<T>, parameter_type, void>
    operator->() const
    {
        if constexpr (QTypeTraits::is_dereferenceable_v<T>)
            return value();
        else
            return;
    }

    parameter_type operator*() const
    {
        return value();
    }

    operator parameter_type() const
    {
        return value();
    }

    constexpr bool hasBinding() const { return false; }

    template<typename Functor>
    QPropertyChangeHandler<Functor> onValueChanged(Functor f)
    {
        static_assert(std::is_invocable_v<Functor>, "Functor callback must be callable without any parameters");
        return QPropertyChangeHandler<Functor>(*this, f);
    }

    template<typename Functor>
    QPropertyChangeHandler<Functor> subscribe(Functor f)
    {
        static_assert(std::is_invocable_v<Functor>, "Functor callback must be callable without any parameters");
        f();
        return onValueChanged(f);
    }

    QtPrivate::QPropertyBindingData &bindingData() const
    {
        auto *storage = const_cast<QBindingStorage *>(qGetBindingStorage(owner()));
        return *storage->bindingData(const_cast<QObjectComputedProperty *>(this), true);
    }

    void markDirty() {
        // computed property can't store a binding, so there's nothing to mark
        auto *storage = const_cast<QBindingStorage *>(qGetBindingStorage(owner()));
        auto bd = storage->bindingData(const_cast<QObjectComputedProperty *>(this), false);
        if (bd)
            bindingData().notifyObservers(this);
    }

private:
};

#define Q_OBJECT_COMPUTED_PROPERTY(Class, Type, name,  ...) \
    static constexpr size_t _qt_property_##name##_offset() { \
        QT_WARNING_PUSH QT_WARNING_DISABLE_INVALID_OFFSETOF \
        return offsetof(Class, name); \
        QT_WARNING_POP \
    } \
    QObjectComputedProperty<Class, Type, Class::_qt_property_##name##_offset, __VA_ARGS__> name;

QT_END_NAMESPACE

#endif // QPROPERTY_H
