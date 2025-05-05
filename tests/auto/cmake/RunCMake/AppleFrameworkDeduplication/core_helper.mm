// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <Foundation/Foundation.h>

// We need at least one Objective-C class or category to be defined in the file, so that
// -ObjC triggers the second loading of the same static framework.
//  Note the duplicate symbol errors happen when linking static frameworks, note static libraries.
// This seems to be a gotcha in both the classic ld linker and in the new ld64 ld_prime linker.

@interface SimpleClass : NSObject
- (void)helloFunc;
@end

@implementation SimpleClass
- (void)helloFunc
{
    return;
}
@end

int core_helper_func() { return 0; };
