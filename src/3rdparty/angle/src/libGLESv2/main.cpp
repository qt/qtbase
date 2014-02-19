#include "precompiled.h"
//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// main.cpp: DLL entry point and management of thread-local data.

#include "libGLESv2/main.h"

#include "libGLESv2/Context.h"

#ifndef QT_OPENGL_ES_2_ANGLE_STATIC

#if !defined(ANGLE_OS_WINRT)
static DWORD currentTLS = TLS_OUT_OF_INDEXES;
#else
static __declspec(thread) void *currentTLS = 0;
#endif

namespace gl { Current *getCurrent(); }

extern "C" BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
      case DLL_PROCESS_ATTACH:
        {
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
            gl::Current *current = gl::getCurrent();

            if (current)
            {
#if !defined(ANGLE_OS_WINRT)
                TlsSetValue(currentTLS, current);
#endif
                current->context = NULL;
                current->display = NULL;
            }
        }
        break;
      case DLL_THREAD_DETACH:
        {
            gl::Current *current = gl::getCurrent();

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
            gl::Current *current = gl::getCurrent();

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

namespace gl
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
    static gl::Current curr = { 0, 0 };
    return &curr;
#endif
}

void makeCurrent(Context *context, egl::Display *display, egl::Surface *surface)
{
    Current *current = getCurrent();

    current->context = context;
    current->display = display;

    if (context && display && surface)
    {
        context->makeCurrent(surface);
    }
}

Context *getContext()
{
    Current *current = getCurrent();

    return current->context;
}

Context *getNonLostContext()
{
    Context *context = getContext();
    
    if (context)
    {
        if (context->isContextLost())
        {
            gl::error(GL_OUT_OF_MEMORY);
            return NULL;
        }
        else
        {
            return context;
        }
    }
    return NULL;
}

egl::Display *getDisplay()
{
    Current *current = getCurrent();

    return current->display;
}

// Records an error code
void error(GLenum errorCode)
{
    gl::Context *context = glGetCurrentContext();

    if (context)
    {
        switch (errorCode)
        {
          case GL_INVALID_ENUM:
            context->recordInvalidEnum();
            TRACE("\t! Error generated: invalid enum\n");
            break;
          case GL_INVALID_VALUE:
            context->recordInvalidValue();
            TRACE("\t! Error generated: invalid value\n");
            break;
          case GL_INVALID_OPERATION:
            context->recordInvalidOperation();
            TRACE("\t! Error generated: invalid operation\n");
            break;
          case GL_OUT_OF_MEMORY:
            context->recordOutOfMemory();
            TRACE("\t! Error generated: out of memory\n");
            break;
          case GL_INVALID_FRAMEBUFFER_OPERATION:
            context->recordInvalidFramebufferOperation();
            TRACE("\t! Error generated: invalid framebuffer operation\n");
            break;
          default: UNREACHABLE();
        }
    }
}

}

