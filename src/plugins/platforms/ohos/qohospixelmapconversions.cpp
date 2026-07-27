// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qohospixelmapconversions.h>

#include "qohosdisplayinfo.h"
#include <QtCore/private/qohoslogger_p.h>
#include <QtCore/qspan.h>
#include <QtCore/qsysinfo.h>
#include <QtGui/private/qohosimageconversions_p.h>
#include <cmath>
#include <cstring>
#include <qarkui/qarkuiutils.h>

QT_BEGIN_NAMESPACE

namespace
{

struct QOhosPixelMapInfo
{
    std::uint32_t width;
    std::uint32_t height;
    ::PIXEL_FORMAT pixelFormat;
};

QOhosPixelMapInfo getPixelMapInfo(::OH_PixelmapNative *pixelMap)
{
    ::OH_Pixelmap_ImageInfo *imageInfoPtr;
    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_PixelmapImageInfo_Create),
        &imageInfoPtr);

    auto imageInfo = std::shared_ptr<::OH_Pixelmap_ImageInfo>(
        imageInfoPtr,
        [](::OH_Pixelmap_ImageInfo *imageInfoPtr) {
            QArkUi::callArkUiOrFailOnErrorResult(
                Q_OHOS_NAMED_FUNC(::OH_PixelmapImageInfo_Release),
                imageInfoPtr);
        });

    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_PixelmapNative_GetImageInfo),
        pixelMap, imageInfo.get());

    std::uint32_t width;
    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_PixelmapImageInfo_GetWidth),
        imageInfo.get(), &width);

    std::uint32_t height;
    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_PixelmapImageInfo_GetHeight),
        imageInfo.get(), &height);

    int pixelFormat;
    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_PixelmapImageInfo_GetPixelFormat),
        imageInfo.get(), &pixelFormat);

    return {
        .width = width,
        .height = height,
        .pixelFormat = static_cast<::PIXEL_FORMAT>(pixelFormat),
    };
}

void readPixelMapData(::OH_PixelmapNative *pixelMap, QSpan<std::uint8_t> output)
{
    std::size_t bufferSize = output.size();
    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_PixelmapNative_ReadPixels),
        pixelMap, output.data(), &bufferSize);
}

void readPixelMapDataAsArgb(::OH_PixelmapNative *pixelMap, QSpan<std::uint8_t> output)
{
    std::size_t bufferSize = output.size();
    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_PixelmapNative_GetArgbPixels),
        pixelMap, output.data(), &bufferSize);
}

}

void readPixelMapDataAsRgba(::OH_PixelmapNative *pixelMap, QSpan<std::uint8_t> output)
{
    readPixelMapDataAsArgb(pixelMap, output);

    for (qsizetype pixelOffset = 0; pixelOffset < output.size(); pixelOffset += 4) {
        std::uint8_t *pixelPtr = &output[pixelOffset];
        std::uint8_t a = pixelPtr[0];
        std::uint8_t r = pixelPtr[1];
        std::uint8_t g = pixelPtr[2];
        std::uint8_t b = pixelPtr[3];
        pixelPtr[0] = r;
        pixelPtr[1] = g;
        pixelPtr[2] = b;
        pixelPtr[3] = a;
    }

    for (unsigned i = 0; i < output.size(); ++i)
        qOhosPrintfDebug("%s: pixelMap[%d]: %02x", Q_FUNC_INFO, i, output[i]);
}

