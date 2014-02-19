#include "precompiled.h"
//
// Copyright (c) 2012-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Renderer.cpp: Implements EGL dependencies for creating and destroying Renderer instances.

#include <EGL/eglext.h>
#include "libGLESv2/main.h"
#include "libGLESv2/Program.h"
#include "libGLESv2/renderer/Renderer.h"
#ifndef ANGLE_ENABLE_D3D11
#include "libGLESv2/renderer/Renderer9.h"
#else
#include "libGLESv2/renderer/Renderer11.h"
#endif
#include "libGLESv2/utilities.h"
#include "third_party/trace_event/trace_event.h"

#if !defined(ANGLE_ENABLE_D3D11)
// Enables use of the Direct3D 11 API for a default display, when available
#define ANGLE_ENABLE_D3D11 0
#endif

#ifndef D3DERR_OUTOFVIDEOMEMORY
#define D3DERR_OUTOFVIDEOMEMORY MAKE_HRESULT(1, 0x876, 380)
#endif

#ifndef D3DCOMPILER_DLL
#define D3DCOMPILER_DLL L"d3dcompiler_43.dll" // Lowest common denominator
#endif

#ifndef QT_D3DCOMPILER_DLL
#define QT_D3DCOMPILER_DLL D3DCOMPILER_DLL
#endif

#if defined(__MINGW32__) || defined(ANGLE_OS_WINPHONE)

//Add define + typedefs for older MinGW-w64 headers (pre 5783)
//Also define these on Windows Phone, which doesn't have a shader compiler

HRESULT WINAPI D3DCompile(const void *data, SIZE_T data_size, const char *filename,
        const D3D_SHADER_MACRO *defines, ID3DInclude *include, const char *entrypoint,
        const char *target, UINT sflags, UINT eflags, ID3DBlob **shader, ID3DBlob **error_messages);
typedef HRESULT (WINAPI *pD3DCompile)(const void *data, SIZE_T data_size, const char *filename,
        const D3D_SHADER_MACRO *defines, ID3DInclude *include, const char *entrypoint,
        const char *target, UINT sflags, UINT eflags, ID3DBlob **shader, ID3DBlob **error_messages);

#endif // __MINGW32__ || ANGLE_OS_WINPHONE

