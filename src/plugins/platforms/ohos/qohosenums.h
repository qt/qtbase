// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSENUMS_H
#define QOHOSENUMS_H

#include <QtCore/qglobal.h>
#include <QtCore/qmetatype.h>
#include <array>
#include <info/application_target_sdk_version.h>

QT_BEGIN_NAMESPACE

namespace QtOhos {

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

namespace abilityAccessCtrl {

enum class PermissionStatus {
    DENIED,
    GRANTED,
    INVALID,
    NOT_DETERMINED,
    RESTRICTED,
};

}

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

namespace wantConstant {

enum class Flags {
    FLAG_ABILITY_ON_COLLABORATE,
    FLAG_AUTH_PERSISTABLE_URI_PERMISSION,
    FLAG_AUTH_READ_URI_PERMISSION,
    FLAG_AUTH_WRITE_URI_PERMISSION,
    FLAG_INSTALL_ON_DEMAND,
    FLAG_START_WITHOUT_TIPS,
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

namespace display {

enum class DisplaySourceMode {
    ALONE,
    EXTEND,
    MAIN,
    MIRROR,
    NONE,
};

enum class Orientation {
    LANDSCAPE,
    LANDSCAPE_INVERTED,
    PORTRAIT,
    PORTRAIT_INVERTED,
};

}

namespace file {

namespace picker {

enum class DocumentSelectMode {
    FILE,
    FOLDER,
    MIXED,
};

}

}

namespace inputMethod {

enum class Direction {
    CURSOR_DOWN,
    CURSOR_LEFT,
    CURSOR_RIGHT,
    CURSOR_UP,
};

enum class EnterKeyType {
    DONE,
    GO,
    NEWLINE,
    NEXT,
    NONE,
    PREVIOUS,
    SEARCH,
    SEND,
    UNSPECIFIED,
};

enum class RequestKeyboardReason {
    MOUSE,
    NONE,
    OTHER,
    TOUCH,
};

enum class TextInputType {
    DATETIME,
    EMAIL_ADDRESS,
    MULTILINE,
    NEW_PASSWORD,
    NONE,
    NUMBER,
    NUMBER_DECIMAL,
    NUMBER_PASSWORD,
    ONE_TIME_CODE,
    PHONE,
    SCREEN_LOCK_PASSWORD,
    TEXT,
    URL,
    USER_NAME,
    VISIBLE_PASSWORD,
};

}

namespace multimodalInput {

namespace pointer {

enum class PointerStyle {
    AECH_DEVELOPER_DEFINED_ICON,
    COLOR_SUCKER,
    CROSS,
    CURSOR_CIRCLE,
    CURSOR_COPY,
    CURSOR_CROSS,
    CURSOR_FORBID,
    DEFAULT,
    DEVELOPER_DEFINED_ICON,
    EAST,
    HAND_GRABBING,
    HAND_OPEN,
    HAND_POINTING,
    HELP,
    HORIZONTAL_TEXT_CURSOR,
    LASER_CURSOR,
    LASER_CURSOR_DOT,
    LASER_CURSOR_DOT_RED,
    LOADING,
    MIDDLE_BTN_EAST,
    MIDDLE_BTN_EAST_WEST,
    MIDDLE_BTN_NORTH,
    MIDDLE_BTN_NORTH_EAST,
    MIDDLE_BTN_NORTH_SOUTH,
    MIDDLE_BTN_NORTH_SOUTH_WEST_EAST,
    MIDDLE_BTN_NORTH_WEST,
    MIDDLE_BTN_SOUTH,
    MIDDLE_BTN_SOUTH_EAST,
    MIDDLE_BTN_SOUTH_WEST,
    MIDDLE_BTN_WEST,
    MOVE,
    NORTH,
    NORTH_EAST,
    NORTH_EAST_SOUTH_WEST,
    NORTH_SOUTH,
    NORTH_WEST,
    NORTH_WEST_SOUTH_EAST,
    RESIZE_LEFT_RIGHT,
    RESIZE_UP_DOWN,
    RUNNING,
    RUNNING_LEFT,
    RUNNING_RIGHT,
    SCREENRECORDER_CURSOR,
    SCREENSHOT_CHOOSE,
    SCREENSHOT_CURSOR,
    SOUTH,
    SOUTH_EAST,
    SOUTH_WEST,
    TEXT_CURSOR,
    WEST,
    WEST_EAST,
    ZOOM_IN,
    ZOOM_OUT,
};

}

}

namespace net {

namespace connection {

enum class NetBearType {
    BEARER_BLUETOOTH,
    BEARER_CELLULAR,
    BEARER_ETHERNET,
    BEARER_VPN,
    BEARER_WIFI,
};

enum class NetCap {
    NET_CAPABILITY_CHECKING_CONNECTIVITY,
    NET_CAPABILITY_INTERNET,
    NET_CAPABILITY_MMS,
    NET_CAPABILITY_NOT_METERED,
    NET_CAPABILITY_NOT_VPN,
    NET_CAPABILITY_PORTAL,
    NET_CAPABILITY_VALIDATED,
};

}

}

namespace notificationManager {

enum class ContentType {
    NOTIFICATION_CONTENT_BASIC_TEXT,
    NOTIFICATION_CONTENT_CONVERSATION,
    NOTIFICATION_CONTENT_LIVE_VIEW,
    NOTIFICATION_CONTENT_LONG_TEXT,
    NOTIFICATION_CONTENT_MULTILINE,
    NOTIFICATION_CONTENT_PICTURE,
    NOTIFICATION_CONTENT_SYSTEM_LIVE_VIEW,
};

}

namespace window {

enum class AnimationType {
    FADE_IN_OUT,
};

enum class AvoidAreaType {
    TYPE_CUTOUT,
    TYPE_KEYBOARD,
    TYPE_NAVIGATION_INDICATOR,
    TYPE_SYSTEM,
    TYPE_SYSTEM_GESTURE,
};

enum class MaximizePresentation {
    ENTER_IMMERSIVE,
    ENTER_IMMERSIVE_DISABLE_TITLE_AND_DOCK_HOVER,
    EXIT_IMMERSIVE,
    FOLLOW_APP_IMMERSIVE_SETTING,
};

enum class ModalityType {
    APPLICATION_MODALITY,
    WINDOW_MODALITY,
};

enum class RectChangeReason {
    DRAG,
    DRAG_END,
    DRAG_START,
    MAXIMIZE,
    MOVE,
    RECOVER,
    UNDEFINED,
};

enum class WindowEventType {
    WINDOW_ACTIVE,
    WINDOW_DESTROYED,
    WINDOW_HIDDEN,
    WINDOW_INACTIVE,
    WINDOW_SHOWN,
};

enum class WindowStatusType {
    FLOATING,
    FULL_SCREEN,
    MAXIMIZE,
    MINIMIZE,
    SPLIT_SCREEN,
    UNDEFINED,
};

}

}

}

template<typename Enum>
struct OhosEnumMeta;

template<>
struct OhosEnumMeta<enums::kit::ShareKit::systemShare::SelectionMode>
{
    using Enum = enums::kit::ShareKit::systemShare::SelectionMode;
    static constexpr const char *fullTypeName = "@kit.ShareKit.systemShare.SelectionMode";
    static constexpr std::array<std::pair<Enum, const char *>, 2> enumeratorsNames = {{
        {Enum::BATCH, "BATCH"},
        {Enum::SINGLE, "SINGLE"},
    }};
};

template<>
struct OhosEnumMeta<enums::kit::ShareKit::systemShare::ShareAbilityType>
{
    using Enum = enums::kit::ShareKit::systemShare::ShareAbilityType;
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
struct OhosEnumMeta<enums::kit::ShareKit::systemShare::SharePreviewMode>
{
    using Enum = enums::kit::ShareKit::systemShare::SharePreviewMode;
    static constexpr const char *fullTypeName = "@kit.ShareKit.systemShare.SharePreviewMode";
    static constexpr std::array<std::pair<Enum, const char *>, 2> enumeratorsNames = {{
        {Enum::DEFAULT, "DEFAULT"},
        {Enum::DETAIL, "DETAIL"},
    }};
};

template<>
struct OhosEnumMeta<enums::ohos::abilityAccessCtrl::PermissionStatus>
{
    using Enum = enums::ohos::abilityAccessCtrl::PermissionStatus;
    static constexpr const char *fullTypeName = "@ohos.abilityAccessCtrl.PermissionStatus";
    static constexpr std::array<std::pair<Enum, const char *>, 5> enumeratorsNames = {{
        {Enum::DENIED, "DENIED"},
        {Enum::GRANTED, "GRANTED"},
        {Enum::INVALID, "INVALID"},
        {Enum::NOT_DETERMINED, "NOT_DETERMINED"},
        {Enum::RESTRICTED, "RESTRICTED"},
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
struct OhosEnumMeta<enums::ohos::app::ability::AbilityConstant::LaunchReason>
{
    using Enum = enums::ohos::app::ability::AbilityConstant::LaunchReason;
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
struct OhosEnumMeta<enums::ohos::app::ability::AbilityConstant::OnContinueResult>
{
    using Enum = enums::ohos::app::ability::AbilityConstant::OnContinueResult;
    static constexpr const char *fullTypeName = "@ohos.app.ability.AbilityConstant.OnContinueResult";
    static constexpr std::array<std::pair<Enum, const char *>, 3> enumeratorsNames = {{
        {Enum::AGREE, "AGREE"},
        {Enum::MISMATCH, "MISMATCH"},
        {Enum::REJECT, "REJECT"},
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
        {Enum::COLOR_MODE_DARK, "COLOR_MODE_DARK"},
        {Enum::COLOR_MODE_LIGHT, "COLOR_MODE_LIGHT"},
        {Enum::COLOR_MODE_NOT_SET, "COLOR_MODE_NOT_SET"},
    }};
};

template<>
struct OhosEnumMeta<enums::ohos::app::ability::contextConstant::ProcessMode>
{
    using Enum = enums::ohos::app::ability::contextConstant::ProcessMode;
    static constexpr const char *fullTypeName = "@ohos.app.ability.contextConstant.ProcessMode";
    static constexpr std::array<std::pair<Enum, const char *>, 3> enumeratorsNames = {{
        {Enum::ATTACH_TO_STATUS_BAR_ITEM, "ATTACH_TO_STATUS_BAR_ITEM"},
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
    static constexpr std::array<std::pair<Enum, const char *>, 6> enumeratorsNames = {{
        {Enum::FLAG_ABILITY_ON_COLLABORATE, "FLAG_ABILITY_ON_COLLABORATE"},
        {Enum::FLAG_AUTH_PERSISTABLE_URI_PERMISSION, "FLAG_AUTH_PERSISTABLE_URI_PERMISSION"},
        {Enum::FLAG_AUTH_READ_URI_PERMISSION, "FLAG_AUTH_READ_URI_PERMISSION"},
        {Enum::FLAG_AUTH_WRITE_URI_PERMISSION, "FLAG_AUTH_WRITE_URI_PERMISSION"},
        {Enum::FLAG_INSTALL_ON_DEMAND, "FLAG_INSTALL_ON_DEMAND"},
        {Enum::FLAG_START_WITHOUT_TIPS, "FLAG_START_WITHOUT_TIPS"},
    }};
};

template<>
struct OhosEnumMeta<enums::ohos::bundle::bundleManager::SupportWindowMode>
{
    using Enum = enums::ohos::bundle::bundleManager::SupportWindowMode;
    static constexpr const char *fullTypeName = "@ohos.bundle.bundleManager.SupportWindowMode";
    static constexpr std::array<std::pair<Enum, const char *>, 3> enumeratorsNames = {{
        {Enum::FLOATING, "FLOATING"},
        {Enum::FULL_SCREEN, "FULL_SCREEN"},
        {Enum::SPLIT, "SPLIT"},
    }};
};

template<>
struct OhosEnumMeta<enums::ohos::display::DisplaySourceMode>
{
    using Enum = enums::ohos::display::DisplaySourceMode;
    static constexpr const char *fullTypeName = "@ohos.display.DisplaySourceMode";
    static constexpr std::array<std::pair<Enum, const char *>, 5> enumeratorsNames = {{
        {Enum::ALONE, "ALONE"},
        {Enum::EXTEND, "EXTEND"},
        {Enum::MAIN, "MAIN"},
        {Enum::MIRROR, "MIRROR"},
        {Enum::NONE, "NONE"},
    }};
};

template<>
struct OhosEnumMeta<enums::ohos::display::Orientation>
{
    using Enum = enums::ohos::display::Orientation;
    static constexpr const char *fullTypeName = "@ohos.display.Orientation";
    static constexpr std::array<std::pair<Enum, const char *>, 4> enumeratorsNames = {{
        {Enum::LANDSCAPE, "LANDSCAPE"},
        {Enum::LANDSCAPE_INVERTED, "LANDSCAPE_INVERTED"},
        {Enum::PORTRAIT, "PORTRAIT"},
        {Enum::PORTRAIT_INVERTED, "PORTRAIT_INVERTED"},
    }};
};

template<>
struct OhosEnumMeta<enums::ohos::file::picker::DocumentSelectMode>
{
    using Enum = enums::ohos::file::picker::DocumentSelectMode;
    static constexpr const char *fullTypeName = "@ohos.file.picker.DocumentSelectMode";
    static constexpr std::array<std::pair<Enum, const char *>, 3> enumeratorsNames = {{
        {Enum::FILE, "FILE"},
        {Enum::FOLDER, "FOLDER"},
        {Enum::MIXED, "MIXED"},
    }};
};

template<>
struct OhosEnumMeta<enums::ohos::inputMethod::Direction>
{
    using Enum = enums::ohos::inputMethod::Direction;
    static constexpr const char *fullTypeName = "@ohos.inputMethod.Direction";
    static constexpr std::array<std::pair<Enum, const char *>, 4> enumeratorsNames = {{
        {Enum::CURSOR_DOWN, "CURSOR_DOWN"},
        {Enum::CURSOR_LEFT, "CURSOR_LEFT"},
        {Enum::CURSOR_RIGHT, "CURSOR_RIGHT"},
        {Enum::CURSOR_UP, "CURSOR_UP"},
    }};
};

template<>
struct OhosEnumMeta<enums::ohos::inputMethod::EnterKeyType>
{
    using Enum = enums::ohos::inputMethod::EnterKeyType;
    static constexpr const char *fullTypeName = "@ohos.inputMethod.EnterKeyType";
    static constexpr std::array<std::pair<Enum, const char *>, 9> enumeratorsNames = {{
        {Enum::DONE, "DONE"},
        {Enum::GO, "GO"},
        {Enum::NEWLINE, "NEWLINE"},
        {Enum::NEXT, "NEXT"},
        {Enum::NONE, "NONE"},
        {Enum::PREVIOUS, "PREVIOUS"},
        {Enum::SEARCH, "SEARCH"},
        {Enum::SEND, "SEND"},
        {Enum::UNSPECIFIED, "UNSPECIFIED"},
    }};
};

template<>
struct OhosEnumMeta<enums::ohos::inputMethod::RequestKeyboardReason>
{
    using Enum = enums::ohos::inputMethod::RequestKeyboardReason;
    static constexpr const char *fullTypeName = "@ohos.inputMethod.RequestKeyboardReason";
    static constexpr std::array<std::pair<Enum, const char *>, 4> enumeratorsNames = {{
        {Enum::MOUSE, "MOUSE"},
        {Enum::NONE, "NONE"},
        {Enum::OTHER, "OTHER"},
        {Enum::TOUCH, "TOUCH"},
    }};
};

template<>
struct OhosEnumMeta<enums::ohos::inputMethod::TextInputType>
{
    using Enum = enums::ohos::inputMethod::TextInputType;
    static constexpr const char *fullTypeName = "@ohos.inputMethod.TextInputType";
    static constexpr std::array<std::pair<Enum, const char *>, 15> enumeratorsNames = {{
        {Enum::DATETIME, "DATETIME"},
        {Enum::EMAIL_ADDRESS, "EMAIL_ADDRESS"},
        {Enum::MULTILINE, "MULTILINE"},
        {Enum::NEW_PASSWORD, "NEW_PASSWORD"},
        {Enum::NONE, "NONE"},
        {Enum::NUMBER, "NUMBER"},
        {Enum::NUMBER_DECIMAL, "NUMBER_DECIMAL"},
        {Enum::NUMBER_PASSWORD, "NUMBER_PASSWORD"},
        {Enum::ONE_TIME_CODE, "ONE_TIME_CODE"},
        {Enum::PHONE, "PHONE"},
        {Enum::SCREEN_LOCK_PASSWORD, "SCREEN_LOCK_PASSWORD"},
        {Enum::TEXT, "TEXT"},
        {Enum::URL, "URL"},
        {Enum::USER_NAME, "USER_NAME"},
        {Enum::VISIBLE_PASSWORD, "VISIBLE_PASSWORD"},
    }};
};

template<>
struct OhosEnumMeta<enums::ohos::multimodalInput::pointer::PointerStyle>
{
    using Enum = enums::ohos::multimodalInput::pointer::PointerStyle;
    static constexpr const char *fullTypeName = "@ohos.multimodalInput.pointer.PointerStyle";
    static constexpr std::array<std::pair<Enum, const char *>, 53> enumeratorsNames = {{
        {Enum::AECH_DEVELOPER_DEFINED_ICON, "AECH_DEVELOPER_DEFINED_ICON"},
        {Enum::COLOR_SUCKER, "COLOR_SUCKER"},
        {Enum::CROSS, "CROSS"},
        {Enum::CURSOR_CIRCLE, "CURSOR_CIRCLE"},
        {Enum::CURSOR_COPY, "CURSOR_COPY"},
        {Enum::CURSOR_CROSS, "CURSOR_CROSS"},
        {Enum::CURSOR_FORBID, "CURSOR_FORBID"},
        {Enum::DEFAULT, "DEFAULT"},
        {Enum::DEVELOPER_DEFINED_ICON, "DEVELOPER_DEFINED_ICON"},
        {Enum::EAST, "EAST"},
        {Enum::HAND_GRABBING, "HAND_GRABBING"},
        {Enum::HAND_OPEN, "HAND_OPEN"},
        {Enum::HAND_POINTING, "HAND_POINTING"},
        {Enum::HELP, "HELP"},
        {Enum::HORIZONTAL_TEXT_CURSOR, "HORIZONTAL_TEXT_CURSOR"},
        {Enum::LASER_CURSOR, "LASER_CURSOR"},
        {Enum::LASER_CURSOR_DOT, "LASER_CURSOR_DOT"},
        {Enum::LASER_CURSOR_DOT_RED, "LASER_CURSOR_DOT_RED"},
        {Enum::LOADING, "LOADING"},
        {Enum::MIDDLE_BTN_EAST, "MIDDLE_BTN_EAST"},
        {Enum::MIDDLE_BTN_EAST_WEST, "MIDDLE_BTN_EAST_WEST"},
        {Enum::MIDDLE_BTN_NORTH, "MIDDLE_BTN_NORTH"},
        {Enum::MIDDLE_BTN_NORTH_EAST, "MIDDLE_BTN_NORTH_EAST"},
        {Enum::MIDDLE_BTN_NORTH_SOUTH, "MIDDLE_BTN_NORTH_SOUTH"},
        {Enum::MIDDLE_BTN_NORTH_SOUTH_WEST_EAST, "MIDDLE_BTN_NORTH_SOUTH_WEST_EAST"},
        {Enum::MIDDLE_BTN_NORTH_WEST, "MIDDLE_BTN_NORTH_WEST"},
        {Enum::MIDDLE_BTN_SOUTH, "MIDDLE_BTN_SOUTH"},
        {Enum::MIDDLE_BTN_SOUTH_EAST, "MIDDLE_BTN_SOUTH_EAST"},
        {Enum::MIDDLE_BTN_SOUTH_WEST, "MIDDLE_BTN_SOUTH_WEST"},
        {Enum::MIDDLE_BTN_WEST, "MIDDLE_BTN_WEST"},
        {Enum::MOVE, "MOVE"},
        {Enum::NORTH, "NORTH"},
        {Enum::NORTH_EAST, "NORTH_EAST"},
        {Enum::NORTH_EAST_SOUTH_WEST, "NORTH_EAST_SOUTH_WEST"},
        {Enum::NORTH_SOUTH, "NORTH_SOUTH"},
        {Enum::NORTH_WEST, "NORTH_WEST"},
        {Enum::NORTH_WEST_SOUTH_EAST, "NORTH_WEST_SOUTH_EAST"},
        {Enum::RESIZE_LEFT_RIGHT, "RESIZE_LEFT_RIGHT"},
        {Enum::RESIZE_UP_DOWN, "RESIZE_UP_DOWN"},
        {Enum::RUNNING, "RUNNING"},
        {Enum::RUNNING_LEFT, "RUNNING_LEFT"},
        {Enum::RUNNING_RIGHT, "RUNNING_RIGHT"},
        {Enum::SCREENRECORDER_CURSOR, "SCREENRECORDER_CURSOR"},
        {Enum::SCREENSHOT_CHOOSE, "SCREENSHOT_CHOOSE"},
        {Enum::SCREENSHOT_CURSOR, "SCREENSHOT_CURSOR"},
        {Enum::SOUTH, "SOUTH"},
        {Enum::SOUTH_EAST, "SOUTH_EAST"},
        {Enum::SOUTH_WEST, "SOUTH_WEST"},
        {Enum::TEXT_CURSOR, "TEXT_CURSOR"},
        {Enum::WEST, "WEST"},
        {Enum::WEST_EAST, "WEST_EAST"},
        {Enum::ZOOM_IN, "ZOOM_IN"},
        {Enum::ZOOM_OUT, "ZOOM_OUT"},
    }};
};

template<>
struct OhosEnumMeta<enums::ohos::notificationManager::ContentType>
{
    using Enum = enums::ohos::notificationManager::ContentType;
    static constexpr const char *fullTypeName = "@ohos.notificationManager.ContentType";
    static constexpr std::array<std::pair<Enum, const char *>, 7> enumeratorsNames = {{
        {Enum::NOTIFICATION_CONTENT_BASIC_TEXT, "NOTIFICATION_CONTENT_BASIC_TEXT"},
        {Enum::NOTIFICATION_CONTENT_CONVERSATION, "NOTIFICATION_CONTENT_CONVERSATION"},
        {Enum::NOTIFICATION_CONTENT_LIVE_VIEW, "NOTIFICATION_CONTENT_LIVE_VIEW"},
        {Enum::NOTIFICATION_CONTENT_LONG_TEXT, "NOTIFICATION_CONTENT_LONG_TEXT"},
        {Enum::NOTIFICATION_CONTENT_MULTILINE, "NOTIFICATION_CONTENT_MULTILINE"},
        {Enum::NOTIFICATION_CONTENT_PICTURE, "NOTIFICATION_CONTENT_PICTURE"},
        {Enum::NOTIFICATION_CONTENT_SYSTEM_LIVE_VIEW, "NOTIFICATION_CONTENT_SYSTEM_LIVE_VIEW"},
    }};
};

template<>
struct OhosEnumMeta<enums::ohos::net::connection::NetBearType>
{
    using Enum = enums::ohos::net::connection::NetBearType;
    static constexpr const char *fullTypeName = "@ohos.net.connection.NetBearType";
    static constexpr std::array<std::pair<Enum, const char *>, 5> enumeratorsNames = {{
        {Enum::BEARER_BLUETOOTH, "BEARER_BLUETOOTH"},
        {Enum::BEARER_CELLULAR, "BEARER_CELLULAR"},
        {Enum::BEARER_ETHERNET, "BEARER_ETHERNET"},
        {Enum::BEARER_VPN, "BEARER_VPN"},
        {Enum::BEARER_WIFI, "BEARER_WIFI"},
    }};
};

template<>
struct OhosEnumMeta<enums::ohos::net::connection::NetCap>
{
    using Enum = enums::ohos::net::connection::NetCap;
    static constexpr const char *fullTypeName = "@ohos.net.connection.NetCap";
    static constexpr std::array<std::pair<Enum, const char *>, 7> enumeratorsNames = {{
        {Enum::NET_CAPABILITY_CHECKING_CONNECTIVITY, "NET_CAPABILITY_CHECKING_CONNECTIVITY"},
        {Enum::NET_CAPABILITY_INTERNET, "NET_CAPABILITY_INTERNET"},
        {Enum::NET_CAPABILITY_MMS, "NET_CAPABILITY_MMS"},
        {Enum::NET_CAPABILITY_NOT_METERED, "NET_CAPABILITY_NOT_METERED"},
        {Enum::NET_CAPABILITY_NOT_VPN, "NET_CAPABILITY_NOT_VPN"},
        {Enum::NET_CAPABILITY_PORTAL, "NET_CAPABILITY_PORTAL"},
        {Enum::NET_CAPABILITY_VALIDATED, "NET_CAPABILITY_VALIDATED"},
    }};
};

template<>
struct OhosEnumMeta<enums::ohos::window::AnimationType>
{
    using Enum = enums::ohos::window::AnimationType;
    static constexpr const char *fullTypeName = "@ohos.window.AnimationType";
    static constexpr std::array<std::pair<Enum, const char *>, 1> enumeratorsNames = {{
        {Enum::FADE_IN_OUT, "FADE_IN_OUT"},
    }};
};

template<>
struct OhosEnumMeta<enums::ohos::window::AvoidAreaType>
{
    using Enum = enums::ohos::window::AvoidAreaType;
    static constexpr const char *fullTypeName = "@ohos.window.AvoidAreaType";
    static constexpr std::array<std::pair<Enum, const char *>, 5> enumeratorsNames = {{
        {Enum::TYPE_CUTOUT, "TYPE_CUTOUT"},
        {Enum::TYPE_KEYBOARD, "TYPE_KEYBOARD"},
        {Enum::TYPE_NAVIGATION_INDICATOR, "TYPE_NAVIGATION_INDICATOR"},
        {Enum::TYPE_SYSTEM, "TYPE_SYSTEM"},
        {Enum::TYPE_SYSTEM_GESTURE, "TYPE_SYSTEM_GESTURE"},
    }};
};

template<>
struct OhosEnumMeta<enums::ohos::window::MaximizePresentation>
{
    using Enum = enums::ohos::window::MaximizePresentation;
    static constexpr const char *fullTypeName = "@ohos.window.MaximizePresentation";
    static constexpr std::array<std::pair<Enum, const char *>, 4> enumeratorsNames = {{
        {Enum::ENTER_IMMERSIVE, "ENTER_IMMERSIVE"},
        {Enum::ENTER_IMMERSIVE_DISABLE_TITLE_AND_DOCK_HOVER, "ENTER_IMMERSIVE_DISABLE_TITLE_AND_DOCK_HOVER"},
        {Enum::EXIT_IMMERSIVE, "EXIT_IMMERSIVE"},
        {Enum::FOLLOW_APP_IMMERSIVE_SETTING, "FOLLOW_APP_IMMERSIVE_SETTING"},
    }};
};

template<>
struct OhosEnumMeta<enums::ohos::window::ModalityType>
{
    using Enum = enums::ohos::window::ModalityType;
    static constexpr const char *fullTypeName = "@ohos.window.ModalityType";
    static constexpr std::array<std::pair<Enum, const char *>, 2> enumeratorsNames = {{
        {Enum::APPLICATION_MODALITY, "APPLICATION_MODALITY"},
        {Enum::WINDOW_MODALITY, "WINDOW_MODALITY"},
    }};
};

template<>
struct OhosEnumMeta<enums::ohos::window::RectChangeReason>
{
    using Enum = enums::ohos::window::RectChangeReason;
    static constexpr const char *fullTypeName = "@ohos.window.RectChangeReason";
    static constexpr std::array<std::pair<Enum, const char *>, 7> enumeratorsNames = {{
        {Enum::DRAG, "DRAG"},
        {Enum::DRAG_END, "DRAG_END"},
        {Enum::DRAG_START, "DRAG_START"},
        {Enum::MAXIMIZE, "MAXIMIZE"},
        {Enum::MOVE, "MOVE"},
        {Enum::RECOVER, "RECOVER"},
        {Enum::UNDEFINED, "UNDEFINED"},
    }};
};

template<>
struct OhosEnumMeta<enums::ohos::window::WindowEventType>
{
    using Enum = enums::ohos::window::WindowEventType;
    static constexpr const char *fullTypeName = "@ohos.window.WindowEventType";
    static constexpr std::array<std::pair<Enum, const char *>, 5> enumeratorsNames = {{
        {Enum::WINDOW_ACTIVE, "WINDOW_ACTIVE"},
        {Enum::WINDOW_DESTROYED, "WINDOW_DESTROYED"},
        {Enum::WINDOW_HIDDEN, "WINDOW_HIDDEN"},
        {Enum::WINDOW_INACTIVE, "WINDOW_INACTIVE"},
        {Enum::WINDOW_SHOWN, "WINDOW_SHOWN"},
    }};
};

template<>
struct OhosEnumMeta<enums::ohos::window::WindowStatusType>
{
    using Enum = enums::ohos::window::WindowStatusType;
    static constexpr const char *fullTypeName = "@ohos.window.WindowStatusType";
    static constexpr std::array<std::pair<Enum, const char *>, 6> enumeratorsNames = {{
        {Enum::FLOATING, "FLOATING"},
        {Enum::FULL_SCREEN, "FULL_SCREEN"},
        {Enum::MAXIMIZE, "MAXIMIZE"},
        {Enum::MINIMIZE, "MINIMIZE"},
        {Enum::SPLIT_SCREEN, "SPLIT_SCREEN"},
        {Enum::UNDEFINED, "UNDEFINED"},
    }};
};

}

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhos::enums::kit::ShareKit::systemShare::SelectionMode));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhos::enums::kit::ShareKit::systemShare::ShareAbilityType));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhos::enums::kit::ShareKit::systemShare::SharePreviewMode));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhos::enums::ohos::abilityAccessCtrl::PermissionStatus));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhos::enums::ohos::app::ability::AbilityConstant::ContinueState));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhos::enums::ohos::app::ability::AbilityConstant::LaunchReason));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhos::enums::ohos::app::ability::AbilityConstant::OnContinueResult));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhos::enums::ohos::app::ability::AbilityConstant::WindowMode));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhos::enums::ohos::app::ability::ConfigurationConstant::ColorMode));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhos::enums::ohos::app::ability::contextConstant::ProcessMode));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhos::enums::ohos::app::ability::contextConstant::StartupVisibility));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhos::enums::ohos::app::ability::wantConstant::Flags));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhos::enums::ohos::bundle::bundleManager::SupportWindowMode));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhos::enums::ohos::display::DisplaySourceMode));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhos::enums::ohos::display::Orientation));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhos::enums::ohos::file::picker::DocumentSelectMode));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhos::enums::ohos::inputMethod::Direction));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhos::enums::ohos::inputMethod::EnterKeyType));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhos::enums::ohos::inputMethod::RequestKeyboardReason));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhos::enums::ohos::inputMethod::TextInputType));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhos::enums::ohos::multimodalInput::pointer::PointerStyle));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhos::enums::ohos::notificationManager::ContentType));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhos::enums::ohos::window::AnimationType));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhos::enums::ohos::window::AvoidAreaType));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhos::enums::ohos::window::MaximizePresentation));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhos::enums::ohos::window::ModalityType));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhos::enums::ohos::window::RectChangeReason));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhos::enums::ohos::window::WindowEventType));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhos::enums::ohos::window::WindowStatusType));

#endif
