// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (c) 2007-2008, Apple, Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef QCOCOAINTROSPECTION_H
#define QCOCOAINTROSPECTION_H

#include <qglobal.h>
#include <objc/objc-class.h>

QT_BEGIN_NAMESPACE

void qt_cocoa_change_implementation(Class baseClass, SEL originalSel, Class proxyClass, SEL replacementSel = nil, SEL backupSel = nil);
void qt_cocoa_change_back_implementation(Class baseClass, SEL originalSel, SEL backupSel);

QT_END_NAMESPACE

#endif // QCOCOAINTROSPECTION_H
