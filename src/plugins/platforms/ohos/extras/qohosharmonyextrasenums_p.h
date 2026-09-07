// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSHARMONYEXTRASENUMS_P_H
#define QOHOSHARMONYEXTRASENUMS_P_H

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

#include <QtCore/qglobal.h>
#include <QtCore/qmetatype.h>
#include <array>
#include <info/application_target_sdk_version.h>

QT_BEGIN_NAMESPACE

namespace QtOhosHarmonyExtras {

namespace enums {

namespace kit {

namespace ShareKit {

namespace systemShare {

enum class SelectionMode {
    BATCH,
    SINGLE,
};

enum class ShareAbilityType {
    COPY_TO_PASTEBOARD,
    PRINT,
    SAVE_AS_FILE,
    SAVE_TO_MEDIA_ASSET,
    SAVE_TO_SUPERHUB,
};

enum class SharePreviewMode {
    DEFAULT,
    DETAIL,
};

}

}

}

namespace ohos {

namespace app {

namespace ability {

namespace AbilityConstant {

enum class ContinueState {
    ACTIVE,
    INACTIVE,
};

enum class LaunchReason {
    APP_RECOVERY,
    AUTO_STARTUP,
    CALL,
    CONTINUATION,
    INSIGHT_INTENT,
    PRELOAD,
    PREPARE_CONTINUATION,
    SHARE,
    START_ABILITY,
    UNKNOWN,
};

enum class OnContinueResult {
    AGREE,
    MISMATCH,
    REJECT,
};

enum class WindowMode {
    WINDOW_MODE_FULLSCREEN,
    WINDOW_MODE_SPLIT_PRIMARY,
    WINDOW_MODE_SPLIT_SECONDARY,
};

}

namespace ConfigurationConstant {

enum class ColorMode {
    COLOR_MODE_DARK,
    COLOR_MODE_LIGHT,
    COLOR_MODE_NOT_SET,
};

}

namespace contextConstant {

enum class ProcessMode {
    ATTACH_TO_STATUS_BAR_ITEM,
    NEW_PROCESS_ATTACH_TO_PARENT,
    NEW_PROCESS_ATTACH_TO_STATUS_BAR_ITEM,
};

enum class StartupVisibility {
    STARTUP_HIDE,
    STARTUP_SHOW,
};

}

}

}

namespace bundle {

namespace bundleManager {

enum class SupportWindowMode {
    FLOATING,
    FULL_SCREEN,
    SPLIT,
};

}

}

namespace window {

enum class AnimationType {
    FADE_IN_OUT,
};

}

}

}

}

