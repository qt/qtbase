// Copyright (C) 2025 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QString>

inline QString androidAbi()
{
#if defined(__aarch64__)
  return "arm64-v8a";
#elif defined(__arm__)
  return "armeabi-v7a";
#elif defined(__i386__)
  return "x86";
#elif defined(__x86_64__)
  return "x86_64";
#endif
}

