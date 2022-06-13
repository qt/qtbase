// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <moc_object1.cpp>
#include <moc_object2.cpp>
#include <moc_object3.cpp>
#include     "object4.h"
#include <moc_object5.cpp>
#include <moc_object6.cpp>
#include <moc_object7.cpp>
#include     "object8.h"
#include <moc_object9.cpp>

int main() { return 0'000; }
/* Included *after* the use of a numeric literal with an *odd* number of digit
   separator tick marks in it (and no subsequent apostrophe to end the
   "single-quoted character literal" that the old parser though it was thus left
   in).  */
#include <moc_digitseparated.cpp>
