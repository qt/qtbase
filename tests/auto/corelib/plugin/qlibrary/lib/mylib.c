// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qglobal.h>

#if defined(Q_CC_MSVC) || defined(Q_CC_MSVC_NET) || defined(Q_CC_BOR)
#define LIB_EXPORT __declspec(dllexport)
#else
#define LIB_EXPORT
#endif

#if defined(Q_CC_BOR)
# define BORLAND_STDCALL __stdcall
#else
# define BORLAND_STDCALL
#endif

LIB_EXPORT int BORLAND_STDCALL mylibversion()
{
    return 1;
}

