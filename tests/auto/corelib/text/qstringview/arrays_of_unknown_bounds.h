// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#pragma once

#include <QtTest/qtest.h>

extern const char string_array[];
extern const int string_array_size;

#ifdef __cpp_char8_t
extern const char8_t u8string_array[];
extern const int u8string_array_size;
#endif

extern const uchar ustring_array[];
extern const int ustring_array_size;

extern const signed char sstring_array[];
extern const int sstring_array_size;

extern const std::byte byte_array[];
extern const int byte_array_size;

extern const char16_t u16string_array[];
extern const int u16string_array_size;

extern const wchar_t wstring_array[];
extern const int wstring_array_size;

template <typename StringView>
void from_array_of_unknown_size()
{
    StringView bv = string_array;
    QCOMPARE(bv.size(), string_array_size);
}

#ifdef __cpp_char8_t
template <typename StringView>
void from_u8array_of_unknown_size()
{
    StringView sv = u8string_array;
    QCOMPARE(sv.size(), u8string_array_size);
}
#endif

template <typename StringView>
void from_uarray_of_unknown_size()
{
    StringView bv = ustring_array;
    QCOMPARE(bv.size(), ustring_array_size);
}

template <typename StringView>
void from_sarray_of_unknown_size()
{
    StringView bv = sstring_array;
    QCOMPARE(bv.size(), sstring_array_size);
}

template <typename StringView>
void from_byte_array_of_unknown_size()
{
    StringView bv = byte_array;
    QCOMPARE(bv.size(), byte_array_size);
}

template <typename StringView>
void from_u16array_of_unknown_size()
{
    StringView sv = u16string_array;
    QCOMPARE(sv.size(), u16string_array_size);
}

template <typename StringView>
void from_warray_of_unknown_size()
{
    StringView sv = wstring_array;
    QCOMPARE(sv.size(), wstring_array_size);
}
