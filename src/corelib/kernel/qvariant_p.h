/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
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

#ifndef QVARIANT_P_H
#define QVARIANT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>
#include <QtCore/qvariant.h>
#include <QtCore/private/qmetatype_p.h>
#include <QtCore/qdebug.h>

#include "qmetatypeswitcher_p.h"

QT_BEGIN_NAMESPACE

template<typename T>
struct QVariantIntegrator
{
    static const bool CanUseInternalSpace = sizeof(T) <= sizeof(QVariant::Private::Data)
            && ((QTypeInfoQuery<T>::isRelocatable) || std::is_enum<T>::value);
    typedef std::integral_constant<bool, CanUseInternalSpace> CanUseInternalSpace_t;
};
static_assert(QVariantIntegrator<double>::CanUseInternalSpace);
static_assert(QVariantIntegrator<long int>::CanUseInternalSpace);
static_assert(QVariantIntegrator<qulonglong>::CanUseInternalSpace);

#ifdef Q_CC_SUN // Sun CC picks the wrong overload, so introduce awful hack

// takes a type, returns the internal void* pointer cast
// to a pointer of the input type
template <typename T>
inline T *v_cast(const QVariant::Private *nd, T * = 0)
{
    QVariant::Private *d = const_cast<QVariant::Private *>(nd);
    return !QVariantIntegrator<T>::CanUseInternalSpace
            ? static_cast<T *>(d->data.shared->ptr)
            : static_cast<T *>(static_cast<void *>(&d->data.c));
}

#else // every other compiler in this world

template <typename T>
inline const T *v_cast(const QVariant::Private *d, T * = nullptr)
{
    return !QVariantIntegrator<T>::CanUseInternalSpace
            ? static_cast<const T *>(d->data.shared->ptr)
            : static_cast<const T *>(static_cast<const void *>(&d->data.c));
}

template <typename T>
inline T *v_cast(QVariant::Private *d, T * = nullptr)
{
    return !QVariantIntegrator<T>::CanUseInternalSpace
            ? static_cast<T *>(d->data.shared->ptr)
            : static_cast<T *>(static_cast<void *>(&d->data.c));
}

#endif

enum QVariantConstructionFlags : uint {
    Default = 0x0,
    PointerType = 0x1,
    ShouldDeleteVariantData = 0x2 // only used in Q*Iterable
};

//a simple template that avoids to allocate 2 memory chunks when creating a QVariant
template <class T> class QVariantPrivateSharedEx : public QVariant::PrivateShared
{
public:
    QVariantPrivateSharedEx() : QVariant::PrivateShared(&m_t), m_t() { }
    QVariantPrivateSharedEx(const T&t) : QVariant::PrivateShared(&m_t), m_t(t) { }

private:
    T m_t;
};

template <class T>
inline void v_construct_helper(QVariant::Private *x, const T &t, std::true_type)
{
    new (&x->data) T(t);
    x->is_shared = false;
}

template <class T>
inline void v_construct_helper(QVariant::Private *x, const T &t, std::false_type)
{
    x->data.shared = new QVariantPrivateSharedEx<T>(t);
    x->is_shared = true;
}

template <class T>
inline void v_construct_helper(QVariant::Private *x, std::true_type)
{
    new (&x->data) T();
    x->is_shared = false;
}

template <class T>
inline void v_construct_helper(QVariant::Private *x, std::false_type)
{
    x->data.shared = new QVariantPrivateSharedEx<T>;
    x->is_shared = true;
}

template <class T>
inline void v_construct(QVariant::Private *x, const T &t)
{
    // dispatch
    v_construct_helper(x, t, typename QVariantIntegrator<T>::CanUseInternalSpace_t());
}

// constructs a new variant if copy is 0, otherwise copy-constructs
template <class T>
inline void v_construct(QVariant::Private *x, const void *copy, T * = nullptr)
{
    if (copy)
        v_construct<T>(x, *static_cast<const T *>(copy));
    else
        v_construct_helper<T>(x, typename QVariantIntegrator<T>::CanUseInternalSpace_t());
}

// deletes the internal structures
template <class T>
inline void v_clear(QVariant::Private *d, T* = nullptr)
{

    if (!QVariantIntegrator<T>::CanUseInternalSpace) {
        //now we need to cast
        //because QVariant::PrivateShared doesn't have a virtual destructor
        delete static_cast<QVariantPrivateSharedEx<T>*>(d->data.shared);
    } else {
        v_cast<T>(d)->~T();
    }

}

Q_CORE_EXPORT const QVariant::Handler *qcoreVariantHandler();

namespace QVariantPrivate {
Q_CORE_EXPORT void registerHandler(const int /* Modules::Names */ name, const QVariant::Handler *handler);
}

#if !defined(QT_NO_DEBUG_STREAM)
template<class Filter>
class QVariantDebugStream
{
    template<typename T, bool IsAcceptedType = Filter::template Acceptor<T>::IsAccepted>
    struct Filtered {
        Filtered(QDebug dbg, QVariant::Private *d)
        {
            dbg.nospace() << *v_cast<T>(d);
        }
    };
    template<typename T>
    struct Filtered<T, /* IsAcceptedType = */ false> {
        Filtered(QDebug /* dbg */, QVariant::Private *)
        {
            // It is not possible to construct not acccepted type, QVariantConstructor creates an invalid variant for them
            Q_ASSERT(false);
        }
    };

public:
    QVariantDebugStream(QDebug dbg, QVariant::Private *d)
        : m_debugStream(dbg)
        , m_d(d)
    {}

    template<typename T>
    void delegate(const T*)
    {
        Filtered<T> streamIt(m_debugStream, m_d);
        Q_UNUSED(streamIt);
    }

    void delegate(const QMetaTypeSwitcher::NotBuiltinType*)
    {
        // QVariantDebugStream class is used only for a built-in type
        Q_ASSERT(false);
    }
    void delegate(const QMetaTypeSwitcher::UnknownType*)
    {
        m_debugStream.nospace() << "QVariant::Invalid";
    }
    void delegate(const void*) { Q_ASSERT(false); }
private:
    QDebug m_debugStream;
    QVariant::Private *m_d;
};
#endif

QT_END_NAMESPACE

#endif // QVARIANT_P_H
