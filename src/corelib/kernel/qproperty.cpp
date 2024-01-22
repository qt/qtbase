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
#include <QScopeGuard>
#include <QtCore/qloggingcategory.h>
#include <QThread>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQPropertyBinding, "qt.qproperty.binding");

using namespace QtPrivate;

void QPropertyBindingPrivatePtr::destroyAndFreeMemory()
{
    QPropertyBindingPrivate::destroyAndFreeMemory(static_cast<QPropertyBindingPrivate *>(d));
}

void QPropertyBindingPrivatePtr::reset(QtPrivate::RefCounted *ptr) noexcept
{
    if (ptr != d) {
        if (ptr)
            ptr->ref++;
        auto *old = qExchange(d, ptr);
        if (old && (--old->ref == 0))
            QPropertyBindingPrivate::destroyAndFreeMemory(static_cast<QPropertyBindingPrivate *>(d));
    }
}


void QPropertyBindingDataPointer::addObserver(QPropertyObserver *observer)
{
    if (auto *b = binding()) {
        observer->prev = &b->firstObserver.ptr;
        observer->next = b->firstObserver.ptr;
        if (observer->next)
            observer->next->prev = &observer->next;
        b->firstObserver.ptr = observer;
    } else {
        auto &d = ptr->d_ref();
        Q_ASSERT(!(d & QPropertyBindingData::BindingBit));
        auto firstObserver = reinterpret_cast<QPropertyObserver*>(d);
        observer->prev = reinterpret_cast<QPropertyObserver**>(&d);
        observer->next = firstObserver;
        if (observer->next)
            observer->next->prev = &observer->next;
        d = reinterpret_cast<quintptr>(observer);
    }
}

/*!
    \internal

    QPropertyDelayedNotifications is used to manage delayed notifications in grouped property updates.
    It acts as a pool allocator for QPropertyProxyBindingData, and has methods to manage delayed
    notifications.

    \sa beginPropertyUpdateGroup, endPropertyUpdateGroup
*/
struct QPropertyDelayedNotifications
{
    // we can't access the dynamic page size as we need a constant value
    // use 4096 as a sensible default
    static constexpr inline auto PageSize = 4096;
    int ref = 0;
    QPropertyDelayedNotifications *next = nullptr; // in case we have more than size dirty properties...
    qsizetype used = 0;
    // Size chosen to avoid allocating more than one page of memory, while still ensuring
    // that we can store many delayed properties without doing further allocations
    static constexpr qsizetype size = (PageSize - 3*sizeof(void *))/sizeof(QPropertyProxyBindingData);
    QPropertyProxyBindingData delayedProperties[size];

    /*!
        \internal
        This method is called when a property attempts to notify its observers while inside of a
        property update group. Instead of actually notifying, it replaces \a bindingData's d_ptr
        with a QPropertyProxyBindingData.
        \a bindingData and \a propertyData are the binding data and property data of the property
        whose notify call gets delayed.
        \sa QPropertyBindingData::notifyObservers
     */
    void addProperty(const QPropertyBindingData *bindingData, QUntypedPropertyData *propertyData) {
        if (bindingData->isNotificationDelayed())
            return;
        auto *data = this;
        while (data->used == size) {
            if (!data->next)
                // add a new page
                data->next = new QPropertyDelayedNotifications;
            data = data->next;
        }
        auto *delayed = data->delayedProperties + data->used;
        *delayed = QPropertyProxyBindingData { bindingData->d_ptr, bindingData, propertyData };
        ++data->used;
        // preserve the binding bit for faster access
        quintptr bindingBit = bindingData->d_ptr & QPropertyBindingData::BindingBit;
        bindingData->d_ptr = reinterpret_cast<quintptr>(delayed) | QPropertyBindingData::DelayedNotificationBit | bindingBit;
        Q_ASSERT(bindingData->d_ptr > 3);
        if (!bindingBit) {
            if (auto observer = reinterpret_cast<QPropertyObserver *>(delayed->d_ptr))
                observer->prev = reinterpret_cast<QPropertyObserver **>(&delayed->d_ptr);
        }
    }

    /*!
        \internal
        Called in Qt::endPropertyUpdateGroup. For the QPropertyProxyBindingData at position
        \a index, it
        \list
            \li restores the original binding data that was modified in addProperty and
            \li evaluates any bindings which depend on properties that were changed inside
                the group.
        \endlist
        Change notifications are sent later with notify (following the logic of separating
        binding updates and notifications used in non-deferred updates).
     */
    void evaluateBindings(qsizetype index, QBindingStatus *status) {
        auto *delayed = delayedProperties + index;
        auto *bindingData = delayed->originalBindingData;
        if (!bindingData)
            return;

        bindingData->d_ptr = delayed->d_ptr;
        Q_ASSERT(!(bindingData->d_ptr & QPropertyBindingData::DelayedNotificationBit));
        if (!bindingData->hasBinding()) {
            if (auto observer = reinterpret_cast<QPropertyObserver *>(bindingData->d_ptr))
                observer->prev = reinterpret_cast<QPropertyObserver **>(&bindingData->d_ptr);
        }

        QPropertyBindingDataPointer bindingDataPointer{bindingData};
        QPropertyObserverPointer observer = bindingDataPointer.firstObserver();
        if (observer)
            observer.evaluateBindings(status);
    }

    /*!
        \internal
        Called in Qt::endPropertyUpdateGroup. For the QPropertyProxyBindingData at position
        \a i, it
        \list
            \li resets the proxy binding data and
            \li sends any pending notifications.
        \endlist
     */
    void notify(qsizetype index) {
        auto *delayed = delayedProperties + index;
        auto *bindingData = delayed->originalBindingData;
        if (!bindingData)
            return;

        delayed->originalBindingData = nullptr;
        delayed->d_ptr = 0;

        QPropertyBindingDataPointer bindingDataPointer{bindingData};
        QPropertyObserverPointer observer = bindingDataPointer.firstObserver();
        if (observer)
            observer.notify(delayed->propertyData);
    }
};

static thread_local QBindingStatus bindingStatus;

/*!
    \since 6.2

    \relates QProperty

    Marks the beginning of a property update group. Inside this group,
    changing a property does neither immediately update any dependent properties
    nor does it trigger change notifications.
    Those are instead deferred until the group is ended by a call to endPropertyUpdateGroup.

    Groups can be nested. In that case, the deferral ends only after the outermost group has been
    ended.

    \note Change notifications are only send after all property values affected by the group have
    been updated to their new values. This allows re-establishing a  class invariant if multiple
    properties need to be updated, preventing any external observer from noticing an inconsistent
    state.

    \sa Qt::endPropertyUpdateGroup
*/
void Qt::beginPropertyUpdateGroup()
{
    QPropertyDelayedNotifications *& groupUpdateData = bindingStatus.groupUpdateData;
    if (!groupUpdateData)
        groupUpdateData = new QPropertyDelayedNotifications;
    ++groupUpdateData->ref;
}

/*!
    \since 6.2
    \relates QProperty

    Ends a property update group. If the outermost group has been ended, and deferred
    binding evaluations and notifications happen now.

    \warning Calling endPropertyUpdateGroup without a preceding call to beginPropertyUpdateGroup
    results in undefined behavior.

    \sa Qt::beginPropertyUpdateGroup
*/
void Qt::endPropertyUpdateGroup()
{
    auto status = &bindingStatus;
    QPropertyDelayedNotifications *& groupUpdateData = status->groupUpdateData;
    auto *data = groupUpdateData;
    Q_ASSERT(data->ref);
    if (--data->ref)
        return;
    groupUpdateData = nullptr;
    // update all delayed properties
    auto start = data;
    while (data) {
        for (qsizetype i = 0; i < data->used; ++i)
            data->evaluateBindings(i, status);
        data = data->next;
    }
    // notify all delayed properties
    data = start;
    while (data) {
        for (qsizetype i = 0; i < data->used; ++i)
            data->notify(i);
        auto *next = data->next;
        delete data;
        data = next;
    }
}

// check everything stored in QPropertyBindingPrivate's union is trivially destructible
// (though the compiler would also complain if that weren't the case)
static_assert(std::is_trivially_destructible_v<QPropertyBindingSourceLocation>);
static_assert(std::is_trivially_destructible_v<std::byte[sizeof(QPropertyBindingSourceLocation)]>);

