// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWASMDOM_H
#define QWASMDOM_H

#include <QtCore/qtconfigmacros.h>

#include <emscripten/val.h>

#include <functional>
#include <memory>
#include <string>

QT_BEGIN_NAMESPACE

class QPoint;

namespace dom {
inline emscripten::val document()
{
    return emscripten::val::global("document");
}

void syncCSSClassWith(emscripten::val element, std::string cssClassName, bool flag);

QPointF mapPoint(emscripten::val source, emscripten::val target, const QPointF &point);
} // namespace dom

QT_END_NAMESPACE
#endif // QWASMDOM_H
