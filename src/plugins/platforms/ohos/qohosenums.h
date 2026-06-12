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
    SINGLE,
    BATCH,
};

enum class ShareAbilityType {
    COPY_TO_PASTEBOARD,
    SAVE_TO_MEDIA_ASSET,
    SAVE_AS_FILE,
    PRINT,
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

enum class OnContinueResult {
    AGREE,
    REJECT,
    MISMATCH,
};

enum class WindowMode {
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

enum class ProcessMode {
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
    FLAG_AUTH_READ_URI_PERMISSION,
    FLAG_AUTH_WRITE_URI_PERMISSION,
};

}

}

}

namespace bundle {

namespace bundleManager {

enum class SupportWindowMode {
    FULL_SCREEN,
    SPLIT,
    FLOATING,
};

}

}

namespace display {

enum class DisplaySourceMode {
    NONE,
    MAIN,
    MIRROR,
    EXTEND,
    ALONE,
};

enum class Orientation {
    PORTRAIT,
    LANDSCAPE,
    PORTRAIT_INVERTED,
    LANDSCAPE_INVERTED,
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
    CURSOR_UP,
    CURSOR_DOWN,
    CURSOR_LEFT,
    CURSOR_RIGHT,
};

enum class EnterKeyType {
    UNSPECIFIED,
    NONE,
    GO,
    SEARCH,
    SEND,
    NEXT,
    DONE,
    PREVIOUS,
    NEWLINE,
};

enum class RequestKeyboardReason {
    NONE,
    MOUSE,
    TOUCH,
    OTHER,
};

enum class TextInputType {
    NONE,
    TEXT,
    MULTILINE,
    NUMBER,
    PHONE,
    DATETIME,
    EMAIL_ADDRESS,
    URL,
    VISIBLE_PASSWORD,
    NUMBER_PASSWORD,
};

}

