// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qohosdisplayinfo.h>
#include <QtCore/private/qnapi_p.h>

QT_BEGIN_NAMESPACE

namespace
{

constexpr double mapPixelsToMillimeters(double pixels, double dpi)
{
    constexpr double millimetersPerInch = 25.4;
    return (pixels / dpi) * millimetersPerInch;
}

}

QOhosDisplayInfo QOhosDisplayInfo::makeFromOhosDisplayObject(QtOhos::JsState &jsState, QNapi::Object displayObject)
{
    constexpr auto forceEmptyTopLevelOffsetPixels  = false;

    auto sourceMode = makeQOhosOptional(
        jsState.mapOhosEnumFromJs<DisplaySourceMode>(
            displayObject.get<QNapi::Number>("sourceMode")));

    QOhosDisplayInfo result = {
        .id = JsDisplayId(displayObject.get<QNapi::Number>("id")),
        .name = QString::fromStdString(displayObject.get<QNapi::String>("name")),
        .sizePixels = QSizeF(
            displayObject.get<QNapi::Number>("width"),
            displayObject.get<QNapi::Number>("height")),
        .densityDPI = displayObject.get<QNapi::Number>("densityDPI"),
        .dpi = QDpi(
            displayObject.get<QNapi::Number>("xDPI"),
            displayObject.get<QNapi::Number>("yDPI")),
        .orientation = jsState.tryMapOhosEnumFromJs<JsDisplayOrientation>(
            displayObject.get<QNapi::Number>("orientation")),
        .sourceMode = sourceMode,
        .topLeftOffsetPixels = !forceEmptyTopLevelOffsetPixels
            ? qAndThen(sourceMode, [&](DisplaySourceMode mode) {
                return mode == DisplaySourceMode::MAIN || mode == DisplaySourceMode::EXTEND
                    ? makeQOhosOptional(
                        QPoint(
                            displayObject.get<QNapi::Number>("x"),
                            displayObject.get<QNapi::Number>("y")))
                    : makeEmptyQOhosOptional();
            })
            : makeEmptyQOhosOptional()
    };

    // The value obtained via displayObject.get<QNapi::Number>("densityDPI") has only float precision.
    // However, since all high-DPI related calculations use double precision, this may introduce
    // rounding errors. Therefore, we calculate the DPI directly using the formula provided
    // in the documentation below.
    // https://developer.huawei.com/consumer/en/doc/harmonyos-references/js-apis-display#display
    result.densityPixels = result.densityScaled = result.densityDPI / 160.;

    return result;
}

QOhosOptional<QNapi::Object> QOhosDisplayInfo::tryGetDisplayById(QtOhos::JsState &jsState, QOhosDisplayInfo::JsDisplayId displayId)
{
    QOhosOptional<QNapi::Object> result;
    try {
        result = jsState.eval<QNapi::Object>(
            "@ohos.display.getDisplayByIdSync(*)", { displayId.value() });
    } catch (const Napi::Error &error) {
        qOhosPrintfError(
            "%s: Failed to retrieve display with id: %f", Q_FUNC_INFO,
            displayId.value());
    }
    return result;
}

QRect QOhosDisplayInfo::displayGeometryPixels() const
{
    return QRect(topLeftOffsetPixels.value_or(QPoint()), sizePixels.toSize());
}

QSizeF QOhosDisplayInfo::physicalSize() const
{
    return QSizeF(
        mapPixelsToMillimeters(sizePixels.width(), dpi.first),
        mapPixelsToMillimeters(sizePixels.height(), dpi.second));
}

bool QOhosDisplayInfo::shouldIgnoreDisplay() const
{
    constexpr int virtualDisplayBaseId = 1000;
    static const QOhosDisplayInfo::DisplaySourceMode sourceModesToIgnore[] = {
       QOhosDisplayInfo::DisplaySourceMode::NONE,
       QOhosDisplayInfo::DisplaySourceMode::MIRROR,
       QOhosDisplayInfo::DisplaySourceMode::ALONE,
    };

    const auto *sourceModeToIgnoreIter = std::find(
        std::begin(sourceModesToIgnore), std::end(sourceModesToIgnore),
        sourceMode);
    bool ignoreBySoureMode = sourceModeToIgnoreIter != std::end(sourceModesToIgnore);

    return sourceMode.has_value()
        ? ignoreBySoureMode
        : id.value() >= virtualDisplayBaseId;
}

bool QOhosDisplayInfo::isDisplayMainOrExtended() const
{
    return sourceMode == QOhosDisplayInfo::DisplaySourceMode::MAIN
        || sourceMode == QOhosDisplayInfo::DisplaySourceMode::EXTEND;
}

QT_END_NAMESPACE
