/****************************************************************************
**
** Copyright (C) 2013 Intel Corporation
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef FFD_ATOMIC_GCC_H
#define FFD_ATOMIC_GCC_H

/* atomics */
/* we'll use the GCC 4.7 atomic builtins
 * See http://gcc.gnu.org/onlinedocs/gcc/_005f_005fatomic-Builtins.html#_005f_005fatomic-Builtins
 * Or in texinfo: C Extensions > __atomic Builtins
 */
typedef int ffd_atomic_int;
#define ffd_atomic_pointer(type)    type*

#define FFD_ATOMIC_INIT(val)    (val)

#define FFD_ATOMIC_RELAXED  __ATOMIC_RELAXED
#define FFD_ATOMIC_ACQUIRE  __ATOMIC_ACQUIRE
#define FFD_ATOMIC_RELEASE  __ATOMIC_RELEASE
// acq_rel & cst not necessary

#if !defined(__GNUC__) || \
    ((__GNUC__ - 0) * 100 + (__GNUC_MINOR__ - 0)) < 407 || \
    (defined(__INTEL_COMPILER) && __INTEL_COMPILER-0 < 1310) || \
    (defined(__clang__) && ((__clang_major__-0) * 100 + (__clang_minor-0)) < 303)
#define ffd_atomic_load_n(ptr,order) *(ptr)
#define ffd_atomic_store_n(ptr,val,order) (*(ptr) = (val), (void)0)
#define ffd_atomic_exchange_n(ptr,val,order) __sync_lock_test_and_set(ptr, val)
#define ffd_atomic_compare_exchange_n(ptr,expected,desired,weak,order1,order2) \
    __sync_bool_compare_and_swap(ptr, *(expected), desired) ? 1 : \
    (*(expected) = *(ptr), 0)
#define ffd_atomic_add_fetch(ptr,val,order) __sync_add_and_fetch(ptr, val)
#else
#define ffd_atomic_load(ptr,order) __atomic_load_n(ptr, order)
#define ffd_atomic_store(ptr,val,order) __atomic_store_n(ptr, val, order)
#define ffd_atomic_exchange(ptr,val,order) __atomic_exchange_n(ptr, val, order)
#define ffd_atomic_compare_exchange(ptr,expected,desired,order1,order2) \
    __atomic_compare_exchange_n(ptr, expected, desired, 1, order1, order2)
#define ffd_atomic_add_fetch(ptr,val,order) __atomic_add_fetch(ptr, val, order)
#endif

#endif
