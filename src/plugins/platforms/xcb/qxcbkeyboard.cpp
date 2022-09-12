/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "qxcbkeyboard.h"
#include "qxcbwindow.h"
#include "qxcbscreen.h"

#include <qpa/qwindowsysteminterface.h>
#include <qpa/qplatforminputcontext.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformcursor.h>

#include <QtCore/QMetaEnum>

#include <private/qguiapplication_p.h>

#include <xcb/xinput.h>

QT_BEGIN_NAMESPACE

Qt::KeyboardModifiers QXcbKeyboard::translateModifiers(int s) const
{
    Qt::KeyboardModifiers ret = Qt::NoModifier;
    if (s & XCB_MOD_MASK_SHIFT)
        ret |= Qt::ShiftModifier;
    if (s & XCB_MOD_MASK_CONTROL)
        ret |= Qt::ControlModifier;
    if (s & rmod_masks.alt)
        ret |= Qt::AltModifier;
    if (s & rmod_masks.meta)
        ret |= Qt::MetaModifier;
    if (s & rmod_masks.altgr)
        ret |= Qt::GroupSwitchModifier;
    return ret;
}

/* Look at a pair of unshifted and shifted key symbols.
 * If the 'unshifted' symbol is uppercase and there is no shifted symbol,
 * return the matching lowercase symbol; otherwise return 0.
 * The caller can then use the previously 'unshifted' symbol as the new
 * 'shifted' (uppercase) symbol and the symbol returned by the function
 * as the new 'unshifted' (lowercase) symbol.) */
static xcb_keysym_t getUnshiftedXKey(xcb_keysym_t unshifted, xcb_keysym_t shifted)
{
    if (shifted != XKB_KEY_NoSymbol) // Has a shifted symbol
        return 0;

    xcb_keysym_t xlower;
    xcb_keysym_t xupper;
    QXkbCommon::xkbcommon_XConvertCase(unshifted, &xlower, &xupper);

    if (xlower != xupper          // Check if symbol is cased
        && unshifted == xupper) { // Unshifted must be upper case
        return xlower;
    }

    return 0;
}

static QByteArray symbolsGroupString(const xcb_keysym_t *symbols, int count)
{
    // Don't output trailing NoSymbols
    while (count > 0 && symbols[count - 1] == XKB_KEY_NoSymbol)
        count--;

    QByteArray groupString;
    for (int symIndex = 0; symIndex < count; symIndex++) {
        xcb_keysym_t sym = symbols[symIndex];
        char symString[64];
        if (sym == XKB_KEY_NoSymbol)
            strcpy(symString, "NoSymbol");
        else
            xkb_keysym_get_name(sym, symString, sizeof(symString));

        if (!groupString.isEmpty())
            groupString += ", ";
        groupString += symString;
    }
    return groupString;
}

struct xkb_keymap *QXcbKeyboard::keymapFromCore(const KeysymModifierMap &keysymMods)
{
    /* Construct an XKB keymap string from information queried from
     * the X server */
    QByteArray keymap;
    keymap += "xkb_keymap {\n";

    const xcb_keycode_t minKeycode = connection()->setup()->min_keycode;
    const xcb_keycode_t maxKeycode = connection()->setup()->max_keycode;

    // Generate symbolic names from keycodes
    {
        keymap +=
            "xkb_keycodes \"core\" {\n"
            "\tminimum = " + QByteArray::number(minKeycode) + ";\n"
            "\tmaximum = " + QByteArray::number(maxKeycode) + ";\n";
        for (int code = minKeycode; code <= maxKeycode; code++) {
            auto codeStr = QByteArray::number(code);
            keymap += "<K" + codeStr + "> = " + codeStr + ";\n";
        }
        /* TODO: indicators?
         */
        keymap += "};\n"; // xkb_keycodes
    }

