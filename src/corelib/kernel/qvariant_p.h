/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
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

// takes a type, returns the internal void* pointer cast
// to a pointer of the input type

#include <QtCore/qglobal.h>
#include <QtCore/qvariant.h>

#include "qmetatypeswitcher_p.h"

QT_BEGIN_NAMESPACE

#ifdef Q_CC_SUN // Sun CC picks the wrong overload, so introduce awful hack

template <typename T>
inline T *v_cast(const QVariant::Private *nd, T * = 0)
{
    QVariant::Private *d = const_cast<QVariant::Private *>(nd);
    return ((sizeof(T) > sizeof(QVariant::Private::Data))
            ? static_cast<T *>(d->data.shared->ptr)
            : static_cast<T *>(static_cast<void *>(&d->data.c)));
}

#else // every other compiler in this world

template <typename T>
inline const T *v_cast(const QVariant::Private *d, T * = 0)
{
    return ((sizeof(T) > sizeof(QVariant::Private::Data))
            ? static_cast<const T *>(d->data.shared->ptr)
            : static_cast<const T *>(static_cast<const void *>(&d->data.c)));
}

template <typename T>
inline T *v_cast(QVariant::Private *d, T * = 0)
{
    return ((sizeof(T) > sizeof(QVariant::Private::Data))
            ? static_cast<T *>(d->data.shared->ptr)
            : static_cast<T *>(static_cast<void *>(&d->data.c)));
}

#endif


//a simple template that avoids to allocate 2 memory chunks when creating a QVariant
template <class T> class QVariantPrivateSharedEx : public QVariant::PrivateShared
{
public:
    QVariantPrivateSharedEx() : QVariant::PrivateShared(&m_t) { }
    QVariantPrivateSharedEx(const T&t) : QVariant::PrivateShared(&m_t), m_t(t) { }

private:
    T m_t;
};

// constructs a new variant if copy is 0, otherwise copy-constructs
template <class T>
inline void v_construct(QVariant::Private *x, const void *copy, T * = 0)
{
    if (sizeof(T) > sizeof(QVariant::Private::Data)) {
        x->data.shared = copy ? new QVariantPrivateSharedEx<T>(*static_cast<const T *>(copy))
                              : new QVariantPrivateSharedEx<T>;
        x->is_shared = true;
    } else {
        if (copy)
            new (&x->data.ptr) T(*static_cast<const T *>(copy));
        else
            new (&x->data.ptr) T;
    }
}

template <class T>
inline void v_construct(QVariant::Private *x, const T &t)
{
    if (sizeof(T) > sizeof(QVariant::Private::Data)) {
        x->data.shared = new QVariantPrivateSharedEx<T>(t);
        x->is_shared = true;
    } else {
        new (&x->data.ptr) T(t);
    }
}

// deletes the internal structures
template <class T>
inline void v_clear(QVariant::Private *d, T* = 0)
{
    
    if (sizeof(T) > sizeof(QVariant::Private::Data)) {
        //now we need to cast
        //because QVariant::PrivateShared doesn't have a virtual destructor
        delete static_cast<QVariantPrivateSharedEx<T>*>(d->data.shared);
    } else {
        v_cast<T>(d)->~T();
    }

}

template<class Filter>
class QVariantComparator {
    template<typename T, bool IsAcceptedType = Filter::template Acceptor<T>::IsAccepted>
    struct FilteredComparator {
        static bool compare(const QVariant::Private *a, const QVariant::Private *b)
        {
            return *v_cast<T>(a) == *v_cast<T>(b);
        }
    };
    template<typename T>
    struct FilteredComparator<T, /* IsAcceptedType = */ false> {
        static bool compare(const QVariant::Private *m_a, const QVariant::Private *m_b)
        {
            if (!QMetaType::isRegistered(m_a->type))
                qFatal("QVariant::compare: type %d unknown to QVariant.", m_a->type);

            const void *a_ptr = m_a->is_shared ? m_a->data.shared->ptr : &(m_a->data.ptr);
            const void *b_ptr = m_b->is_shared ? m_b->data.shared->ptr : &(m_b->data.ptr);

            const char *const typeName = QMetaType::typeName(m_a->type);
            uint typeNameLen = qstrlen(typeName);
            if (typeNameLen > 0 && typeName[typeNameLen - 1] == '*')
                return *static_cast<void *const *>(a_ptr) == *static_cast<void *const *>(b_ptr);

            if (m_a->is_null && m_b->is_null)
                return true;

            return !memcmp(a_ptr, b_ptr, QMetaType::sizeOf(m_a->type));
        }
    };
public:
    QVariantComparator(const QVariant::Private *a, const QVariant::Private *b)
        : m_a(a), m_b(b)
    {
        Q_ASSERT(a->type == b->type);
    }

