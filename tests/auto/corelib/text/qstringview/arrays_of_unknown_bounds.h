// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#pragma once

#include <QtTest/qtest.h>

extern const char16_t u16string_array[];
extern const int u16string_array_size;
extern const wchar_t wstring_array[];
extern const int wstring_array_size;

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
