//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "libGLESv2/renderer/d3d/HLSLCompiler.h"
#include "libGLESv2/Program.h"
#include "libGLESv2/main.h"

#include "common/utilities.h"
#include "common/platform.h"

#if defined(__MINGW32__) && !defined(D3DCOMPILER_DLL)

// Add define + typedefs for older MinGW-w64 headers (pre 5783)

#define D3DCOMPILER_DLL L"d3dcompiler_43.dll"

HRESULT WINAPI D3DCompile(const void *data, SIZE_T data_size, const char *filename,
        const D3D_SHADER_MACRO *defines, ID3DInclude *include, const char *entrypoint,
        const char *target, UINT sflags, UINT eflags, ID3DBlob **shader, ID3DBlob **error_messages);
typedef HRESULT (WINAPI *pD3DCompile)(const void *data, SIZE_T data_size, const char *filename,
        const D3D_SHADER_MACRO *defines, ID3DInclude *include, const char *entrypoint,
        const char *target, UINT sflags, UINT eflags, ID3DBlob **shader, ID3DBlob **error_messages);

#endif // __MINGW32__ && !D3DCOMPILER_DLL

#ifndef QT_D3DCOMPILER_DLL
#define QT_D3DCOMPILER_DLL D3DCOMPILER_DLL
#endif

namespace rx
{

HLSLCompiler::HLSLCompiler()
    : mD3DCompilerModule(NULL),
      mD3DCompileFunc(NULL)
{
}

HLSLCompiler::~HLSLCompiler()
{
    release();
}

bool HLSLCompiler::initialize()
{
#if !defined(ANGLE_PLATFORM_WINRT)
#if defined(ANGLE_PRELOADED_D3DCOMPILER_MODULE_NAMES)
    // Find a D3DCompiler module that had already been loaded based on a predefined list of versions.
    static const char *d3dCompilerNames[] = ANGLE_PRELOADED_D3DCOMPILER_MODULE_NAMES;

    for (size_t i = 0; i < ArraySize(d3dCompilerNames); ++i)
    {
        if (GetModuleHandleExA(0, d3dCompilerNames[i], &mD3DCompilerModule))
        {
            break;
        }
    }
#endif  // ANGLE_PRELOADED_D3DCOMPILER_MODULE_NAMES

    // Load the compiler DLL specified by the environment, or default to QT_D3DCOMPILER_DLL
    const wchar_t *defaultCompiler = _wgetenv(L"QT_D3DCOMPILER_DLL");
    if (!defaultCompiler)
        defaultCompiler = QT_D3DCOMPILER_DLL;

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
        mD3DCompilerModule = LoadLibrary(compilerDlls[i]);
        if (mD3DCompilerModule)
            break;
    }

    if (!mD3DCompilerModule)
    {
        ERR("No D3D compiler module found - aborting!\n");
        return false;
    }

    mD3DCompileFunc = reinterpret_cast<CompileFuncPtr>(GetProcAddress(mD3DCompilerModule, "D3DCompile"));
    ASSERT(mD3DCompileFunc);
#else
    mD3DCompileFunc = reinterpret_cast<CompileFuncPtr>(&D3DCompile);
#endif
    return mD3DCompileFunc != NULL;
}

void HLSLCompiler::release()
{
    if (mD3DCompilerModule)
    {
        FreeLibrary(mD3DCompilerModule);
        mD3DCompilerModule = NULL;
        mD3DCompileFunc = NULL;
    }
}

ShaderBlob *HLSLCompiler::compileToBinary(gl::InfoLog &infoLog, const char *hlsl, const char *profile,
                                          const UINT optimizationFlags[], const char *flagNames[], int attempts) const
{
#if !defined(ANGLE_PLATFORM_WINRT)
    ASSERT(mD3DCompilerModule && mD3DCompileFunc);
#endif

    if (!hlsl)
    {
        return NULL;
    }

    pD3DCompile compileFunc = reinterpret_cast<pD3DCompile>(mD3DCompileFunc);
    for (int i = 0; i < attempts; ++i)
    {
        ID3DBlob *errorMessage = NULL;
        ID3DBlob *binary = NULL;

        HRESULT result = compileFunc(hlsl, strlen(hlsl), gl::g_fakepath, NULL, NULL, "main", profile, optimizationFlags[i], 0, &binary, &errorMessage);

        if (errorMessage)
        {
            const char *message = (const char*)errorMessage->GetBufferPointer();

            infoLog.appendSanitized(message);
            TRACE("\n%s", hlsl);
            TRACE("\n%s", message);

            SafeRelease(errorMessage);
        }

        if (SUCCEEDED(result))
        {
            return (ShaderBlob*)binary;
        }
        else
        {
            if (result == E_OUTOFMEMORY)
            {
                return gl::error(GL_OUT_OF_MEMORY, (ShaderBlob*)NULL);
            }

            infoLog.append("Warning: D3D shader compilation failed with %s flags.", flagNames[i]);

            if (i + 1 < attempts)
            {
                infoLog.append(" Retrying with %s.\n", flagNames[i + 1]);
            }
        }
    }

    return NULL;
}

}
