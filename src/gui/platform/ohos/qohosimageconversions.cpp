// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosimageconversions_p.h"

#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/private/qohoslogger_p.h>
#include <cstdint>
#include <cstring>
#include <optional>

QT_BEGIN_NAMESPACE

namespace
{

template<typename Func, Func f, typename... FuncArgs>
QOhosInvokeResult<Func, FuncArgs...> callOhosCApiFunc(QOhosNamedFunc<Func, f> func, FuncArgs &&...funcArgs)
{
    return func.ptr()(std::forward<FuncArgs>(funcArgs)...);
}

template<typename Func, Func f, typename... FuncArgs>
void callOhosCApiFuncOrFailOnErrorResult(QOhosNamedFunc<Func, f> func, FuncArgs &&...funcArgs)
{
    const auto result = func.ptr()(std::forward<FuncArgs>(funcArgs)...);
    if (static_cast<int>(result) != 0) {
        qOhosReportFatalErrorAndAbort(
            "OHOS C API call %s failed with error: %d", func.name(), static_cast<int>(result));
    }
}

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

    callOhosCApiFuncOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_PixelmapInitializationOptions_Create),
        &initOptionsPtr);

    return std::shared_ptr<::OH_Pixelmap_InitializationOptions>(
        initOptionsPtr,
        [](auto *initOptionsPtr) {
            if (initOptionsPtr != nullptr) {
                callOhosCApiFuncOrFailOnErrorResult(
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

    callOhosCApiFuncOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_PixelmapInitializationOptions_SetWidth),
        initOptions.get(), width);

    callOhosCApiFuncOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_PixelmapInitializationOptions_SetHeight),
        initOptions.get(), height);

    callOhosCApiFuncOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_PixelmapInitializationOptions_SetSrcPixelFormat),
        initOptions.get(), static_cast<int>(pixelFormat));

    callOhosCApiFuncOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_PixelmapInitializationOptions_SetPixelFormat),
        initOptions.get(), static_cast<int>(pixelFormat));

    callOhosCApiFuncOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_PixelmapInitializationOptions_SetAlphaType),
        initOptions.get(), static_cast<int>(alphaType));

    return initOptions;
}

std::optional<::PIXEL_FORMAT> tryMapQtPixelFormatToOhosPixelFormat(QImage::Format format)
{
    static_assert(
        Q_BYTE_ORDER == Q_LITTLE_ENDIAN,
        "Pixel format mapping is currently supported for little-endian targets only");

    switch (format) {
        case QImage::Format_RGBA8888:
            return ::PIXEL_FORMAT::PIXEL_FORMAT_RGBA_8888;
        case QImage::Format_RGB888:
            return ::PIXEL_FORMAT::PIXEL_FORMAT_RGB_888;
        case QImage::Format_Alpha8:
            return ::PIXEL_FORMAT::PIXEL_FORMAT_ALPHA_8;
        case QImage::Format_ARGB32:
            return ::PIXEL_FORMAT::PIXEL_FORMAT_BGRA_8888;
        case QImage::Format_RGBA16FPx4:
            return ::PIXEL_FORMAT::PIXEL_FORMAT_RGBA_F16;
        case QImage::Format_RGB16:
            return ::PIXEL_FORMAT::PIXEL_FORMAT_RGB_565;
        default:
            return {};
    }
}

}

std::shared_ptr<::OH_PixelmapNative> makeOhosNativePixelMapFromQImage(QImage qImage)
{
    if (qImage.isNull())
        return makeEmptyOhosNativePixelMap();

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

    callOhosCApiFuncOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_PixelmapNative_CreatePixelmap),
        effectiveImage.bits(), effectiveImage.sizeInBytes(), initOptions.get(), &pixelMap);

    return std::shared_ptr<::OH_PixelmapNative>(
        pixelMap,
        [](::OH_PixelmapNative *pixelMap) {
            if (pixelMap != nullptr) {
                int res = callOhosCApiFunc(Q_OHOS_NAMED_FUNC(::OH_PixelmapNative_Release), pixelMap);
                if (res != ::Image_ErrorCode::IMAGE_SUCCESS) {
                    qOhosPrintfError(
                        "%s: cannot release the pixel map (error code: %d).",
                        Q_FUNC_INFO, res);
                }
            }
        });
}

std::shared_ptr<::OH_PixelmapNative> wrapOhosNativePixelMapPtr(::OH_PixelmapNative *pixelMap)
{
    return std::shared_ptr<::OH_PixelmapNative>(
        pixelMap,
        [](::OH_PixelmapNative *pixelMap) {
            if (pixelMap != nullptr) {
                int res = callOhosCApiFunc(Q_OHOS_NAMED_FUNC(::OH_PixelmapNative_Release), pixelMap);
                if (res != ::Image_ErrorCode::IMAGE_SUCCESS) {
                    qOhosPrintfError(
                        "%s: cannot release the pixel map (error code: %d).",
                        Q_FUNC_INFO, res);
                }
            }
        });
}

std::shared_ptr<::OH_PixelmapNative> makeEmptyOhosNativePixelMap()
{
    constexpr auto pixelFormat = ::PIXEL_FORMAT::PIXEL_FORMAT_RGBA_8888;
    constexpr auto alphaType = ::PIXELMAP_ALPHA_TYPE::PIXELMAP_ALPHA_TYPE_OPAQUE;

    auto initOptions = makeNativePixelMapInitializationOptions(1, 1, pixelFormat, alphaType);

    ::OH_PixelmapNative *pixelMap = {};
    callOhosCApiFuncOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_PixelmapNative_CreateEmptyPixelmap),
        initOptions.get(), &pixelMap);

    return wrapOhosNativePixelMapPtr(pixelMap);
}

QNapi::Object makeOhosNapiPixelMapFromQImage(QOhosJsState &jsState, const QImage &image)
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