namespace multimodalInput {

namespace pointer {

enum class PointerStyle {
    DEFAULT,
    EAST,
    WEST,
    SOUTH,
    NORTH,
    WEST_EAST,
    NORTH_SOUTH,
    NORTH_EAST,
    NORTH_WEST,
    SOUTH_EAST,
    SOUTH_WEST,
    NORTH_EAST_SOUTH_WEST,
    NORTH_WEST_SOUTH_EAST,
    CROSS,
    CURSOR_COPY,
    CURSOR_FORBID,
    COLOR_SUCKER,
    HAND_GRABBING,
    HAND_OPEN,
    HAND_POINTING,
    HELP,
    MOVE,
    RESIZE_LEFT_RIGHT,
    RESIZE_UP_DOWN,
    SCREENSHOT_CHOOSE,
    SCREENSHOT_CURSOR,
    TEXT_CURSOR,
    ZOOM_IN,
    ZOOM_OUT,
    MIDDLE_BTN_EAST,
    MIDDLE_BTN_WEST,
    MIDDLE_BTN_SOUTH,
    MIDDLE_BTN_NORTH,
    MIDDLE_BTN_NORTH_SOUTH,
    MIDDLE_BTN_NORTH_EAST,
    MIDDLE_BTN_NORTH_WEST,
    MIDDLE_BTN_SOUTH_EAST,
    MIDDLE_BTN_SOUTH_WEST,
    MIDDLE_BTN_NORTH_SOUTH_WEST_EAST,
    HORIZONTAL_TEXT_CURSOR,
    CURSOR_CROSS,
    CURSOR_CIRCLE,
    LOADING,
    RUNNING,
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

enum class AnimationType {
    FADE_IN_OUT,
};

enum class AvoidAreaType {
    TYPE_SYSTEM,
    TYPE_CUTOUT,
    TYPE_SYSTEM_GESTURE,
    TYPE_KEYBOARD,
    TYPE_NAVIGATION_INDICATOR,
};

enum class MaximizePresentation {
    FOLLOW_APP_IMMERSIVE_SETTING,
    EXIT_IMMERSIVE,
    ENTER_IMMERSIVE,
};

enum class ModalityType {
    WINDOW_MODALITY,
    APPLICATION_MODALITY,
};

enum class RectChangeReason {
    UNDEFINED,
    MAXIMIZE,
    RECOVER,
    MOVE,
    DRAG,
    DRAG_START,
    DRAG_END,
};

enum class WindowEventType {
    WINDOW_SHOWN,
    WINDOW_ACTIVE,
    WINDOW_INACTIVE,
    WINDOW_HIDDEN,
    WINDOW_DESTROYED,
};

enum class WindowStatusType {
    UNDEFINED,
    FULL_SCREEN,
    MAXIMIZE,
    MINIMIZE,
    FLOATING,
    SPLIT_SCREEN,
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
        {Enum::SINGLE, "SINGLE"},
        {Enum::BATCH, "BATCH"},
    }};
};

template<>
struct OhosEnumMeta<enums::kit::ShareKit::systemShare::ShareAbilityType>
{
    using Enum = enums::kit::ShareKit::systemShare::ShareAbilityType;
    static constexpr const char *fullTypeName = "@kit.ShareKit.systemShare.ShareAbilityType";
    static constexpr std::array<std::pair<Enum, const char *>, 5> enumeratorsNames = {{
        {Enum::COPY_TO_PASTEBOARD, "COPY_TO_PASTEBOARD"},
        {Enum::SAVE_TO_MEDIA_ASSET, "SAVE_TO_MEDIA_ASSET"},
        {Enum::SAVE_AS_FILE, "SAVE_AS_FILE"},
        {Enum::PRINT, "PRINT"},
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
struct OhosEnumMeta<enums::ohos::display::DisplaySourceMode>
{
    using Enum = enums::ohos::display::DisplaySourceMode;
    static constexpr const char *fullTypeName = "@ohos.display.DisplaySourceMode";
    static constexpr std::array<std::pair<Enum, const char *>, 5> enumeratorsNames = {{
        {Enum::NONE, "NONE"},
        {Enum::MAIN, "MAIN"},
        {Enum::MIRROR, "MIRROR"},
        {Enum::EXTEND, "EXTEND"},
        {Enum::ALONE, "ALONE"},
    }};
};

template<>
struct OhosEnumMeta<enums::ohos::display::Orientation>
{
    using Enum = enums::ohos::display::Orientation;
    static constexpr const char *fullTypeName = "@ohos.display.Orientation";
    static constexpr std::array<std::pair<Enum, const char *>, 4> enumeratorsNames = {{
        {Enum::PORTRAIT, "PORTRAIT"},
        {Enum::LANDSCAPE, "LANDSCAPE"},
        {Enum::PORTRAIT_INVERTED, "PORTRAIT_INVERTED"},
        {Enum::LANDSCAPE_INVERTED, "LANDSCAPE_INVERTED"},
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
        {Enum::CURSOR_UP, "CURSOR_UP"},
        {Enum::CURSOR_DOWN, "CURSOR_DOWN"},
        {Enum::CURSOR_LEFT, "CURSOR_LEFT"},
        {Enum::CURSOR_RIGHT, "CURSOR_RIGHT"},
    }};
};

template<>
struct OhosEnumMeta<enums::ohos::inputMethod::EnterKeyType>
{
    using Enum = enums::ohos::inputMethod::EnterKeyType;
    static constexpr const char *fullTypeName = "@ohos.inputMethod.EnterKeyType";
    static constexpr std::array<std::pair<Enum, const char *>, 9> enumeratorsNames = {{
        {Enum::UNSPECIFIED, "UNSPECIFIED"},
        {Enum::NONE, "NONE"},
        {Enum::GO, "GO"},
        {Enum::SEARCH, "SEARCH"},
        {Enum::SEND, "SEND"},
        {Enum::NEXT, "NEXT"},
        {Enum::DONE, "DONE"},
        {Enum::PREVIOUS, "PREVIOUS"},
        {Enum::NEWLINE, "NEWLINE"},
    }};
};

template<>
struct OhosEnumMeta<enums::ohos::inputMethod::RequestKeyboardReason>
{
    using Enum = enums::ohos::inputMethod::RequestKeyboardReason;
    static constexpr const char *fullTypeName = "@ohos.inputMethod.RequestKeyboardReason";
    static constexpr std::array<std::pair<Enum, const char *>, 4> enumeratorsNames = {{
        {Enum::NONE, "NONE"},
        {Enum::MOUSE, "MOUSE"},
        {Enum::TOUCH, "TOUCH"},
        {Enum::OTHER, "OTHER"},
    }};
};

template<>
struct OhosEnumMeta<enums::ohos::inputMethod::TextInputType>
{
    using Enum = enums::ohos::inputMethod::TextInputType;
    static constexpr const char *fullTypeName = "@ohos.inputMethod.TextInputType";
    static constexpr std::array<std::pair<Enum, const char *>, 10> enumeratorsNames = {{
        {Enum::NONE, "NONE"},
        {Enum::TEXT, "TEXT"},
        {Enum::MULTILINE, "MULTILINE"},
        {Enum::NUMBER, "NUMBER"},
        {Enum::PHONE, "PHONE"},
        {Enum::DATETIME, "DATETIME"},
        {Enum::EMAIL_ADDRESS, "EMAIL_ADDRESS"},
        {Enum::URL, "URL"},
        {Enum::VISIBLE_PASSWORD, "VISIBLE_PASSWORD"},
        {Enum::NUMBER_PASSWORD, "NUMBER_PASSWORD"},
    }};
};

template<>
struct OhosEnumMeta<enums::ohos::multimodalInput::pointer::PointerStyle>
{
    using Enum = enums::ohos::multimodalInput::pointer::PointerStyle;
    static constexpr const char *fullTypeName = "@ohos.multimodalInput.pointer.PointerStyle";
    static constexpr std::array<std::pair<Enum, const char *>, 44> enumeratorsNames = {{
        {Enum::DEFAULT, "DEFAULT"},
        {Enum::EAST, "EAST"},
        {Enum::WEST, "WEST"},
        {Enum::SOUTH, "SOUTH"},
        {Enum::NORTH, "NORTH"},
        {Enum::WEST_EAST, "WEST_EAST"},
        {Enum::NORTH_SOUTH, "NORTH_SOUTH"},
        {Enum::NORTH_EAST, "NORTH_EAST"},
        {Enum::NORTH_WEST, "NORTH_WEST"},
        {Enum::SOUTH_EAST, "SOUTH_EAST"},
        {Enum::SOUTH_WEST, "SOUTH_WEST"},
        {Enum::NORTH_EAST_SOUTH_WEST, "NORTH_EAST_SOUTH_WEST"},
        {Enum::NORTH_WEST_SOUTH_EAST, "NORTH_WEST_SOUTH_EAST"},
        {Enum::CROSS, "CROSS"},
        {Enum::CURSOR_COPY, "CURSOR_COPY"},
        {Enum::CURSOR_FORBID, "CURSOR_FORBID"},
        {Enum::COLOR_SUCKER, "COLOR_SUCKER"},
        {Enum::HAND_GRABBING, "HAND_GRABBING"},
        {Enum::HAND_OPEN, "HAND_OPEN"},
        {Enum::HAND_POINTING, "HAND_POINTING"},
        {Enum::HELP, "HELP"},
        {Enum::MOVE, "MOVE"},
        {Enum::RESIZE_LEFT_RIGHT, "RESIZE_LEFT_RIGHT"},
        {Enum::RESIZE_UP_DOWN, "RESIZE_UP_DOWN"},
        {Enum::SCREENSHOT_CHOOSE, "SCREENSHOT_CHOOSE"},
        {Enum::SCREENSHOT_CURSOR, "SCREENSHOT_CURSOR"},
        {Enum::TEXT_CURSOR, "TEXT_CURSOR"},
        {Enum::ZOOM_IN, "ZOOM_IN"},
        {Enum::ZOOM_OUT, "ZOOM_OUT"},
        {Enum::MIDDLE_BTN_EAST, "MIDDLE_BTN_EAST"},
        {Enum::MIDDLE_BTN_WEST, "MIDDLE_BTN_WEST"},
        {Enum::MIDDLE_BTN_SOUTH, "MIDDLE_BTN_SOUTH"},
        {Enum::MIDDLE_BTN_NORTH, "MIDDLE_BTN_NORTH"},
        {Enum::MIDDLE_BTN_NORTH_SOUTH, "MIDDLE_BTN_NORTH_SOUTH"},
        {Enum::MIDDLE_BTN_NORTH_EAST, "MIDDLE_BTN_NORTH_EAST"},
        {Enum::MIDDLE_BTN_NORTH_WEST, "MIDDLE_BTN_NORTH_WEST"},
        {Enum::MIDDLE_BTN_SOUTH_EAST, "MIDDLE_BTN_SOUTH_EAST"},
        {Enum::MIDDLE_BTN_SOUTH_WEST, "MIDDLE_BTN_SOUTH_WEST"},
        {Enum::MIDDLE_BTN_NORTH_SOUTH_WEST_EAST, "MIDDLE_BTN_NORTH_SOUTH_WEST_EAST"},
        {Enum::HORIZONTAL_TEXT_CURSOR, "HORIZONTAL_TEXT_CURSOR"},
        {Enum::CURSOR_CROSS, "CURSOR_CROSS"},
        {Enum::CURSOR_CIRCLE, "CURSOR_CIRCLE"},
        {Enum::LOADING, "LOADING"},
        {Enum::RUNNING, "RUNNING"},
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
        {Enum::TYPE_SYSTEM, "TYPE_SYSTEM"},
        {Enum::TYPE_CUTOUT, "TYPE_CUTOUT"},
        {Enum::TYPE_SYSTEM_GESTURE, "TYPE_SYSTEM_GESTURE"},
        {Enum::TYPE_KEYBOARD, "TYPE_KEYBOARD"},
        {Enum::TYPE_NAVIGATION_INDICATOR, "TYPE_NAVIGATION_INDICATOR"},
    }};
};

template<>
struct OhosEnumMeta<enums::ohos::window::MaximizePresentation>
{
    using Enum = enums::ohos::window::MaximizePresentation;
    static constexpr const char *fullTypeName = "@ohos.window.MaximizePresentation";
    static constexpr std::array<std::pair<Enum, const char *>, 3> enumeratorsNames = {{
        {Enum::FOLLOW_APP_IMMERSIVE_SETTING, "FOLLOW_APP_IMMERSIVE_SETTING"},
        {Enum::EXIT_IMMERSIVE, "EXIT_IMMERSIVE"},
        {Enum::ENTER_IMMERSIVE, "ENTER_IMMERSIVE"},
    }};
};

template<>
struct OhosEnumMeta<enums::ohos::window::ModalityType>
{
    using Enum = enums::ohos::window::ModalityType;
    static constexpr const char *fullTypeName = "@ohos.window.ModalityType";
    static constexpr std::array<std::pair<Enum, const char *>, 2> enumeratorsNames = {{
        {Enum::WINDOW_MODALITY, "WINDOW_MODALITY"},
        {Enum::APPLICATION_MODALITY, "APPLICATION_MODALITY"},
    }};
};

template<>
struct OhosEnumMeta<enums::ohos::window::RectChangeReason>
{
    using Enum = enums::ohos::window::RectChangeReason;
    static constexpr const char *fullTypeName = "@ohos.window.RectChangeReason";
    static constexpr std::array<std::pair<Enum, const char *>, 7> enumeratorsNames = {{
        {Enum::UNDEFINED, "UNDEFINED"},
        {Enum::MAXIMIZE, "MAXIMIZE"},
        {Enum::RECOVER, "RECOVER"},
        {Enum::MOVE, "MOVE"},
        {Enum::DRAG, "DRAG"},
        {Enum::DRAG_START, "DRAG_START"},
        {Enum::DRAG_END, "DRAG_END"},
    }};
};

template<>
struct OhosEnumMeta<enums::ohos::window::WindowEventType>
{
    using Enum = enums::ohos::window::WindowEventType;
    static constexpr const char *fullTypeName = "@ohos.window.WindowEventType";
    static constexpr std::array<std::pair<Enum, const char *>, 5> enumeratorsNames = {{
        {Enum::WINDOW_SHOWN, "WINDOW_SHOWN"},
        {Enum::WINDOW_ACTIVE, "WINDOW_ACTIVE"},
        {Enum::WINDOW_INACTIVE, "WINDOW_INACTIVE"},
        {Enum::WINDOW_HIDDEN, "WINDOW_HIDDEN"},
        {Enum::WINDOW_DESTROYED, "WINDOW_DESTROYED"},
    }};
};

template<>
struct OhosEnumMeta<enums::ohos::window::WindowStatusType>
{
    using Enum = enums::ohos::window::WindowStatusType;
    static constexpr const char *fullTypeName = "@ohos.window.WindowStatusType";
    static constexpr std::array<std::pair<Enum, const char *>, 6> enumeratorsNames = {{
        {Enum::UNDEFINED, "UNDEFINED"},
        {Enum::FULL_SCREEN, "FULL_SCREEN"},
        {Enum::MAXIMIZE, "MAXIMIZE"},
        {Enum::MINIMIZE, "MINIMIZE"},
        {Enum::FLOATING, "FLOATING"},
        {Enum::SPLIT_SCREEN, "SPLIT_SCREEN"},
    }};
};

}

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhos::enums::kit::ShareKit::systemShare::SelectionMode));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhos::enums::kit::ShareKit::systemShare::ShareAbilityType));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhos::enums::kit::ShareKit::systemShare::SharePreviewMode));
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
