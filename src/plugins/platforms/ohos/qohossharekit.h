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

enum class SelectionMode
{
    SINGLE,
    BATCH,
};

enum class SharePreviewMode
{
    DEFAULT,
    DETAIL,
};

enum class ShareAbilityType
{
    COPY_TO_PASTEBOARD,
    SAVE_TO_MEDIA_ASSET,
    SAVE_AS_FILE,
    PRINT,
    SAVE_TO_SUPERHUB,
};

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

namespace QtOhos {

template<>
struct OhosEnumMeta<QOhosShareKit::SelectionMode>
{
    static constexpr const char *fullTypeName = "@kit.ShareKit.systemShare.SelectionMode";
    static constexpr std::array<std::pair<QOhosShareKit::SelectionMode, const char *>, 2> enumeratorsNames = {{
        {QOhosShareKit::SelectionMode::SINGLE, "SINGLE"},
        {QOhosShareKit::SelectionMode::BATCH, "BATCH"},
    }};
};

template<>
struct OhosEnumMeta<QOhosShareKit::SharePreviewMode>
{
    static constexpr const char *fullTypeName = "@kit.ShareKit.systemShare.SharePreviewMode";
    static constexpr std::array<std::pair<QOhosShareKit::SharePreviewMode, const char *>, 2> enumeratorsNames = {{
        {QOhosShareKit::SharePreviewMode::DEFAULT, "DEFAULT"},
        {QOhosShareKit::SharePreviewMode::DETAIL, "DETAIL"},
    }};
};

template<>
struct OhosEnumMeta<QOhosShareKit::ShareAbilityType>
{
    static constexpr const char *fullTypeName = "@kit.ShareKit.systemShare.ShareAbilityType";
    static constexpr std::array<std::pair<QOhosShareKit::ShareAbilityType, const char *>, 5> enumeratorsNames = {{
        {QOhosShareKit::ShareAbilityType::COPY_TO_PASTEBOARD, "COPY_TO_PASTEBOARD"},
        {QOhosShareKit::ShareAbilityType::SAVE_TO_MEDIA_ASSET, "SAVE_TO_MEDIA_ASSET"},
        {QOhosShareKit::ShareAbilityType::SAVE_AS_FILE, "SAVE_AS_FILE"},
        {QOhosShareKit::ShareAbilityType::PRINT, "PRINT"},
        {QOhosShareKit::ShareAbilityType::SAVE_TO_SUPERHUB, "SAVE_TO_SUPERHUB"},
    }};
};

}

QT_END_NAMESPACE

#endif // QOHOSSHAREKIT_H
