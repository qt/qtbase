// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qohospixelmapconversions.h>

#include <QtCore/private/qohoslogger_p.h>
#include <QtCore/qspan.h>
#include <QtCore/qsysinfo.h>
#include <cstring>
#include <qarkui/qarkuiutils.h>
#include <qohosimageformat.h>

QT_BEGIN_NAMESPACE

namespace
{

struct QOhosPixelMapInfo
{
    std::uint32_t width;
    std::uint32_t height;
    ::PIXEL_FORMAT pixelFormat;
};

::PIXELMAP_ALPHA_TYPE mapQtPixelFormatToOhosPixelMapAlphaType(const QPixelFormat &pixelFormat)
{
    return pixelFormat.alphaUsage() == QPixelFormat::UsesAlpha
        ? pixelFormat.premultiplied() == QPixelFormat::Premultiplied
            ? ::PIXELMAP_ALPHA_TYPE::PIXELMAP_ALPHA_TYPE_PREMULTIPLIED
            : ::PIXELMAP_ALPHA_TYPE::PIXELMAP_ALPHA_TYPE_UNPREMULTIPLIED
        : ::PIXELMAP_ALPHA_TYPE::PIXELMAP_ALPHA_TYPE_OPAQUE;
}

std::shared_ptr<::OH_Pixelmap_InitializationOptions> createOhPixelmapInitializationOptions()
{
    ::OH_Pixelmap_InitializationOptions *initOptionsPtr {};

    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_PixelmapInitializationOptions_Create),
        &initOptionsPtr);

    return std::shared_ptr<::OH_Pixelmap_InitializationOptions>(
        initOptionsPtr,
        [](auto *initOptionsPtr) {
            if (initOptionsPtr != nullptr) {
                QArkUi::callArkUiOrFailOnErrorResult(
                    Q_OHOS_NAMED_FUNC(::OH_PixelmapInitializationOptions_Release),
                    initOptionsPtr);
            }
        });
}

std::shared_ptr<::OH_Pixelmap_InitializationOptions> makeNativePixelMapInitializationOptions(
    std::uint32_t width, std::uint32_t height, ::PIXEL_FORMAT pixelFormat,
    ::PIXELMAP_ALPHA_TYPE alphaType)
{
    auto initOptions = createOhPixelmapInitializationOptions();

    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_PixelmapInitializationOptions_SetWidth),
        initOptions.get(), width);

    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_PixelmapInitializationOptions_SetHeight),
        initOptions.get(), height);

    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_PixelmapInitializationOptions_SetSrcPixelFormat),
        initOptions.get(), static_cast<int>(pixelFormat));

    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_PixelmapInitializationOptions_SetPixelFormat),
        initOptions.get(), static_cast<int>(pixelFormat));

    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_PixelmapInitializationOptions_SetAlphaType),
        initOptions.get(), static_cast<int>(alphaType));

    return initOptions;
}

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

std::shared_ptr<::OH_PixelmapNative> createNativePixelMapFromQImage(QImage qImage)
{
    ::OH_PixelmapNative *pixelMap {};

    QImage effectiveImage =
        tryMapQtPixelFormatToOhosPixelFormat(qImage.format()).has_value()
            ? qImage
            : qImage.convertToFormat(QImage::Format_RGBA8888);
    auto optOhosPixelFormat = tryMapQtPixelFormatToOhosPixelFormat(effectiveImage.format());
    Q_ASSERT(optOhosPixelFormat.has_value());
    auto ohosPixelFormat = optOhosPixelFormat.value();

    auto initOptions = makeNativePixelMapInitializationOptions(
        static_cast<std::uint32_t>(effectiveImage.width()),
        static_cast<std::uint32_t>(effectiveImage.height()), ohosPixelFormat,
        mapQtPixelFormatToOhosPixelMapAlphaType(effectiveImage.pixelFormat()));

    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_PixelmapNative_CreatePixelmap),
        effectiveImage.bits(), effectiveImage.sizeInBytes(), initOptions.get(), &pixelMap);

    return std::shared_ptr<::OH_PixelmapNative>(
        pixelMap,
        [](::OH_PixelmapNative *pixelMap) {
            if (pixelMap != nullptr) {
                int res = QArkUi::callArkUi(Q_OHOS_NAMED_FUNC(::OH_PixelmapNative_Release), pixelMap);
                if (res != ::Image_ErrorCode::IMAGE_SUCCESS) {
                    qOhosPrintfError(
                        "%s: cannot release the pixel map (error code: %d).",
                        Q_FUNC_INFO, res);
                }
            }
        });
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

std::shared_ptr<::OH_PixelmapNative> wrapNativePixelMapPtr(::OH_PixelmapNative *pixelMap)
{
    return std::shared_ptr<::OH_PixelmapNative>(
        pixelMap,
        [](::OH_PixelmapNative *pixelMap) {
            if (pixelMap != nullptr) {
                int res = QArkUi::callArkUi(Q_OHOS_NAMED_FUNC(::OH_PixelmapNative_Release), pixelMap);
                if (res != ::Image_ErrorCode::IMAGE_SUCCESS) {
                    qOhosPrintfError(
                        "%s: cannot release the pixel map (error code: %d).",
                        Q_FUNC_INFO, res);
                }
            }
        });
}

std::shared_ptr<::OH_PixelmapNative> makeEmptyNativePixelMap()
{
    constexpr auto pixelFormat = ::PIXEL_FORMAT::PIXEL_FORMAT_RGBA_8888;
    constexpr auto alphaType = ::PIXELMAP_ALPHA_TYPE::PIXELMAP_ALPHA_TYPE_OPAQUE;

    auto initOptions = makeNativePixelMapInitializationOptions(1, 1, pixelFormat, alphaType);

    ::OH_PixelmapNative *pixelMap = {};
    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_PixelmapNative_CreateEmptyPixelmap),
        initOptions.get(), &pixelMap);

    return wrapNativePixelMapPtr(pixelMap);
}

QNapi::Object createNapiPixelMapFromQImage(QtOhos::JsState &jsState, const QImage &image)
{
    QImage effectiveImage =
        tryMapQtPixelFormatToOhosPixelFormat(image.format()).has_value()
            ? image
            : image.convertToFormat(QImage::Format_RGBA8888);
    auto optOhosPixelFormat = tryMapQtPixelFormatToOhosPixelFormat(effectiveImage.format());
    Q_ASSERT(optOhosPixelFormat.has_value());
    auto ohosPixelFormat = optOhosPixelFormat.value();

    auto *env = jsState.env();

    auto destBytesPerLine = effectiveImage.bytesPerLine();
    auto arrayBuffer = QNapi::ArrayBuffer::New(env, effectiveImage.height() * destBytesPerLine);

    for (int y = 0; y < effectiveImage.height(); ++y) {
        std::memcpy(
            static_cast<uchar *>(arrayBuffer.Data()) + y * destBytesPerLine, effectiveImage.scanLine(y), destBytesPerLine);
    }

    const auto options = QNapi::makeObject(
        env,
        {
            {
                "size",
                QNapi::makeObject(
                    env,
                    {
                        {"width", effectiveImage.size().width()},
                        {"height", effectiveImage.size().height()},
                    })
            },
            {"pixelFormat", static_cast<int>(ohosPixelFormat)},
            {"srcPixelFormat", static_cast<int>(ohosPixelFormat)},
        });

    return jsState.eval<QNapi::Object>("@ohos.multimedia.image.createPixelMapSync(*)", {arrayBuffer, options});
}

QT_END_NAMESPACE
