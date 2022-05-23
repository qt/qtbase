// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: MIT

// This is the test found in https://sourceware.org/bugzilla/show_bug.cgi?id=29087

#include "lib.h"

struct Local : S { };
int main()
{
    Local l;
}
