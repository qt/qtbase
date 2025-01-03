// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWASMCSSSTYLE_H
#define QWASMCSSSTYLE_H

#include <QtCore/qglobal.h>

#include <emscripten/val.h>

QT_BEGIN_NAMESPACE

namespace QWasmCSSStyle {
emscripten::val createStyleElement(emscripten::val parent);
}

QT_END_NAMESPACE
#endif // QWASMINLINESTYLEREGISTRY_H
