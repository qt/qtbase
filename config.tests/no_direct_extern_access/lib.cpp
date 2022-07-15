// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: MIT

// This is the test found in https://sourceware.org/bugzilla/show_bug.cgi?id=29087

#define BUILD
#include "lib.h"

void *S::ptr = nullptr;
S::~S() { }
void *S::f() { return ptr; }