QImage createQImageFromNativePixelMap(::OH_PixelmapNative *pixelMap)
{
    auto pixelMapInfo = getPixelMapInfo(pixelMap);

    QImage::Format qImagePixelFormat;
    void (*readPixelMapDataFunc)(::OH_PixelmapNative *, QSpan<std::uint8_t>);
    std::uint32_t pixelMapBytesPerPixel;
    switch (pixelMapInfo.pixelFormat) {
    case ::PIXEL_FORMAT::PIXEL_FORMAT_RGBA_8888:
        qImagePixelFormat = QImage::Format_RGBA8888;
        readPixelMapDataFunc = &readPixelMapData;
        pixelMapBytesPerPixel = 4;
        break;
    case ::PIXEL_FORMAT::PIXEL_FORMAT_RGB_888:
        qImagePixelFormat = QImage::Format_RGB888;
        readPixelMapDataFunc = &readPixelMapData;
        pixelMapBytesPerPixel = 3;
        break;
    case ::PIXEL_FORMAT::PIXEL_FORMAT_ALPHA_8:
        qImagePixelFormat = QImage::Format_Alpha8;
        readPixelMapDataFunc = &readPixelMapData;
        pixelMapBytesPerPixel = 1;
        break;
    case ::PIXEL_FORMAT::PIXEL_FORMAT_BGRA_8888:
        if (QSysInfo::ByteOrder == QSysInfo::LittleEndian) {
            qImagePixelFormat = QImage::Format_ARGB32;
            readPixelMapDataFunc = &readPixelMapData;
        } else {
            qImagePixelFormat = QImage::Format_RGBA8888;
            readPixelMapDataFunc = &readPixelMapDataAsRgba;
        }
        pixelMapBytesPerPixel = 4;
        break;
    default:
        qImagePixelFormat = QImage::Format_RGBA8888;
        readPixelMapDataFunc = &readPixelMapDataAsRgba;
        pixelMapBytesPerPixel = 4;
        break;
    }

    QImage resultImage(
        QSize {static_cast<int>(pixelMapInfo.width), static_cast<int>(pixelMapInfo.height)},
        qImagePixelFormat);

    auto pixelMapDataBytesPerLine = pixelMapInfo.width * pixelMapBytesPerPixel;
    auto pixelMapDataSize = pixelMapInfo.height * pixelMapDataBytesPerLine;

    if (resultImage.sizeInBytes() >= pixelMapDataSize) {
        auto *pixelMapDataBufferStart = resultImage.bits() + (resultImage.sizeInBytes() - pixelMapDataSize);
        readPixelMapDataFunc(pixelMap, QSpan(pixelMapDataBufferStart, pixelMapDataSize));
        if (pixelMapDataBufferStart > resultImage.bits()) {
            for (std::uint32_t row = 0; row < pixelMapInfo.height; ++row) {
                std::memmove(
                    resultImage.scanLine(row),
                    pixelMapDataBufferStart + row * pixelMapDataBytesPerLine,
                    pixelMapDataBytesPerLine);
            }
        }
    } else {
        std::vector<std::uint8_t> pixelMapData(pixelMapDataSize);
        readPixelMapDataFunc(pixelMap, QSpan(pixelMapData.data(), pixelMapData.size()));
        for (std::uint32_t row = 0; row < pixelMapInfo.height; ++row) {
            std::memcpy(
                resultImage.scanLine(row),
                pixelMapData.data() + row * pixelMapDataBytesPerLine,
                resultImage.bytesPerLine());
        }
    }

    return resultImage;
}

static double getPrimaryDisplayPixelDensity(QtOhos::JsState &jsState)
{
    constexpr auto fallbackDisplayDensity = 1.0;

    auto primaryDisplay = jsState.eval<QNapi::Object>("@ohos.display.getPrimaryDisplaySync()");
    auto displayInfo = QOhosDisplayInfo::makeFromOhosDisplayObject(jsState, primaryDisplay);

    double density;
    if (displayInfo.densityPixels > 0.0) {
        density = std::lround(displayInfo.densityPixels);
    } else {
        qOhosPrintfDebug("%s: invalid display densityPixels: %.3f", Q_FUNC_INFO, displayInfo.densityPixels);
        density = 0.0;
    }

    return density > 0.0 ? density : fallbackDisplayDensity;
}

QNapi::Object makeDisplayDensityScaledJsPixelMapFromQImage(QtOhos::JsState &jsState, const QImage &image)
{
    qOhosPrintfDebug(
        "%s: image dimensions: %dx%d, format: %d, bytes: %lld",
        Q_FUNC_INFO, image.width(), image.height(), static_cast<int>(image.format()), image.sizeInBytes());

    const int sourceWidth = image.width();
    const int sourceHeight = image.height();

    double density = getPrimaryDisplayPixelDensity(jsState);

    const double widthVp = std::lround(sourceWidth / density);
    const double heightVp = std::lround(sourceHeight / density);

    QImage newImage = image.scaled(widthVp, heightVp, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    const int width = newImage.width();
    const int height = newImage.height();
    const double finalWidthVp = width / density;
    const double finalHeightVp = height / density;

    qOhosPrintfDebug(
        "%s: density=%.3f, vp %.2f x %.2f -> %.2f x %.2f, px %dx%d -> %dx%d",
        Q_FUNC_INFO, density, widthVp, heightVp, finalWidthVp, finalHeightVp, sourceWidth, sourceHeight, width, height);

    return makeOhosNapiPixelMapFromQImage(jsState, newImage);
}

QT_END_NAMESPACE