    /* Set up default types (xkbcommon automatically assigns these to
     * symbols, but doesn't have shift info) */
    keymap +=
        "xkb_types \"core\" {\n"
        "virtual_modifiers NumLock,Alt,LevelThree;\n"
        "type \"ONE_LEVEL\" {\n"
            "modifiers= none;\n"
            "level_name[Level1] = \"Any\";\n"
        "};\n"
        "type \"TWO_LEVEL\" {\n"
            "modifiers= Shift;\n"
            "map[Shift]= Level2;\n"
            "level_name[Level1] = \"Base\";\n"
            "level_name[Level2] = \"Shift\";\n"
        "};\n"
        "type \"ALPHABETIC\" {\n"
            "modifiers= Shift+Lock;\n"
            "map[Shift]= Level2;\n"
            "map[Lock]= Level2;\n"
            "level_name[Level1] = \"Base\";\n"
            "level_name[Level2] = \"Caps\";\n"
        "};\n"
        "type \"KEYPAD\" {\n"
            "modifiers= Shift+NumLock;\n"
            "map[Shift]= Level2;\n"
            "map[NumLock]= Level2;\n"
            "level_name[Level1] = \"Base\";\n"
            "level_name[Level2] = \"Number\";\n"
        "};\n"
        "type \"FOUR_LEVEL\" {\n"
            "modifiers= Shift+LevelThree;\n"
            "map[Shift]= Level2;\n"
            "map[LevelThree]= Level3;\n"
            "map[Shift+LevelThree]= Level4;\n"
            "level_name[Level1] = \"Base\";\n"
            "level_name[Level2] = \"Shift\";\n"
            "level_name[Level3] = \"Alt Base\";\n"
            "level_name[Level4] = \"Shift Alt\";\n"
        "};\n"
        "type \"FOUR_LEVEL_ALPHABETIC\" {\n"
            "modifiers= Shift+Lock+LevelThree;\n"
            "map[Shift]= Level2;\n"
            "map[Lock]= Level2;\n"
            "map[LevelThree]= Level3;\n"
            "map[Shift+LevelThree]= Level4;\n"
            "map[Lock+LevelThree]= Level4;\n"
            "map[Shift+Lock+LevelThree]= Level3;\n"
            "level_name[Level1] = \"Base\";\n"
            "level_name[Level2] = \"Shift\";\n"
            "level_name[Level3] = \"Alt Base\";\n"
            "level_name[Level4] = \"Shift Alt\";\n"
        "};\n"
        "type \"FOUR_LEVEL_SEMIALPHABETIC\" {\n"
            "modifiers= Shift+Lock+LevelThree;\n"
            "map[Shift]= Level2;\n"
            "map[Lock]= Level2;\n"
            "map[LevelThree]= Level3;\n"
            "map[Shift+LevelThree]= Level4;\n"
            "map[Lock+LevelThree]= Level3;\n"
            "preserve[Lock+LevelThree]= Lock;\n"
            "map[Shift+Lock+LevelThree]= Level4;\n"
            "preserve[Shift+Lock+LevelThree]= Lock;\n"
            "level_name[Level1] = \"Base\";\n"
            "level_name[Level2] = \"Shift\";\n"
            "level_name[Level3] = \"Alt Base\";\n"
            "level_name[Level4] = \"Shift Alt\";\n"
        "};\n"
        "type \"FOUR_LEVEL_KEYPAD\" {\n"
            "modifiers= Shift+NumLock+LevelThree;\n"
            "map[Shift]= Level2;\n"
            "map[NumLock]= Level2;\n"
            "map[LevelThree]= Level3;\n"
            "map[Shift+LevelThree]= Level4;\n"
            "map[NumLock+LevelThree]= Level4;\n"
            "map[Shift+NumLock+LevelThree]= Level3;\n"
            "level_name[Level1] = \"Base\";\n"
            "level_name[Level2] = \"Number\";\n"
            "level_name[Level3] = \"Alt Base\";\n"
            "level_name[Level4] = \"Alt Number\";\n"
        "};\n"
        "};\n"; // xkb_types

