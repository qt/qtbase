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
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>
#include <QtCore/qmetatype.h>
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

template <typename T> class QProperty;

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

    QPropertyBindingError(Type type = NoError);
    QPropertyBindingError(const QPropertyBindingError &other);
    QPropertyBindingError &operator=(const QPropertyBindingError &other);
    QPropertyBindingError(QPropertyBindingError &&other);
    QPropertyBindingError &operator=(QPropertyBindingError &&other);
    ~QPropertyBindingError();

    Type type() const;
    void setDescription(const QString &description);
    QString description() const;
    QPropertyBindingSourceLocation location() const;

private:
    QSharedDataPointer<QPropertyBindingErrorPrivate> d;
};

class Q_CORE_EXPORT QUntypedPropertyBinding
{
public:
    using BindingEvaluationResult = QPropertyBindingError;
    // writes binding result into dataPtr
    using BindingEvaluationFunction = std::function<BindingEvaluationResult(const QMetaType &metaType, void *dataPtr)>;

    QUntypedPropertyBinding();
    QUntypedPropertyBinding(const QMetaType &metaType, BindingEvaluationFunction function, const QPropertyBindingSourceLocation &location);
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
    friend class QtPrivate::QPropertyBase;
    friend class QPropertyBindingPrivate;
    template <typename> friend class QPropertyBinding;
    QPropertyBindingPrivatePtr d;
};

template <typename PropertyType>
class QPropertyBinding : public QUntypedPropertyBinding
{
    template <typename Functor>
    struct BindingAdaptor
    {
        Functor impl;
        QUntypedPropertyBinding::BindingEvaluationResult operator()(const QMetaType &/*metaType*/, void *dataPtr)
        {
            std::variant<PropertyType, QPropertyBindingError> result(impl());
            if (auto errorPtr = std::get_if<QPropertyBindingError>(&result))
                return *errorPtr;

            if (auto valuePtr = std::get_if<PropertyType>(&result)) {
                PropertyType *propertyPtr = reinterpret_cast<PropertyType *>(dataPtr);
                *propertyPtr = std::move(*valuePtr);
                return {};
            }

            return {};
        }
    };

public:
    QPropertyBinding() = default;

    template<typename Functor>
    QPropertyBinding(Functor &&f, const QPropertyBindingSourceLocation &location)
        : QUntypedPropertyBinding(QMetaType::fromType<PropertyType>(), BindingAdaptor<Functor>{std::forward<Functor>(f)}, location)
    {}

    QPropertyBinding(const QProperty<PropertyType> &property)
        : QUntypedPropertyBinding(property.d.priv.binding())
    {}

private:
    // Internal
    explicit QPropertyBinding(const QUntypedPropertyBinding &binding)
        : QUntypedPropertyBinding(binding)
    {}
    friend class QProperty<PropertyType>;
};

namespace QtPrivate {
    template<typename... Ts>
    constexpr auto is_variant_v = false;
    template<typename... Ts>
    constexpr auto is_variant_v<std::variant<Ts...>> = true;
}

namespace Qt {
    template <typename Functor>
    auto makePropertyBinding(Functor &&f, const QPropertyBindingSourceLocation &location = QT_PROPERTY_DEFAULT_BINDING_LOCATION,
                             std::enable_if_t<std::is_invocable_v<Functor>> * = 0)
    {
        if constexpr (QtPrivate::is_variant_v<std::invoke_result_t<Functor>>) {
            return QPropertyBinding<std::variant_alternative_t<0, std::invoke_result_t<Functor>>>(std::forward<Functor>(f), location);
        } else {
            return QPropertyBinding<std::invoke_result_t<Functor>>(std::forward<Functor>(f), location);
        }
        // Work around bogus warning
        Q_UNUSED(QtPrivate::is_variant_v<bool>)
    }
}

struct QPropertyBasePointer;

template <typename T>
class QProperty
{
public:
    using value_type = T;

