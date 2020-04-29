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
#include <QtCore/QExplicitlySharedDataPointer>
#include <QtCore/qtaggedpointer.h>

QT_BEGIN_NAMESPACE

class QUntypedPropertyBinding;
class QPropertyBindingPrivate;
using QPropertyBindingPrivatePtr = QExplicitlySharedDataPointer<QPropertyBindingPrivate>;
struct QPropertyBasePointer;

namespace QtPrivate {

class Q_CORE_EXPORT QPropertyBase
{
    // Mutable because the address of the observer of the currently evaluating binding is stored here, for
    // notification later when the value changes.
    mutable quintptr d_ptr = 0;
    friend struct QT_PREPEND_NAMESPACE(QPropertyBasePointer);
public:
    QPropertyBase() = default;
    Q_DISABLE_COPY(QPropertyBase)
    QPropertyBase(QPropertyBase &&other) = delete;
    QPropertyBase(QPropertyBase &&other, void *propertyDataPtr);
    QPropertyBase &operator=(QPropertyBase &&other) = delete;
    ~QPropertyBase();

    void moveAssign(QPropertyBase &&other, void *propertyDataPtr);

    bool hasBinding() const { return d_ptr & BindingBit; }

    QUntypedPropertyBinding setBinding(const QUntypedPropertyBinding &newBinding,
                                       void *propertyDataPtr, void *staticObserver = nullptr,
                                       void (*staticObserverCallback)(void*) = nullptr);
    QPropertyBindingPrivate *binding();

    void evaluateIfDirty();
    void removeBinding();

    void registerWithCurrentlyEvaluatingBinding() const;
    void notifyObservers(void *propertyDataPtr);

    void setExtraBit(bool b)
    {
        if (b)
            d_ptr |= ExtraBit;
        else
            d_ptr &= ~ExtraBit;
    }

    bool extraBit() const { return d_ptr & ExtraBit; }

    static const quintptr ExtraBit = 0x1;   // Used for QProperty<bool> specialization
    static const quintptr BindingBit = 0x2; // Is d_ptr pointing to a binding (1) or list of notifiers (0)?
    static const quintptr FlagMask = BindingBit | ExtraBit;
};

template <typename T>
struct QPropertyValueStorage
{
private:
    T value;
public:
    QPropertyBase priv;

    QPropertyValueStorage() : value() {}
    Q_DISABLE_COPY(QPropertyValueStorage)
    explicit QPropertyValueStorage(const T &initialValue) : value(initialValue) {}
    QPropertyValueStorage &operator=(const T &newValue) { value = newValue; return *this; }
    explicit QPropertyValueStorage(T &&initialValue) : value(std::move(initialValue)) {}
    QPropertyValueStorage &operator=(T &&newValue) { value = std::move(newValue); return *this; }
    QPropertyValueStorage(QPropertyValueStorage &&other) : value(std::move(other.value)), priv(std::move(other.priv), this) {}
    QPropertyValueStorage &operator=(QPropertyValueStorage &&other) { value = std::move(other.value); priv.moveAssign(std::move(other.priv), &value); return *this; }

    T getValue() const { return value; }
    bool setValueAndReturnTrueIfChanged(T &&v)
    {
        if (v == value)
            return false;
        value = std::move(v);
        return true;
    }
    bool setValueAndReturnTrueIfChanged(const T &v)
    {
        if (v == value)
            return false;
        value = v;
        return true;
    }
};

template<>
struct QPropertyValueStorage<bool>
{
    QPropertyBase priv;

    QPropertyValueStorage() = default;
    Q_DISABLE_COPY(QPropertyValueStorage)
    explicit QPropertyValueStorage(bool initialValue) { priv.setExtraBit(initialValue); }
    QPropertyValueStorage &operator=(bool newValue) { priv.setExtraBit(newValue); return *this; }
    QPropertyValueStorage(QPropertyValueStorage &&other) : priv(std::move(other.priv), this) {}
    QPropertyValueStorage &operator=(QPropertyValueStorage &&other) { priv.moveAssign(std::move(other.priv), this); return *this; }

    bool getValue() const { return priv.extraBit(); }
    bool setValueAndReturnTrueIfChanged(bool v)
    {
        if (v == priv.extraBit())
            return false;
        priv.setExtraBit(v);
        return true;
    }
};

template <typename T, typename Tag>
class QTagPreservingPointerToPointer
{
public:
    QTagPreservingPointerToPointer() = default;

    QTagPreservingPointerToPointer(T **ptr)
        : d(reinterpret_cast<quintptr*>(ptr))
    {}

    QTagPreservingPointerToPointer<T, Tag> &operator=(T **ptr)
    {
        d = reinterpret_cast<quintptr*>(ptr);
        return *this;
    }

    QTagPreservingPointerToPointer<T, Tag> &operator=(QTaggedPointer<T, Tag> *ptr)
    {
        d = reinterpret_cast<quintptr*>(ptr);
        return *this;
    }

    void clear()
    {
        d = nullptr;
    }

    void setPointer(T *ptr)
    {
        *d = (reinterpret_cast<quintptr>(ptr) & QTaggedPointer<T, Tag>::pointerMask()) | (*d & QTaggedPointer<T, Tag>::tagMask());
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

} // namespace QtPrivate

QT_END_NAMESPACE

#endif // QPROPERTYPRIVATE_H