    // Generate mapping between symbolic names and keysyms
    {
        QVector<xcb_keysym_t> xkeymap;
        int keysymsPerKeycode = 0;
        {
            int keycodeCount = maxKeycode - minKeycode + 1;
            if (auto keymapReply = Q_XCB_REPLY(xcb_get_keyboard_mapping, xcb_connection(),
                                               minKeycode, keycodeCount)) {
                keysymsPerKeycode = keymapReply->keysyms_per_keycode;
                int numSyms = keycodeCount * keysymsPerKeycode;
                auto keymapPtr = xcb_get_keyboard_mapping_keysyms(keymapReply.get());
                xkeymap.resize(numSyms);
                for (int i = 0; i < numSyms; i++)
                    xkeymap[i] = keymapPtr[i];
            }
        }
        if (xkeymap.isEmpty())
            return nullptr;

        static const char *const builtinModifiers[] =
        { "Shift", "Lock", "Control", "Mod1", "Mod2", "Mod3", "Mod4", "Mod5" };

        /* Level 3 symbols (e.g. AltGr+something) seem to come in two flavors:
         * - as a proper level 3 in group 1, at least on recent X.org versions
         * - 'disguised' as group 2, on 'legacy' X servers
         * In the 2nd case, remap group 2 to level 3, that seems to work better
         * in practice */
        bool mapGroup2ToLevel3 = keysymsPerKeycode < 5;

        keymap += "xkb_symbols \"core\" {\n";
        for (int code = minKeycode; code <= maxKeycode; code++) {
            auto codeMap = xkeymap.constData() + (code - minKeycode) * keysymsPerKeycode;

            const int maxGroup1 = 4; // We only support 4 shift states anyway
            const int maxGroup2 = 2; // Only 3rd and 4th keysym are group 2
            xcb_keysym_t symbolsGroup1[maxGroup1];
            xcb_keysym_t symbolsGroup2[maxGroup2];
            for (int i = 0; i < maxGroup1 + maxGroup2; i++) {
                xcb_keysym_t sym = i < keysymsPerKeycode ? codeMap[i] : XKB_KEY_NoSymbol;
                if (mapGroup2ToLevel3) {
                    // Merge into single group
                    if (i < maxGroup1)
                        symbolsGroup1[i] = sym;
                } else {
                    // Preserve groups
                    if (i < 2)
                        symbolsGroup1[i] = sym;
                    else if (i < 4)
                        symbolsGroup2[i - 2] = sym;
                    else
                        symbolsGroup1[i - 2] = sym;
                }
            }

            /* Fix symbols so the unshifted and shifted symbols have
             * lower resp. upper case */
            if (auto lowered = getUnshiftedXKey(symbolsGroup1[0], symbolsGroup1[1])) {
                symbolsGroup1[1] = symbolsGroup1[0];
                symbolsGroup1[0] = lowered;
            }
            if (auto lowered = getUnshiftedXKey(symbolsGroup2[0], symbolsGroup2[1])) {
                symbolsGroup2[1] = symbolsGroup2[0];
                symbolsGroup2[0] = lowered;
            }

            QByteArray groupStr1 = symbolsGroupString(symbolsGroup1, maxGroup1);
            if (groupStr1.isEmpty())
                continue;

            keymap += "key <K" + QByteArray::number(code) + "> { ";
            keymap += "symbols[Group1] = [ " + groupStr1 + " ]";
            QByteArray groupStr2 = symbolsGroupString(symbolsGroup2, maxGroup2);
            if (!groupStr2.isEmpty())
                keymap += ", symbols[Group2] = [ " + groupStr2 + " ]";

            // See if this key code is for a modifier
            xcb_keysym_t modifierSym = XKB_KEY_NoSymbol;
            for (int symIndex = 0; symIndex < keysymsPerKeycode; symIndex++) {
                xcb_keysym_t sym = codeMap[symIndex];

                if (sym == XKB_KEY_Alt_L
                    || sym == XKB_KEY_Meta_L
                    || sym == XKB_KEY_Mode_switch
                    || sym == XKB_KEY_Super_L
                    || sym == XKB_KEY_Super_R
                    || sym == XKB_KEY_Hyper_L
                    || sym == XKB_KEY_Hyper_R) {
                    modifierSym = sym;
                    break;
                }
            }

            // AltGr
            if (modifierSym == XKB_KEY_Mode_switch)
                keymap += ", virtualMods=LevelThree";
            keymap += " };\n"; // key

            // Generate modifier mappings
            int modNum = keysymMods.value(modifierSym, -1);
            if (modNum != -1) {
                // Here modNum is always < 8 (see keysymsToModifiers())
                keymap += QByteArray("modifier_map ") + builtinModifiers[modNum]
                    + " { <K" + QByteArray::number(code) + "> };\n";
            }
        }
        // TODO: indicators?
        keymap += "};\n"; // xkb_symbols
    }

    // We need an "Alt" modifier, provide via the xkb_compatibility section
    keymap +=
        "xkb_compatibility \"core\" {\n"
        "virtual_modifiers NumLock,Alt,LevelThree;\n"
        "interpret Alt_L+AnyOf(all) {\n"
            "virtualModifier= Alt;\n"
            "action= SetMods(modifiers=modMapMods,clearLocks);\n"
        "};\n"
        "interpret Alt_R+AnyOf(all) {\n"
            "virtualModifier= Alt;\n"
            "action= SetMods(modifiers=modMapMods,clearLocks);\n"
        "};\n"
        "};\n";

    /* TODO: There is an issue with modifier state not being handled
     * correctly if using Xming with XKEYBOARD disabled. */

    keymap += "};\n"; // xkb_keymap

    return xkb_keymap_new_from_buffer(m_xkbContext.get(),
                                      keymap.constData(),
                                      keymap.size(),
                                      XKB_KEYMAP_FORMAT_TEXT_V1,
                                      XKB_KEYMAP_COMPILE_NO_FLAGS);
}

void QXcbKeyboard::updateKeymap(xcb_mapping_notify_event_t *event)
{
    if (connection()->hasXKB() || event->request == XCB_MAPPING_POINTER)
        return;

    xcb_refresh_keyboard_mapping(m_key_symbols, event);
    updateKeymap();
}

void QXcbKeyboard::updateKeymap()
{
    KeysymModifierMap keysymMods;
    if (!connection()->hasXKB())
        keysymMods = keysymsToModifiers();
    updateModifiers(keysymMods);

    m_config = true;

    if (!m_xkbContext) {
        m_xkbContext.reset(xkb_context_new(XKB_CONTEXT_NO_DEFAULT_INCLUDES));
        if (!m_xkbContext) {
            qCWarning(lcQpaKeyboard, "failed to create XKB context");
            m_config = false;
            return;
        }
        xkb_log_level logLevel = lcQpaKeyboard().isDebugEnabled() ?
                                 XKB_LOG_LEVEL_DEBUG : XKB_LOG_LEVEL_CRITICAL;
        xkb_context_set_log_level(m_xkbContext.get(), logLevel);
    }

    if (connection()->hasXKB()) {
        m_xkbKeymap.reset(xkb_x11_keymap_new_from_device(m_xkbContext.get(), xcb_connection(),
                                                         core_device_id, XKB_KEYMAP_COMPILE_NO_FLAGS));
        if (m_xkbKeymap)
            m_xkbState.reset(xkb_x11_state_new_from_device(m_xkbKeymap.get(), xcb_connection(), core_device_id));
    } else {
        m_xkbKeymap.reset(keymapFromCore(keysymMods));
        if (m_xkbKeymap)
            m_xkbState.reset(xkb_state_new(m_xkbKeymap.get()));
    }

    if (!m_xkbKeymap) {
        qCWarning(lcQpaKeyboard, "failed to compile a keymap");
        m_config = false;
        return;
    }
    if (!m_xkbState) {
        qCWarning(lcQpaKeyboard, "failed to create XKB state");
        m_config = false;
        return;
    }

    updateXKBMods();

    QXkbCommon::verifyHasLatinLayout(m_xkbKeymap.get());
}

