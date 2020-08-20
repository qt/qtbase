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

#ifndef QPROPERTYPRIVATE_H
#define QPROPERTYPRIVATE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qapplication_*.cpp, qwidget*.cpp and qfiledialog.cpp.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>
#include <QtCore/qtaggedpointer.h>
#include <QtCore/qmetatype.h>

#include <functional>

QT_BEGIN_NAMESPACE

namespace QtPrivate {
// QPropertyBindingPrivatePtr operates on a RefCountingMixin solely so that we can inline
// the constructor and copy constructor
struct RefCounted {
    int ref = 0;
    void addRef() {++ref;}
    bool deref() {--ref; return ref;}
};
}

class QPropertyBindingPrivate;
class QPropertyBindingPrivatePtr
{
public:
    using T = QtPrivate::RefCounted;
    T &operator*() const { return *d; }
    T *operator->() noexcept { return d; }
    T *operator->() const noexcept { return d; }
    explicit operator T *() { return d; }
    explicit operator const T *() const noexcept { return d; }
    T *data() const noexcept { return d; }
    T *get() const noexcept { return d; }
    const T *constData() const noexcept { return d; }
    T *take() noexcept { T *x = d; d = nullptr; return x; }

    QPropertyBindingPrivatePtr() noexcept : d(nullptr) { }
    ~QPropertyBindingPrivatePtr()
    {
        if (d && (--d->ref == 0))
            destroyAndFreeMemory();
    }
    Q_CORE_EXPORT void destroyAndFreeMemory();

    explicit QPropertyBindingPrivatePtr(T *data) noexcept : d(data) { if (d) d->addRef(); }
    QPropertyBindingPrivatePtr(const QPropertyBindingPrivatePtr &o) noexcept
        : d(o.d) { if (d) d->addRef(); }

    void reset(T *ptr = nullptr) noexcept;

    QPropertyBindingPrivatePtr &operator=(const QPropertyBindingPrivatePtr &o) noexcept
    {
        reset(o.d);
        return *this;
    }
    QPropertyBindingPrivatePtr &operator=(T *o) noexcept
    {
        reset(o);
        return *this;
    }
    QPropertyBindingPrivatePtr(QPropertyBindingPrivatePtr &&o) noexcept : d(qExchange(o.d, nullptr)) {}
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(QPropertyBindingPrivatePtr)

    operator bool () const noexcept { return d != nullptr; }
    bool operator!() const noexcept { return d == nullptr; }

    void swap(QPropertyBindingPrivatePtr &other) noexcept
    { qSwap(d, other.d); }

    friend bool operator==(const QPropertyBindingPrivatePtr &p1, const QPropertyBindingPrivatePtr &p2) noexcept
    { return p1.d == p2.d; }
    friend bool operator!=(const QPropertyBindingPrivatePtr &p1, const QPropertyBindingPrivatePtr &p2) noexcept
    { return p1.d != p2.d; }
    friend bool operator==(const QPropertyBindingPrivatePtr &p1, const T *ptr) noexcept
    { return p1.d == ptr; }
    friend bool operator!=(const QPropertyBindingPrivatePtr &p1, const T *ptr) noexcept
    { return p1.d != ptr; }
    friend bool operator==(const T *ptr, const QPropertyBindingPrivatePtr &p2) noexcept
    { return ptr == p2.d; }
    friend bool operator!=(const T *ptr, const QPropertyBindingPrivatePtr &p2) noexcept
    { return ptr != p2.d; }
    friend bool operator==(const QPropertyBindingPrivatePtr &p1, std::nullptr_t) noexcept
    { return !p1; }
    friend bool operator!=(const QPropertyBindingPrivatePtr &p1, std::nullptr_t) noexcept
    { return p1; }
    friend bool operator==(std::nullptr_t, const QPropertyBindingPrivatePtr &p2) noexcept
    { return !p2; }
    friend bool operator!=(std::nullptr_t, const QPropertyBindingPrivatePtr &p2) noexcept
    { return p2; }

private:
    QtPrivate::RefCounted *d;
};


class QUntypedPropertyBinding;
class QPropertyBindingPrivate;
struct QPropertyBindingDataPointer;

class QUntypedPropertyData
{
public:
    // sentinel to check whether a class inherits QUntypedPropertyData
    struct InheritsQUntypedPropertyData {};
};

template <typename T>
class QPropertyData;

namespace QtPrivate {
struct BindingEvaluationState;

struct BindingFunctionVTable
{
    using CallFn = bool(*)(QMetaType, QUntypedPropertyData *, void *);
    using DtorFn = void(*)(void *);
    using MoveCtrFn = void(*)(void *, void *);
    const CallFn call;
    const DtorFn destroy;
    const MoveCtrFn moveConstruct;
    const qsizetype size;

