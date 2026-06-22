// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/private/qohoscommon_p.h>
#include <array>
#include <cstdint>
#include <qarkui/qarkuiutils.h>
#include <qohoskeymodifiers.h>
#include <qohosplugincore.h>

QT_BEGIN_NAMESPACE

namespace {

QOhosOptional<std::uint64_t> tryGetUiInputEventModifierKeyStates(::ArkUI_UIInputEvent *uiInputEvent)
{
std::uint64_t keys;
    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_ArkUI_UIInputEvent_GetModifierKeyStates),
        uiInputEvent, &keys);
    return makeQOhosOptional(keys);
}

QFlags<OhosKeyboardModifier> mapArkUiModifierKeyStatesToOhosKeyboardModifiers(std::uint64_t modifierKeyStates)
{
    static const std::pair<std::uint64_t, OhosKeyboardModifier> arkUiToQtModifiersMap[] = {
        {::ArkUI_ModifierKeyName::ARKUI_MODIFIER_KEY_CTRL, OhosKeyboardModifier::CTRL},
        {::ArkUI_ModifierKeyName::ARKUI_MODIFIER_KEY_SHIFT, OhosKeyboardModifier::SHIFT},
        {::ArkUI_ModifierKeyName::ARKUI_MODIFIER_KEY_ALT, OhosKeyboardModifier::ALT},
    };

    QFlags<OhosKeyboardModifier> modifiers;
    for (const auto &arkUiToQtModifier : arkUiToQtModifiersMap)
        modifiers.setFlag(arkUiToQtModifier.second, (modifierKeyStates & arkUiToQtModifier.first) != 0);

    return modifiers;
}

QFlags<OhosKeyboardModifier> readKeyStandardModifiersFromKeyState()
{
    constexpr int ohosKeyboardModifierCount = 3;
    static const std::array<OhosKeyToModifier, ohosKeyboardModifierCount> keysToStandardModifiers = {{
        {
            OhosKeyboardModifier::CTRL,
            {::Input_KeyCode::KEYCODE_CTRL_LEFT, ::Input_KeyCode::KEYCODE_CTRL_RIGHT},
            &::OH_Input_GetKeyPressed,
            ::Input_KeyStateAction::KEY_PRESSED
        },
        {
            OhosKeyboardModifier::ALT,
            {::Input_KeyCode::KEYCODE_ALT_LEFT, ::Input_KeyCode::KEYCODE_ALT_RIGHT},
            &::OH_Input_GetKeyPressed,
            ::Input_KeyStateAction::KEY_PRESSED
        },
        {
            OhosKeyboardModifier::SHIFT,
            {::Input_KeyCode::KEYCODE_SHIFT_LEFT, ::Input_KeyCode::KEYCODE_SHIFT_RIGHT},
            &::OH_Input_GetKeyPressed,
            ::Input_KeyStateAction::KEY_PRESSED
        },
    }};

    return readKeyModifiersFromKeyState(
        QSpan(keysToStandardModifiers.data(), keysToStandardModifiers.size()));
}

QFlags<OhosKeyboardModifier> readKeyStandardModifiers(::ArkUI_UIInputEvent *uiInputEvent)
{
    const auto uiInputEventModifierKeyStats = tryGetUiInputEventModifierKeyStates(uiInputEvent);
    return uiInputEventModifierKeyStats.has_value()
        ? mapArkUiModifierKeyStatesToOhosKeyboardModifiers(uiInputEventModifierKeyStats.value())
        : readKeyStandardModifiersFromKeyState();
}

QFlags<OhosKeyboardModifier> readKeyExtendedModifiers()
{
    constexpr int ohosKeyboardModifierCount = 3;
    static const std::array<OhosKeyToModifier, ohosKeyboardModifierCount> keysToExtendedModifiers = {{
        {
            OhosKeyboardModifier::LOGO,
            {::Input_KeyCode::KEYCODE_META_LEFT, ::Input_KeyCode::KEYCODE_META_RIGHT},
            &::OH_Input_GetKeyPressed,
            ::Input_KeyStateAction::KEY_PRESSED
        },
        {
            OhosKeyboardModifier::CAPS_LOCK,
            {::Input_KeyCode::KEYCODE_CAPS_LOCK},
            &::OH_Input_GetKeySwitch,
            ::Input_KeyStateAction::KEY_SWITCH_ON
        },
        {
            OhosKeyboardModifier::NUM_LOCK,
            {::Input_KeyCode::KEYCODE_NUM_LOCK},
            &::OH_Input_GetKeySwitch,
            ::Input_KeyStateAction::KEY_SWITCH_ON
        },
    }};

    return readKeyModifiersFromKeyState(
        QSpan(keysToExtendedModifiers.data(), keysToExtendedModifiers.size()));
}

}

QFlags<OhosKeyboardModifier> readKeyModifiersFromKeyState(QSpan<const OhosKeyToModifier> keysToModifiers)
{
    auto keyStatusDeleter = [](::Input_KeyState *ptr) { ::OH_Input_DestroyKeyState(&ptr); };
    std::unique_ptr<::Input_KeyState, decltype(keyStatusDeleter)> keyState(::OH_Input_CreateKeyState(), keyStatusDeleter);
    if (keyState == nullptr) {
        qOhosReportFatalErrorAndAbort(
            "Acquisition of Input_KeyState object failed. Cannot read modifiers keys state.");
    }

    QFlags<OhosKeyboardModifier> keyModifiers = {};
    for (const auto &entry : keysToModifiers) {
        keyModifiers.setFlag(
            entry.modifier,
            std::any_of(
                entry.keysToCheck.begin(), entry.keysToCheck.end(),
                [&](::Input_KeyCode keyCode) {
                    ::OH_Input_SetKeyCode(keyState.get(), keyCode);
                    if (::OH_Input_GetKeyState(keyState.get()) != ::INPUT_SUCCESS) {
                        qOhosWarning(QtForOhos) << "Cannot get key state for key: " << keyCode;
                        return false;
                    }
                    return entry.getKeyStateActionFunc(keyState.get()) == entry.keyStateActionKeyActive;
                }));
    }

    return keyModifiers;
}

QFlags<OhosKeyboardModifier> readKeyModifiersFromOhosUiInputEvent(::ArkUI_UIInputEvent *uiInputEvent)
{
    return readKeyStandardModifiers(uiInputEvent) | readKeyExtendedModifiers();
}

Qt::KeyboardModifiers convertOhosToQtKeyboardModifiers(QFlags<OhosKeyboardModifier> ohosKeysModifiers)
{
    Qt::KeyboardModifiers keyboardModifiers = Qt::NoModifier;
    keyboardModifiers.setFlag(
        Qt::ShiftModifier, ohosKeysModifiers.testFlag(OhosKeyboardModifier::SHIFT));
    keyboardModifiers.setFlag(
        Qt::AltModifier, ohosKeysModifiers.testFlag(OhosKeyboardModifier::ALT));
    keyboardModifiers.setFlag(
        Qt::ControlModifier, ohosKeysModifiers.testFlag(OhosKeyboardModifier::CTRL));
    keyboardModifiers.setFlag(
        Qt::MetaModifier, ohosKeysModifiers.testFlag(OhosKeyboardModifier::LOGO));

    return keyboardModifiers;
}

QT_END_NAMESPACE
