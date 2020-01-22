/****************************************************************************
**
** Copyright (C) 2019 Intel Corporation.
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
** THE SOFTWARE.
**
****************************************************************************/

#ifndef FFD_ATOMIC_C11_H
#define FFD_ATOMIC_C11_H

/* atomics */
/* Using the C11 <stdatomic.h> header or C++11's <atomic>
 */

#if defined(__cplusplus)
#  include <atomic>
#  define ffd_atomic_pointer(type)  std::atomic<type*>
#  define FFD_ATOMIC_RELAXED        std::memory_order_relaxed
#  define FFD_ATOMIC_ACQUIRE        std::memory_order_acquire
#  define FFD_ATOMIC_RELEASE        std::memory_order_release
// acq_rel & cst not necessary
typedef std::atomic<int> ffd_atomic_int;
#else
#  include <stdatomic.h>
#  define ffd_atomic_pointer(type)  _Atomic(type*)
#  define FFD_ATOMIC_RELAXED        memory_order_relaxed
#  define FFD_ATOMIC_ACQUIRE        memory_order_acquire
#  define FFD_ATOMIC_RELEASE        memory_order_release
// acq_rel & cst not necessary

typedef atomic_int ffd_atomic_int;
#endif

#define FFD_ATOMIC_INIT(val)        ATOMIC_VAR_INIT(val)

#define ffd_atomic_load(ptr, order) \
    atomic_load_explicit(ptr, order)
#define ffd_atomic_store(ptr, val, order) \
    atomic_store_explicit(ptr, val, order)
#define ffd_atomic_exchange(ptr,val,order) \
    atomic_exchange_explicit(ptr, val, order)
#define ffd_atomic_compare_exchange(ptr, expected, desired, order1, order2) \
    atomic_compare_exchange_strong_explicit(ptr, expected, desired, order1, order2)
#define ffd_atomic_add_fetch(ptr, val, order) \
    (atomic_fetch_add_explicit(ptr, val, order) + (val))

#endif
