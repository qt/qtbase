// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSSHAREKIT_H
#define QOHOSSHAREKIT_H

#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qmap.h>
#include <QtCore/qpoint.h>
#include <QtCore/qsize.h>
#include <QtCore/qvariant.h>
#include <QtGui/qwindow.h>
#include <memory>
#include <qohosenums.h>
#include <qohosplugincore.h>
#include <string>
#include <vector>

QT_BEGIN_NAMESPACE

namespace QOhosShareKit {

constexpr const char *mimeTextUriList = "text/uri-list";

struct SharedRecord
{
    std::string mimeType;

    QOhosOptional<std::string> content;
    QOhosOptional<std::string> filePath;

    QOhosOptional<std::string> title;
    QOhosOptional<std::string> label;
    QOhosOptional<std::string> description;
    QOhosOptional<QByteArray> thumbnail;
    QOhosOptional<std::string> thumbnailFilePath;
    QOhosOptional<QVariantMap> extraData;
};

struct ShareControllerAnchor
{
    QPoint windowOffset;
    QOhosOptional<QSize> size;
};

using SelectionMode = QtOhos::enums::kit::ShareKit::systemShare::SelectionMode;

using SharePreviewMode = QtOhos::enums::kit::ShareKit::systemShare::SharePreviewMode;

using ShareAbilityType = QtOhos::enums::kit::ShareKit::systemShare::ShareAbilityType;

struct ControllerOptions
{
    QOhosOptional<ShareControllerAnchor> anchor;
    QOhosOptional<SelectionMode> selectionMode;
    QOhosOptional<SharePreviewMode> previewMode;
    QOhosOptional<std::vector<ShareAbilityType>> excludedAbilities;
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

QT_END_NAMESPACE

#endif // QOHOSSHAREKIT_H