namespace QtOhos {

template<typename Enum>
struct OhosEnumMeta;

template<>
struct OhosEnumMeta<QtOhosHarmonyExtras::enums::kit::ShareKit::systemShare::SelectionMode>
{
    using Enum = QtOhosHarmonyExtras::enums::kit::ShareKit::systemShare::SelectionMode;
    static constexpr const char *fullTypeName = "@kit.ShareKit.systemShare.SelectionMode";
    static constexpr std::array<std::pair<Enum, const char *>, 2> enumeratorsNames = {{
        {Enum::BATCH, "BATCH"},
        {Enum::SINGLE, "SINGLE"},
    }};
};

template<>
struct OhosEnumMeta<QtOhosHarmonyExtras::enums::kit::ShareKit::systemShare::ShareAbilityType>
{
    using Enum = QtOhosHarmonyExtras::enums::kit::ShareKit::systemShare::ShareAbilityType;
    static constexpr const char *fullTypeName = "@kit.ShareKit.systemShare.ShareAbilityType";
    static constexpr std::array<std::pair<Enum, const char *>, 5> enumeratorsNames = {{
        {Enum::COPY_TO_PASTEBOARD, "COPY_TO_PASTEBOARD"},
        {Enum::PRINT, "PRINT"},
        {Enum::SAVE_AS_FILE, "SAVE_AS_FILE"},
        {Enum::SAVE_TO_MEDIA_ASSET, "SAVE_TO_MEDIA_ASSET"},
        {Enum::SAVE_TO_SUPERHUB, "SAVE_TO_SUPERHUB"},
    }};
};

template<>
struct OhosEnumMeta<QtOhosHarmonyExtras::enums::kit::ShareKit::systemShare::SharePreviewMode>
{
    using Enum = QtOhosHarmonyExtras::enums::kit::ShareKit::systemShare::SharePreviewMode;
    static constexpr const char *fullTypeName = "@kit.ShareKit.systemShare.SharePreviewMode";
    static constexpr std::array<std::pair<Enum, const char *>, 2> enumeratorsNames = {{
        {Enum::DEFAULT, "DEFAULT"},
        {Enum::DETAIL, "DETAIL"},
    }};
};

template<>
struct OhosEnumMeta<QtOhosHarmonyExtras::enums::ohos::app::ability::AbilityConstant::ContinueState>
{
    using Enum = QtOhosHarmonyExtras::enums::ohos::app::ability::AbilityConstant::ContinueState;
    static constexpr const char *fullTypeName = "@ohos.app.ability.AbilityConstant.ContinueState";
    static constexpr std::array<std::pair<Enum, const char *>, 2> enumeratorsNames = {{
        {Enum::ACTIVE, "ACTIVE"},
        {Enum::INACTIVE, "INACTIVE"},
    }};
};

template<>
struct OhosEnumMeta<QtOhosHarmonyExtras::enums::ohos::app::ability::AbilityConstant::LaunchReason>
{
    using Enum = QtOhosHarmonyExtras::enums::ohos::app::ability::AbilityConstant::LaunchReason;
    static constexpr const char *fullTypeName = "@ohos.app.ability.AbilityConstant.LaunchReason";
    static constexpr std::array<std::pair<Enum, const char *>, 10> enumeratorsNames = {{
        {Enum::APP_RECOVERY, "APP_RECOVERY"},
        {Enum::AUTO_STARTUP, "AUTO_STARTUP"},
        {Enum::CALL, "CALL"},
        {Enum::CONTINUATION, "CONTINUATION"},
        {Enum::INSIGHT_INTENT, "INSIGHT_INTENT"},
        {Enum::PRELOAD, "PRELOAD"},
        {Enum::PREPARE_CONTINUATION, "PREPARE_CONTINUATION"},
        {Enum::SHARE, "SHARE"},
        {Enum::START_ABILITY, "START_ABILITY"},
        {Enum::UNKNOWN, "UNKNOWN"},
    }};
};

template<>
struct OhosEnumMeta<QtOhosHarmonyExtras::enums::ohos::app::ability::AbilityConstant::OnContinueResult>
{
    using Enum = QtOhosHarmonyExtras::enums::ohos::app::ability::AbilityConstant::OnContinueResult;
    static constexpr const char *fullTypeName = "@ohos.app.ability.AbilityConstant.OnContinueResult";
    static constexpr std::array<std::pair<Enum, const char *>, 3> enumeratorsNames = {{
        {Enum::AGREE, "AGREE"},
        {Enum::MISMATCH, "MISMATCH"},
        {Enum::REJECT, "REJECT"},
    }};
};

template<>
struct OhosEnumMeta<QtOhosHarmonyExtras::enums::ohos::app::ability::AbilityConstant::WindowMode>
{
    using Enum = QtOhosHarmonyExtras::enums::ohos::app::ability::AbilityConstant::WindowMode;
    static constexpr const char *fullTypeName = "@ohos.app.ability.AbilityConstant.WindowMode";
    static constexpr std::array<std::pair<Enum, const char *>, 3> enumeratorsNames = {{
        {Enum::WINDOW_MODE_FULLSCREEN, "WINDOW_MODE_FULLSCREEN"},
        {Enum::WINDOW_MODE_SPLIT_PRIMARY, "WINDOW_MODE_SPLIT_PRIMARY"},
        {Enum::WINDOW_MODE_SPLIT_SECONDARY, "WINDOW_MODE_SPLIT_SECONDARY"},
    }};
};

template<>
struct OhosEnumMeta<QtOhosHarmonyExtras::enums::ohos::app::ability::ConfigurationConstant::ColorMode>
{
    using Enum = QtOhosHarmonyExtras::enums::ohos::app::ability::ConfigurationConstant::ColorMode;
    static constexpr const char *fullTypeName = "@ohos.app.ability.ConfigurationConstant.ColorMode";
    static constexpr std::array<std::pair<Enum, const char *>, 3> enumeratorsNames = {{
        {Enum::COLOR_MODE_DARK, "COLOR_MODE_DARK"},
        {Enum::COLOR_MODE_LIGHT, "COLOR_MODE_LIGHT"},
        {Enum::COLOR_MODE_NOT_SET, "COLOR_MODE_NOT_SET"},
    }};
};

template<>
struct OhosEnumMeta<QtOhosHarmonyExtras::enums::ohos::app::ability::contextConstant::ProcessMode>
{
    using Enum = QtOhosHarmonyExtras::enums::ohos::app::ability::contextConstant::ProcessMode;
    static constexpr const char *fullTypeName = "@ohos.app.ability.contextConstant.ProcessMode";
    static constexpr std::array<std::pair<Enum, const char *>, 3> enumeratorsNames = {{
        {Enum::ATTACH_TO_STATUS_BAR_ITEM, "ATTACH_TO_STATUS_BAR_ITEM"},
        {Enum::NEW_PROCESS_ATTACH_TO_PARENT, "NEW_PROCESS_ATTACH_TO_PARENT"},
        {Enum::NEW_PROCESS_ATTACH_TO_STATUS_BAR_ITEM, "NEW_PROCESS_ATTACH_TO_STATUS_BAR_ITEM"},
    }};
};

template<>
struct OhosEnumMeta<QtOhosHarmonyExtras::enums::ohos::app::ability::contextConstant::StartupVisibility>
{
    using Enum = QtOhosHarmonyExtras::enums::ohos::app::ability::contextConstant::StartupVisibility;
    static constexpr const char *fullTypeName = "@ohos.app.ability.contextConstant.StartupVisibility";
    static constexpr std::array<std::pair<Enum, const char *>, 2> enumeratorsNames = {{
        {Enum::STARTUP_HIDE, "STARTUP_HIDE"},
        {Enum::STARTUP_SHOW, "STARTUP_SHOW"},
    }};
};

template<>
struct OhosEnumMeta<QtOhosHarmonyExtras::enums::ohos::bundle::bundleManager::SupportWindowMode>
{
    using Enum = QtOhosHarmonyExtras::enums::ohos::bundle::bundleManager::SupportWindowMode;
    static constexpr const char *fullTypeName = "@ohos.bundle.bundleManager.SupportWindowMode";
    static constexpr std::array<std::pair<Enum, const char *>, 3> enumeratorsNames = {{
        {Enum::FLOATING, "FLOATING"},
        {Enum::FULL_SCREEN, "FULL_SCREEN"},
        {Enum::SPLIT, "SPLIT"},
    }};
};

template<>
struct OhosEnumMeta<QtOhosHarmonyExtras::enums::ohos::window::AnimationType>
{
    using Enum = QtOhosHarmonyExtras::enums::ohos::window::AnimationType;
    static constexpr const char *fullTypeName = "@ohos.window.AnimationType";
    static constexpr std::array<std::pair<Enum, const char *>, 1> enumeratorsNames = {{
        {Enum::FADE_IN_OUT, "FADE_IN_OUT"},
    }};
};

}

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhosHarmonyExtras::enums::kit::ShareKit::systemShare::SelectionMode));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhosHarmonyExtras::enums::kit::ShareKit::systemShare::ShareAbilityType));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhosHarmonyExtras::enums::kit::ShareKit::systemShare::SharePreviewMode));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhosHarmonyExtras::enums::ohos::app::ability::AbilityConstant::ContinueState));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhosHarmonyExtras::enums::ohos::app::ability::AbilityConstant::LaunchReason));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhosHarmonyExtras::enums::ohos::app::ability::AbilityConstant::OnContinueResult));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhosHarmonyExtras::enums::ohos::app::ability::AbilityConstant::WindowMode));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhosHarmonyExtras::enums::ohos::app::ability::ConfigurationConstant::ColorMode));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhosHarmonyExtras::enums::ohos::app::ability::contextConstant::ProcessMode));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhosHarmonyExtras::enums::ohos::app::ability::contextConstant::StartupVisibility));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhosHarmonyExtras::enums::ohos::bundle::bundleManager::SupportWindowMode));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhosHarmonyExtras::enums::ohos::window::AnimationType));

#endif
