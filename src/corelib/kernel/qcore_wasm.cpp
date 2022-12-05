// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qrect.h>

#include <emscripten/val.h>

#if !defined(Q_OS_WASM)
#error This is a wasm-only file.
#endif // !defined(Q_OS_WASM)

QT_BEGIN_NAMESPACE

/*!
    Converts the DOMRect (https://www.w3.org/TR/geometry-1/) \a domRect to QRectF. The behavior is
    undefined if the provided parameter is not a DOMRect.

    \since 6.5
    \ingroup platform-type-conversions

    \sa toDOMRect()
*/
QRectF QRectF::fromDOMRect(emscripten::val domRect)
{
    Q_ASSERT_X(domRect["constructor"]["name"].as<std::string>() == "DOMRect", Q_FUNC_INFO,
               "Passed object is not a DOMRect");

    return QRectF(domRect["left"].as<qreal>(), domRect["top"].as<qreal>(),
                  domRect["width"].as<qreal>(), domRect["height"].as<qreal>());
}

/*!
    Converts this object to a DOMRect (https://www.w3.org/TR/geometry-1/).

    \since 6.5
    \ingroup platform-type-conversions

    \sa fromDOMRect()
*/
emscripten::val QRectF::toDOMRect() const
{
    return emscripten::val::global("DOMRect").new_(left(), top(), width(), height());
}

QT_END_NAMESPACE
