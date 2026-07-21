// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSDISPLAYINFO_H
#define QOHOSDISPLAYINFO_H

#include <QtCore/QSizeF>
#include <QtCore/private/qnapi_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qstring.h>
#include <optional>
#include <qohosenums.h>
#include <qohosplugincore.h>
#include <qohosutils.h>
#include <qpa/qplatformscreen.h>

QT_BEGIN_NAMESPACE

struct QOhosDisplayInfo
{
    using JsDisplayOrientation = QtOhos::enums::ohos::display::Orientation;

    using DisplaySourceMode = QtOhos::enums::ohos::display::DisplaySourceMode;

    using JsDisplayId = QtOhos::TypedId<double, struct JsDisplayIdTag>;

    static QOhosDisplayInfo makeFromOhosDisplayObject(QtOhos::JsState &jsState, QNapi::Object displayObject);
    static std::optional<QNapi::Object> tryGetDisplayById(QtOhos::JsState &jsState, QOhosDisplayInfo::JsDisplayId displayId);

    JsDisplayId id;
    QString name;
    QSizeF sizePixels;
    double densityDPI;
    double densityPixels;
    double densityScaled;
    uint refreshRate;
    QDpi dpi;
    std::optional<JsDisplayOrientation> orientation;
    std::optional<DisplaySourceMode> sourceMode;
    std::optional<QPoint> topLeftOffsetPixels;

    QRect displayGeometryPixels() const;
    QSizeF physicalSize() const;
    bool shouldIgnoreDisplay() const;
    bool isDisplayMainOrExtended() const;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QOhosDisplayInfo))
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QOhosDisplayInfo::JsDisplayId));

#endif