    template<typename Callable, typename PropertyType=void>
    static constexpr BindingFunctionVTable createFor()
    {
        static_assert (alignof(Callable) <= alignof(std::max_align_t), "Bindings do not support overaligned functors!");
        return {
            /*call=*/[](QMetaType metaType, QUntypedPropertyData *dataPtr, void *f){
                if constexpr (!std::is_invocable_v<Callable>) {
                    // we got an untyped callable
                    static_assert (std::is_invocable_r_v<bool, Callable, QMetaType, QUntypedPropertyData *> );
                    auto untypedEvaluationFunction = static_cast<Callable *>(f);
                    return std::invoke(*untypedEvaluationFunction, metaType, dataPtr);
                } else {
                    Q_UNUSED(metaType);
                    QPropertyData<PropertyType> *propertyPtr = static_cast<QPropertyData<PropertyType> *>(dataPtr);
                    // That is allowed by POSIX even if Callable is a function pointer
                    auto evaluationFunction = static_cast<Callable *>(f);
                    PropertyType newValue = std::invoke(*evaluationFunction);
                    if constexpr (QTypeTraits::has_operator_equal_v<PropertyType>) {
                        if (newValue == propertyPtr->valueBypassingBindings())
                            return false;
                    }
                    propertyPtr->setValueBypassingBindings(std::move(newValue));
                    return true;
                }
            },
            /*destroy*/[](void *f){ static_cast<Callable *>(f)->~Callable(); },
            /*moveConstruct*/[](void *addr, void *other){
                new (addr) Callable(std::move(*static_cast<Callable *>(other)));
            },
            /*size*/sizeof(Callable)
        };
    }
};

template<typename Callable, typename PropertyType=void>
inline constexpr BindingFunctionVTable bindingFunctionVTable = BindingFunctionVTable::createFor<Callable, PropertyType>();


// writes binding result into dataPtr
struct QPropertyBindingFunction {
    const QtPrivate::BindingFunctionVTable *vtable;
    void *functor;
};

using QPropertyObserverCallback = void (*)(QUntypedPropertyData *);
using QPropertyBindingWrapper = bool(*)(QMetaType, QUntypedPropertyData *dataPtr, QPropertyBindingFunction);

class Q_CORE_EXPORT QPropertyBindingData
{
    // Mutable because the address of the observer of the currently evaluating binding is stored here, for
    // notification later when the value changes.
    mutable quintptr d_ptr = 0;
    friend struct QT_PREPEND_NAMESPACE(QPropertyBindingDataPointer);
    Q_DISABLE_COPY(QPropertyBindingData)
public:
    QPropertyBindingData() = default;
    QPropertyBindingData(QPropertyBindingData &&other);
    QPropertyBindingData &operator=(QPropertyBindingData &&other) = delete;
    ~QPropertyBindingData();

    static inline constexpr quintptr BindingBit = 0x1; // Is d_ptr pointing to a binding (1) or list of notifiers (0)?

    bool hasBinding() const { return d_ptr & BindingBit; }

    QUntypedPropertyBinding setBinding(const QUntypedPropertyBinding &newBinding,
                                       QUntypedPropertyData *propertyDataPtr,
                                       QPropertyObserverCallback staticObserverCallback = nullptr,
                                       QPropertyBindingWrapper bindingWrapper = nullptr);

    QPropertyBindingPrivate *binding() const
    {
        if (d_ptr & BindingBit)
            return reinterpret_cast<QPropertyBindingPrivate*>(d_ptr - BindingBit);
        return nullptr;

    }

    void evaluateIfDirty(const QUntypedPropertyData *property) const;
    void markDirty();

    void removeBinding()
    {
        if (hasBinding())
            removeBinding_helper();
    }

    void registerWithCurrentlyEvaluatingBinding(QtPrivate::BindingEvaluationState *currentBinding) const
    {
        if (!currentBinding)
            return;
        registerWithCurrentlyEvaluatingBinding_helper(currentBinding);
    }
    void registerWithCurrentlyEvaluatingBinding() const;
    void notifyObservers(QUntypedPropertyData *propertyDataPtr) const;
private:
    void registerWithCurrentlyEvaluatingBinding_helper(BindingEvaluationState *currentBinding) const;
    void removeBinding_helper();
};

template <typename T, typename Tag>
class QTagPreservingPointerToPointer
{
public:
    constexpr QTagPreservingPointerToPointer() = default;

    QTagPreservingPointerToPointer(T **ptr)
        : d(reinterpret_cast<quintptr*>(ptr))
    {}

    QTagPreservingPointerToPointer<T, Tag> &operator=(T **ptr)
    {
        d = reinterpret_cast<quintptr *>(ptr);
        return *this;
    }

    QTagPreservingPointerToPointer<T, Tag> &operator=(QTaggedPointer<T, Tag> *ptr)
    {
        d = reinterpret_cast<quintptr *>(ptr);
        return *this;
    }

    void clear()
    {
        d = nullptr;
    }

    void setPointer(T *ptr)
    {
        *d = reinterpret_cast<quintptr>(ptr) | (*d & QTaggedPointer<T, Tag>::tagMask());
    }

    T *get() const
    {
        return reinterpret_cast<T*>(*d & QTaggedPointer<T, Tag>::pointerMask());
    }

    explicit operator bool() const
    {
        return d != nullptr;
    }

private:
    quintptr *d = nullptr;
};

namespace detail {
    template <typename F>
    struct ExtractClassFromFunctionPointer;

    template<typename T, typename C>
    struct ExtractClassFromFunctionPointer<T C::*> { using Class = C; };

    constexpr size_t getOffset(size_t o)
    {
        return o;
    }
    constexpr size_t getOffset(size_t (*offsetFn)())
    {
        return offsetFn();
    }
}

} // namespace QtPrivate

QT_END_NAMESPACE

#endif // QPROPERTYPRIVATE_H
