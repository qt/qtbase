// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSDISPLAYINFO_H
#define QOHOSDISPLAYINFO_H

#include <QtCore/QSizeF>
#include <QtCore/private/qnapi_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qstring.h>
#include <qohosplugincore.h>
#include <qohosutils.h>
#include <qpa/qplatformscreen.h>

QT_BEGIN_NAMESPACE

struct QOhosDisplayInfo
{
    enum class JsDisplayOrientation
    {
        PORTRAIT,
        LANDSCAPE,
        PORTRAIT_INVERTED,
        LANDSCAPE_INVERTED,
    };

    enum class DisplaySourceMode
    {
        NONE,
        MAIN,
        MIRROR,
        EXTEND,
        ALONE,
    };

    using JsDisplayId = QtOhos::TypedId<double, struct JsDisplayIdTag>;

    static QOhosDisplayInfo makeFromOhosDisplayObject(QtOhos::JsState &jsState, QNapi::Object displayObject);

    JsDisplayId id;
    QString name;
    QSizeF sizePixels;
    double densityDPI;
    double densityPixels;
    double densityScaled;
    QDpi dpi;
    QOhosOptional<JsDisplayOrientation> orientation;
    QOhosOptional<DisplaySourceMode> sourceMode;
    QOhosOptional<QPoint> topLeftOffsetPixels;

    QRect displayGeometryPixels() const;
    QSizeF physicalSize() const;
};

namespace QtOhos
{

template<>
struct OhosEnumMeta<QOhosDisplayInfo::JsDisplayOrientation>
{
    static constexpr const char *fullTypeName = "@ohos.display.Orientation";
    static constexpr std::array<std::pair<QOhosDisplayInfo::JsDisplayOrientation, const char*>, 4> enumeratorsNames = {{
        {QOhosDisplayInfo::JsDisplayOrientation::PORTRAIT, "PORTRAIT"},
        {QOhosDisplayInfo::JsDisplayOrientation::LANDSCAPE, "LANDSCAPE"},
        {QOhosDisplayInfo::JsDisplayOrientation::PORTRAIT_INVERTED, "PORTRAIT_INVERTED"},
        {QOhosDisplayInfo::JsDisplayOrientation::LANDSCAPE_INVERTED, "LANDSCAPE_INVERTED"},
    }};
};

template<>
struct OhosEnumMeta<QOhosDisplayInfo::DisplaySourceMode>
{
    static constexpr const char *fullTypeName = "@ohos.display.DisplaySourceMode";
    static constexpr std::array<std::pair<QOhosDisplayInfo::DisplaySourceMode, const char*>, 5> enumeratorsNames = {{
        {QOhosDisplayInfo::DisplaySourceMode::NONE, "NONE"},
        {QOhosDisplayInfo::DisplaySourceMode::MAIN, "MAIN"},
        {QOhosDisplayInfo::DisplaySourceMode::MIRROR, "MIRROR"},
        {QOhosDisplayInfo::DisplaySourceMode::EXTEND, "EXTEND"},
        {QOhosDisplayInfo::DisplaySourceMode::ALONE, "ALONE"},
    }};
};

}

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QOhosDisplayInfo))
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QOhosDisplayInfo::JsDisplayId));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QOhosDisplayInfo::JsDisplayOrientation));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QOhosDisplayInfo::DisplaySourceMode));

#endif