QList<int> QXcbKeyboard::possibleKeys(const QKeyEvent *event) const
{
    return QXkbCommon::possibleKeys(m_xkbState.get(), event, m_superAsMeta, m_hyperAsMeta);
}

void QXcbKeyboard::updateXKBState(xcb_xkb_state_notify_event_t *state)
{
    if (m_config && connection()->hasXKB()) {
        const xkb_state_component changedComponents
                = xkb_state_update_mask(m_xkbState.get(),
                                  state->baseMods,
                                  state->latchedMods,
                                  state->lockedMods,
                                  state->baseGroup,
                                  state->latchedGroup,
                                  state->lockedGroup);

        handleStateChanges(changedComponents);
    }
}

static xkb_layout_index_t lockedGroup(quint16 state)
{
    return (state >> 13) & 3; // bits 13 and 14 report the state keyboard group
}

void QXcbKeyboard::updateXKBStateFromCore(quint16 state)
{
    if (m_config && !connection()->hasXKB()) {
        struct xkb_state *xkbState = m_xkbState.get();
        xkb_mod_mask_t modsDepressed = xkb_state_serialize_mods(xkbState, XKB_STATE_MODS_DEPRESSED);
        xkb_mod_mask_t modsLatched = xkb_state_serialize_mods(xkbState, XKB_STATE_MODS_LATCHED);
        xkb_mod_mask_t modsLocked = xkb_state_serialize_mods(xkbState, XKB_STATE_MODS_LOCKED);
        xkb_mod_mask_t xkbMask = xkbModMask(state);

        xkb_mod_mask_t latched = modsLatched & xkbMask;
        xkb_mod_mask_t locked = modsLocked & xkbMask;
        xkb_mod_mask_t depressed = modsDepressed & xkbMask;
        // set modifiers in depressed if they don't appear in any of the final masks
        depressed |= ~(depressed | latched | locked) & xkbMask;

        xkb_state_component changedComponents = xkb_state_update_mask(
                    xkbState, depressed, latched, locked, 0, 0, lockedGroup(state));

        handleStateChanges(changedComponents);
    }
}

void QXcbKeyboard::updateXKBStateFromXI(void *modInfo, void *groupInfo)
{
    if (m_config && !connection()->hasXKB()) {
        auto *mods = static_cast<xcb_input_modifier_info_t *>(modInfo);
        auto *group = static_cast<xcb_input_group_info_t *>(groupInfo);
        const xkb_state_component changedComponents
                = xkb_state_update_mask(m_xkbState.get(),
                                        mods->base,
                                        mods->latched,
                                        mods->locked,
                                        group->base,
                                        group->latched,
                                        group->locked);

        handleStateChanges(changedComponents);
    }
}

void QXcbKeyboard::handleStateChanges(xkb_state_component changedComponents)
{
    // Note: Ubuntu (with Unity) always creates a new keymap when layout is changed
    // via system settings, which means that the layout change would not be detected
    // by this code. That can be solved by emitting KeyboardLayoutChange also from updateKeymap().
    if ((changedComponents & XKB_STATE_LAYOUT_EFFECTIVE) == XKB_STATE_LAYOUT_EFFECTIVE)
        qCDebug(lcQpaKeyboard, "TODO: Support KeyboardLayoutChange on QPA (QTBUG-27681)");
}

xkb_mod_mask_t QXcbKeyboard::xkbModMask(quint16 state)
{
    xkb_mod_mask_t xkb_mask = 0;

    if ((state & XCB_MOD_MASK_SHIFT) && xkb_mods.shift != XKB_MOD_INVALID)
        xkb_mask |= (1 << xkb_mods.shift);
    if ((state & XCB_MOD_MASK_LOCK) && xkb_mods.lock != XKB_MOD_INVALID)
        xkb_mask |= (1 << xkb_mods.lock);
    if ((state & XCB_MOD_MASK_CONTROL) && xkb_mods.control != XKB_MOD_INVALID)
        xkb_mask |= (1 << xkb_mods.control);
    if ((state & XCB_MOD_MASK_1) && xkb_mods.mod1 != XKB_MOD_INVALID)
        xkb_mask |= (1 << xkb_mods.mod1);
    if ((state & XCB_MOD_MASK_2) && xkb_mods.mod2 != XKB_MOD_INVALID)
        xkb_mask |= (1 << xkb_mods.mod2);
    if ((state & XCB_MOD_MASK_3) && xkb_mods.mod3 != XKB_MOD_INVALID)
        xkb_mask |= (1 << xkb_mods.mod3);
    if ((state & XCB_MOD_MASK_4) && xkb_mods.mod4 != XKB_MOD_INVALID)
        xkb_mask |= (1 << xkb_mods.mod4);
    if ((state & XCB_MOD_MASK_5) && xkb_mods.mod5 != XKB_MOD_INVALID)
        xkb_mask |= (1 << xkb_mods.mod5);

    return xkb_mask;
}

