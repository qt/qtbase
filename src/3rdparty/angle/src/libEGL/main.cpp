#include "../libGLESv2/precompiled.h"
//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// main.cpp: DLL entry point and management of thread-local data.

#include "libEGL/main.h"

#include "common/debug.h"

#ifndef QT_OPENGL_ES_2_ANGLE_STATIC

#if !defined(ANGLE_OS_WINRT)
static DWORD currentTLS = TLS_OUT_OF_INDEXES;
#else
static __declspec(thread) void *currentTLS = 0;
#endif

namespace egl { Current *getCurrent(); }

extern "C" BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
      case DLL_PROCESS_ATTACH:
        {
#if !defined(ANGLE_DISABLE_TRACE)
            FILE *debug = fopen(TRACE_OUTPUT_FILE, "rt");

            if (debug)
            {
                fclose(debug);
                debug = fopen(TRACE_OUTPUT_FILE, "wt");   // Erase
                
                if (debug)
                {
                    fclose(debug);
                }
            }
#endif

#if !defined(ANGLE_OS_WINRT)
            currentTLS = TlsAlloc();

            if (currentTLS == TLS_OUT_OF_INDEXES)
            {
                return FALSE;
            }
#endif
        }
        // Fall throught to initialize index
      case DLL_THREAD_ATTACH:
        {
            egl::Current *current = egl::getCurrent();

            if (current)
            {
#if !defined(ANGLE_OS_WINRT)
                TlsSetValue(currentTLS, current);
#endif
                current->error = EGL_SUCCESS;
                current->API = EGL_OPENGL_ES_API;
                current->display = EGL_NO_DISPLAY;
                current->drawSurface = EGL_NO_SURFACE;
                current->readSurface = EGL_NO_SURFACE;
            }
        }
        break;
      case DLL_THREAD_DETACH:
        {
            egl::Current *current = egl::getCurrent();

            if (current)
            {
#if !defined(ANGLE_OS_WINRT)
                LocalFree((HLOCAL)current);
#else
                HeapFree(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, current);
                currentTLS = 0;
#endif
            }
        }
        break;
      case DLL_PROCESS_DETACH:
        {
            egl::Current *current = egl::getCurrent();

            if (current)
            {
#if !defined(ANGLE_OS_WINRT)
                LocalFree((HLOCAL)current);
            }

            TlsFree(currentTLS);
#else
                HeapFree(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, current);
                currentTLS = 0;
            }
#endif
        }
        break;
      default:
        break;
    }

    return TRUE;
}

#endif // !QT_OPENGL_ES_2_ANGLE_STATIC

namespace egl
{
Current *getCurrent()
{
#ifndef QT_OPENGL_ES_2_ANGLE_STATIC
#if !defined(ANGLE_OS_WINRT)
    Current *current = (Current*)TlsGetValue(currentTLS);
    if (!current)
        current = (Current*)LocalAlloc(LPTR, sizeof(Current));
    return current;
#else
    if (!currentTLS)
        currentTLS = HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS|HEAP_ZERO_MEMORY, sizeof(Current));
    return (Current*)currentTLS;
#endif
#else
    // No precautions for thread safety taken as ANGLE is used single-threaded in Qt.
    static Current curr = { EGL_SUCCESS, EGL_OPENGL_ES_API, EGL_NO_DISPLAY, EGL_NO_SURFACE, EGL_NO_SURFACE };
    return &curr;
#endif
}

void setCurrentError(EGLint error)
{
    Current *current = getCurrent();

    current->error = error;
}

EGLint getCurrentError()
{
    Current *current = getCurrent();

    return current->error;
}

void setCurrentAPI(EGLenum API)
{
    Current *current = getCurrent();

    current->API = API;
}

EGLenum getCurrentAPI()
{
    Current *current = getCurrent();

    return current->API;
}

void setCurrentDisplay(EGLDisplay dpy)
{
    Current *current = getCurrent();

    current->display = dpy;
}

EGLDisplay getCurrentDisplay()
{
    Current *current = getCurrent();

    return current->display;
}

void setCurrentDrawSurface(EGLSurface surface)
{
    Current *current = getCurrent();

    current->drawSurface = surface;
}

EGLSurface getCurrentDrawSurface()
{
    Current *current = getCurrent();

    return current->drawSurface;
}

void setCurrentReadSurface(EGLSurface surface)
{
    Current *current = getCurrent();

    current->readSurface = surface;
}

EGLSurface getCurrentReadSurface()
{
    Current *current = getCurrent();

    return current->readSurface;
}

void error(EGLint errorCode)
{
    egl::setCurrentError(errorCode);
}

}
