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

static inline egl::Current *current()
{
    return (egl::Current*)TlsGetValue(currentTLS);
}

#else // !QT_OPENGL_ES_2_ANGLE_STATIC

static egl::Current *current()
{
    // No precautions for thread safety taken as ANGLE is used single-threaded in Qt.
    static egl::Current curr = { EGL_SUCCESS, EGL_OPENGL_ES_API, EGL_NO_DISPLAY, EGL_NO_SURFACE, EGL_NO_SURFACE };
    return &curr;
}

#endif // QT_OPENGL_ES_2_ANGLE_STATIC

namespace egl
{
void setCurrentError(EGLint error)
{
    current()->error = error;
}

EGLint getCurrentError()
{
    return current()->error;
}

void setCurrentAPI(EGLenum API)
{
    current()->API = API;
}

EGLenum getCurrentAPI()
{
    return current()->API;
}

void setCurrentDisplay(EGLDisplay dpy)
{
    current()->display = dpy;
}

EGLDisplay getCurrentDisplay()
{
    return current()->display;
}

void setCurrentDrawSurface(EGLSurface surface)
{
    current()->drawSurface = surface;
}

EGLSurface getCurrentDrawSurface()
{
    return current()->drawSurface;
}

void setCurrentReadSurface(EGLSurface surface)
{
    current()->readSurface = surface;
}

EGLSurface getCurrentReadSurface()
{
    return current()->readSurface;
}

void error(EGLint errorCode)
{
    egl::setCurrentError(errorCode);
}

}
