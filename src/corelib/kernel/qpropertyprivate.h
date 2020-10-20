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
#include <QtCore/qmetatype.h>

#include <functional>

QT_BEGIN_NAMESPACE

class QUntypedPropertyBinding;
class QPropertyBindingPrivate;
using QPropertyBindingPrivatePtr = QExplicitlySharedDataPointer<QPropertyBindingPrivate>;
struct QPropertyBindingDataPointer;

class QUntypedPropertyData
{
public:
    // sentinel to check whether a class inherits QUntypedPropertyData
    struct InheritsQUntypedPropertyData {};
};

namespace QtPrivate {

// writes binding result into dataPtr
using QPropertyBindingFunction = std::function<bool(QMetaType metaType, QUntypedPropertyData *dataPtr)>;
using QPropertyObserverCallback = void (*)(QUntypedPropertyData *);
using QPropertyBindingWrapper = bool(*)(QMetaType, QUntypedPropertyData *dataPtr, const QPropertyBindingFunction &);

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

    bool hasBinding() const { return d_ptr & BindingBit; }

    QUntypedPropertyBinding setBinding(const QUntypedPropertyBinding &newBinding,
                                       QUntypedPropertyData *propertyDataPtr,
                                       QPropertyObserverCallback staticObserverCallback = nullptr,
                                       QPropertyBindingWrapper bindingWrapper = nullptr);

    QPropertyBindingPrivate *binding() const;

    void evaluateIfDirty(const QUntypedPropertyData *property) const;
    void removeBinding();

    void registerWithCurrentlyEvaluatingBinding() const;
    void notifyObservers(QUntypedPropertyData *propertyDataPtr) const;

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
