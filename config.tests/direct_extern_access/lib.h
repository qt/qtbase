// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: MIT

// This is the test found in https://sourceware.org/bugzilla/show_bug.cgi?id=29087

#ifdef BUILD
#  define LIB_API __attribute__((visibility("protected")))
#else
#  define LIB_API __attribute__((visibility("default")))
#endif

struct LIB_API S
{
    virtual ~S();
    virtual void *f();
    static void *ptr;
};