    QProperty() = default;
    explicit QProperty(const T &initialValue) : d(initialValue) {}
    explicit QProperty(T &&initialValue) : d(std::move(initialValue)) {}
    QProperty(QProperty &&other) : d(std::move(other.d)) { notify(); }
    QProperty &operator=(QProperty &&other) { d = std::move(other.d); notify(); return *this; }
    QProperty(const QPropertyBinding<T> &binding)
        : QProperty()
    { operator=(binding); }
    QProperty(QPropertyBinding<T> &&binding)
        : QProperty()
    { operator=(std::move(binding)); }
#ifndef Q_CLANG_QDOC
    template <typename Functor>
    explicit QProperty(Functor &&f, const QPropertyBindingSourceLocation &location = QT_PROPERTY_DEFAULT_BINDING_LOCATION,
                       typename std::enable_if_t<std::is_invocable_r_v<T, Functor&>> * = 0)
        : QProperty(QPropertyBinding<T>(std::forward<Functor>(f), location))
    {}
#else
    template <typename Functor>
    explicit QProperty(Functor &&f);
#endif
    ~QProperty() = default;

    T value() const
    {
        if (d.priv.hasBinding())
            d.priv.evaluateIfDirty();
        d.priv.registerWithCurrentlyEvaluatingBinding();
        return d.getValue();
    }

    operator T() const
    {
        return value();
    }

    void setValue(T &&newValue)
    {
        d.priv.removeBinding();
        if (d.setValueAndReturnTrueIfChanged(std::move(newValue)))
            notify();
    }

    void setValue(const T &newValue)
    {
        d.priv.removeBinding();
        if (d.setValueAndReturnTrueIfChanged(newValue))
            notify();
    }

    QProperty<T> &operator=(T &&newValue)
    {
        setValue(std::move(newValue));
        return *this;
    }

    QProperty<T> &operator=(const T &newValue)
    {
        setValue(newValue);
        return *this;
    }

    QProperty<T> &operator=(const QPropertyBinding<T> &newBinding)
    {
        setBinding(newBinding);
        return *this;
    }

    QProperty<T> &operator=(QPropertyBinding<T> &&newBinding)
    {
        setBinding(std::move(newBinding));
        return *this;
    }

    QPropertyBinding<T> setBinding(const QPropertyBinding<T> &newBinding)
    {
        QPropertyBinding<T> oldBinding(d.priv.setBinding(newBinding, &d));
        notify();
        return oldBinding;
    }

    QPropertyBinding<T> setBinding(QPropertyBinding<T> &&newBinding)
    {
        QPropertyBinding<T> b(std::move(newBinding));
        QPropertyBinding<T> oldBinding(d.priv.setBinding(b, &d));
        notify();
        return oldBinding;
    }

