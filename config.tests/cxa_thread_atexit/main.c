// Copyright (C) 2024 Intel Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include <stddef.h>

#if defined(__vxworks) || defined(__VXWORKS__)
/*
 * In VxWorks pthread implementation, when thread is finishing it calls pthread_exit(), which:
 *  - calls pthread cleanup procedure
 *  - clears pthread data (setting current thread to null)
 *  - calls `taskExit` VxWorks system procedure
 * `taskExit` procedure calls thread_local object destructors, which if call directly or indirectly
 * `pthread_self()`, terminate with error, due to not being able to obtain pthread entity.
 * Thus, we need to treat VxWorks pthread implementation as `broken` in terms of thread_local variables.
 */
#   error "VxWorks threadlocal destructors are called after pthread data is destroyed, causing crash."
#endif

typedef void (*dtor_func) (void *);
int TEST_FUNC(dtor_func func, void *obj, void *dso_symbol);
int main()
{
    return TEST_FUNC(NULL, NULL, NULL);
}