    template<typename T>
    bool delegate(const T*)
    {
        return FilteredComparator<T>::compare(m_a, m_b);
    }

    bool delegate(const void*) { return true; }

protected:
    const QVariant::Private *m_a;
    const QVariant::Private *m_b;
};


Q_CORE_EXPORT const QVariant::Handler *qcoreVariantHandler();

template<class Filter>
class QVariantIsNull
{
    /// \internal
    /// This class checks if a type T has method called isNull. Result is kept in the Value property
    /// TODO Can we somehow generalize it? A macro version?
    template<typename T, bool IsClass = QTypeInfo<T>::isComplex>
    class HasIsNullMethod
    {
        struct Yes { char unused[1]; };
        struct No { char unused[2]; };
        Q_STATIC_ASSERT(sizeof(Yes) != sizeof(No));

        struct FallbackMixin { bool isNull() const; };
        struct Derived : public T, public FallbackMixin {};
        template<class C, C> struct TypeCheck {};

        template<class C> static Yes test(...);
        template<class C> static No test(TypeCheck<bool (FallbackMixin::*)() const, &C::isNull> *);
    public:
        static const bool Value = (sizeof(test<Derived>(0)) == sizeof(Yes));
    };

    // We need to exclude primitive types as they won't compile with HasIsNullMethod::Check classes
    // anyway it is not a problem as the types do not have isNull method.
    template<typename T>
    class HasIsNullMethod<T, /* IsClass = */ false> {
    public:
        static const bool Value = false;
    };

    // TODO This part should go to autotests during HasIsNullMethod generalization.
    Q_STATIC_ASSERT(!HasIsNullMethod<bool>::Value);
    struct SelfTest1 { bool isNull() const; };
    Q_STATIC_ASSERT(HasIsNullMethod<SelfTest1>::Value);
    struct SelfTest2 {};
    Q_STATIC_ASSERT(!HasIsNullMethod<SelfTest2>::Value);
    struct SelfTest3 : public SelfTest1 {};
    Q_STATIC_ASSERT(HasIsNullMethod<SelfTest3>::Value);

    template<typename T, bool HasIsNull = HasIsNullMethod<T>::Value>
    struct CallFilteredIsNull
    {
        static bool isNull(const QVariant::Private *d)
        {
            return v_cast<T>(d)->isNull();
        }
    };
    template<typename T>
    struct CallFilteredIsNull<T, /* HasIsNull = */ false>
    {
        static bool isNull(const QVariant::Private *d)
        {
            return d->is_null;
        }
    };

    template<typename T, bool IsAcceptedType = Filter::template Acceptor<T>::IsAccepted>
    struct CallIsNull
    {
        static bool isNull(const QVariant::Private *d)
        {
            return CallFilteredIsNull<T>::isNull(d);
        }
    };
    template<typename T>
    struct CallIsNull<T, /* IsAcceptedType = */ false>
    {
        static bool isNull(const QVariant::Private *d)
        {
            return CallFilteredIsNull<T, false>::isNull(d);
        }
    };

public:
    QVariantIsNull(const QVariant::Private *d)
        : m_d(d)
    {}
    template<typename T>
    bool delegate(const T*)
    {
        CallIsNull<T> null;
        return null.isNull(m_d);
    }
    // we need that as sizof(void) is undefined and it is needed in HasIsNullMethod
    bool delegate(const void *) { return m_d->is_null; }
protected:
    const QVariant::Private *m_d;
};

template<class Filter>
class QVariantConstructor
{
    template<typename T, bool IsSmall = (sizeof(T) <= sizeof(QVariant::Private::Data))>
    struct CallConstructor {};

    template<typename T>
    struct CallConstructor<T, /* IsSmall = */ true>
    {
        CallConstructor(const QVariantConstructor &tc)
        {
            if (tc.m_copy)
                new (&tc.m_x->data.ptr) T(*static_cast<const T*>(tc.m_copy));
            else
                new (&tc.m_x->data.ptr) T();
            tc.m_x->is_shared = false;
        }
    };

