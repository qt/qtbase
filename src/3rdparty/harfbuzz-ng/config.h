/*
* Copyright (C) 2015 The Qt Company Ltd.
* Copyright (C) 2015 Konstantin Ritt
*
* Permission is hereby granted, without written agreement and without
* license or royalty fees, to use, copy, modify, and distribute this
* software and its documentation for any purpose, provided that the
* above copyright notice and the following two paragraphs appear in
* all copies of this software.
*
* IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
* DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
* ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
* IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
* DAMAGE.
*
* THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
* BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
* FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
* ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
* PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
*
*/

#include <QtCore/qatomic.h>

QT_USE_NAMESPACE

namespace {

// We need to cast hb_atomic_int_t to QAtomicInt and pointers to
// QAtomicPointer instead of using QAtomicOps, otherwise we get a failed
// overload resolution of the template arguments for testAndSetOrdered.
template <typename T>
inline QAtomicPointer<T> *makeAtomicPointer(T * const &ptr)
{
    return reinterpret_cast<QAtomicPointer<T> *>(const_cast<T **>(&ptr));
}

} // namespace

typedef int hb_atomic_int_impl_t;
#define HB_ATOMIC_INT_IMPL_INIT(V)             (V)
#define hb_atomic_int_impl_add(AI, V)          reinterpret_cast<QAtomicInt &>(AI).fetchAndAddOrdered(V)

#define hb_atomic_ptr_impl_get(P)              makeAtomicPointer(*(P))->loadAcquire()
#define hb_atomic_ptr_impl_cmpexch(P,O,N)      makeAtomicPointer(*(P))->testAndSetOrdered((O), (N))