    bool setBinding(const QUntypedPropertyBinding &newBinding)
    {
        if (newBinding.valueMetaType().id() != qMetaTypeId<T>())
            return false;
        d.priv.setBinding(newBinding, &d);
        notify();
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

    bool hasBinding() const { return d.priv.hasBinding(); }

    QPropertyBinding<T> binding() const
    {
        return QPropertyBinding<T>(*this);
    }

    QPropertyBinding<T> takeBinding()
    {
        return QPropertyBinding<T>(d.priv.setBinding(QUntypedPropertyBinding(), &d));
    }

    template<typename Functor>
    QPropertyChangeHandler<Functor> onValueChanged(Functor f);
    template<typename Functor>
    QPropertyChangeHandler<Functor> subscribe(Functor f);

private:
    void notify()
    {
        d.priv.notifyObservers(&d);
    }

    Q_DISABLE_COPY(QProperty)

    friend struct QPropertyBasePointer;
    friend class QPropertyBinding<T>;
    friend class QPropertyObserver;
    // Mutable because querying for the value may require evalating the binding expression, calling
    // non-const functions on QPropertyBase.
    mutable QtPrivate::QPropertyValueStorage<T> d;
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

struct QPropertyObserverPrivate;
struct QPropertyObserverPointer;

class Q_CORE_EXPORT QPropertyObserver
{
public:
    // Internal
    enum ObserverTag {
        ObserverNotifiesBinding = 0x0,
        ObserverNotifiesChangeHandler = 0x1,
    };
    Q_DECLARE_FLAGS(ObserverTags, ObserverTag)

    QPropertyObserver();
    QPropertyObserver(QPropertyObserver &&other);
    QPropertyObserver &operator=(QPropertyObserver &&other);
    ~QPropertyObserver();

    template <typename PropertyType>
    void setSource(const QProperty<PropertyType> &property)
    { setSource(property.d.priv); }

protected:
    QPropertyObserver(void (*callback)(QPropertyObserver*, void *));

private:
    void setSource(QtPrivate::QPropertyBase &property);

    QTaggedPointer<QPropertyObserver, ObserverTags> next;
    // prev is a pointer to the "next" element within the previous node, or to the "firstObserverPtr" if it is the
    // first node.
    QtPrivate::QTagPreservingPointerToPointer<QPropertyObserver, ObserverTags> prev;

    union {
        QPropertyBindingPrivate *bindingToMarkDirty = nullptr;
        void (*changeHandler)(QPropertyObserver*, void *);
    };

    QPropertyObserver(const QPropertyObserver &) = delete;
    QPropertyObserver &operator=(const QPropertyObserver &) = delete;

    friend struct QPropertyObserverPointer;
    friend struct QPropertyBasePointer;
    friend class QPropertyBindingPrivate;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QPropertyObserver::ObserverTags)

template <typename Functor>
class QPropertyChangeHandler : public QPropertyObserver
{
    Functor m_handler;
public:
    QPropertyChangeHandler(Functor handler)
        : QPropertyObserver([](QPropertyObserver *self, void *) {
              auto This = static_cast<QPropertyChangeHandler<Functor>*>(self);
              This->m_handler();
          })
        , m_handler(handler)
    {
    }

    template <typename PropertyType>
    QPropertyChangeHandler(const QProperty<PropertyType> &property, Functor handler)
        : QPropertyObserver([](QPropertyObserver *self, void *) {
              auto This = static_cast<QPropertyChangeHandler<Functor>*>(self);
              This->m_handler();
          })
        , m_handler(handler)
    {
        setSource(property);
    }
};

template <typename T>
template<typename Functor>
QPropertyChangeHandler<Functor> QProperty<T>::onValueChanged(Functor f)
{
#if defined(__cpp_lib_is_invocable) && (__cpp_lib_is_invocable >= 201703L)
    static_assert(std::is_invocable_v<Functor>, "Functor callback must be callable without any parameters");
#endif
    return QPropertyChangeHandler<Functor>(*this, f);
}

template <typename T>
template<typename Functor>
QPropertyChangeHandler<Functor> QProperty<T>::subscribe(Functor f)
{
#if defined(__cpp_lib_is_invocable) && (__cpp_lib_is_invocable >= 201703L)
    static_assert(std::is_invocable_v<Functor>, "Functor callback must be callable without any parameters");
#endif
    f();
    return onValueChanged(f);
}

template <auto propertyMember, auto callbackMember>
struct QPropertyMemberChangeHandler;

template<typename Class, typename PropertyType, PropertyType Class::* PropertyMember, void(Class::*Callback)()>
struct QPropertyMemberChangeHandler<PropertyMember, Callback> : public QPropertyObserver
{
    QPropertyMemberChangeHandler(Class *obj)
        : QPropertyObserver(notify)
    {
        setSource(obj->*PropertyMember);
    }

    static void notify(QPropertyObserver *, void *propertyDataPtr)
    {
        // memberOffset is the offset of the QProperty<> member within the class. We get the absolute address
        // of that member and subtracting the relative offset gives us the address of the class instance.
        const size_t memberOffset = reinterpret_cast<size_t>(&(static_cast<Class *>(nullptr)->*PropertyMember));
        Class *obj = reinterpret_cast<Class *>(reinterpret_cast<char *>(propertyDataPtr) - memberOffset);
        (obj->*Callback)();
    }
};


QT_END_NAMESPACE

#endif // QPROPERTY_H