    template<typename T>
    struct CallConstructor<T, /* IsSmall = */ false>
    {
        CallConstructor(const QVariantConstructor &tc)
        {
            Q_STATIC_ASSERT(QTypeInfo<T>::isComplex);
            tc.m_x->data.shared = tc.m_copy ? new QVariantPrivateSharedEx<T>(*static_cast<const T*>(tc.m_copy))
                                      : new QVariantPrivateSharedEx<T>;
            tc.m_x->is_shared = true;
        }
    };

    template<typename T, bool IsAcceptedType = Filter::template Acceptor<T>::IsAccepted>
    struct FilteredConstructor {
        FilteredConstructor(const QVariantConstructor &tc)
        {
            CallConstructor<T> tmp(tc);
            tc.m_x->is_null = !tc.m_copy;
        }
    };
    template<typename T>
    struct FilteredConstructor<T, /* IsAcceptedType = */ false> {
        FilteredConstructor(const QVariantConstructor &tc)
        {
            // ignore types that lives outside of the current library
            tc.m_x->type = QVariant::Invalid;
        }
    };
public:
    QVariantConstructor(QVariant::Private *x, const void *copy)
        : m_x(x)
        , m_copy(copy)
    {}

    template<typename T>
    void delegate(const T*)
    {
        FilteredConstructor<T>(*this);
    }

    void delegate(const QMetaTypeSwitcher::UnknownType*)
    {
        if (m_x->type == QVariant::UserType) {
            // TODO get rid of it
            // And yes! we can support historical magic, unkonwn/unconstructed user type isn't that
            // awesome? this QVariant::isValid will be true!
            m_x->is_null = !m_copy;
            m_x->is_shared = false;
            return;
        }
        // it is not a static known type, lets ask QMetaType if it can be constructed for us.
        const uint size = QMetaType::sizeOf(m_x->type);

        if (size && size <= sizeof(QVariant::Private::Data)) {
            void *ptr = QMetaType::construct(m_x->type, &m_x->data.ptr, m_copy);
            if (!ptr) {
                m_x->type = QVariant::Invalid;
            }
            m_x->is_shared = false;
        } else {
            void *ptr = QMetaType::create(m_x->type, m_copy);
            if (!ptr) {
                m_x->type = QVariant::Invalid;
            } else {
                m_x->is_shared = true;
                m_x->data.shared = new QVariant::PrivateShared(ptr);
            }
        }
    }

    void delegate(const void*)
    {
        // QMetaType::Void == QVariant::Invalid, creating an invalid value creates invalid QVariant
        // TODO it might go away, check is needed
        m_x->is_shared = false;
        m_x->is_null = !m_copy;
    }
private:
    QVariant::Private *m_x;
    const void *m_copy;
};

template<class Filter>
class QVariantDestructor
{
    template<typename T, bool IsAcceptedType = Filter::template Acceptor<T>::IsAccepted>
    struct FilteredDestructor {
        FilteredDestructor(QVariant::Private *d)
        {
            v_clear<T>(d);
        }
    };
    template<typename T>
    struct FilteredDestructor<T, /* IsAcceptedType = */ false> {
        FilteredDestructor(QVariant::Private *) {} // ignore non accessible types
    };

public:
    QVariantDestructor(QVariant::Private *d)
        : m_d(d)
    {}
    ~QVariantDestructor()
    {
        m_d->type = QVariant::Invalid;
        m_d->is_null = true;
        m_d->is_shared = false;
    }

    template<typename T>
    void delegate(const T*)
    {
        FilteredDestructor<T> cleaner(m_d);
    }

    void delegate(const QMetaTypeSwitcher::UnknownType*)
    {
        // This is not a static type, so lets delegate everyting to QMetaType
        if (!m_d->is_shared) {
            QMetaType::destruct(m_d->type, &m_d->data.ptr);
        } else {
            QMetaType::destroy(m_d->type, m_d->data.shared->ptr);
            delete m_d->data.shared;
        }
    }
    // Ignore nonconstructible type
    void delegate(const void*) {}
private:
    QVariant::Private *m_d;
};

QT_END_NAMESPACE

#endif // QVARIANT_P_H
