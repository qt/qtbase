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

/*!
    Converts the \l {https://262.ecma-international.org/#sec-string-object}{ECMAScript string} \a
    jsString to QString. Behavior is undefined if the provided parameter is not a string.

    \since 6.6
    \ingroup platform-type-conversions

    \sa toEcmaString()
*/
QString QString::fromEcmaString(emscripten::val jsString)
{
    Q_ASSERT_X(jsString.isString(), Q_FUNC_INFO, "Passed object is not a string");

    const double length = jsString["length"].as<double>();

    Q_ASSERT_X((double(uint64_t(length)) != double(uint64_t(length) - 1)
                && double(uint64_t(length)) != double(uint64_t(length) + 1))
                       || !std::numeric_limits<double>::is_iec559,
               Q_FUNC_INFO, "The floating-point length cannot precisely represent an integer");

    constexpr int zeroTerminatorLength = 1;
    const auto lengthOfUtf16 = (length + zeroTerminatorLength) * 2;

    Q_ASSERT_X((double(uint64_t(lengthOfUtf16)) != double(uint64_t(lengthOfUtf16) - 1)
                && double(uint64_t(lengthOfUtf16)) != double(uint64_t(lengthOfUtf16) + 1))
                       || !std::numeric_limits<double>::is_iec559,
               Q_FUNC_INFO,
               "The floating-point lengthOfUtf16 cannot precisely represent an integer");

    const QString result(uint64_t(length), Qt::Uninitialized);

    static const emscripten::val stringToUTF16(emscripten::val::module_property("stringToUTF16"));
    stringToUTF16(jsString, emscripten::val(quintptr(result.data())),
                  emscripten::val(lengthOfUtf16));
    return result;
}

/*!
    Converts this object to an
    \l {https://262.ecma-international.org/#sec-string-object}{ECMAScript string}.

    \since 6.6
    \ingroup platform-type-conversions

    \sa fromEcmaString()
*/
emscripten::val QString::toEcmaString() const
{
    static const emscripten::val UTF16ToString(emscripten::val::module_property("UTF16ToString"));
    return UTF16ToString(emscripten::val(quintptr(utf16())));
}

QT_END_NAMESPACE
