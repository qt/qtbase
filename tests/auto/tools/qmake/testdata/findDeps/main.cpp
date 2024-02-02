// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#define spurious \
 / #include "needed.cpp"
// if not ignored, symbol needed() won't be available ...

// Check we're not confused by string juxtaposition:
static const char text[] = "lorem ""ipsum /*";

            #include <moc_object1.cpp>
/**/        #include "\
moc_object2.cpp\
"
/**//**/    #include <moc_\
o\
b\
j\
e\
c\
t\
3\
.cpp>
/*'"*/      #include <moc_object4.cpp>
/*"'
*/          #include <moc_object5.cpp> /*
#include "missing.cpp"
*/// a backslash newline does make the next line part of this comment \
/* so this text is in last line's C++-style comment, not a C-comment !
#include <moc_object6.cpp>
#if 0
#pragma "ignore me" '&' L"me"
#line 4321 "main.cpp" more /* preprocessing */ tokens
#endif

static void function1();
#include/* every comment
gets replaced (in phase 3) by a single
space */<moc_object7.cpp>
static void function2(); /**/
#include \
<moc_object8.cpp>
static void function3(); //
#include <moc_object9.cpp>
/* backslash-newline elimination happens in phase 2 *\
/ # /* and that's valid here, too. *\
/ include/* and, of course, here *\
/<moc_objecta.cpp>// while we're here, ... \
#include "needed.cpp"

int main () {
    extern int needed(void);
    return needed();
}

/*
  Deliberately end file in a #include, with nothing after it but the mandatory
  (unescaped) newline at the end of every source file.
*/
#include "moc_objectf.cpp"
