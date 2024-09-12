/****************************************************************************
** Resource object code
** Copyright (C) 2024 Intel Corporation.
** SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
**
** Created by: The Resource Compiler for Qt version 6.8.1
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#ifdef _MSC_VER
// disable informational message "function ... selected for automatic inline expansion"
#pragma warning (disable: 4711)
#endif

static const unsigned char qt_resource_data[] = {
  // parentdir.txt
  0x0,0x0,0x0,0x1a,
  0x61,
  0x62,0x63,0x64,0x65,0x66,0x67,0x69,0x68,0x6b,0x6c,0x6d,0x6e,0x6f,0x70,0x71,0x72,
  0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0xa,
  
};

static const unsigned char qt_resource_name[] = {
  // parentdir.txt
  0x0,0xd,
  0x6,0x14,0xd1,0x74,
  0x0,0x70,
  0x0,0x61,0x0,0x72,0x0,0x65,0x0,0x6e,0x0,0x74,0x0,0x64,0x0,0x69,0x0,0x72,0x0,0x2e,0x0,0x74,0x0,0x78,0x0,0x74,
  
};

static const unsigned char qt_resource_struct[] = {
  // :
  0x0,0x0,0x0,0x0,0x0,0x2,0x0,0x0,0x0,0x1,0x0,0x0,0x0,0x1,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  // :/parentdir.txt
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x1,0x0,0x0,0x0,0x0,
TIMESTAMP:../parentdir.txt

};

#ifdef QT_NAMESPACE
#  define QT_RCC_PREPEND_NAMESPACE(name) ::QT_NAMESPACE::name
#  define QT_RCC_MANGLE_NAMESPACE0(x) x
#  define QT_RCC_MANGLE_NAMESPACE1(a, b) a##_##b
#  define QT_RCC_MANGLE_NAMESPACE2(a, b) QT_RCC_MANGLE_NAMESPACE1(a,b)
#  define QT_RCC_MANGLE_NAMESPACE(name) QT_RCC_MANGLE_NAMESPACE2( \
        QT_RCC_MANGLE_NAMESPACE0(name), QT_RCC_MANGLE_NAMESPACE0(QT_NAMESPACE))
#else
#   define QT_RCC_PREPEND_NAMESPACE(name) name
#   define QT_RCC_MANGLE_NAMESPACE(name) name
#endif

#if defined(QT_INLINE_NAMESPACE)
inline namespace QT_NAMESPACE {
#elif defined(QT_NAMESPACE)
namespace QT_NAMESPACE {
#endif

bool qRegisterResourceData(int, const unsigned char *, const unsigned char *, const unsigned char *);
bool qUnregisterResourceData(int, const unsigned char *, const unsigned char *, const unsigned char *);

#ifdef QT_NAMESPACE
}
#endif

int QT_RCC_MANGLE_NAMESPACE(qInitResources)();
int QT_RCC_MANGLE_NAMESPACE(qInitResources)()
{
    int version = 3;
    QT_RCC_PREPEND_NAMESPACE(qRegisterResourceData)
        (version, qt_resource_struct, qt_resource_name, qt_resource_data);
    return 1;
}

int QT_RCC_MANGLE_NAMESPACE(qCleanupResources)();
int QT_RCC_MANGLE_NAMESPACE(qCleanupResources)()
{
    int version = 3;
    QT_RCC_PREPEND_NAMESPACE(qUnregisterResourceData)
       (version, qt_resource_struct, qt_resource_name, qt_resource_data);
    return 1;
}

#ifdef __clang__
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif

namespace {
   struct initializer {
       initializer() { QT_RCC_MANGLE_NAMESPACE(qInitResources)(); }
       ~initializer() { QT_RCC_MANGLE_NAMESPACE(qCleanupResources)(); }
   } dummy;
}

#ifdef __clang__
#   pragma clang diagnostic pop
#endif
