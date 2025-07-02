// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmstring.h"

QT_BEGIN_NAMESPACE

using namespace emscripten;

val QWasmString::fromQString(const QString &str)
{
    static const val UTF16ToString(
        val::module_property("UTF16ToString"));

    auto ptr = quintptr(str.utf16());
    return UTF16ToString(val(ptr));
}

QString QWasmString::toQString(const val &v)
{
    QString result;
    if (!v.isString())
        return result;

    static const val stringToUTF16(
        val::module_property("stringToUTF16"));
    static const val length("length");

    int len = v[length].as<int>();
    result.resize(len);
    auto ptr = quintptr(result.utf16());
    stringToUTF16(v, val(ptr), val((len + 1) * 2));
    return result;
}

QT_END_NAMESPACE
