//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// main.cpp: DLL entry point and management of thread-local data.

#include "libEGL/main.h"

#include "common/debug.h"

#ifndef QT_OPENGL_ES_2_ANGLE_STATIC

static DWORD currentTLS = TLS_OUT_OF_INDEXES;

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

            currentTLS = TlsAlloc();

            if (currentTLS == TLS_OUT_OF_INDEXES)
            {
                return FALSE;
            }
        }
        // Fall throught to initialize index
      case DLL_THREAD_ATTACH:
        {
            egl::Current *current = (egl::Current*)LocalAlloc(LPTR, sizeof(egl::Current));

            if (current)
            {
                TlsSetValue(currentTLS, current);

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
            void *current = TlsGetValue(currentTLS);

            if (current)
            {
                LocalFree((HLOCAL)current);
            }
        }
        break;
      case DLL_PROCESS_DETACH:
        {
            void *current = TlsGetValue(currentTLS);

            if (current)
            {
                LocalFree((HLOCAL)current);
            }

            TlsFree(currentTLS);
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
    return (Current*)TlsGetValue(currentTLS);
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
