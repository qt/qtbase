// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: MIT

// This is combining the tests found in:
// https://sourceware.org/bugzilla/show_bug.cgi?id=29087
// https://sourceware.org/bugzilla/show_bug.cgi?id=29377

#include "lib.h"

extern void foo(); // other.cpp
void (*get_foo())()
{
    return foo;
}

struct Local : S { };
int main()
{
    Local l;
}