void QXcbKeyboard::updateXKBMods()
{
    xkb_mods.shift = xkb_keymap_mod_get_index(m_xkbKeymap.get(), XKB_MOD_NAME_SHIFT);
    xkb_mods.lock = xkb_keymap_mod_get_index(m_xkbKeymap.get(), XKB_MOD_NAME_CAPS);
    xkb_mods.control = xkb_keymap_mod_get_index(m_xkbKeymap.get(), XKB_MOD_NAME_CTRL);
    xkb_mods.mod1 = xkb_keymap_mod_get_index(m_xkbKeymap.get(), "Mod1");
    xkb_mods.mod2 = xkb_keymap_mod_get_index(m_xkbKeymap.get(), "Mod2");
    xkb_mods.mod3 = xkb_keymap_mod_get_index(m_xkbKeymap.get(), "Mod3");
    xkb_mods.mod4 = xkb_keymap_mod_get_index(m_xkbKeymap.get(), "Mod4");
    xkb_mods.mod5 = xkb_keymap_mod_get_index(m_xkbKeymap.get(), "Mod5");
}

QXcbKeyboard::QXcbKeyboard(QXcbConnection *connection)
    : QXcbObject(connection)
{
    core_device_id = 0;
    if (connection->hasXKB()) {
        selectEvents();
        core_device_id = xkb_x11_get_core_keyboard_device_id(xcb_connection());
        if (core_device_id == -1) {
            qCWarning(lcQpaXcb, "failed to get core keyboard device info");
            return;
        }
    } else {
        m_key_symbols = xcb_key_symbols_alloc(xcb_connection());
    }

    updateKeymap();
}

QXcbKeyboard::~QXcbKeyboard()
{
    if (m_key_symbols)
        xcb_key_symbols_free(m_key_symbols);
}

void QXcbKeyboard::initialize()
{
    auto inputContext = QGuiApplicationPrivate::platformIntegration()->inputContext();
    QXkbCommon::setXkbContext(inputContext, m_xkbContext.get());
}

void QXcbKeyboard::selectEvents()
{
    const uint16_t required_map_parts = (XCB_XKB_MAP_PART_KEY_TYPES |
        XCB_XKB_MAP_PART_KEY_SYMS |
        XCB_XKB_MAP_PART_MODIFIER_MAP |
        XCB_XKB_MAP_PART_EXPLICIT_COMPONENTS |
        XCB_XKB_MAP_PART_KEY_ACTIONS |
        XCB_XKB_MAP_PART_KEY_BEHAVIORS |
        XCB_XKB_MAP_PART_VIRTUAL_MODS |
        XCB_XKB_MAP_PART_VIRTUAL_MOD_MAP);

    const uint16_t required_events = (XCB_XKB_EVENT_TYPE_NEW_KEYBOARD_NOTIFY |
        XCB_XKB_EVENT_TYPE_MAP_NOTIFY |
        XCB_XKB_EVENT_TYPE_STATE_NOTIFY);

    // XKB events are reported to all interested clients without regard
    // to the current keyboard input focus or grab state
    xcb_void_cookie_t select = xcb_xkb_select_events_checked(
                xcb_connection(),
                XCB_XKB_ID_USE_CORE_KBD,
                required_events,
                0,
                required_events,
                required_map_parts,
                required_map_parts,
                nullptr);

    xcb_generic_error_t *error = xcb_request_check(xcb_connection(), select);
    if (error) {
        free(error);
        qCWarning(lcQpaXcb, "failed to select notify events from XKB");
    }
}

