// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSENUMS_H
#define QOHOSENUMS_H

#include <QtCore/qglobal.h>
#include <array>
#include <info/application_target_sdk_version.h>

QT_BEGIN_NAMESPACE

namespace QtOhos {

namespace enums {

namespace ohos {

namespace app {

namespace ability {

namespace AbilityConstant {

enum class LaunchReason {
    UNKNOWN,
    START_ABILITY,
    CALL,
    CONTINUATION,
    APP_RECOVERY,
    SHARE,
    AUTO_STARTUP,
    INSIGHT_INTENT,
    PREPARE_CONTINUATION,
    PRELOAD,
};

enum class ContinueState {
    ACTIVE,
    INACTIVE,
};

enum class OnContinueResult
{
    AGREE,
    REJECT,
    MISMATCH,
};

enum class WindowMode
{
    WINDOW_MODE_SPLIT_PRIMARY,
    WINDOW_MODE_SPLIT_SECONDARY,
    WINDOW_MODE_FULLSCREEN,
};

}

namespace ConfigurationConstant {

enum class ColorMode {
    COLOR_MODE_NOT_SET,
    COLOR_MODE_DARK,
    COLOR_MODE_LIGHT,
};

}

namespace contextConstant {

enum class ProcessMode
{
    NEW_PROCESS_ATTACH_TO_PARENT,
    NEW_PROCESS_ATTACH_TO_STATUS_BAR_ITEM,
};

enum class StartupVisibility
{
    STARTUP_HIDE,
    STARTUP_SHOW,
};

}

namespace wantConstant {

enum class Flags
{
    FLAG_AUTH_READ_URI_PERMISSION,
    FLAG_AUTH_WRITE_URI_PERMISSION,
};

}

}

}

namespace bundle {

namespace bundleManager {

enum class SupportWindowMode
{
    FULL_SCREEN,
    SPLIT,
    FLOATING,
};

}

}

namespace notificationManager {

enum class ContentType {
    NOTIFICATION_CONTENT_BASIC_TEXT,
    NOTIFICATION_CONTENT_LONG_TEXT,
    NOTIFICATION_CONTENT_PICTURE,
    NOTIFICATION_CONTENT_CONVERSATION,
    NOTIFICATION_CONTENT_MULTILINE,
    NOTIFICATION_CONTENT_SYSTEM_LIVE_VIEW,
    NOTIFICATION_CONTENT_LIVE_VIEW,
};

}

namespace window {

namespace WindowCreateParams {

enum class AnimationType
{
    FADE_IN_OUT,
};

}

}

}

}

template<typename Enum>
struct OhosEnumMeta;

template<>
struct OhosEnumMeta<enums::ohos::app::ability::AbilityConstant::LaunchReason>
{
    using Enum = enums::ohos::app::ability::AbilityConstant::LaunchReason;
    static constexpr const char *fullTypeName = "@ohos.app.ability.AbilityConstant.LaunchReason";
    static constexpr size_t enumeratorsSize = 10;
    static constexpr std::array<std::pair<Enum, const char *>, enumeratorsSize> enumeratorsNames = {{
        {Enum::UNKNOWN, "UNKNOWN"},
        {Enum::START_ABILITY, "START_ABILITY"},
        {Enum::CALL, "CALL"},
        {Enum::CONTINUATION, "CONTINUATION"},
        {Enum::APP_RECOVERY, "APP_RECOVERY"},
        {Enum::SHARE, "SHARE"},
        {Enum::AUTO_STARTUP, "AUTO_STARTUP"},
        {Enum::INSIGHT_INTENT, "INSIGHT_INTENT"},
        {Enum::PREPARE_CONTINUATION, "PREPARE_CONTINUATION"},
        {Enum::PRELOAD, "PRELOAD"},
    }};
};

template<>
struct OhosEnumMeta<enums::ohos::app::ability::AbilityConstant::ContinueState>
{
    using Enum = enums::ohos::app::ability::AbilityConstant::ContinueState;
    static constexpr const char *fullTypeName = "@ohos.app.ability.AbilityConstant.ContinueState";
    static constexpr std::array<std::pair<Enum, const char *>, 2> enumeratorsNames = {{
        {Enum::ACTIVE, "ACTIVE"},
        {Enum::INACTIVE, "INACTIVE"},
    }};
};

template<>
struct OhosEnumMeta<enums::ohos::app::ability::AbilityConstant::OnContinueResult>
{
    using Enum = enums::ohos::app::ability::AbilityConstant::OnContinueResult;
    static constexpr const char *fullTypeName = "@ohos.app.ability.AbilityConstant.OnContinueResult";
    static constexpr std::array<std::pair<Enum, const char *>, 3> enumeratorsNames = {{
        {Enum::AGREE, "AGREE"},
        {Enum::REJECT, "REJECT"},
        {Enum::MISMATCH, "MISMATCH"},
    }};
};

template<>
struct OhosEnumMeta<enums::ohos::app::ability::AbilityConstant::WindowMode>
{
    using Enum = enums::ohos::app::ability::AbilityConstant::WindowMode;
    static constexpr const char *fullTypeName = "@ohos.app.ability.AbilityConstant.WindowMode";
    static constexpr std::array<std::pair<Enum, const char *>, 3> enumeratorsNames = {{
        {Enum::WINDOW_MODE_FULLSCREEN, "WINDOW_MODE_FULLSCREEN"},
        {Enum::WINDOW_MODE_SPLIT_PRIMARY, "WINDOW_MODE_SPLIT_PRIMARY"},
        {Enum::WINDOW_MODE_SPLIT_SECONDARY, "WINDOW_MODE_SPLIT_SECONDARY"},
    }};
};

template<>
struct OhosEnumMeta<enums::ohos::app::ability::ConfigurationConstant::ColorMode>
{
    using Enum = enums::ohos::app::ability::ConfigurationConstant::ColorMode;
    static constexpr const char *fullTypeName = "@ohos.app.ability.ConfigurationConstant.ColorMode";
    static constexpr std::array<std::pair<Enum, const char *>, 3> enumeratorsNames = {{
        {Enum::COLOR_MODE_NOT_SET, "COLOR_MODE_NOT_SET"},
        {Enum::COLOR_MODE_DARK, "COLOR_MODE_DARK"},
        {Enum::COLOR_MODE_LIGHT, "COLOR_MODE_LIGHT"},
    }};
};

template<>
struct OhosEnumMeta<enums::ohos::app::ability::contextConstant::ProcessMode>
{
    using Enum = enums::ohos::app::ability::contextConstant::ProcessMode;
    static constexpr const char *fullTypeName = "@ohos.app.ability.contextConstant.ProcessMode";
    static constexpr std::array<std::pair<Enum, const char *>, 2> enumeratorsNames = {{
        {Enum::NEW_PROCESS_ATTACH_TO_PARENT, "NEW_PROCESS_ATTACH_TO_PARENT"},
        {Enum::NEW_PROCESS_ATTACH_TO_STATUS_BAR_ITEM, "NEW_PROCESS_ATTACH_TO_STATUS_BAR_ITEM"},
    }};
};

template<>
struct OhosEnumMeta<enums::ohos::app::ability::contextConstant::StartupVisibility>
{
    using Enum = enums::ohos::app::ability::contextConstant::StartupVisibility;
    static constexpr const char *fullTypeName = "@ohos.app.ability.contextConstant.StartupVisibility";
    static constexpr std::array<std::pair<Enum, const char *>, 2> enumeratorsNames = {{
        {Enum::STARTUP_HIDE, "STARTUP_HIDE"},
        {Enum::STARTUP_SHOW, "STARTUP_SHOW"},
    }};
};

template<>
struct OhosEnumMeta<enums::ohos::app::ability::wantConstant::Flags>
{
    using Enum = enums::ohos::app::ability::wantConstant::Flags;
    static constexpr const char *fullTypeName = "@ohos.app.ability.wantConstant.Flags";
    static constexpr std::array<std::pair<Enum, const char *>, 2> enumeratorsNames = {{
        {Enum::FLAG_AUTH_READ_URI_PERMISSION, "FLAG_AUTH_READ_URI_PERMISSION"},
        {Enum::FLAG_AUTH_WRITE_URI_PERMISSION, "FLAG_AUTH_WRITE_URI_PERMISSION"},
    }};
};

template<>
struct OhosEnumMeta<enums::ohos::bundle::bundleManager::SupportWindowMode>
{
    using Enum = enums::ohos::bundle::bundleManager::SupportWindowMode;
    static constexpr const char *fullTypeName = "@ohos.bundle.bundleManager.SupportWindowMode";
    static constexpr std::array<std::pair<Enum, const char *>, 3> enumeratorsNames = {{
        {Enum::FULL_SCREEN, "FULL_SCREEN"},
        {Enum::SPLIT, "SPLIT"},
        {Enum::FLOATING, "FLOATING"},
    }};
};

template<>
struct OhosEnumMeta<enums::ohos::notificationManager::ContentType>
{
    using Enum = enums::ohos::notificationManager::ContentType;
    static constexpr const char *fullTypeName = "@ohos.notificationManager.ContentType";
    static constexpr std::array<std::pair<Enum, const char *>, 7> enumeratorsNames = {{
        {Enum::NOTIFICATION_CONTENT_BASIC_TEXT, "NOTIFICATION_CONTENT_BASIC_TEXT"},
        {Enum::NOTIFICATION_CONTENT_LONG_TEXT, "NOTIFICATION_CONTENT_LONG_TEXT"},
        {Enum::NOTIFICATION_CONTENT_PICTURE, "NOTIFICATION_CONTENT_PICTURE"},
        {Enum::NOTIFICATION_CONTENT_CONVERSATION, "NOTIFICATION_CONTENT_CONVERSATION"},
        {Enum::NOTIFICATION_CONTENT_MULTILINE, "NOTIFICATION_CONTENT_MULTILINE"},
        {Enum::NOTIFICATION_CONTENT_SYSTEM_LIVE_VIEW, "NOTIFICATION_CONTENT_SYSTEM_LIVE_VIEW"},
        {Enum::NOTIFICATION_CONTENT_LIVE_VIEW, "NOTIFICATION_CONTENT_LIVE_VIEW"},
    }};
};

template<>
struct OhosEnumMeta<enums::ohos::window::WindowCreateParams::AnimationType>
{
    using Enum = enums::ohos::window::WindowCreateParams::AnimationType;
    static constexpr const char *fullTypeName = "@ohos.window.AnimationType";
    static constexpr std::array<std::pair<Enum, const char *>, 1> enumeratorsNames = {{
        {Enum::FADE_IN_OUT, "FADE_IN_OUT"},
    }};
};

}

QT_END_NAMESPACE

#endif
