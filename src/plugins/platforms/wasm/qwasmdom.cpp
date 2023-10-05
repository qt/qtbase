// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmdom.h"

#include <QMimeData>
#include <QtCore/qpoint.h>
#include <QtCore/qrect.h>
#include <QtGui/qimage.h>
#include <private/qstdweb_p.h>

#include <utility>

QT_BEGIN_NAMESPACE

namespace dom {

void syncCSSClassWith(emscripten::val element, std::string cssClassName, bool flag)
{
    if (flag) {
        element["classList"].call<void>("add", emscripten::val(std::move(cssClassName)));
        return;
    }

    element["classList"].call<void>("remove", emscripten::val(std::move(cssClassName)));
}

QPointF mapPoint(emscripten::val source, emscripten::val target, const QPointF &point)
{
    const auto sourceBoundingRect =
            QRectF::fromDOMRect(source.call<emscripten::val>("getBoundingClientRect"));
    const auto targetBoundingRect =
            QRectF::fromDOMRect(target.call<emscripten::val>("getBoundingClientRect"));

    const auto offset = sourceBoundingRect.topLeft() - targetBoundingRect.topLeft();
    return point + offset;
}

void drawImageToWebImageDataArray(const QImage &sourceImage, emscripten::val destinationImageData,
                                  const QRect &sourceRect)
{
    Q_ASSERT_X(destinationImageData["constructor"]["name"].as<std::string>() == "ImageData",
               Q_FUNC_INFO, "The destination should be an ImageData instance");

    constexpr int BytesPerColor = 4;
    if (sourceRect.width() == sourceImage.width()) {
        // Copy a contiguous chunk of memory
        // ...............
        // OOOOOOOOOOOOOOO
        // OOOOOOOOOOOOOOO -> image data
        // OOOOOOOOOOOOOOO
        // ...............
        auto imageMemory = emscripten::typed_memory_view(sourceRect.width() * sourceRect.height()
                                                                 * BytesPerColor,
                                                         sourceImage.constScanLine(sourceRect.y()));
        destinationImageData["data"].call<void>(
                "set", imageMemory, sourceRect.y() * sourceImage.width() * BytesPerColor);
    } else {
        // Go through the scanlines manually to set the individual lines in bulk. This is
        // marginally less performant than the above.
        // ...............
        // ...OOOOOOOOO... r = 0  -> image data
        // ...OOOOOOOOO... r = 1  -> image data
        // ...OOOOOOOOO... r = 2  -> image data
        // ...............
        for (int row = 0; row < sourceRect.height(); ++row) {
            auto scanlineMemory =
                    emscripten::typed_memory_view(sourceRect.width() * BytesPerColor,
                                                  sourceImage.constScanLine(row + sourceRect.y())
                                                          + BytesPerColor * sourceRect.x());
            destinationImageData["data"].call<void>("set", scanlineMemory,
                                                    (sourceRect.y() + row) * sourceImage.width()
                                                                    * BytesPerColor
                                                            + sourceRect.x() * BytesPerColor);
        }
    }
}

} // namespace dom

QT_END_NAMESPACE
