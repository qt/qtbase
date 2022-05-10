// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#pragma once

#include <qstring.h>

#include <emscripten/val.h>

QT_BEGIN_NAMESPACE

class QWasmString
{
public:
    static emscripten::val fromQString(const QString &str);
    static QString toQString(const emscripten::val &v);
};
QT_END_NAMESPACE