QPropertyBindingPrivate::~QPropertyBindingPrivate()
{
    if (firstObserver)
        firstObserver.unlink();
    if (vtable->size)
        vtable->destroy(reinterpret_cast<std::byte *>(this)
                        + QPropertyBindingPrivate::getSizeEnsuringAlignment());
}

void QPropertyBindingPrivate::unlinkAndDeref()
{
    clearDependencyObservers();
    propertyDataPtr = nullptr;
    if (--ref == 0)
        destroyAndFreeMemory(this);
}

void QPropertyBindingPrivate::evaluateRecursive(QBindingStatus *status)
{
    if (!status)
        status = &bindingStatus;
    return evaluateRecursive_inline(status);
}

void QPropertyBindingPrivate::notifyRecursive()
{
    if (!pendingNotify)
        return;
    pendingNotify = false;
    Q_ASSERT(!updating);
    updating = true;
    if (firstObserver) {
        firstObserver.noSelfDependencies(this);
        firstObserver.notify(propertyDataPtr);
    }
    if (hasStaticObserver)
        staticObserverCallback(propertyDataPtr);
    updating = false;
}

/*!
  Constructs a null QUntypedPropertyBinding.

  \sa isNull()
*/
QUntypedPropertyBinding::QUntypedPropertyBinding() = default;

/*!
  \fn template<typename Functor>
  QUntypedPropertyBinding(QMetaType metaType, Functor &&f, const QPropertyBindingSourceLocation &location)

  \internal
*/

/*!
    \internal

    Constructs QUntypedPropertyBinding. Assumes that \a metaType, \a function and \a vtable match.
    Unless a specialization of \c BindingFunctionVTable is used, this function should never be called
    directly.
*/
QUntypedPropertyBinding::QUntypedPropertyBinding(QMetaType metaType, const BindingFunctionVTable *vtable, void *function,
                                                 const QPropertyBindingSourceLocation &location)
{
    std::byte *mem = new std::byte[QPropertyBindingPrivate::getSizeEnsuringAlignment() + vtable->size]();
    d = new(mem) QPropertyBindingPrivate(metaType, vtable, std::move(location));
    vtable->moveConstruct(mem + QPropertyBindingPrivate::getSizeEnsuringAlignment(), function);
}

/*!
    Move-constructs a QUntypedPropertyBinding from \a other.

    \a other is left in a null state.
    \sa isNull()
*/
QUntypedPropertyBinding::QUntypedPropertyBinding(QUntypedPropertyBinding &&other)
    : d(std::move(other.d))
{
}

/*!
    Copy-constructs a QUntypedPropertyBinding from \a other.
*/
QUntypedPropertyBinding::QUntypedPropertyBinding(const QUntypedPropertyBinding &other)
    : d(other.d)
{
}

/*!
    Copy-assigns \a other to this QUntypedPropertyBinding.
*/
QUntypedPropertyBinding &QUntypedPropertyBinding::operator=(const QUntypedPropertyBinding &other)
{
    d = other.d;
    return *this;
}

/*!
    Move-assigns \a other to this QUntypedPropertyBinding.

    \a other is left in a null state.
    \sa isNull
*/
QUntypedPropertyBinding &QUntypedPropertyBinding::operator=(QUntypedPropertyBinding &&other)
{
    d = std::move(other.d);
    return *this;
}

/*!
   \internal
*/
QUntypedPropertyBinding::QUntypedPropertyBinding(QPropertyBindingPrivate *priv)
    : d(priv)
{
}

/*!
  Destroys the QUntypedPropertyBinding.
*/
QUntypedPropertyBinding::~QUntypedPropertyBinding()
{
}

/*!
    Returns \c true if the \c QUntypedPropertyBinding is null.
    This is only true for default-constructed and moved-from instances.

    \sa isNull()
*/
bool QUntypedPropertyBinding::isNull() const
{
    return !d;
}

/*!
    Returns the error state of the binding.

    \sa QPropertyBindingError
*/
QPropertyBindingError QUntypedPropertyBinding::error() const
{
    if (!d)
        return QPropertyBindingError();
    return static_cast<QPropertyBindingPrivate *>(d.get())->bindingError();
}

/*!
    Returns the meta-type of the binding.
    If the QUntypedProperyBinding is null, an invalid QMetaType is returned.
*/
QMetaType QUntypedPropertyBinding::valueMetaType() const
{
    if (!d)
        return QMetaType();
    return static_cast<QPropertyBindingPrivate *>(d.get())->valueMetaType();
}

QPropertyBindingData::~QPropertyBindingData()
{
    QPropertyBindingDataPointer d{this};
    for (auto observer = d.firstObserver(); observer;) {
        auto next = observer.nextObserver();
        observer.unlink();
        observer = next;
    }
    if (auto binding = d.binding())
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

    auto &data = d_ref();
    if (auto *existingBinding = d.binding()) {
        if (existingBinding == newBinding.data())
            return QUntypedPropertyBinding(static_cast<QPropertyBindingPrivate *>(oldBinding.data()));
        if (existingBinding->isUpdating()) {
            existingBinding->setError({QPropertyBindingError::BindingLoop, QStringLiteral("Binding set during binding evaluation!")});
            return QUntypedPropertyBinding(static_cast<QPropertyBindingPrivate *>(oldBinding.data()));
        }
        oldBinding = QPropertyBindingPrivatePtr(existingBinding);
        observer = static_cast<QPropertyBindingPrivate *>(oldBinding.data())->takeObservers();
        static_cast<QPropertyBindingPrivate *>(oldBinding.data())->unlinkAndDeref();
        data = 0;
    } else {
        observer = d.firstObserver();
    }

    if (newBinding) {
        newBinding.data()->addRef();
        data = reinterpret_cast<quintptr>(newBinding.data());
        data |= BindingBit;
        auto newBindingRaw = static_cast<QPropertyBindingPrivate *>(newBinding.data());
        newBindingRaw->setProperty(propertyDataPtr);
        if (observer)
            newBindingRaw->prependObserver(observer);
        newBindingRaw->setStaticObserver(staticObserverCallback, guardCallback);

        newBindingRaw->evaluateRecursive();
        newBindingRaw->notifyRecursive();
    } else if (observer) {
        d.setObservers(observer.ptr);
    } else {
        data = 0;
    }

    if (oldBinding)
        static_cast<QPropertyBindingPrivate *>(oldBinding.data())->detachFromProperty();

    return QUntypedPropertyBinding(static_cast<QPropertyBindingPrivate *>(oldBinding.data()));
}

QPropertyBindingData::QPropertyBindingData(QPropertyBindingData &&other) : d_ptr(std::exchange(other.d_ptr, 0))
{
    QPropertyBindingDataPointer::fixupAfterMove(this);
}

BindingEvaluationState::BindingEvaluationState(QPropertyBindingPrivate *binding, QBindingStatus *status)
    : binding(binding)
{
    Q_ASSERT(status);
    QBindingStatus *s = status;
    // store a pointer to the currentBindingEvaluationState to avoid a TLS lookup in
    // the destructor (as these come with a non zero cost)
    currentState = &s->currentlyEvaluatingBinding;
    previousState = *currentState;
    *currentState = this;
    binding->clearDependencyObservers();
}

CompatPropertySafePoint::CompatPropertySafePoint(QBindingStatus *status, QUntypedPropertyData *property)
    : property(property)
{
    // store a pointer to the currentBindingEvaluationState to avoid a TLS lookup in
    // the destructor (as these come with a non zero cost)
    currentState = &status->currentCompatProperty;
    previousState = *currentState;
    *currentState = this;

    currentlyEvaluatingBindingList = &bindingStatus.currentlyEvaluatingBinding;
    bindingState = *currentlyEvaluatingBindingList;
    *currentlyEvaluatingBindingList = nullptr;
}

QPropertyBindingPrivate *QPropertyBindingPrivate::currentlyEvaluatingBinding()
{
    auto currentState = bindingStatus.currentlyEvaluatingBinding ;
    return currentState ? currentState->binding : nullptr;
}

// ### Unused, kept for BC with 6.0
void QPropertyBindingData::evaluateIfDirty(const QUntypedPropertyData *) const
{
}

void QPropertyBindingData::removeBinding_helper()
{
    QPropertyBindingDataPointer d{this};

    auto *existingBinding = d.binding();
    Q_ASSERT(existingBinding);
    if (existingBinding->isSticky()) {
        return;
    }

    auto observer = existingBinding->takeObservers();
    d_ref() = 0;
    if (observer)
        d.setObservers(observer.ptr);
    existingBinding->unlinkAndDeref();
}