void QXcbKeyboard::updateVModMapping()
{
    xcb_xkb_get_names_value_list_t names_list;

    memset(&vmod_masks, 0, sizeof(vmod_masks));

    auto name_reply = Q_XCB_REPLY(xcb_xkb_get_names, xcb_connection(),
                                  XCB_XKB_ID_USE_CORE_KBD,
                                  XCB_XKB_NAME_DETAIL_VIRTUAL_MOD_NAMES);
    if (!name_reply) {
        qWarning("Qt: failed to retrieve the virtual modifier names from XKB");
        return;
    }

    const void *buffer = xcb_xkb_get_names_value_list(name_reply.get());
    xcb_xkb_get_names_value_list_unpack(buffer,
                                        name_reply->nTypes,
                                        name_reply->indicators,
                                        name_reply->virtualMods,
                                        name_reply->groupNames,
                                        name_reply->nKeys,
                                        name_reply->nKeyAliases,
                                        name_reply->nRadioGroups,
                                        name_reply->which,
                                        &names_list);

    int count = 0;
    uint vmod_mask, bit;
    char *vmod_name;
    vmod_mask = name_reply->virtualMods;
    // find the virtual modifiers for which names are defined.
    for (bit = 1; vmod_mask; bit <<= 1) {
        vmod_name = nullptr;

        if (!(vmod_mask & bit))
            continue;

        vmod_mask &= ~bit;
        // virtualModNames - the list of virtual modifier atoms beginning with the lowest-numbered
        // virtual modifier for which a name is defined and proceeding to the highest.
        QByteArray atomName = connection()->atomName(names_list.virtualModNames[count]);
        vmod_name = atomName.data();
        count++;

        if (!vmod_name)
            continue;

        // similarly we could retrieve NumLock, Super, Hyper modifiers if needed.
        if (qstrcmp(vmod_name, "Alt") == 0)
            vmod_masks.alt = bit;
        else if (qstrcmp(vmod_name, "Meta") == 0)
            vmod_masks.meta = bit;
        else if (qstrcmp(vmod_name, "AltGr") == 0)
            vmod_masks.altgr = bit;
        else if (qstrcmp(vmod_name, "Super") == 0)
            vmod_masks.super = bit;
        else if (qstrcmp(vmod_name, "Hyper") == 0)
            vmod_masks.hyper = bit;
    }
}

