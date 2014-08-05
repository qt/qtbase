//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// tls.cpp: Simple cross-platform interface for thread local storage.

#include "common/tls.h"

#include <assert.h>

#if defined(ANGLE_PLATFORM_WINRT)
#include <vector>
std::vector<void *> *tls = nullptr;
std::vector<TLSIndex> *freeIndices = nullptr;
#endif

TLSIndex CreateTLSIndex()
{
    TLSIndex index;

#if defined(ANGLE_PLATFORM_WINRT)
    if (!tls)
        tls = new std::vector<void *>;
    if (freeIndices && !freeIndices->empty()) {
        index = freeIndices->back();
        freeIndices->pop_back();
        return index;
    } else {
        tls->push_back(nullptr);
        return tls->size() - 1;
    }
#elif defined(ANGLE_PLATFORM_WINDOWS)
    index = TlsAlloc();
#elif defined(ANGLE_PLATFORM_POSIX)
    // Create global pool key
    if ((pthread_key_create(&index, NULL)) != 0)
    {
        index = TLS_INVALID_INDEX;
    }
#endif

    assert(index != TLS_INVALID_INDEX && "CreateTLSIndex(): Unable to allocate Thread Local Storage");
    return index;
}

bool DestroyTLSIndex(TLSIndex index)
{
    assert(index != TLS_INVALID_INDEX && "DestroyTLSIndex(): Invalid TLS Index");
    if (index == TLS_INVALID_INDEX)
    {
        return false;
    }

#if defined(ANGLE_PLATFORM_WINRT)
    if (!freeIndices)
        freeIndices = new std::vector<TLSIndex>;
    freeIndices->push_back(index);
    return true;
#elif ANGLE_PLATFORM_WINDOWS
    return (TlsFree(index) == TRUE);
#elif defined(ANGLE_PLATFORM_POSIX)
    return (pthread_key_delete(index) == 0);
#endif
}

bool SetTLSValue(TLSIndex index, void *value)
{
    assert(index != TLS_INVALID_INDEX && "SetTLSValue(): Invalid TLS Index");
    if (index == TLS_INVALID_INDEX)
    {
        return false;
    }

#if defined(ANGLE_PLATFORM_WINRT)
    tls->at(index) = value;
    return true;
#elif defined(ANGLE_PLATFORM_WINDOWS)
    return (TlsSetValue(index, value) == TRUE);
#elif defined(ANGLE_PLATFORM_POSIX)
    return (pthread_setspecific(index, value) == 0);
#endif
}

void *GetTLSValue(TLSIndex index)
{
#if !defined(ANGLE_PLATFORM_WINRT) // Valid on WinRT, as Alloc handles the index creation
    assert(index != TLS_INVALID_INDEX && "GetTLSValue(): Invalid TLS Index");
#endif
    if (index == TLS_INVALID_INDEX)
    {
        return NULL;
    }

#if defined(ANGLE_PLATFORM_WINRT)
    return tls->at(index);
#elif defined(ANGLE_PLATFORM_WINDOWS)
    return TlsGetValue(index);
#elif defined(ANGLE_PLATFORM_POSIX)
    return pthread_getspecific(index);
#endif
}
