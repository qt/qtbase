// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSSHAREKITBACKEND_P_H
#define QOHOSSHAREKITBACKEND_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qohosenums_p.h"
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qmap.h>
#include <QtCore/qpoint.h>
#include <QtCore/qsize.h>
#include <QtCore/qvariant.h>
#include <QtGui/qwindow.h>
#include <memory>
#include <optional>
#include <string>
#include <vector>

QT_BEGIN_NAMESPACE

namespace QtOhosAppKit {

namespace QOhosShareKit {

constexpr const char *mimeTextUriList = "text/uri-list";

struct SharedRecord
{
    std::string mimeType;

    std::optional<std::string> content;
    std::optional<std::string> filePath;

    std::optional<std::string> title;
    std::optional<std::string> label;
    std::optional<std::string> description;
    std::optional<QByteArray> thumbnail;
    std::optional<std::string> thumbnailFilePath;
    std::optional<QVariantMap> extraData;
};

struct ShareControllerAnchor
{
    QPoint windowOffset;
    std::optional<QSize> size;
};

using SelectionMode = QtOhos::enums::kit::ShareKit::systemShare::SelectionMode;

using SharePreviewMode = QtOhos::enums::kit::ShareKit::systemShare::SharePreviewMode;

using ShareAbilityType = QtOhos::enums::kit::ShareKit::systemShare::ShareAbilityType;

struct ControllerOptions
{
    std::optional<ShareControllerAnchor> anchor;
    std::optional<SelectionMode> selectionMode;
    std::optional<SharePreviewMode> previewMode;
    std::optional<std::vector<ShareAbilityType>> excludedAbilities;
};

struct ShareOperationResult
{
    std::string targetAbilityName;
};

std::shared_ptr<void> shareData(
    QWindow *optInstanceMainWindow, const std::vector<SharedRecord> &recordsToShare,
    ControllerOptions controllerOptions, std::function<void()> panelClosedCallback,
    QOhosConsumer<ShareOperationResult> shareCompletedCallback);

}

}

QT_END_NAMESPACE

#endif // QOHOSSHAREKITBACKEND_P_H