void QPropertyBindingData::registerWithCurrentlyEvaluatingBinding() const
{
    auto currentState = bindingStatus.currentlyEvaluatingBinding;
    if (!currentState)
        return;
    registerWithCurrentlyEvaluatingBinding_helper(currentState);
}


void QPropertyBindingData::registerWithCurrentlyEvaluatingBinding_helper(BindingEvaluationState *currentState) const
{
    QPropertyBindingDataPointer d{this};

    QPropertyObserverPointer dependencyObserver = currentState->binding->allocateDependencyObserver();
    Q_ASSERT(QPropertyObserver::ObserverNotifiesBinding == 0);
    dependencyObserver.setBindingToNotify_unsafe(currentState->binding);
    d.addObserver(dependencyObserver.ptr);
}

void QPropertyBindingData::notifyObservers(QUntypedPropertyData *propertyDataPtr) const
{
    notifyObservers(propertyDataPtr, nullptr);
}

void QPropertyBindingData::notifyObservers(QUntypedPropertyData *propertyDataPtr, QBindingStorage *storage) const
{
    if (isNotificationDelayed())
        return;
    QPropertyBindingDataPointer d{this};

    if (QPropertyObserverPointer observer = d.firstObserver()) {
        auto status = storage ? storage->bindingStatus : nullptr;
        QPropertyDelayedNotifications *delay;
#ifndef QT_HAS_FAST_CURRENT_THREAD_ID
        status = &bindingStatus;
#else
        if (!status || status->threadId != QThread::currentThreadId())
            status = &bindingStatus;
#endif
        delay = status->groupUpdateData;
        if (delay) {
            delay->addProperty(this, propertyDataPtr);
            return;
        }
        observer.evaluateBindings(status);
    } else {
        return;
    }

    // evaluateBindings() can trash the observers. We need to re-fetch here.
    if (QPropertyObserverPointer observer = d.firstObserver())
        observer.notify(propertyDataPtr);
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

QPropertyObserver::QPropertyObserver(QUntypedPropertyData *data)
{
    aliasData = data;
    next.setTag(ObserverIsAlias);
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

QPropertyObserver::QPropertyObserver(QPropertyObserver &&other) noexcept
{
    binding = std::exchange(other.binding, {});
    next = std::exchange(other.next, {});
    prev = std::exchange(other.prev, {});
    if (next)
        next->prev = &next;
    if (prev)
        prev.setPointer(this);
}

QPropertyObserver &QPropertyObserver::operator=(QPropertyObserver &&other) noexcept
{
    if (this == &other)
        return *this;

    QPropertyObserverPointer d{this};
    d.unlink();
    binding = nullptr;

    binding = std::exchange(other.binding, {});
    next = std::exchange(other.next, {});
    prev = std::exchange(other.prev, {});
    if (next)
        next->prev = &next;
    if (prev)
        prev.setPointer(this);

    return *this;
}

#define UNLINK_COMMON \
    if (ptr->next) \
        ptr->next->prev = ptr->prev; \
    if (ptr->prev) \
        ptr->prev.setPointer(ptr->next.data()); \
    ptr->next = nullptr; \
    ptr->prev.clear();

/*!
    \internal
    Unlinks
 */
void QPropertyObserverPointer::unlink()
{
    UNLINK_COMMON
    if (ptr->next.tag() == QPropertyObserver::ObserverIsAlias)
        ptr->aliasData = nullptr;
}

/*!
    \internal
    Like unlink, but does not handle ObserverIsAlias.
    Must only be called in places where we know that we are not dealing
    with such an observer.
 */
void QPropertyObserverPointer::unlink_fast()
{
    Q_ASSERT(ptr->next.tag() != QPropertyObserver::ObserverIsAlias);
    UNLINK_COMMON
}
#undef UNLINK_COMMON

void QPropertyObserverPointer::setChangeHandler(QPropertyObserver::ChangeHandler changeHandler)
{
    Q_ASSERT(ptr->next.tag() != QPropertyObserver::ObserverIsPlaceholder);
    ptr->changeHandler = changeHandler;
    ptr->next.setTag(QPropertyObserver::ObserverNotifiesChangeHandler);
}

void QPropertyObserverPointer::setBindingToNotify(QPropertyBindingPrivate *binding)
{
    Q_ASSERT(ptr->next.tag() != QPropertyObserver::ObserverIsPlaceholder);
    ptr->binding = binding;
    ptr->next.setTag(QPropertyObserver::ObserverNotifiesBinding);
}

/*!
    \internal
    The same as as setBindingToNotify, but assumes that the tag is already correct.
 */
void QPropertyObserverPointer::setBindingToNotify_unsafe(QPropertyBindingPrivate *binding)
{
    Q_ASSERT(ptr->next.tag() == QPropertyObserver::ObserverNotifiesBinding);
    ptr->binding = binding;
}

/*!
 \internal
 QPropertyObserverNodeProtector is a RAII wrapper which takes care of the internal switching logic
 for QPropertyObserverPointer::notify (described ibidem)
*/
struct [[nodiscard]] QPropertyObserverNodeProtector {
    QPropertyObserverBase m_placeHolder;
    QPropertyObserverNodeProtector(QPropertyObserver *observer)
    {
        // insert m_placeholder after observer into the linked list
        QPropertyObserver *next = observer->next.data();
        m_placeHolder.next = next;
        observer->next = static_cast<QPropertyObserver *>(&m_placeHolder);
        if (next)
            next->prev = &m_placeHolder.next;
        m_placeHolder.prev = &observer->next;
        m_placeHolder.next.setTag(QPropertyObserver::ObserverIsPlaceholder);
    }

    QPropertyObserver *next() const { return m_placeHolder.next.data(); }

    ~QPropertyObserverNodeProtector() {
        QPropertyObserverPointer d{static_cast<QPropertyObserver *>(&m_placeHolder)};
        d.unlink_fast();
    }
};

/*! \internal
  \a propertyDataPtr is a pointer to the observed property's property data
*/
void QPropertyObserverPointer::notify(QUntypedPropertyData *propertyDataPtr)
{
    auto observer = const_cast<QPropertyObserver*>(ptr);
    /*
     * The basic idea of the loop is as follows: We iterate over all observers in the linked list,
     * and execute the functionality corresponding to their tag.
     * However, complication arise due to the fact that the triggered operations might modify the list,
     * which includes deletion and move of the current and next nodes.
     * Therefore, we take a few safety precautions:
     * 1. Before executing any action which might modify the list, we insert a placeholder node after the current node.
     *    As that one is stack allocated and owned by us, we can rest assured that it is
     *    still there after the action has executed, and placeHolder->next points to the actual next node in the list.
     *    Note that taking next at the beginning of the loop does not work, as the executed action might either move
     *    or delete that node.
     * 2. After the triggered action has finished, we can use the next pointer in the placeholder node as a safe way to
     *    retrieve the next node.
     * 3. Some care needs to be taken to avoid infinite recursion with change handlers, so we add an extra test there, that
     *    checks whether we're already have the same change handler in our call stack. This can be done by checking whether
     *    the node after the current one is a placeholder node.
     */
    while (observer) {
        QPropertyObserver *next = observer->next.data();
        switch (QPropertyObserver::ObserverTag(observer->next.tag())) {
        case QPropertyObserver::ObserverNotifiesChangeHandler:
        {
            auto handlerToCall = observer->changeHandler;
            // prevent recursion
            if (next && next->next.tag() == QPropertyObserver::ObserverIsPlaceholder) {
                observer = next->next.data();
                continue;
            }
            // handlerToCall might modify the list
            QPropertyObserverNodeProtector protector(observer);
            handlerToCall(observer, propertyDataPtr);
            next = protector.next();
            break;
        }
        case QPropertyObserver::ObserverNotifiesBinding:
        {
            auto bindingToNotify =  observer->binding;
            QPropertyObserverNodeProtector protector(observer);
            bindingToNotify->notifyRecursive();
            next = protector.next();
            break;
        }
        case QPropertyObserver::ObserverIsPlaceholder:
            // recursion is already properly handled somewhere else
            break;
        case QPropertyObserver::ObserverIsAlias:
            break;
        default: Q_UNREACHABLE();
        }
        observer = next;
    }
}

#ifndef QT_NO_DEBUG
void QPropertyObserverPointer::noSelfDependencies(QPropertyBindingPrivate *binding)
{
    auto observer = const_cast<QPropertyObserver*>(ptr);
    // See also comment in notify()
    while (observer) {
        if (QPropertyObserver::ObserverTag(observer->next.tag()) == QPropertyObserver::ObserverNotifiesBinding)
            if (observer->binding == binding) {
                qCritical("Property depends on itself!");
                break;
            }

        observer = observer->next.data();
    }

}
#endif

void QPropertyObserverPointer::evaluateBindings(QBindingStatus *status)
{
    Q_ASSERT(status);
    auto observer = const_cast<QPropertyObserver*>(ptr);
    // See also comment in notify()
    while (observer) {
        QPropertyObserver *next = observer->next.data();

        if (QPropertyObserver::ObserverTag(observer->next.tag()) == QPropertyObserver::ObserverNotifiesBinding) {
            auto bindingToEvaluate = observer->binding;
            QPropertyObserverNodeProtector protector(observer);
            bindingToEvaluate->evaluateRecursive_inline(status);
            next = protector.next();
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

/*!
    \class QPropertyBindingError
    \inmodule QtCore
    \ingroup tools
    \since 6.0

    QPropertyBindingError is used by \l{The Property System}{the property
    system} to report errors that occurred when a binding was evaluated. Use \l
    type()  to query which error occurred, and \l
    description() to extract an error message which might contain
    more details.
    If there is no error, QPropertyBindingError has type
    \c QPropertyBindingError::NoError and \c hasError() returns false.

    \code
    extern QProperty<int> prop;

    QPropertyBindingError error = prop.binding().error();
    if (error.hasError())
         qDebug() << error.description();
    \endcode
*/

/*!
    \enum QPropertyBindingError::Type

    This enum specifies which error occurred.

    \value NoError
        No error occurred while evaluating the binding.
    \value BindingLoop
        Binding evaluation was stopped because a property depended on its own
        value.
    \value EvaluationError
        Binding evaluation was stopped for any other reason than a binding loop.
        For example, this value is used in the QML engine when an exception occurs
        while a binding is evaluated.
    \value UnknownError
        A generic error type used when neither of the other values is suitable.
        Calling \l description() might provide details.
*/

/*!
    Default constructs QPropertyBindingError.
    hasError() will return false, type will return \c NoError and
    \l description() will return an empty string.
*/
QPropertyBindingError::QPropertyBindingError()
{
}

/*!
    Constructs a QPropertyBindingError of type \a type with \a description as its
    description.
*/
QPropertyBindingError::QPropertyBindingError(Type type, const QString &description)
{
    if (type != NoError) {
        d = new QPropertyBindingErrorPrivate;
        d->type = type;
        d->description = description;
    }
}

/*!
    Copy-constructs QPropertyBindingError from \a other.
*/
QPropertyBindingError::QPropertyBindingError(const QPropertyBindingError &other)
    : d(other.d)
{
}

/*!
    Copies \a other to this QPropertyBindingError.
*/
QPropertyBindingError &QPropertyBindingError::operator=(const QPropertyBindingError &other)
{
    d = other.d;
    return *this;
}

/*!
    Move-constructs QPropertyBindingError from \a other.
    \a other will be left in its default state.
*/
QPropertyBindingError::QPropertyBindingError(QPropertyBindingError &&other)
    : d(std::move(other.d))
{
}

/*!
    Move-assigns \a other to this QPropertyBindingError.
    \a other will be left in its default state.
*/
QPropertyBindingError &QPropertyBindingError::operator=(QPropertyBindingError &&other)
{
    d = std::move(other.d);
    return *this;
}

/*!
    Destroys the QPropertyBindingError.
*/
QPropertyBindingError::~QPropertyBindingError()
{
}

/*!
    Returns the type of the QPropertyBindingError.

    \sa QPropertyBindingError::Type
*/
QPropertyBindingError::Type QPropertyBindingError::type() const
{
    if (!d)
        return QPropertyBindingError::NoError;
    return d->type;
}

/*!
    Returns a descriptive error message for the QPropertyBindingError if
    it has been set.
*/
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

  You should usually call value() and setValue() on QProperty<T> or QObjectBindableProperty<T>, not use
  the low level mechanisms provided in this class.
*/

/*! \fn template <typename T> QPropertyData<T>::parameter_type QPropertyData<T>::valueBypassingBindings() const

    Returns the data stored in this property.

    \note As this will bypass any binding evaluation it might return an outdated value if a
    binding is set on this property. Using this method will also not register the property
    access with any currently executing binding.
*/

/*! \fn template <typename T> void QPropertyData<T>::setValueBypassingBindings(parameter_type v)

    Sets the data value stored in this property to \a v.

    \note Using this method will bypass any potential binding registered for this property.
*/

/*! \fn template <typename T> void QPropertyData<T>::setValueBypassingBindings(rvalue_ref v)
    \overload

    Sets the data value stored in this property to \a v.

    \note Using this method will bypass any potential binding registered for this property.
*/

/*!
  \class QUntypedBindable
  \inmodule QtCore
  \brief QUntypedBindable is a uniform interface over bindable properties like \c QProperty\<T\>
         and \c QObjectBindableProperty of any type \c T.
  \since 6.0

  \ingroup tools

  QUntypedBindable is a fully type-erased generic interface to wrap bindable properties.
  You can use it to interact with properties without knowing their type nor caring what
  kind of bindable property they are (e.g. QProperty or QObjectBindableProperty).
  For most use cases, using QBindable\<T\> (which is generic over the property implementation
  but has a fixed type) should be preferred.
*/

/*!
  \fn QUntypedBindable::QUntypedBindable()

  Default-constructs a QUntypedBindable. It is in an invalid state.
  \sa isValid()
*/

/*!
   \fn template<typename Property> QUntypedBindable::QUntypedBindable(Property *property)

   Constructs a QUntypedBindable from the property \a property. If Property is const,
   the QUntypedBindable will be read only. If \a property is null, the QUntypedBindable
   will be invalid.

   \sa isValid(), isReadOnly()
*/

/*!
   \fn bool QUntypedBindable::isValid() const

   Returns true if the QUntypedBindable is valid. Methods called on an invalid
   QUntypedBindable generally have no effect, unless otherwise noted.
*/

/*!
   \fn bool QUntypedBindable::isReadOnly() const
   \since 6.1

   Returns true if the QUntypedBindable is read-only.
*/

/*!
   \fn bool QUntypedBindable::isBindable() const
   \internal

   Returns true if the underlying property's binding can be queried
   with binding() and, if not read-only, changed with setBinding.
   Only QObjectComputedProperty currently leads to this method returning
   false.

   \sa isReadOnly()
*/

/*!
  \fn QUntypedPropertyBinding QUntypedBindable::makeBinding(const QPropertyBindingSourceLocation &location) const

  Creates a binding returning the underlying properties' value, using a specified source \a location.
*/

/*!
  \fn void QUntypedBindable::observe(QPropertyObserver *observer)
  \internal

  Installs the observer on the underlying property.
*/

/*!
  \fn template<typename Functor> QPropertyChangeHandler<Functor> QUntypedBindable::onValueChanged(Functor f) const

  Installs \a f as a change handler. Whenever the underlying property changes, \a f will be called, as
  long as the returned \c QPropertyChangeHandler and the property are kept alive.
  On each value change, the handler is either called immediately, or deferred, depending on the context.

  \sa onValueChanged(), subscribe()
*/

/*!
    \fn template<typename Functor> QPropertyChangeHandler<Functor> QUntypedBindable::subscribe(Functor f) const

    Behaves like a call to \a f followed by \c onValueChanged(f),

    \sa onValueChanged()
*/

/*!
  \fn template<typename Functor> QPropertyNotifier QUntypedBindable::addNotifier(Functor f)

  Installs \a f as a change handler. Whenever the underlying property changes, \a f will be called, as
  long as the returned \c QPropertyNotifier and the property are kept alive.

  This method is in some cases easier to use than onValueChanged(), as the returned object is not a template.
  It can therefore more easily be stored, e.g. as a member in a class.

  \sa onValueChanged(), subscribe()
*/

/*!
  \fn QUntypedPropertyBinding QUntypedBindable::binding() const

  Returns the underlying property's binding if there is any, or a default
  constructed QUntypedPropertyBinding otherwise.

  \sa hasBinding()
*/

/*!
  \fn QUntypedPropertyBinding QUntypedBindable::takeBinding()

  Removes the currently set binding from the property and returns it.
  Returns a default-constructed QUntypedPropertyBinding if no binding is set.

  \since 6.1
*/

/*!
  \fn bool QUntypedBindable::setBinding(const QUntypedPropertyBinding &binding)

  Sets the underlying property's binding to \a binding. This does not have any effect
  if the QUntypedBindable is read-only, null or if \a binding's type does match the
  underlying property's type.

  \return \c true when the binding was successfully set.

  //! \sa QUntypedPropertyBinding::valueMetaType()
*/

/*!
  \fn bool QUntypedBindable::hasBinding() const

  Returns \c true if the underlying property has a binding.
*/

/*!
  \fn QMetaType QUntypedBindable::metaType() const
  \since 6.2

  Returns the metatype of the property from which the QUntypedBindable was created.
  If the bindable is invalid, an invalid metatype will be returned.

  \sa isValid()
  //! \sa QUntypedPropertyBinding::valueMetaType()
*/

/*!
  \class QBindable
  \inmodule QtCore
  \brief QBindable is a wrapper class around binding-enabled properties. It allows type-safe
         operations while abstracting the differences between the various property classes away.
  \inherits QUntypedBindable

   \ingroup tools

   QBindable\<T\> helps to integrate Qt's traditional Q_PROPERTY with
   \l {Qt Bindable Properties}{binding-enabled} properties.
   If a property is backed by a QProperty, QObjectBindableProperty or QObjectComputedProperty,
   you can add \c BINDABLE bindablePropertyName to the Q_PROPERTY
   declaration, where bindablePropertyName is a function returning an instance of QBindable
   constructed from the QProperty. The returned QBindable allows users of the property to set
   and query bindings of the property, without having to know the exact kind of binding-enabled
   property used.

   \snippet code/src_corelib_kernel_qproperty.cpp 0
   \snippet code/src_corelib_kernel_qproperty.cpp 3

   \sa QMetaProperty::isBindable, QProperty, QObjectBindableProperty,
       QObjectComputedProperty, {Qt Bindable Properties}
*/

/*!
  \fn template<typename T> QPropertyBinding<T> QBindable<T>::makeBinding(const QPropertyBindingSourceLocation &location) const

  Constructs a binding evaluating to the underlying property's value, using a specified source
  \a location.
*/

/*!
  \fn template <typename T> QPropertyBinding<T> QBindable<T>::binding() const

   Returns the currently set binding of the underlying property. If the property does not
   have a binding, the returned \c QPropertyBinding<T> will be invalid.

   \sa setBinding, hasBinding
   //! \sa QPropertyBinding::isValid()
*/

/*!
  \fn template <typename T> QPropertyBinding<T> QBindable<T>::takeBinding()

   Removes the currently set binding of the underlying property and returns it.
   If the property does not have a binding, the returned \c QPropertyBinding<T> will be invalid.

   \sa binding, setBinding, hasBinding
   //! \sa QPropertyBinding::isValid()
*/


/*!
  \fn template <typename T> void QBindable<T>::setBinding(const QPropertyBinding<T> &binding)

   Sets the underlying property's binding to \a binding. Does nothing if the QBindable is
   read-only or invalid.

   \sa binding, isReadOnly(), isValid()
   //! \sa QPropertyBinding::isValid()
*/

/*!
  \fn  template <typename T> template <typename Functor> QPropertyBinding<T> QBindable<T>::setBinding(Functor f);
  \overload

  Creates a \c QPropertyBinding<T> from \a f, and sets it as the underlying property's binding.
*/

/*!
  \fn template <typename T> T QBindable<T>::value() const

  Returns the underlying property's current value. If the QBindable is invalid,
  a default constructed \c T is returned.

  \sa isValid()
*/

/*!
  \fn template <typename T> void QBindable<T>::setValue(const T &value)

  Sets the underlying property's value to \a value. This removes any currenltly set
  binding from it. This function has no effect if the QBindable is read-only or invalid.

  \sa isValid(), isReadOnly(), setBinding()
*/

/*!
  \class QProperty
  \inmodule QtCore
  \brief The QProperty class is a template class that enables automatic property bindings.
  \since 6.0

  \ingroup tools

  QProperty\<T\> is one of the classes implementing \l {Qt Bindable Properties}.
  It is a container that holds an instance of T. You can assign
  a value to it and you can read it via the value() function or the T conversion
  operator. You can also tie the property to an expression that computes the value
  dynamically, the binding expression. It is represented as a C++ lambda and
  can be used to express relationships between different properties in your
  application.
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
  \fn template <typename T> void QProperty<T>::setValue(rvalue_ref newValue)
  \fn template <typename T> void QProperty<T>::setValue(parameter_type newValue)

  Assigns \a newValue to this property and removes the property's associated
  binding, if present.
*/

/*!
  \fn template <typename T> QProperty<T> &QProperty<T>::operator=(rvalue_ref newValue)
  \fn template <typename T> QProperty<T> &QProperty<T>::operator=(parameter_type newValue)

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

  \sa {Formulating a Property Binding}
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
  the value of the property changes. On each value change, the handler
  is either called immediately, or deferred, depending on the context.

  The callback \a f is expected to be a type that has a plain call operator () without any
  parameters. This means that you can provide a C++ lambda expression, a std::function
  or even a custom struct with a call operator.

  The returned property change handler object keeps track of the registration. When it
  goes out of scope, the callback is de-registered.
*/

/*!
  \fn template <typename T> template <typename Functor> QPropertyChangeHandler<T, Functor> QProperty<T>::subscribe(Functor f)

  Subscribes the given functor \a f as a callback that is called immediately and whenever
  the value of the property changes in the future. On each value change, the handler
  is either called immediately, or deferred, depending on the context.

  The callback \a f is expected to be a type that can be copied and has a plain call
  operator() without any parameters. This means that you can provide a C++ lambda expression,
  a std::function or even a custom struct with a call operator.

  The returned property change handler object keeps track of the subscription. When it
  goes out of scope, the callback is unsubscribed.
*/

/*!
  \fn template <typename T> template <typename Functor> QPropertyNotifier QProperty<T>::addNotifier(Functor f)

  Subscribes the given functor \a f as a callback that is called whenever
  the value of the property changes.

  The callback \a f is expected to be a type that has a plain call operator () without any
  parameters. This means that you can provide a C++ lambda expression, a std::function
  or even a custom struct with a call operator.

  The returned property change handler object keeps track of the subscription. When it
  goes out of scope, the callback is unsubscribed.

  This method is in some cases easier to use than onValueChanged(), as the returned object is not a template.
  It can therefore more easily be stored, e.g. as a member in a class.

  \sa onValueChanged(), subscribe()
*/

/*!
  \fn template <typename T> QtPrivate::QPropertyBindingData &QProperty<T>::bindingData() const
  \internal
*/

/*!
  \class QObjectBindableProperty
  \inmodule QtCore
  \brief The QObjectBindableProperty class is a template class that enables automatic property bindings
         for property data stored in QObject derived classes.
  \since 6.0

  \ingroup tools

  QObjectBindableProperty is a generic container that holds an
  instance of T and behaves mostly like \l QProperty.
  It is one of the classes implementing \l {Qt Bindable Properties}.
  Unlike QProperty, it stores its management data structure in
  the surrounding QObject.
  The extra template parameters are used to identify the surrounding
  class and a member function of that class acting as a change handler.

  You can use QObjectBindableProperty to add binding support to code that uses Q_PROPERTY.
  The getter and setter methods must be adapted carefully according to the
  rules described in \l {Bindable Property Getters and Setters}.

  In order to invoke the change signal on property changes, use
  QObjectBindableProperty and pass the change signal as a callback.

  A simple example is given in the following.

  \snippet code/src_corelib_kernel_qproperty.cpp 4

  QObjectBindableProperty is usually not used directly, instead an instance of it is created by
  using the Q_OBJECT_BINDABLE_PROPERTY macro.

  Use the Q_OBJECT_BINDABLE_PROPERTY macro in the class declaration to declare
  the property as bindable.

  \snippet code/src_corelib_kernel_qproperty.cpp 0

  If you need to directly initialize the property with some non-default value,
  you can use the Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS macro. It accepts a
  value for the initialization as one of its parameters.

  \snippet code/src_corelib_kernel_qproperty.cpp 1

  Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS does not support multiple arguments
  directly. If your property requires multiple arguments for initialization,
  please explicitly call the specific constructor.

  \snippet code/src_corelib_kernel_qproperty.cpp 2

  The change handler can optionally accept one argument, of the same type as the property,
  in which case it is passed the new value of the property. Otherwise, it should take no
  arguments.

  If the property does not need a changed notification, you can leave out the
  "NOTIFY xChanged" in the Q_PROPERTY macro as well as the last argument
  of the Q_OBJECT_BINDABLE_PROPERTY and Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS
  macros.

  \sa Q_OBJECT_BINDABLE_PROPERTY, Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS, QProperty,
      QObjectComputedProperty, {Qt's Property System}, {Qt Bindable Properties}
*/

/*!
  \macro Q_OBJECT_BINDABLE_PROPERTY(containingClass, type, name, signal)
  \since 6.0
  \relates QObjectBindableProperty
  \brief Declares a \l QObjectBindableProperty inside \a containingClass
  of type \a type with name \a name. If the optional argument \a signal is given,
  this signal will be emitted when the property is marked dirty.

  \sa {Qt's Property System}, {Qt Bindable Properties}
*/

/*!
  \macro Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(containingClass, type, name, initialvalue, signal)
  \since 6.0
  \relates QObjectBindableProperty
  \brief Declares a \l QObjectBindableProperty inside \a containingClass
  of type \a type with name \a name which is initialized to \a initialvalue.
  If the optional argument \a signal is given, this signal will be emitted when
  the property is marked dirty.

  \sa {Qt's Property System}, {Qt Bindable Properties}
*/

/*!
  \class QObjectCompatProperty
  \inmodule QtCore
  \brief The QObjectCompatProperty class is a template class to help port old
         properties to the bindable property system.
  \since 6.0
  \ingroup tools
  \internal

  QObjectCompatProperty is a generic container that holds an
  instance of \c T and behaves mostly like QProperty, just like
  QObjectBindableProperty. It's one of the Qt internal classes implementing
  \l {Qt Bindable Properties}. Like QObjectBindableProperty,
  QObjectCompatProperty stores its management data structure in the surrounding
  QObject. The last template parameter specifies a method (of the owning
  class) to be called when the property is changed through the binding.
  This is usually a setter.

  As explained in \l {Qt Bindable Properties}, getters and setters for bindable
  properties have to be almost trivial to be correct. However, in legacy code,
  there is often complex logic in the setter. QObjectCompatProperty is a helper
  to port these properties to the bindable property system.

  With QObjectCompatProperty, the same rules as described in
  \l {Bindable Property Getters and Setters} hold for the getter.
  For the setter, the rules are different. It remains that every possible code
  path in the setter must write to the underlying QObjectCompatProperty,
  otherwise calling the setter might not remove a pre-existing binding, as
  it should. However, as QObjectCompatProperty will call the setter on every
  change, the setter is allowed to contain code like updating class internals
  or emitting signals. Every write to the QObjectCompatProperty has to
  be analyzed carefully to comply with the rules given in
  \l {Writing to a Bindable Property}.

  \section2 Properties with Virtual Setters

  Some of the pre-existing Qt classes (for example, \l QAbstractProxyModel)
  have properties with virtual setters. Special care must be taken when
  making such properties bindable.

  For the binding to work properly, the property must be correctly handled in
  all reimplemented methods of each derived class.

  Unless the derived class has access to the underlying property object, the
  base implementation \e must be called for the binding to work correctly.

  If the derived class can directly access the property instance, there is no
  need to explicitly call the base implementation, but the property's value
  \e must be correctly updated.

  Refer to \l {Bindable Properties with Virtual Setters and Getters} for more
  details.

  In both cases the expected behavior \e must be documented in the property's
  documentation, so that users can correctly override the setter.

  Properties for which these conditions cannot be met should not be made
  bindable.

  \sa Q_OBJECT_COMPAT_PROPERTY, QObjectBindableProperty, {Qt's Property System}, {Qt Bindable
  Properties}
*/

/*!
  \macro Q_OBJECT_COMPAT_PROPERTY(containingClass, type, name, callback)
  \since 6.0
  \relates QObjectCompatProperty
  \internal
  \brief Declares a \l QObjectCompatProperty inside \a containingClass
  of type \a type with name \a name. The argument \a callback specifies
  a setter function to be called when the property is changed through the binding.

  \sa QObjectBindableProperty, {Qt's Property System}, {Qt Bindable Properties}
*/

/*!
  \macro Q_OBJECT_COMPAT_PROPERTY_WITH_ARGS(containingClass, type, name, callback, value)
  \since 6.0
  \relates QObjectCompatProperty
  \internal
  \brief Declares a \l QObjectCompatProperty inside of \a containingClass
  of type \a type with name \a name. The argument \a callback specifies
  a setter function to be called when the property is changed through the binding.
  \a value specifies an initialization value.
*/

/*!
  \class QObjectComputedProperty
  \inmodule QtCore
  \brief The QObjectComputedProperty class is a template class to help port old
         properties to the bindable property system.
  \since 6.0
  \ingroup tools

  QObjectComputedProperty is a read-only property which is recomputed on each read.
  It does not store the computed value.
  It is one of the Qt internal classes implementing \l {Qt Bindable Properties}.
  QObjectComputedProperty is usually not used directly, instead an instance of it is created by
  using the Q_OBJECT_COMPUTED_PROPERTY macro.

  See the following example.

  \snippet code/src_corelib_kernel_qproperty.cpp 5

  The rules for getters in \l {Bindable Property Getters and Setters}
  also apply for QObjectComputedProperty. Especially, the getter
  should be trivial and only return the value of the QObjectComputedProperty object.
  The callback given to the QObjectComputedProperty should usually be a private
  method which is only called by the QObjectComputedProperty.

  No setter is required or allowed, as QObjectComputedProperty is read-only.

  To correctly participate in dependency handling, QObjectComputedProperty
  has to know when its value, the result of the callback given to it, might
  have changed. Whenever a bindable property used in the callback changes,
  this happens automatically. If the result of the callback might change
  because of a change in a value which is not a bindable property,
  it is the developer's responsibility to call markDirty
  on the QObjectComputedProperty object.
  This will inform dependent properties about the potential change.

  Note that calling markDirty might trigger change handlers in dependent
  properties, which might in turn use the object the QObjectComputedProperty
  is a member of. So markDirty must not be called when in a transitional
  or invalid state.

  QObjectComputedProperty is not suitable for use with a computation that depends
  on any input that might change without notice, such as the contents of a file.

  \sa Q_OBJECT_COMPUTED_PROPERTY, QProperty, QObjectBindableProperty,
      {Qt's Property System}, {Qt Bindable Properties}
*/

/*!
  \macro Q_OBJECT_COMPUTED_PROPERTY(containingClass, type, name, callback)
  \since 6.0
  \relates QObjectComputedProperty
  \brief Declares a \l QObjectComputedProperty inside \a containingClass
  of type \a type with name \a name. The argument \a callback specifies
  a GETTER function to be called when the property is evaluated.

  \sa QObjectBindableProperty, {Qt's Property System}, {Qt Bindable Properties}
*/

/*!
  \fn template <typename Class, typename T, auto offset, auto Callback> QObjectBindableProperty<Class, T, offset, Callback>::QObjectBindableProperty()

  Constructs a property with a default constructed instance of T.
*/

/*!
  \fn template <typename Class, typename T, auto offset, auto Callback> explicit QObjectBindableProperty<Class, T, offset, Callback>::QObjectBindableProperty(const T &initialValue)

  Constructs a property with the provided \a initialValue.
*/

/*!
  \fn template <typename Class, typename T, auto offset, auto Callback> explicit QObjectBindableProperty<Class, T, offset, Callback>::QObjectBindableProperty(T &&initialValue)

  Move-Constructs a property with the provided \a initialValue.
*/

/*!
  \fn template <typename Class, typename T, auto offset, auto Callback> QObjectBindableProperty<Class, T, offset, Callback>::QObjectBindableProperty(Class *owner, const QPropertyBinding<T> &binding)

  Constructs a property that is tied to the provided \a binding expression. The
  first time the property value is read, the binding is evaluated. Whenever a
  dependency of the binding changes, the binding will be re-evaluated the next
  time the value of this property is read. When the property value changes \a
  owner is notified via the Callback function.
*/

/*!
  \fn template <typename Class, typename T, auto offset, auto Callback> QObjectBindableProperty<Class, T, offset, Callback>::QObjectBindableProperty(Class *owner, QPropertyBinding<T> &&binding)

  Constructs a property that is tied to the provided \a binding expression. The
  first time the property value is read, the binding is evaluated. Whenever a
  dependency of the binding changes, the binding will be re-evaluated the next
  time the value of this property is read. When the property value changes \a
  owner is notified via the Callback function.
*/

/*!
  \fn template <typename Class, typename T, auto offset, auto Callback> template <typename Functor> QObjectBindableProperty<Class, T, offset, Callback>::QObjectBindableProperty(Functor &&f)

  Constructs a property that is tied to the provided binding expression \a f. The
  first time the property value is read, the binding is evaluated. Whenever a
  dependency of the binding changes, the binding will be re-evaluated the next
  time the value of this property is read.
*/

/*!
  \fn template <typename Class, typename T, auto offset, auto Callback> QObjectBindableProperty<Class, T, offset, Callback>::~QObjectBindableProperty()

  Destroys the property.
*/

/*!
  \fn template <typename Class, typename T, auto offset, auto Callback> T QObjectBindableProperty<Class, T, offset, Callback>::value() const

  Returns the value of the property. This may evaluate a binding expression that
  is tied to this property, before returning the value.
*/

/*!
  \fn template <typename Class, typename T, auto offset, auto Callback> void QObjectBindableProperty<Class, T, offset, Callback>::setValue(parameter_type newValue)
  \fn template <typename Class, typename T, auto offset, auto Callback> void QObjectBindableProperty<Class, T, offset, Callback>::setValue(rvalue_ref newValue)

  Assigns \a newValue to this property and removes the property's associated
  binding, if present. If the property value changes as a result, calls the
  Callback function on \a owner.
*/

/*!
  \fn template <typename Class, typename T, auto offset, auto Callback> void QObjectBindableProperty<Class, T, offset, Callback>::notify()

  Programmatically signals a change of the property. Any binding which depend on it will
  be notified, and if the property has a signal, it will be emitted.

  This can be useful in combination with setValueBypassingBindings to defer signalling the change
  until a class invariant has been restored.

  \note If this property has a binding (i.e. hasBinding() returns true), that binding is not reevaluated when
  notify() is called. Any binding depending on this property is still reevaluated as usual.

  \sa Qt::beginPropertyUpdateGroup(), setValueBypassingBindings()
*/

/*!
  \fn template <typename Class, typename T, auto offset, auto Callback> QPropertyBinding<T> QObjectBindableProperty<Class, T, offset, Callback>::setBinding(const QPropertyBinding<T> &newBinding)

  Associates the value of this property with the provided \a newBinding
  expression and returns the previously associated binding. The first time the
  property value is read, the binding is evaluated. Whenever a dependency of the
  binding changes, the binding will be re-evaluated the next time the value of
  this property is read. When the property value changes, the owner is notified
  via the Callback function.
*/

/*!
  \fn template <typename Class, typename T, auto offset, auto Callback> template <typename Functor> QPropertyBinding<T> QObjectBindableProperty<Class, T, offset, Callback>::setBinding(Functor f)
  \overload

  Associates the value of this property with the provided functor \a f and
  returns the previously associated binding. The first time the property value
  is read, the binding is evaluated by invoking the call operator () of \a f.
  Whenever a dependency of the binding changes, the binding will be re-evaluated
  the next time the value of this property is read. When the property value
  changes, the owner is notified via the Callback function.

  \sa {Formulating a Property Binding}
*/

/*!
  \fn template <typename Class, typename T, auto offset, auto Callback> QPropertyBinding<T> bool QObjectBindableProperty<Class, T, offset, Callback>::setBinding(const QUntypedPropertyBinding &newBinding)
  \overload

  Associates the value of this property with the provided \a newBinding
  expression. The first time the property value is read, the binding is evaluated.
  Whenever a dependency of the binding changes, the binding will be re-evaluated
  the next time the value of this property is read.

  Returns \c true if the type of this property is the same as the type the binding
  function returns; \c false otherwise.
*/

/*!
  \fn template <typename Class, typename T, auto offset, auto Callback> bool QObjectBindableProperty<Class, T, offset, Callback>::hasBinding() const

  Returns true if the property is associated with a binding; false otherwise.
*/


/*!
  \fn template <typename Class, typename T, auto offset, auto Callback> QPropertyBinding<T> QObjectBindableProperty<Class, T, offset, Callback>::binding() const

  Returns the binding expression that is associated with this property. A
  default constructed QPropertyBinding<T> will be returned if no such
  association exists.
*/

/*!
  \fn template <typename Class, typename T, auto offset, auto Callback> QPropertyBinding<T> QObjectBindableProperty<Class, T, offset, Callback>::takeBinding()

  Disassociates the binding expression from this property and returns it. After
  calling this function, the value of the property will only change if you
  assign a new value to it, or when a new binding is set.
*/

/*!
  \fn template <typename Class, typename T, auto offset, auto Callback> template <typename Functor> QPropertyChangeHandler<T, Functor> QObjectBindableProperty<Class, T, offset, Callback>::onValueChanged(Functor f)

  Registers the given functor \a f as a callback that shall be called whenever
  the value of the property changes. On each value change, the handler
  is either called immediately, or deferred, depending on the context.

  The callback \a f is expected to be a type that has a plain call operator () without any
  parameters. This means that you can provide a C++ lambda expression, a std::function
  or even a custom struct with a call operator.

  The returned property change handler object keeps track of the registration. When it
  goes out of scope, the callback is de-registered.
*/

/*!
  \fn template <typename Class, typename T, auto offset, auto Callback> template <typename Functor> QPropertyChangeHandler<T, Functor> QObjectBindableProperty<Class, T, offset, Callback>::subscribe(Functor f)

  Subscribes the given functor \a f as a callback that is called immediately and whenever
  the value of the property changes in the future. On each value change, the handler
  is either called immediately, or deferred, depending on the context.

  The callback \a f is expected to be a type that has a plain call operator () without any
  parameters. This means that you can provide a C++ lambda expression, a std::function
  or even a custom struct with a call operator.

  The returned property change handler object keeps track of the subscription. When it
  goes out of scope, the callback is unsubscribed.
*/

/*!
  \fn template <typename Class, typename T, auto offset, auto Callback> template <typename Functor> QPropertyNotifier QObjectBindableProperty<Class, T, offset, Callback>::addNotifier(Functor f)

  Subscribes the given functor \a f as a callback that is called whenever
  the value of the property changes.

  The callback \a f is expected to be a type that has a plain call operator () without any
  parameters. This means that you can provide a C++ lambda expression, a std::function
  or even a custom struct with a call operator.

  The returned property change handler object keeps track of the subscription. When it
  goes out of scope, the callback is unsubscribed.

  This method is in some cases easier to use than onValueChanged(), as the returned object is not a template.
  It can therefore more easily be stored, e.g. as a member in a class.

  \sa onValueChanged(), subscribe()
*/

/*!
  \fn template <typename T> QtPrivate::QPropertyBase &QObjectBindableProperty<Class, T, offset, Callback>::propertyBase() const
  \internal
*/

/*!
  \class QPropertyChangeHandler
  \inmodule QtCore
  \brief The QPropertyChangeHandler class controls the lifecycle of change callback installed on a QProperty.

  \ingroup tools

  QPropertyChangeHandler\<Functor\> is created when registering a
  callback on a QProperty to listen to changes to the property's value, using QProperty::onValueChanged
  and QProperty::subscribe. As long as the change handler is alive, the callback remains installed.

  A handler instance can be transferred between C++ scopes using move semantics.
*/

/*!
  \class QPropertyNotifier
  \inmodule QtCore
  \brief The QPropertyNotifier class controls the lifecycle of change callback installed on a QProperty.

  \ingroup tools

  QPropertyNotifier is created when registering a
  callback on a QProperty to listen to changes to the property's value, using QProperty::addNotifier.
  As long as the change handler is alive, the callback remains installed.

  A handler instance can be transferred between C++ scopes using move semantics.
*/

/*!
  \class QPropertyAlias
  \inmodule QtCore
  \internal

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

    QProperty<QString> fullname;
    fullname.setBinding([&]() { return nameAlias.value() + " age: " + QString::number(ageAlias.value()); });

    qDebug() << fullname.value(); // Prints "John age: 41"

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
  \fn template <typename T> template <typename Functor> QPropertyBinding<T> QPropertyAlias<T>::setBinding(Functor f)
  \overload

  Associates the value of the aliased property with the provided functor \a f
  expression. The first time the property value is read, either from the
  property itself or from any alias, the binding is evaluated. Whenever a
  dependency of the binding changes, the binding will be re-evaluated the next
  time the value of this property is read.

  Returns any previous binding associated with the property, or a
  default-constructed QPropertyBinding<T>.

  \sa {Formulating a Property Binding}
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
  the value of the aliased property changes. On each value change, the handler
  is either called immediately, or deferred, depending on the context.

  The callback \a f is expected to be a type that has a plain call operator () without any
  parameters. This means that you can provide a C++ lambda expression, a std::function
  or even a custom struct with a call operator.

  The returned property change handler object keeps track of the registration. When it
  goes out of scope, the callback is de-registered.
*/

/*!
  \fn template <typename T> template <typename Functor> QPropertyChangeHandler<T, Functor> QPropertyAlias<T>::subscribe(Functor f)

  Subscribes the given functor \a f as a callback that is called immediately and whenever
  the value of the aliased property changes in the future. On each value change, the handler
  is either called immediately, or deferred, depending on the context.

  The callback \a f is expected to be a type that has a plain call operator () without any
  parameters. This means that you can provide a C++ lambda expression, a std::function
  or even a custom struct with a call operator.

  The returned property change handler object keeps track of the subscription. When it
  goes out of scope, the callback is unsubscribed.
*/

/*!
  \fn template <typename T> template <typename Functor> QPropertyNotifier QPropertyAlias<T>::addNotifier(Functor f)

  Subscribes the given functor \a f as a callback that is called whenever
  the value of the aliased property changes.

  The callback \a f is expected to be a type that has a plain call operator () without any
  parameters. This means that you can provide a C++ lambda expression, a std::function
  or even a custom struct with a call operator.

  The returned property change handler object keeps track of the subscription. When it
  goes out of scope, the callback is unsubscribed.

  This method is in some cases easier to use than onValueChanged(), as the returned object is not a template.
  It can therefore more easily be stored, e.g. as a member in a class.

  \sa onValueChanged(), subscribe()
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
                Q_ASSERT(newData->size && (newData->size & (newData->size - 1)) == 0); // size is a power of two
                size_t index = qHash(p->data) & (newData->size - 1);
                while (pp[index].data) {
                    ++index;
                    if (index == newData->size)
                        index = 0;
                }
                new (pp + index) Pair{p->data, QPropertyBindingData(std::move(p->bindingData))};
            }
        }
        // data has been moved, no need to call destructors on old Pairs
        free(d);
        d = newData;
    }

    QBindingStoragePrivate(QBindingStorageData *&_d) : d(_d) {}

    QPropertyBindingData *get(const QUntypedPropertyData *data)
    {
        Q_ASSERT(d);
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
    QPropertyBindingData *get(QUntypedPropertyData *data, bool create)
    {
        if (!d) {
            if (!create)
                return nullptr;
            reallocate(8);
        }
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
        if (!create)
            return nullptr;
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
    Any property in a QObject can be made bindable by using the Q_OBJECT_BINDABLE_PROPERTY
    macro to declare it. A setter and a getter for the property and a declaration using
    Q_PROPERTY have to be made as usual.
    Binding related data will automatically be stored within the QBindingStorage
    inside the QObject.
*/

QBindingStorage::QBindingStorage()
{
    bindingStatus = &QT_PREPEND_NAMESPACE(bindingStatus);
    Q_ASSERT(bindingStatus);
}

QBindingStorage::~QBindingStorage()
{
    QBindingStoragePrivate(d).destroy();
}

void QBindingStorage::reinitAfterThreadMove()
{
    bindingStatus = &QT_PREPEND_NAMESPACE(bindingStatus);
    Q_ASSERT(bindingStatus);
}

void QBindingStorage::clear()
{
    QBindingStoragePrivate(d).destroy();
    d = nullptr;
    bindingStatus = nullptr;
}

// ### Unused, retained for BC with 6.0
void QBindingStorage::maybeUpdateBindingAndRegister_helper(const QUntypedPropertyData *data) const
{
    registerDependency_helper(data);
}

void QBindingStorage::registerDependency_helper(const QUntypedPropertyData *data) const
{
    Q_ASSERT(bindingStatus);
    // Use ::bindingStatus to get the binding from TLS. This is required, so that reads from
    // another thread do not register as dependencies
    QtPrivate::BindingEvaluationState *currentBinding;
#ifdef QT_HAS_FAST_CURRENT_THREAD_ID
    const bool threadMatches = (QThread::currentThreadId() == bindingStatus->threadId);
    if (Q_LIKELY(threadMatches))
        currentBinding = bindingStatus->currentlyEvaluatingBinding;
    else
        currentBinding = QT_PREPEND_NAMESPACE(bindingStatus).currentlyEvaluatingBinding;
#else
    currentBinding = QT_PREPEND_NAMESPACE(bindingStatus).currentlyEvaluatingBinding;
#endif
    QUntypedPropertyData *dd = const_cast<QUntypedPropertyData *>(data);
    if (!currentBinding)
        return;
    auto storage = QBindingStoragePrivate(d).get(dd, true);
    if (!storage)
        return;
    storage->registerWithCurrentlyEvaluatingBinding(currentBinding);
}


QPropertyBindingData *QBindingStorage::bindingData_helper(const QUntypedPropertyData *data) const
{
    return QBindingStoragePrivate(d).get(data);
}

const QBindingStatus *QBindingStorage::status(QtPrivate::QBindingStatusAccessToken) const
{
    return bindingStatus;
}

QPropertyBindingData *QBindingStorage::bindingData_helper(QUntypedPropertyData *data, bool create)
{
    return QBindingStoragePrivate(d).get(data, create);
}


namespace QtPrivate {


void initBindingStatusThreadId()
{
    bindingStatus.threadId = QThread::currentThreadId();
}

BindingEvaluationState *suspendCurrentBindingStatus()
{
    auto ret = bindingStatus.currentlyEvaluatingBinding;
    bindingStatus.currentlyEvaluatingBinding = nullptr;
    return ret;
}

void restoreBindingStatus(BindingEvaluationState *status)
{
    bindingStatus.currentlyEvaluatingBinding = status;
}

/*!
    \internal
    This function can be used to detect whether we are currently
    evaluating a binding. This can e.g. be used to defer the allocation
    of extra data for a QPropertyBindingStorage in a getter.
    Note that this function accesses TLS storage, and is therefore soemwhat
    costly to call.
*/
bool isAnyBindingEvaluating()
{
    return bindingStatus.currentlyEvaluatingBinding != nullptr;
}

bool isPropertyInBindingWrapper(const QUntypedPropertyData *property)
{
    return bindingStatus.currentCompatProperty &&
           bindingStatus.currentCompatProperty->property == property;
}

namespace BindableWarnings {

void printUnsuitableBindableWarning(QAnyStringView prefix, BindableWarnings::Reason reason)
{
    switch (reason) {
    case QtPrivate::BindableWarnings::NonBindableInterface:
        qCWarning(lcQPropertyBinding).noquote() << prefix.toString()
                                                << "The QBindable does not allow interaction with the binding.";
        break;
    case QtPrivate::BindableWarnings::ReadOnlyInterface:
        qCWarning(lcQPropertyBinding).noquote() << prefix.toString()
                                                << "The QBindable is read-only.";
        break;
    default:
    case QtPrivate::BindableWarnings::InvalidInterface:
        qCWarning(lcQPropertyBinding).noquote() << prefix.toString()
                                                << "The QBindable is invalid.";
        break;
    }
}

void printMetaTypeMismatch(QMetaType actual, QMetaType expected)
{
    qCWarning(lcQPropertyBinding) << "setBinding: Could not set binding as the property expects it to be of type"
                                  << actual.name()
                                  << "but got" << expected.name() << "instead.";
}

} // namespace BindableWarnings end

/*!
    \internal
    Returns the binding statusof the current thread.
 */
QBindingStatus* getBindingStatus(QtPrivate::QBindingStatusAccessToken) { return &QT_PREPEND_NAMESPACE(bindingStatus); }

} // namespace QtPrivate end

QT_END_NAMESPACE