void QXcbKeyboard::updateVModToRModMapping()
{
    xcb_xkb_get_map_map_t map;

    memset(&rmod_masks, 0, sizeof(rmod_masks));

    auto map_reply = Q_XCB_REPLY(xcb_xkb_get_map,
                                 xcb_connection(),
                                 XCB_XKB_ID_USE_CORE_KBD,
                                 XCB_XKB_MAP_PART_VIRTUAL_MODS,
                                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    if (!map_reply) {
        qWarning("Qt: failed to retrieve the virtual modifier map from XKB");
        return;
    }

    const void *buffer = xcb_xkb_get_map_map(map_reply.get());
    xcb_xkb_get_map_map_unpack(buffer,
                               map_reply->nTypes,
                               map_reply->nKeySyms,
                               map_reply->nKeyActions,
                               map_reply->totalActions,
                               map_reply->totalKeyBehaviors,
                               map_reply->nVModMapKeys,
                               map_reply->totalKeyExplicit,
                               map_reply->totalModMapKeys,
                               map_reply->totalVModMapKeys,
                               map_reply->present,
                               &map);

    uint vmod_mask, bit;
    // the virtual modifiers mask for which a set of corresponding
    // real modifiers is to be returned
    vmod_mask = map_reply->virtualMods;
    int count = 0;

    for (bit = 1; vmod_mask; bit <<= 1) {
        uint modmap;

        if (!(vmod_mask & bit))
            continue;

        vmod_mask &= ~bit;
        // real modifier bindings for the specified virtual modifiers
        modmap = map.vmods_rtrn[count];
        count++;

        if (vmod_masks.alt == bit)
            rmod_masks.alt = modmap;
        else if (vmod_masks.meta == bit)
            rmod_masks.meta = modmap;
        else if (vmod_masks.altgr == bit)
            rmod_masks.altgr = modmap;
        else if (vmod_masks.super == bit)
            rmod_masks.super = modmap;
        else if (vmod_masks.hyper == bit)
            rmod_masks.hyper = modmap;
    }
}

// Small helper: set modifier bit, if modifier position is valid
static inline void applyModifier(uint *mask, int modifierBit)
{
    if (modifierBit >= 0 && modifierBit < 8)
        *mask |= 1 << modifierBit;
}

void QXcbKeyboard::updateModifiers(const KeysymModifierMap &keysymMods)
{
    if (connection()->hasXKB()) {
        updateVModMapping();
        updateVModToRModMapping();
    } else {
        memset(&rmod_masks, 0, sizeof(rmod_masks));
        // Compute X modifier bits for Qt modifiers
        applyModifier(&rmod_masks.alt,   keysymMods.value(XKB_KEY_Alt_L,       -1));
        applyModifier(&rmod_masks.alt,   keysymMods.value(XKB_KEY_Alt_R,       -1));
        applyModifier(&rmod_masks.meta,  keysymMods.value(XKB_KEY_Meta_L,      -1));
        applyModifier(&rmod_masks.meta,  keysymMods.value(XKB_KEY_Meta_R,      -1));
        applyModifier(&rmod_masks.altgr, keysymMods.value(XKB_KEY_Mode_switch, -1));
        applyModifier(&rmod_masks.super, keysymMods.value(XKB_KEY_Super_L,     -1));
        applyModifier(&rmod_masks.super, keysymMods.value(XKB_KEY_Super_R,     -1));
        applyModifier(&rmod_masks.hyper, keysymMods.value(XKB_KEY_Hyper_L,     -1));
        applyModifier(&rmod_masks.hyper, keysymMods.value(XKB_KEY_Hyper_R,     -1));
    }

    resolveMaskConflicts();
}

// Small helper: check if an array of xcb_keycode_t contains a certain code
static inline bool keycodes_contains(xcb_keycode_t *codes, xcb_keycode_t which)
{
    while (*codes != XCB_NO_SYMBOL) {
        if (*codes == which) return true;
        codes++;
    }
    return false;
}

QXcbKeyboard::KeysymModifierMap QXcbKeyboard::keysymsToModifiers()
{
    // The core protocol does not provide a convenient way to determine the mapping
    // of modifier bits. Clients must retrieve and search the modifier map to determine
    // the keycodes bound to each modifier, and then retrieve and search the keyboard
    // mapping to determine the keysyms bound to the keycodes. They must repeat this
    // process for all modifiers whenever any part of the modifier mapping is changed.

    KeysymModifierMap map;

    auto modMapReply = Q_XCB_REPLY(xcb_get_modifier_mapping, xcb_connection());
    if (!modMapReply) {
        qWarning("Qt: failed to get modifier mapping");
        return map;
    }

    // for Alt and Meta L and R are the same
    static const xcb_keysym_t symbols[] = {
        XKB_KEY_Alt_L, XKB_KEY_Meta_L, XKB_KEY_Mode_switch, XKB_KEY_Super_L, XKB_KEY_Super_R,
        XKB_KEY_Hyper_L, XKB_KEY_Hyper_R
    };
    static const size_t numSymbols = sizeof symbols / sizeof *symbols;

    // Figure out the modifier mapping, ICCCM 6.6
    xcb_keycode_t* modKeyCodes[numSymbols];
    for (size_t i = 0; i < numSymbols; ++i)
        modKeyCodes[i] = xcb_key_symbols_get_keycode(m_key_symbols, symbols[i]);

    xcb_keycode_t *modMap = xcb_get_modifier_mapping_keycodes(modMapReply.get());
    const int modMapLength = xcb_get_modifier_mapping_keycodes_length(modMapReply.get());
    /* For each modifier of "Shift, Lock, Control, Mod1, Mod2, Mod3,
     * Mod4, and Mod5" the modifier map contains keycodes_per_modifier
     * key codes that are associated with a modifier.
     *
     * As an example, take this 'xmodmap' output:
     *   xmodmap: up to 4 keys per modifier, (keycodes in parentheses):
     *
     *   shift       Shift_L (0x32),  Shift_R (0x3e)
     *   lock        Caps_Lock (0x42)
     *   control     Control_L (0x25),  Control_R (0x69)
     *   mod1        Alt_L (0x40),  Alt_R (0x6c),  Meta_L (0xcd)
     *   mod2        Num_Lock (0x4d)
     *   mod3
     *   mod4        Super_L (0x85),  Super_R (0x86),  Super_L (0xce),  Hyper_L (0xcf)
     *   mod5        ISO_Level3_Shift (0x5c),  Mode_switch (0xcb)
     *
     * The corresponding raw modifier map would contain keycodes for:
     *   Shift_L (0x32), Shift_R (0x3e), 0, 0,
     *   Caps_Lock (0x42), 0, 0, 0,
     *   Control_L (0x25), Control_R (0x69), 0, 0,
     *   Alt_L (0x40), Alt_R (0x6c), Meta_L (0xcd), 0,
     *   Num_Lock (0x4d), 0, 0, 0,
     *   0,0,0,0,
     *   Super_L (0x85),  Super_R (0x86),  Super_L (0xce),  Hyper_L (0xcf),
     *   ISO_Level3_Shift (0x5c),  Mode_switch (0xcb), 0, 0
     */

    /* Create a map between a modifier keysym (as per the symbols array)
     * and the modifier bit it's associated with (if any).
     * As modMap contains key codes, search modKeyCodes for a match;
     * if one is found we can look up the associated keysym.
     * Together with the modifier index this will be used
     * to compute a mapping between X modifier bits and Qt's
     * modifiers (Alt, Ctrl etc). */
    for (int i = 0; i < modMapLength; i++) {
        if (modMap[i] == XCB_NO_SYMBOL)
            continue;
        // Get key symbol for key code
        for (size_t k = 0; k < numSymbols; k++) {
            if (modKeyCodes[k] && keycodes_contains(modKeyCodes[k], modMap[i])) {
                // Key code is for modifier. Record mapping
                xcb_keysym_t sym = symbols[k];
                /* As per modMap layout explanation above, dividing
                 * by keycodes_per_modifier gives the 'row' in the
                 * modifier map, which in turn is the modifier bit. */
                map[sym] = i / modMapReply->keycodes_per_modifier;
                break;
            }
        }
    }

    for (size_t i = 0; i < numSymbols; ++i)
        free(modKeyCodes[i]);

    return map;
}

void QXcbKeyboard::resolveMaskConflicts()
{
    // if we don't have a meta key (or it's hidden behind alt), use super or hyper to generate
    // Qt::Key_Meta and Qt::MetaModifier, since most newer XFree86/Xorg installations map the Windows
    // key to Super
    if (rmod_masks.alt == rmod_masks.meta)
        rmod_masks.meta = 0;

    if (rmod_masks.meta == 0) {
        // no meta keys... s/meta/super,
        rmod_masks.meta = rmod_masks.super;
        if (rmod_masks.meta == 0) {
            // no super keys either? guess we'll use hyper then
            rmod_masks.meta = rmod_masks.hyper;
        }
    }

    // translate Super/Hyper keys to Meta if we're using them as the MetaModifier
    if (rmod_masks.meta && rmod_masks.meta == rmod_masks.super)
        m_superAsMeta = true;
    if (rmod_masks.meta && rmod_masks.meta == rmod_masks.hyper)
        m_hyperAsMeta = true;
}

void QXcbKeyboard::handleKeyEvent(xcb_window_t sourceWindow, QEvent::Type type, xcb_keycode_t code,
                                  quint16 state, xcb_timestamp_t time, bool fromSendEvent)
{
    if (!m_config)
        return;

    QXcbWindow *source = connection()->platformWindowFromId(sourceWindow);
    QXcbWindow *targetWindow = connection()->focusWindow() ? connection()->focusWindow() : source;
    if (!targetWindow || !source)
        return;
    if (type == QEvent::KeyPress)
        targetWindow->updateNetWmUserTime(time);

    QXkbCommon::ScopedXKBState sendEventState;
    if (fromSendEvent) {
        // Have a temporary keyboard state filled in from state
        // this way we allow for synthetic events to have different state
        // from the current state i.e. you can have Alt+Ctrl pressed
        // and receive a synthetic key event that has neither Alt nor Ctrl pressed
        sendEventState.reset(xkb_state_new(m_xkbKeymap.get()));
        if (!sendEventState)
            return;

        xkb_mod_mask_t depressed = xkbModMask(state);
        xkb_state_update_mask(sendEventState.get(), depressed, 0, 0, 0, 0, lockedGroup(state));
    }

    struct xkb_state *xkbState = fromSendEvent ? sendEventState.get() : m_xkbState.get();

    xcb_keysym_t sym = xkb_state_key_get_one_sym(xkbState, code);
    QString text = QXkbCommon::lookupString(xkbState, code);

    Qt::KeyboardModifiers modifiers = translateModifiers(state);
    if (QXkbCommon::isKeypad(sym))
        modifiers |= Qt::KeypadModifier;

    int qtcode = QXkbCommon::keysymToQtKey(sym, modifiers, xkbState, code, m_superAsMeta, m_hyperAsMeta);

    if (type == QEvent::KeyPress) {
        if (m_isAutoRepeat && m_autoRepeatCode != code)
            // Some other key was pressed while we are auto-repeating on a different key.
            m_isAutoRepeat = false;
    } else {
        m_isAutoRepeat = false;
        // Look at the next event in the queue to see if we are auto-repeating.
        connection()->eventQueue()->peek(QXcbEventQueue::PeekRetainMatch,
                                         [this, time, code](xcb_generic_event_t *event, int type) {
            if (type == XCB_KEY_PRESS) {
                auto keyPress = reinterpret_cast<xcb_key_press_event_t *>(event);
                m_isAutoRepeat = keyPress->time == time && keyPress->detail == code;
                if (m_isAutoRepeat)
                    m_autoRepeatCode = code;
            }
            return true;
        });
    }

    bool filtered = false;
    if (auto inputContext = QGuiApplicationPrivate::platformIntegration()->inputContext()) {
        QKeyEvent event(type, qtcode, modifiers, code, sym, state, text, m_isAutoRepeat, text.size());
        event.setTimestamp(time);
        filtered = inputContext->filterEvent(&event);
    }

    if (!filtered) {
        QWindow *window = targetWindow->window();
#ifndef QT_NO_CONTEXTMENU
        if (type == QEvent::KeyPress && qtcode == Qt::Key_Menu) {
            const QPoint globalPos = window->screen()->handle()->cursor()->pos();
            const QPoint pos = window->mapFromGlobal(globalPos);
            QWindowSystemInterface::handleContextMenuEvent(window, false, pos, globalPos, modifiers);
        }
#endif
        QWindowSystemInterface::handleExtendedKeyEvent(window, time, type, qtcode, modifiers,
                                                       code, sym, state, text, m_isAutoRepeat);
    }
}

static bool fromSendEvent(const void *event)
{
    // From X11 protocol: Every event contains an 8-bit type code. The most
    // significant bit in this code is set if the event was generated from
    // a SendEvent request.
    const xcb_generic_event_t *e = reinterpret_cast<const xcb_generic_event_t *>(event);
    return (e->response_type & 0x80) != 0;
}

void QXcbKeyboard::handleKeyPressEvent(const xcb_key_press_event_t *e)
{
    handleKeyEvent(e->event, QEvent::KeyPress, e->detail, e->state, e->time, fromSendEvent(e));
}

void QXcbKeyboard::handleKeyReleaseEvent(const xcb_key_release_event_t *e)
{
    handleKeyEvent(e->event, QEvent::KeyRelease, e->detail, e->state, e->time, fromSendEvent(e));
}

QT_END_NAMESPACE