namespace rx
{

Renderer::Renderer(egl::Display *display) : mDisplay(display)
{
    mD3dCompilerModule = NULL;
    mD3DCompileFunc = NULL;
}

Renderer::~Renderer()
{
    if (mD3dCompilerModule)
    {
        FreeLibrary(mD3dCompilerModule);
        mD3dCompilerModule = NULL;
    }
}

bool Renderer::initializeCompiler()
{
    TRACE_EVENT0("gpu", "initializeCompiler");
#if defined(ANGLE_PRELOADED_D3DCOMPILER_MODULE_NAMES)
    // Find a D3DCompiler module that had already been loaded based on a predefined list of versions.
    static TCHAR* d3dCompilerNames[] = ANGLE_PRELOADED_D3DCOMPILER_MODULE_NAMES;

    for (size_t i = 0; i < ArraySize(d3dCompilerNames); ++i)
    {
        if (GetModuleHandleEx(0, d3dCompilerNames[i], &mD3dCompilerModule))
        {
            break;
        }
    }
#else
    // Load the compiler DLL specified by the environment, or default to QT_D3DCOMPILER_DLL
#if !defined(ANGLE_OS_WINRT)
    const wchar_t *defaultCompiler = _wgetenv(L"QT_D3DCOMPILER_DLL");
    if (!defaultCompiler)
        defaultCompiler = QT_D3DCOMPILER_DLL;
#else // !ANGLE_OS_WINRT
#  ifdef _DEBUG
    const wchar_t *defaultCompiler = L"d3dcompiler_qtd.dll";
#  else
    const wchar_t *defaultCompiler = L"d3dcompiler_qt.dll";
#  endif
#endif // ANGLE_OS_WINRT

    const wchar_t *compilerDlls[] = {
        defaultCompiler,
        L"d3dcompiler_47.dll",
        L"d3dcompiler_46.dll",
        L"d3dcompiler_45.dll",
        L"d3dcompiler_44.dll",
        L"d3dcompiler_43.dll",
        0
    };

    // Load the first available known compiler DLL
    for (int i = 0; compilerDlls[i]; ++i)
    {
#if !defined(ANGLE_OS_WINRT)
        mD3dCompilerModule = LoadLibrary(compilerDlls[i]);
#else
        mD3dCompilerModule = LoadPackagedLibrary(compilerDlls[i], NULL);
#endif
        if (mD3dCompilerModule)
            break;
    }
#endif  // ANGLE_PRELOADED_D3DCOMPILER_MODULE_NAMES

    if (!mD3dCompilerModule)
    {
        ERR("No D3D compiler module found - aborting!\n");
        return false;
    }

    mD3DCompileFunc = reinterpret_cast<pCompileFunc>(GetProcAddress(mD3dCompilerModule, "D3DCompile"));
    ASSERT(mD3DCompileFunc);

    return mD3DCompileFunc != NULL;
}

// Compiles HLSL code into executable binaries
ShaderBlob *Renderer::compileToBinary(gl::InfoLog &infoLog, const char *hlsl, const char *profile, UINT optimizationFlags, bool alternateFlags)
{
    if (!hlsl)
    {
        return NULL;
    }

    HRESULT result = S_OK;
    UINT flags = 0;
    std::string sourceText;
    if (gl::perfActive())
    {
        flags |= D3DCOMPILE_DEBUG;

#ifdef NDEBUG
        flags |= optimizationFlags;
#else
        flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

        std::string sourcePath = getTempPath();
        sourceText = std::string("#line 2 \"") + sourcePath + std::string("\"\n\n") + std::string(hlsl);
        writeFile(sourcePath.c_str(), sourceText.c_str(), sourceText.size());
    }
    else
    {
        flags |= optimizationFlags;
        sourceText = hlsl;
    }

    // Sometimes D3DCompile will fail with the default compilation flags for complicated shaders when it would otherwise pass with alternative options.
    // Try the default flags first and if compilation fails, try some alternatives.
    const static UINT extraFlags[] =
    {
        0,
        D3DCOMPILE_AVOID_FLOW_CONTROL,
        D3DCOMPILE_PREFER_FLOW_CONTROL
    };

    const static char * const extraFlagNames[] =
    {
        "default",
        "avoid flow control",
        "prefer flow control"
    };

    int attempts = alternateFlags ? ArraySize(extraFlags) : 1;
    pD3DCompile compileFunc = reinterpret_cast<pD3DCompile>(mD3DCompileFunc);
    for (int i = 0; i < attempts; ++i)
    {
        ID3DBlob *errorMessage = NULL;
        ID3DBlob *binary = NULL;

        result = compileFunc(hlsl, strlen(hlsl), gl::g_fakepath, NULL, NULL,
                             "main", profile, flags | extraFlags[i], 0, &binary, &errorMessage);
        if (errorMessage)
        {
            const char *message = (const char*)errorMessage->GetBufferPointer();

            infoLog.appendSanitized(message);
            TRACE("\n%s", hlsl);
            TRACE("\n%s", message);

            errorMessage->Release();
            errorMessage = NULL;
        }

        if (SUCCEEDED(result))
        {
            return (ShaderBlob*)binary;
        }
        else
        {
            if (result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY)
            {
                return gl::error(GL_OUT_OF_MEMORY, (ShaderBlob*) NULL);
            }

            infoLog.append("Warning: D3D shader compilation failed with ");
            infoLog.append(extraFlagNames[i]);
            infoLog.append(" flags.");
            if (i + 1 < attempts)
            {
                infoLog.append(" Retrying with ");
                infoLog.append(extraFlagNames[i + 1]);
                infoLog.append(".\n");
            }
        }
    }

    return NULL;
}

}

extern "C"
{

rx::Renderer *glCreateRenderer(egl::Display *display, HDC hDc, EGLNativeDisplayType displayId)
{
    rx::Renderer *renderer = NULL;
    EGLint status = EGL_BAD_ALLOC;

#if ANGLE_ENABLE_D3D11
    renderer = new rx::Renderer11(display, hDc);
#else
    bool softwareDevice = (displayId == EGL_SOFTWARE_DISPLAY_ANGLE);
    renderer = new rx::Renderer9(display, hDc, softwareDevice);
#endif

    if (renderer)
    {
        status = renderer->initialize();
    }

    if (status == EGL_SUCCESS)
    {
        return renderer;
    }

    return NULL;
}

void glDestroyRenderer(rx::Renderer *renderer)
{
    delete renderer;
}

}
