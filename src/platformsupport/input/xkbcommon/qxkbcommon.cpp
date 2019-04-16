/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qxkbcommon_p.h"

#include <private/qmakearray_p.h>

#include <QtCore/QMetaMethod>
#include <QtGui/QKeyEvent>
#include <QtGui/private/qguiapplication_p.h>

#include <qpa/qplatforminputcontext.h>
#include <qpa/qplatformintegration.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcXkbcommon, "qt.xkbcommon")

static int keysymToQtKey_internal(xkb_keysym_t keysym, Qt::KeyboardModifiers modifiers,
                                  xkb_state *state, xkb_keycode_t code,
                                  bool superAsMeta, bool hyperAsMeta);

typedef struct xkb2qt
{
    unsigned int xkb;
    unsigned int qt;

    constexpr bool operator <=(const xkb2qt &that) const noexcept
    {
        return xkb <= that.xkb;
    }

    constexpr bool operator <(const xkb2qt &that) const noexcept
    {
        return xkb < that.xkb;
    }
} xkb2qt_t;

template<std::size_t Xkb, std::size_t Qt>
struct Xkb2Qt
{
    using Type = xkb2qt_t;
    static constexpr Type data() noexcept { return Type{Xkb, Qt}; }
};

static constexpr const auto KeyTbl = qMakeArray(
    QSortedData<
        // misc keys

        Xkb2Qt<XKB_KEY_Escape,                  Qt::Key_Escape>,
        Xkb2Qt<XKB_KEY_Tab,                     Qt::Key_Tab>,
        Xkb2Qt<XKB_KEY_ISO_Left_Tab,            Qt::Key_Backtab>,
        Xkb2Qt<XKB_KEY_BackSpace,               Qt::Key_Backspace>,
        Xkb2Qt<XKB_KEY_Return,                  Qt::Key_Return>,
        Xkb2Qt<XKB_KEY_Insert,                  Qt::Key_Insert>,
        Xkb2Qt<XKB_KEY_Delete,                  Qt::Key_Delete>,
        Xkb2Qt<XKB_KEY_Clear,                   Qt::Key_Delete>,
        Xkb2Qt<XKB_KEY_Pause,                   Qt::Key_Pause>,
        Xkb2Qt<XKB_KEY_Print,                   Qt::Key_Print>,
        Xkb2Qt<0x1005FF60,                      Qt::Key_SysReq>,         // hardcoded Sun SysReq
        Xkb2Qt<0x1007ff00,                      Qt::Key_SysReq>,         // hardcoded X386 SysReq

        // cursor movement

        Xkb2Qt<XKB_KEY_Home,                    Qt::Key_Home>,
        Xkb2Qt<XKB_KEY_End,                     Qt::Key_End>,
        Xkb2Qt<XKB_KEY_Left,                    Qt::Key_Left>,
        Xkb2Qt<XKB_KEY_Up,                      Qt::Key_Up>,
        Xkb2Qt<XKB_KEY_Right,                   Qt::Key_Right>,
        Xkb2Qt<XKB_KEY_Down,                    Qt::Key_Down>,
        Xkb2Qt<XKB_KEY_Prior,                   Qt::Key_PageUp>,
        Xkb2Qt<XKB_KEY_Next,                    Qt::Key_PageDown>,

        // modifiers

        Xkb2Qt<XKB_KEY_Shift_L,                 Qt::Key_Shift>,
        Xkb2Qt<XKB_KEY_Shift_R,                 Qt::Key_Shift>,
        Xkb2Qt<XKB_KEY_Shift_Lock,              Qt::Key_Shift>,
        Xkb2Qt<XKB_KEY_Control_L,               Qt::Key_Control>,
        Xkb2Qt<XKB_KEY_Control_R,               Qt::Key_Control>,
        Xkb2Qt<XKB_KEY_Meta_L,                  Qt::Key_Meta>,
        Xkb2Qt<XKB_KEY_Meta_R,                  Qt::Key_Meta>,
        Xkb2Qt<XKB_KEY_Alt_L,                   Qt::Key_Alt>,
        Xkb2Qt<XKB_KEY_Alt_R,                   Qt::Key_Alt>,
        Xkb2Qt<XKB_KEY_Caps_Lock,               Qt::Key_CapsLock>,
        Xkb2Qt<XKB_KEY_Num_Lock,                Qt::Key_NumLock>,
        Xkb2Qt<XKB_KEY_Scroll_Lock,             Qt::Key_ScrollLock>,
        Xkb2Qt<XKB_KEY_Super_L,                 Qt::Key_Super_L>,
        Xkb2Qt<XKB_KEY_Super_R,                 Qt::Key_Super_R>,
        Xkb2Qt<XKB_KEY_Menu,                    Qt::Key_Menu>,
        Xkb2Qt<XKB_KEY_Hyper_L,                 Qt::Key_Hyper_L>,
        Xkb2Qt<XKB_KEY_Hyper_R,                 Qt::Key_Hyper_R>,
        Xkb2Qt<XKB_KEY_Help,                    Qt::Key_Help>,
        Xkb2Qt<0x1000FF74,                      Qt::Key_Backtab>,        // hardcoded HP backtab
        Xkb2Qt<0x1005FF10,                      Qt::Key_F11>,            // hardcoded Sun F36 (labeled F11)
        Xkb2Qt<0x1005FF11,                      Qt::Key_F12>,            // hardcoded Sun F37 (labeled F12)

        // numeric and function keypad keys

        Xkb2Qt<XKB_KEY_KP_Space,                Qt::Key_Space>,
        Xkb2Qt<XKB_KEY_KP_Tab,                  Qt::Key_Tab>,
        Xkb2Qt<XKB_KEY_KP_Enter,                Qt::Key_Enter>,
        Xkb2Qt<XKB_KEY_KP_Home,                 Qt::Key_Home>,
        Xkb2Qt<XKB_KEY_KP_Left,                 Qt::Key_Left>,
        Xkb2Qt<XKB_KEY_KP_Up,                   Qt::Key_Up>,
        Xkb2Qt<XKB_KEY_KP_Right,                Qt::Key_Right>,
        Xkb2Qt<XKB_KEY_KP_Down,                 Qt::Key_Down>,
        Xkb2Qt<XKB_KEY_KP_Prior,                Qt::Key_PageUp>,
        Xkb2Qt<XKB_KEY_KP_Next,                 Qt::Key_PageDown>,
        Xkb2Qt<XKB_KEY_KP_End,                  Qt::Key_End>,
        Xkb2Qt<XKB_KEY_KP_Begin,                Qt::Key_Clear>,
        Xkb2Qt<XKB_KEY_KP_Insert,               Qt::Key_Insert>,
        Xkb2Qt<XKB_KEY_KP_Delete,               Qt::Key_Delete>,
        Xkb2Qt<XKB_KEY_KP_Equal,                Qt::Key_Equal>,
        Xkb2Qt<XKB_KEY_KP_Multiply,             Qt::Key_Asterisk>,
        Xkb2Qt<XKB_KEY_KP_Add,                  Qt::Key_Plus>,
        Xkb2Qt<XKB_KEY_KP_Separator,            Qt::Key_Comma>,
        Xkb2Qt<XKB_KEY_KP_Subtract,             Qt::Key_Minus>,
        Xkb2Qt<XKB_KEY_KP_Decimal,              Qt::Key_Period>,
        Xkb2Qt<XKB_KEY_KP_Divide,               Qt::Key_Slash>,

        // special non-XF86 function keys

        Xkb2Qt<XKB_KEY_Undo,                    Qt::Key_Undo>,
        Xkb2Qt<XKB_KEY_Redo,                    Qt::Key_Redo>,
        Xkb2Qt<XKB_KEY_Find,                    Qt::Key_Find>,
        Xkb2Qt<XKB_KEY_Cancel,                  Qt::Key_Cancel>,

        // International input method support keys

        // International & multi-key character composition
        Xkb2Qt<XKB_KEY_ISO_Level3_Shift,        Qt::Key_AltGr>,
        Xkb2Qt<XKB_KEY_Multi_key,               Qt::Key_Multi_key>,
        Xkb2Qt<XKB_KEY_Codeinput,               Qt::Key_Codeinput>,
        Xkb2Qt<XKB_KEY_SingleCandidate,         Qt::Key_SingleCandidate>,
        Xkb2Qt<XKB_KEY_MultipleCandidate,       Qt::Key_MultipleCandidate>,
        Xkb2Qt<XKB_KEY_PreviousCandidate,       Qt::Key_PreviousCandidate>,

        // Misc Functions
        Xkb2Qt<XKB_KEY_Mode_switch,             Qt::Key_Mode_switch>,
        Xkb2Qt<XKB_KEY_script_switch,           Qt::Key_Mode_switch>,

        // Japanese keyboard support
        Xkb2Qt<XKB_KEY_Kanji,                   Qt::Key_Kanji>,
        Xkb2Qt<XKB_KEY_Muhenkan,                Qt::Key_Muhenkan>,
        //Xkb2Qt<XKB_KEY_Henkan_Mode,           Qt::Key_Henkan_Mode>,
        Xkb2Qt<XKB_KEY_Henkan_Mode,             Qt::Key_Henkan>,
        Xkb2Qt<XKB_KEY_Henkan,                  Qt::Key_Henkan>,
        Xkb2Qt<XKB_KEY_Romaji,                  Qt::Key_Romaji>,
        Xkb2Qt<XKB_KEY_Hiragana,                Qt::Key_Hiragana>,
        Xkb2Qt<XKB_KEY_Katakana,                Qt::Key_Katakana>,
        Xkb2Qt<XKB_KEY_Hiragana_Katakana,       Qt::Key_Hiragana_Katakana>,
        Xkb2Qt<XKB_KEY_Zenkaku,                 Qt::Key_Zenkaku>,
        Xkb2Qt<XKB_KEY_Hankaku,                 Qt::Key_Hankaku>,
        Xkb2Qt<XKB_KEY_Zenkaku_Hankaku,         Qt::Key_Zenkaku_Hankaku>,
        Xkb2Qt<XKB_KEY_Touroku,                 Qt::Key_Touroku>,
        Xkb2Qt<XKB_KEY_Massyo,                  Qt::Key_Massyo>,
        Xkb2Qt<XKB_KEY_Kana_Lock,               Qt::Key_Kana_Lock>,
        Xkb2Qt<XKB_KEY_Kana_Shift,              Qt::Key_Kana_Shift>,
        Xkb2Qt<XKB_KEY_Eisu_Shift,              Qt::Key_Eisu_Shift>,
        Xkb2Qt<XKB_KEY_Eisu_toggle,             Qt::Key_Eisu_toggle>,
        //Xkb2Qt<XKB_KEY_Kanji_Bangou,          Qt::Key_Kanji_Bangou>,
        //Xkb2Qt<XKB_KEY_Zen_Koho,              Qt::Key_Zen_Koho>,
        //Xkb2Qt<XKB_KEY_Mae_Koho,              Qt::Key_Mae_Koho>,
        Xkb2Qt<XKB_KEY_Kanji_Bangou,            Qt::Key_Codeinput>,
        Xkb2Qt<XKB_KEY_Zen_Koho,                Qt::Key_MultipleCandidate>,
        Xkb2Qt<XKB_KEY_Mae_Koho,                Qt::Key_PreviousCandidate>,

        // Korean keyboard support
        Xkb2Qt<XKB_KEY_Hangul,                  Qt::Key_Hangul>,
        Xkb2Qt<XKB_KEY_Hangul_Start,            Qt::Key_Hangul_Start>,
        Xkb2Qt<XKB_KEY_Hangul_End,              Qt::Key_Hangul_End>,
        Xkb2Qt<XKB_KEY_Hangul_Hanja,            Qt::Key_Hangul_Hanja>,
        Xkb2Qt<XKB_KEY_Hangul_Jamo,             Qt::Key_Hangul_Jamo>,
        Xkb2Qt<XKB_KEY_Hangul_Romaja,           Qt::Key_Hangul_Romaja>,
        //Xkb2Qt<XKB_KEY_Hangul_Codeinput,      Qt::Key_Hangul_Codeinput>,
        Xkb2Qt<XKB_KEY_Hangul_Codeinput,        Qt::Key_Codeinput>,
        Xkb2Qt<XKB_KEY_Hangul_Jeonja,           Qt::Key_Hangul_Jeonja>,
        Xkb2Qt<XKB_KEY_Hangul_Banja,            Qt::Key_Hangul_Banja>,
        Xkb2Qt<XKB_KEY_Hangul_PreHanja,         Qt::Key_Hangul_PreHanja>,
        Xkb2Qt<XKB_KEY_Hangul_PostHanja,        Qt::Key_Hangul_PostHanja>,
        //Xkb2Qt<XKB_KEY_Hangul_SingleCandidate,Qt::Key_Hangul_SingleCandidate>,
        //Xkb2Qt<XKB_KEY_Hangul_MultipleCandidate,Qt::Key_Hangul_MultipleCandidate>,
        //Xkb2Qt<XKB_KEY_Hangul_PreviousCandidate,Qt::Key_Hangul_PreviousCandidate>,
        Xkb2Qt<XKB_KEY_Hangul_SingleCandidate,  Qt::Key_SingleCandidate>,
        Xkb2Qt<XKB_KEY_Hangul_MultipleCandidate,Qt::Key_MultipleCandidate>,
        Xkb2Qt<XKB_KEY_Hangul_PreviousCandidate,Qt::Key_PreviousCandidate>,
        Xkb2Qt<XKB_KEY_Hangul_Special,          Qt::Key_Hangul_Special>,
        //Xkb2Qt<XKB_KEY_Hangul_switch,         Qt::Key_Hangul_switch>,
        Xkb2Qt<XKB_KEY_Hangul_switch,           Qt::Key_Mode_switch>,

        // dead keys
        Xkb2Qt<XKB_KEY_dead_grave,              Qt::Key_Dead_Grave>,
        Xkb2Qt<XKB_KEY_dead_acute,              Qt::Key_Dead_Acute>,
        Xkb2Qt<XKB_KEY_dead_circumflex,         Qt::Key_Dead_Circumflex>,
        Xkb2Qt<XKB_KEY_dead_tilde,              Qt::Key_Dead_Tilde>,
        Xkb2Qt<XKB_KEY_dead_macron,             Qt::Key_Dead_Macron>,
        Xkb2Qt<XKB_KEY_dead_breve,              Qt::Key_Dead_Breve>,
        Xkb2Qt<XKB_KEY_dead_abovedot,           Qt::Key_Dead_Abovedot>,
        Xkb2Qt<XKB_KEY_dead_diaeresis,          Qt::Key_Dead_Diaeresis>,
        Xkb2Qt<XKB_KEY_dead_abovering,          Qt::Key_Dead_Abovering>,
        Xkb2Qt<XKB_KEY_dead_doubleacute,        Qt::Key_Dead_Doubleacute>,
        Xkb2Qt<XKB_KEY_dead_caron,              Qt::Key_Dead_Caron>,
        Xkb2Qt<XKB_KEY_dead_cedilla,            Qt::Key_Dead_Cedilla>,
        Xkb2Qt<XKB_KEY_dead_ogonek,             Qt::Key_Dead_Ogonek>,
        Xkb2Qt<XKB_KEY_dead_iota,               Qt::Key_Dead_Iota>,
        Xkb2Qt<XKB_KEY_dead_voiced_sound,       Qt::Key_Dead_Voiced_Sound>,
        Xkb2Qt<XKB_KEY_dead_semivoiced_sound,   Qt::Key_Dead_Semivoiced_Sound>,
        Xkb2Qt<XKB_KEY_dead_belowdot,           Qt::Key_Dead_Belowdot>,
        Xkb2Qt<XKB_KEY_dead_hook,               Qt::Key_Dead_Hook>,
        Xkb2Qt<XKB_KEY_dead_horn,               Qt::Key_Dead_Horn>,
        Xkb2Qt<XKB_KEY_dead_stroke,             Qt::Key_Dead_Stroke>,
        Xkb2Qt<XKB_KEY_dead_abovecomma,         Qt::Key_Dead_Abovecomma>,
        Xkb2Qt<XKB_KEY_dead_abovereversedcomma, Qt::Key_Dead_Abovereversedcomma>,
        Xkb2Qt<XKB_KEY_dead_doublegrave,        Qt::Key_Dead_Doublegrave>,
        Xkb2Qt<XKB_KEY_dead_belowring,          Qt::Key_Dead_Belowring>,
        Xkb2Qt<XKB_KEY_dead_belowmacron,        Qt::Key_Dead_Belowmacron>,
        Xkb2Qt<XKB_KEY_dead_belowcircumflex,    Qt::Key_Dead_Belowcircumflex>,
        Xkb2Qt<XKB_KEY_dead_belowtilde,         Qt::Key_Dead_Belowtilde>,
        Xkb2Qt<XKB_KEY_dead_belowbreve,         Qt::Key_Dead_Belowbreve>,
        Xkb2Qt<XKB_KEY_dead_belowdiaeresis,     Qt::Key_Dead_Belowdiaeresis>,
        Xkb2Qt<XKB_KEY_dead_invertedbreve,      Qt::Key_Dead_Invertedbreve>,
        Xkb2Qt<XKB_KEY_dead_belowcomma,         Qt::Key_Dead_Belowcomma>,
        Xkb2Qt<XKB_KEY_dead_currency,           Qt::Key_Dead_Currency>,
        Xkb2Qt<XKB_KEY_dead_a,                  Qt::Key_Dead_a>,
        Xkb2Qt<XKB_KEY_dead_A,                  Qt::Key_Dead_A>,
        Xkb2Qt<XKB_KEY_dead_e,                  Qt::Key_Dead_e>,
        Xkb2Qt<XKB_KEY_dead_E,                  Qt::Key_Dead_E>,
        Xkb2Qt<XKB_KEY_dead_i,                  Qt::Key_Dead_i>,
        Xkb2Qt<XKB_KEY_dead_I,                  Qt::Key_Dead_I>,
        Xkb2Qt<XKB_KEY_dead_o,                  Qt::Key_Dead_o>,
        Xkb2Qt<XKB_KEY_dead_O,                  Qt::Key_Dead_O>,
        Xkb2Qt<XKB_KEY_dead_u,                  Qt::Key_Dead_u>,
        Xkb2Qt<XKB_KEY_dead_U,                  Qt::Key_Dead_U>,
        Xkb2Qt<XKB_KEY_dead_small_schwa,        Qt::Key_Dead_Small_Schwa>,
        Xkb2Qt<XKB_KEY_dead_capital_schwa,      Qt::Key_Dead_Capital_Schwa>,
        Xkb2Qt<XKB_KEY_dead_greek,              Qt::Key_Dead_Greek>,
        Xkb2Qt<XKB_KEY_dead_lowline,            Qt::Key_Dead_Lowline>,
        Xkb2Qt<XKB_KEY_dead_aboveverticalline,  Qt::Key_Dead_Aboveverticalline>,
        Xkb2Qt<XKB_KEY_dead_belowverticalline,  Qt::Key_Dead_Belowverticalline>,
        Xkb2Qt<XKB_KEY_dead_longsolidusoverlay, Qt::Key_Dead_Longsolidusoverlay>,

        // Special keys from X.org - This include multimedia keys,
        // wireless/bluetooth/uwb keys, special launcher keys, etc.
        Xkb2Qt<XKB_KEY_XF86Back,                Qt::Key_Back>,
        Xkb2Qt<XKB_KEY_XF86Forward,             Qt::Key_Forward>,
        Xkb2Qt<XKB_KEY_XF86Stop,                Qt::Key_Stop>,
        Xkb2Qt<XKB_KEY_XF86Refresh,             Qt::Key_Refresh>,
        Xkb2Qt<XKB_KEY_XF86Favorites,           Qt::Key_Favorites>,
        Xkb2Qt<XKB_KEY_XF86AudioMedia,          Qt::Key_LaunchMedia>,
        Xkb2Qt<XKB_KEY_XF86OpenURL,             Qt::Key_OpenUrl>,
        Xkb2Qt<XKB_KEY_XF86HomePage,            Qt::Key_HomePage>,
        Xkb2Qt<XKB_KEY_XF86Search,              Qt::Key_Search>,
        Xkb2Qt<XKB_KEY_XF86AudioLowerVolume,    Qt::Key_VolumeDown>,
        Xkb2Qt<XKB_KEY_XF86AudioMute,           Qt::Key_VolumeMute>,
        Xkb2Qt<XKB_KEY_XF86AudioRaiseVolume,    Qt::Key_VolumeUp>,
        Xkb2Qt<XKB_KEY_XF86AudioPlay,           Qt::Key_MediaPlay>,
        Xkb2Qt<XKB_KEY_XF86AudioStop,           Qt::Key_MediaStop>,
        Xkb2Qt<XKB_KEY_XF86AudioPrev,           Qt::Key_MediaPrevious>,
        Xkb2Qt<XKB_KEY_XF86AudioNext,           Qt::Key_MediaNext>,
        Xkb2Qt<XKB_KEY_XF86AudioRecord,         Qt::Key_MediaRecord>,
        Xkb2Qt<XKB_KEY_XF86AudioPause,          Qt::Key_MediaPause>,
        Xkb2Qt<XKB_KEY_XF86Mail,                Qt::Key_LaunchMail>,
        Xkb2Qt<XKB_KEY_XF86MyComputer,          Qt::Key_Launch0>,  // ### Qt 6: remap properly
        Xkb2Qt<XKB_KEY_XF86Calculator,          Qt::Key_Launch1>,
        Xkb2Qt<XKB_KEY_XF86Memo,                Qt::Key_Memo>,
        Xkb2Qt<XKB_KEY_XF86ToDoList,            Qt::Key_ToDoList>,
        Xkb2Qt<XKB_KEY_XF86Calendar,            Qt::Key_Calendar>,
        Xkb2Qt<XKB_KEY_XF86PowerDown,           Qt::Key_PowerDown>,
        Xkb2Qt<XKB_KEY_XF86ContrastAdjust,      Qt::Key_ContrastAdjust>,
        Xkb2Qt<XKB_KEY_XF86Standby,             Qt::Key_Standby>,
        Xkb2Qt<XKB_KEY_XF86MonBrightnessUp,     Qt::Key_MonBrightnessUp>,
        Xkb2Qt<XKB_KEY_XF86MonBrightnessDown,   Qt::Key_MonBrightnessDown>,
        Xkb2Qt<XKB_KEY_XF86KbdLightOnOff,       Qt::Key_KeyboardLightOnOff>,
        Xkb2Qt<XKB_KEY_XF86KbdBrightnessUp,     Qt::Key_KeyboardBrightnessUp>,
        Xkb2Qt<XKB_KEY_XF86KbdBrightnessDown,   Qt::Key_KeyboardBrightnessDown>,
        Xkb2Qt<XKB_KEY_XF86PowerOff,            Qt::Key_PowerOff>,
        Xkb2Qt<XKB_KEY_XF86WakeUp,              Qt::Key_WakeUp>,
        Xkb2Qt<XKB_KEY_XF86Eject,               Qt::Key_Eject>,
        Xkb2Qt<XKB_KEY_XF86ScreenSaver,         Qt::Key_ScreenSaver>,
        Xkb2Qt<XKB_KEY_XF86WWW,                 Qt::Key_WWW>,
        Xkb2Qt<XKB_KEY_XF86Sleep,               Qt::Key_Sleep>,
        Xkb2Qt<XKB_KEY_XF86LightBulb,           Qt::Key_LightBulb>,
        Xkb2Qt<XKB_KEY_XF86Shop,                Qt::Key_Shop>,
        Xkb2Qt<XKB_KEY_XF86History,             Qt::Key_History>,
        Xkb2Qt<XKB_KEY_XF86AddFavorite,         Qt::Key_AddFavorite>,
        Xkb2Qt<XKB_KEY_XF86HotLinks,            Qt::Key_HotLinks>,
        Xkb2Qt<XKB_KEY_XF86BrightnessAdjust,    Qt::Key_BrightnessAdjust>,
        Xkb2Qt<XKB_KEY_XF86Finance,             Qt::Key_Finance>,
        Xkb2Qt<XKB_KEY_XF86Community,           Qt::Key_Community>,
        Xkb2Qt<XKB_KEY_XF86AudioRewind,         Qt::Key_AudioRewind>,
        Xkb2Qt<XKB_KEY_XF86BackForward,         Qt::Key_BackForward>,
        Xkb2Qt<XKB_KEY_XF86ApplicationLeft,     Qt::Key_ApplicationLeft>,
        Xkb2Qt<XKB_KEY_XF86ApplicationRight,    Qt::Key_ApplicationRight>,
        Xkb2Qt<XKB_KEY_XF86Book,                Qt::Key_Book>,
        Xkb2Qt<XKB_KEY_XF86CD,                  Qt::Key_CD>,
        Xkb2Qt<XKB_KEY_XF86Calculater,          Qt::Key_Calculator>,
        Xkb2Qt<XKB_KEY_XF86Clear,               Qt::Key_Clear>,
        Xkb2Qt<XKB_KEY_XF86ClearGrab,           Qt::Key_ClearGrab>,
        Xkb2Qt<XKB_KEY_XF86Close,               Qt::Key_Close>,
        Xkb2Qt<XKB_KEY_XF86Copy,                Qt::Key_Copy>,
        Xkb2Qt<XKB_KEY_XF86Cut,                 Qt::Key_Cut>,
        Xkb2Qt<XKB_KEY_XF86Display,             Qt::Key_Display>,
        Xkb2Qt<XKB_KEY_XF86DOS,                 Qt::Key_DOS>,
        Xkb2Qt<XKB_KEY_XF86Documents,           Qt::Key_Documents>,
        Xkb2Qt<XKB_KEY_XF86Excel,               Qt::Key_Excel>,
        Xkb2Qt<XKB_KEY_XF86Explorer,            Qt::Key_Explorer>,
        Xkb2Qt<XKB_KEY_XF86Game,                Qt::Key_Game>,
        Xkb2Qt<XKB_KEY_XF86Go,                  Qt::Key_Go>,
        Xkb2Qt<XKB_KEY_XF86iTouch,              Qt::Key_iTouch>,
        Xkb2Qt<XKB_KEY_XF86LogOff,              Qt::Key_LogOff>,
        Xkb2Qt<XKB_KEY_XF86Market,              Qt::Key_Market>,
        Xkb2Qt<XKB_KEY_XF86Meeting,             Qt::Key_Meeting>,
        Xkb2Qt<XKB_KEY_XF86MenuKB,              Qt::Key_MenuKB>,
        Xkb2Qt<XKB_KEY_XF86MenuPB,              Qt::Key_MenuPB>,
        Xkb2Qt<XKB_KEY_XF86MySites,             Qt::Key_MySites>,
        Xkb2Qt<XKB_KEY_XF86New,                 Qt::Key_New>,
        Xkb2Qt<XKB_KEY_XF86News,                Qt::Key_News>,
        Xkb2Qt<XKB_KEY_XF86OfficeHome,          Qt::Key_OfficeHome>,
        Xkb2Qt<XKB_KEY_XF86Open,                Qt::Key_Open>,
        Xkb2Qt<XKB_KEY_XF86Option,              Qt::Key_Option>,
        Xkb2Qt<XKB_KEY_XF86Paste,               Qt::Key_Paste>,
        Xkb2Qt<XKB_KEY_XF86Phone,               Qt::Key_Phone>,
        Xkb2Qt<XKB_KEY_XF86Reply,               Qt::Key_Reply>,
        Xkb2Qt<XKB_KEY_XF86Reload,              Qt::Key_Reload>,
        Xkb2Qt<XKB_KEY_XF86RotateWindows,       Qt::Key_RotateWindows>,
        Xkb2Qt<XKB_KEY_XF86RotationPB,          Qt::Key_RotationPB>,
        Xkb2Qt<XKB_KEY_XF86RotationKB,          Qt::Key_RotationKB>,
        Xkb2Qt<XKB_KEY_XF86Save,                Qt::Key_Save>,
        Xkb2Qt<XKB_KEY_XF86Send,                Qt::Key_Send>,
        Xkb2Qt<XKB_KEY_XF86Spell,               Qt::Key_Spell>,
        Xkb2Qt<XKB_KEY_XF86SplitScreen,         Qt::Key_SplitScreen>,
        Xkb2Qt<XKB_KEY_XF86Support,             Qt::Key_Support>,
        Xkb2Qt<XKB_KEY_XF86TaskPane,            Qt::Key_TaskPane>,
        Xkb2Qt<XKB_KEY_XF86Terminal,            Qt::Key_Terminal>,
        Xkb2Qt<XKB_KEY_XF86Tools,               Qt::Key_Tools>,
        Xkb2Qt<XKB_KEY_XF86Travel,              Qt::Key_Travel>,
        Xkb2Qt<XKB_KEY_XF86Video,               Qt::Key_Video>,
        Xkb2Qt<XKB_KEY_XF86Word,                Qt::Key_Word>,
        Xkb2Qt<XKB_KEY_XF86Xfer,                Qt::Key_Xfer>,
        Xkb2Qt<XKB_KEY_XF86ZoomIn,              Qt::Key_ZoomIn>,
        Xkb2Qt<XKB_KEY_XF86ZoomOut,             Qt::Key_ZoomOut>,
        Xkb2Qt<XKB_KEY_XF86Away,                Qt::Key_Away>,
        Xkb2Qt<XKB_KEY_XF86Messenger,           Qt::Key_Messenger>,
        Xkb2Qt<XKB_KEY_XF86WebCam,              Qt::Key_WebCam>,
        Xkb2Qt<XKB_KEY_XF86MailForward,         Qt::Key_MailForward>,
        Xkb2Qt<XKB_KEY_XF86Pictures,            Qt::Key_Pictures>,
        Xkb2Qt<XKB_KEY_XF86Music,               Qt::Key_Music>,
        Xkb2Qt<XKB_KEY_XF86Battery,             Qt::Key_Battery>,
        Xkb2Qt<XKB_KEY_XF86Bluetooth,           Qt::Key_Bluetooth>,
        Xkb2Qt<XKB_KEY_XF86WLAN,                Qt::Key_WLAN>,
        Xkb2Qt<XKB_KEY_XF86UWB,                 Qt::Key_UWB>,
        Xkb2Qt<XKB_KEY_XF86AudioForward,        Qt::Key_AudioForward>,
        Xkb2Qt<XKB_KEY_XF86AudioRepeat,         Qt::Key_AudioRepeat>,
        Xkb2Qt<XKB_KEY_XF86AudioRandomPlay,     Qt::Key_AudioRandomPlay>,
        Xkb2Qt<XKB_KEY_XF86Subtitle,            Qt::Key_Subtitle>,
        Xkb2Qt<XKB_KEY_XF86AudioCycleTrack,     Qt::Key_AudioCycleTrack>,
        Xkb2Qt<XKB_KEY_XF86Time,                Qt::Key_Time>,
        Xkb2Qt<XKB_KEY_XF86Select,              Qt::Key_Select>,
        Xkb2Qt<XKB_KEY_XF86View,                Qt::Key_View>,
        Xkb2Qt<XKB_KEY_XF86TopMenu,             Qt::Key_TopMenu>,
        Xkb2Qt<XKB_KEY_XF86Red,                 Qt::Key_Red>,
        Xkb2Qt<XKB_KEY_XF86Green,               Qt::Key_Green>,
        Xkb2Qt<XKB_KEY_XF86Yellow,              Qt::Key_Yellow>,
        Xkb2Qt<XKB_KEY_XF86Blue,                Qt::Key_Blue>,
        Xkb2Qt<XKB_KEY_XF86Bluetooth,           Qt::Key_Bluetooth>,
        Xkb2Qt<XKB_KEY_XF86Suspend,             Qt::Key_Suspend>,
        Xkb2Qt<XKB_KEY_XF86Hibernate,           Qt::Key_Hibernate>,
        Xkb2Qt<XKB_KEY_XF86TouchpadToggle,      Qt::Key_TouchpadToggle>,
        Xkb2Qt<XKB_KEY_XF86TouchpadOn,          Qt::Key_TouchpadOn>,
        Xkb2Qt<XKB_KEY_XF86TouchpadOff,         Qt::Key_TouchpadOff>,
        Xkb2Qt<XKB_KEY_XF86AudioMicMute,        Qt::Key_MicMute>,
        Xkb2Qt<XKB_KEY_XF86Launch0,             Qt::Key_Launch2>, // ### Qt 6: remap properly
        Xkb2Qt<XKB_KEY_XF86Launch1,             Qt::Key_Launch3>,
        Xkb2Qt<XKB_KEY_XF86Launch2,             Qt::Key_Launch4>,
        Xkb2Qt<XKB_KEY_XF86Launch3,             Qt::Key_Launch5>,
        Xkb2Qt<XKB_KEY_XF86Launch4,             Qt::Key_Launch6>,
        Xkb2Qt<XKB_KEY_XF86Launch5,             Qt::Key_Launch7>,
        Xkb2Qt<XKB_KEY_XF86Launch6,             Qt::Key_Launch8>,
        Xkb2Qt<XKB_KEY_XF86Launch7,             Qt::Key_Launch9>,
        Xkb2Qt<XKB_KEY_XF86Launch8,             Qt::Key_LaunchA>,
        Xkb2Qt<XKB_KEY_XF86Launch9,             Qt::Key_LaunchB>,
        Xkb2Qt<XKB_KEY_XF86LaunchA,             Qt::Key_LaunchC>,
        Xkb2Qt<XKB_KEY_XF86LaunchB,             Qt::Key_LaunchD>,
        Xkb2Qt<XKB_KEY_XF86LaunchC,             Qt::Key_LaunchE>,
        Xkb2Qt<XKB_KEY_XF86LaunchD,             Qt::Key_LaunchF>,
        Xkb2Qt<XKB_KEY_XF86LaunchE,             Qt::Key_LaunchG>,
        Xkb2Qt<XKB_KEY_XF86LaunchF,             Qt::Key_LaunchH>
    >::Data{}
);

xkb_keysym_t QXkbCommon::qxkbcommon_xkb_keysym_to_upper(xkb_keysym_t ks)
{
    xkb_keysym_t lower, upper;

    xkbcommon_XConvertCase(ks, &lower, &upper);

    return upper;
}

QString QXkbCommon::lookupString(struct xkb_state *state, xkb_keycode_t code)
{
    QVarLengthArray<char, 32> chars(32);
    const int size = xkb_state_key_get_utf8(state, code, chars.data(), chars.size());
    if (Q_UNLIKELY(size + 1 > chars.size())) { // +1 for NUL
        chars.resize(size + 1);
        xkb_state_key_get_utf8(state, code, chars.data(), chars.size());
    }
    return QString::fromUtf8(chars.constData(), size);
}

QString QXkbCommon::lookupStringNoKeysymTransformations(xkb_keysym_t keysym)
{
    QVarLengthArray<char, 32> chars(32);
    const int size = xkb_keysym_to_utf8(keysym, chars.data(), chars.size());
    if (size == 0)
        return QString(); // the keysym does not have a Unicode representation

    if (Q_UNLIKELY(size > chars.size())) {
        chars.resize(size);
        xkb_keysym_to_utf8(keysym, chars.data(), chars.size());
    }
    return QString::fromUtf8(chars.constData(), size - 1);
}

QVector<xkb_keysym_t> QXkbCommon::toKeysym(QKeyEvent *event)
{
    QVector<xkb_keysym_t> keysyms;
    int qtKey = event->key();

    if (qtKey >= Qt::Key_F1 && qtKey <= Qt::Key_F35) {
        keysyms.append(XKB_KEY_F1 + (qtKey - Qt::Key_F1));
    } else if (event->modifiers() & Qt::KeypadModifier) {
        if (qtKey >= Qt::Key_0 && qtKey <= Qt::Key_9)
            keysyms.append(XKB_KEY_KP_0 + (qtKey - Qt::Key_0));
    } else if (isLatin(qtKey) && event->text().isUpper()) {
        keysyms.append(qtKey);
    }

    if (!keysyms.isEmpty())
        return keysyms;

    // check if we have a direct mapping
    auto it = std::find_if(KeyTbl.cbegin(), KeyTbl.cend(), [&qtKey](xkb2qt_t elem) {
        return elem.qt == static_cast<uint>(qtKey);
    });
    if (it != KeyTbl.end()) {
        keysyms.append(it->xkb);
        return keysyms;
    }

    QVector<uint> ucs4;
    if (event->text().isEmpty())
        ucs4.append(qtKey);
    else
        ucs4 = event->text().toUcs4();

    // From libxkbcommon keysym-utf.c:
    // "We allow to represent any UCS character in the range U-00000000 to
    // U-00FFFFFF by a keysym value in the range 0x01000000 to 0x01ffffff."
    for (uint utf32 : qAsConst(ucs4))
        keysyms.append(utf32 | 0x01000000);

    return keysyms;
}

int QXkbCommon::keysymToQtKey(xkb_keysym_t keysym, Qt::KeyboardModifiers modifiers)
{
    return keysymToQtKey(keysym, modifiers, nullptr, 0);
}

int QXkbCommon::keysymToQtKey(xkb_keysym_t keysym, Qt::KeyboardModifiers modifiers,
                              xkb_state *state, xkb_keycode_t code,
                              bool superAsMeta, bool hyperAsMeta)
{
    // Note 1: All standard key sequences on linux (as defined in platform theme)
    // that use a latin character also contain a control modifier, which is why
    // checking for Qt::ControlModifier is sufficient here. It is possible to
    // override QPlatformTheme::keyBindings() and provide custom sequences for
    // QKeySequence::StandardKey. Custom sequences probably should respect this
    // convention (alternatively, we could test against other modifiers here).
    // Note 2: The possibleKeys() shorcut mechanism is not affected by this value
    // adjustment and does its own thing.
    if (modifiers & Qt::ControlModifier) {
        // With standard shortcuts we should prefer a latin character, this is
        // for checks like "some qkeyevent == QKeySequence::Copy" to work even
        // when using for example 'russian' keyboard layout.
        if (!QXkbCommon::isLatin(keysym)) {
            xkb_keysym_t latinKeysym = QXkbCommon::lookupLatinKeysym(state, code);
            if (latinKeysym != XKB_KEY_NoSymbol)
                keysym = latinKeysym;
        }
    }

    return keysymToQtKey_internal(keysym, modifiers, state, code, superAsMeta, hyperAsMeta);
}

static int keysymToQtKey_internal(xkb_keysym_t keysym, Qt::KeyboardModifiers modifiers,
                                  xkb_state *state, xkb_keycode_t code,
                                  bool superAsMeta, bool hyperAsMeta)
{
    int qtKey = 0;

    // lookup from direct mapping
    if (keysym >= XKB_KEY_F1 && keysym <= XKB_KEY_F35) {
        // function keys
        qtKey = Qt::Key_F1 + (keysym - XKB_KEY_F1);
    } else if (keysym >= XKB_KEY_KP_0 && keysym <= XKB_KEY_KP_9) {
        // numeric keypad keys
        qtKey = Qt::Key_0 + (keysym - XKB_KEY_KP_0);
    } else if (QXkbCommon::isLatin(keysym)) {
        qtKey = QXkbCommon::qxkbcommon_xkb_keysym_to_upper(keysym);
    } else {
        // check if we have a direct mapping
        xkb2qt_t searchKey{keysym, 0};
        auto it = std::lower_bound(KeyTbl.cbegin(), KeyTbl.cend(), searchKey);
        if (it != KeyTbl.end() && !(searchKey < *it))
            qtKey = it->qt;
    }

    if (qtKey)
        return qtKey;

    // lookup from unicode
    QString text;
    if (!state || modifiers & Qt::ControlModifier) {
        // Control modifier changes the text to ASCII control character, therefore we
        // can't use this text to map keysym to a qt key. We can use the same keysym
        // (it is not affectd by transformation) to obtain untransformed text. For details
        // see "Appendix A. Default Symbol Transformations" in the XKB specification.
        text = QXkbCommon::lookupStringNoKeysymTransformations(keysym);
    } else {
        text = QXkbCommon::lookupString(state, code);
    }
    if (!text.isEmpty()) {
         if (text.unicode()->isDigit()) {
             // Ensures that also non-latin digits are mapped to corresponding qt keys,
             // e.g CTRL + ۲ (arabic two), is mapped to CTRL + Qt::Key_2.
             qtKey = Qt::Key_0 + text.unicode()->digitValue();
         } else {
             qtKey = text.unicode()->toUpper().unicode();
         }
    }

    // translate Super/Hyper keys to Meta if we're using them as the MetaModifier
    if (superAsMeta && (qtKey == Qt::Key_Super_L || qtKey == Qt::Key_Super_R))
        qtKey = Qt::Key_Meta;
    if (hyperAsMeta && (qtKey == Qt::Key_Hyper_L || qtKey == Qt::Key_Hyper_R))
        qtKey = Qt::Key_Meta;

    return qtKey;
}

Qt::KeyboardModifiers QXkbCommon::modifiers(struct xkb_state *state)
{
    Qt::KeyboardModifiers modifiers = Qt::NoModifier;

    if (xkb_state_mod_name_is_active(state, XKB_MOD_NAME_CTRL, XKB_STATE_MODS_EFFECTIVE) > 0)
        modifiers |= Qt::ControlModifier;
    if (xkb_state_mod_name_is_active(state, XKB_MOD_NAME_ALT, XKB_STATE_MODS_EFFECTIVE) > 0)
        modifiers |= Qt::AltModifier;
    if (xkb_state_mod_name_is_active(state, XKB_MOD_NAME_SHIFT, XKB_STATE_MODS_EFFECTIVE) > 0)
        modifiers |= Qt::ShiftModifier;
    if (xkb_state_mod_name_is_active(state, XKB_MOD_NAME_LOGO, XKB_STATE_MODS_EFFECTIVE) > 0)
        modifiers |= Qt::MetaModifier;

    return modifiers;
}

// Possible modifier states.
static const Qt::KeyboardModifiers ModsTbl[] = {
    Qt::NoModifier,                                             // 0
    Qt::ShiftModifier,                                          // 1
    Qt::ControlModifier,                                        // 2
    Qt::ControlModifier | Qt::ShiftModifier,                    // 3
    Qt::AltModifier,                                            // 4
    Qt::AltModifier | Qt::ShiftModifier,                        // 5
    Qt::AltModifier | Qt::ControlModifier,                      // 6
    Qt::AltModifier | Qt::ShiftModifier | Qt::ControlModifier,  // 7
    Qt::NoModifier                                              // Fall-back to raw Key_*, for non-latin1 kb layouts
};

QList<int> QXkbCommon::possibleKeys(xkb_state *state, const QKeyEvent *event,
                                    bool superAsMeta, bool hyperAsMeta)
{
    QList<int> result;
    quint32 keycode = event->nativeScanCode();
    Qt::KeyboardModifiers modifiers = event->modifiers();
    xkb_keymap *keymap = xkb_state_get_keymap(state);
    // turn off the modifier bits which doesn't participate in shortcuts
    Qt::KeyboardModifiers notNeeded = Qt::KeypadModifier | Qt::GroupSwitchModifier;
    modifiers &= ~notNeeded;
    // create a fresh kb state and test against the relevant modifier combinations
    ScopedXKBState scopedXkbQueryState(xkb_state_new(keymap));
    xkb_state *queryState = scopedXkbQueryState.get();
    if (!queryState) {
        qCWarning(lcXkbcommon) << Q_FUNC_INFO << "failed to compile xkb keymap";
        return result;
    }
    // get kb state from the master state and update the temporary state
    xkb_layout_index_t lockedLayout = xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_LOCKED);
    xkb_mod_mask_t latchedMods = xkb_state_serialize_mods(state, XKB_STATE_MODS_LATCHED);
    xkb_mod_mask_t lockedMods = xkb_state_serialize_mods(state, XKB_STATE_MODS_LOCKED);
    xkb_mod_mask_t depressedMods = xkb_state_serialize_mods(state, XKB_STATE_MODS_DEPRESSED);
    xkb_state_update_mask(queryState, depressedMods, latchedMods, lockedMods, 0, 0, lockedLayout);
    // handle shortcuts for level three and above
    xkb_layout_index_t layoutIndex = xkb_state_key_get_layout(queryState, keycode);
    xkb_level_index_t levelIndex = 0;
    if (layoutIndex != XKB_LAYOUT_INVALID) {
        levelIndex = xkb_state_key_get_level(queryState, keycode, layoutIndex);
        if (levelIndex == XKB_LEVEL_INVALID)
            levelIndex = 0;
    }
    if (levelIndex <= 1)
        xkb_state_update_mask(queryState, 0, latchedMods, lockedMods, 0, 0, lockedLayout);

    xkb_keysym_t sym = xkb_state_key_get_one_sym(queryState, keycode);
    if (sym == XKB_KEY_NoSymbol)
        return result;

    int baseQtKey = keysymToQtKey_internal(sym, modifiers, queryState, keycode, superAsMeta, hyperAsMeta);
    if (baseQtKey)
        result += (baseQtKey + modifiers);

    xkb_mod_index_t shiftMod = xkb_keymap_mod_get_index(keymap, "Shift");
    xkb_mod_index_t altMod = xkb_keymap_mod_get_index(keymap, "Alt");
    xkb_mod_index_t controlMod = xkb_keymap_mod_get_index(keymap, "Control");
    xkb_mod_index_t metaMod = xkb_keymap_mod_get_index(keymap, "Meta");

    Q_ASSERT(shiftMod < 32);
    Q_ASSERT(altMod < 32);
    Q_ASSERT(controlMod < 32);

    xkb_mod_mask_t depressed;
    int qtKey = 0;
    // obtain a list of possible shortcuts for the given key event
    for (uint i = 1; i < sizeof(ModsTbl) / sizeof(*ModsTbl) ; ++i) {
        Qt::KeyboardModifiers neededMods = ModsTbl[i];
        if ((modifiers & neededMods) == neededMods) {
            if (i == 8) {
                if (isLatin(baseQtKey))
                    continue;
                // add a latin key as a fall back key
                sym = lookupLatinKeysym(state, keycode);
            } else {
                depressed = 0;
                if (neededMods & Qt::AltModifier)
                    depressed |= (1 << altMod);
                if (neededMods & Qt::ShiftModifier)
                    depressed |= (1 << shiftMod);
                if (neededMods & Qt::ControlModifier)
                    depressed |= (1 << controlMod);
                if (metaMod < 32 && neededMods & Qt::MetaModifier)
                    depressed |= (1 << metaMod);
                xkb_state_update_mask(queryState, depressed, latchedMods, lockedMods, 0, 0, lockedLayout);
                sym = xkb_state_key_get_one_sym(queryState, keycode);
            }
            if (sym == XKB_KEY_NoSymbol)
                continue;

            Qt::KeyboardModifiers mods = modifiers & ~neededMods;
            qtKey = keysymToQtKey_internal(sym, mods, queryState, keycode, superAsMeta, hyperAsMeta);
            if (!qtKey || qtKey == baseQtKey)
                continue;

            // catch only more specific shortcuts, i.e. Ctrl+Shift+= also generates Ctrl++ and +,
            // but Ctrl++ is more specific than +, so we should skip the last one
            bool ambiguous = false;
            for (int shortcut : qAsConst(result)) {
                if (int(shortcut & ~Qt::KeyboardModifierMask) == qtKey && (shortcut & mods) == mods) {
                    ambiguous = true;
                    break;
                }
            }
            if (ambiguous)
                continue;

            result += (qtKey + mods);
        }
    }

    return result;
}

void QXkbCommon::verifyHasLatinLayout(xkb_keymap *keymap)
{
    const xkb_layout_index_t layoutCount = xkb_keymap_num_layouts(keymap);
    const xkb_keycode_t minKeycode = xkb_keymap_min_keycode(keymap);
    const xkb_keycode_t maxKeycode = xkb_keymap_max_keycode(keymap);

    const xkb_keysym_t *keysyms = nullptr;
    int nrLatinKeys = 0;
    for (xkb_layout_index_t layout = 0; layout < layoutCount; ++layout) {
        for (xkb_keycode_t code = minKeycode; code < maxKeycode; ++code) {
            xkb_keymap_key_get_syms_by_level(keymap, code, layout, 0, &keysyms);
            if (keysyms && isLatin(keysyms[0]))
                nrLatinKeys++;
            if (nrLatinKeys > 10) // arbitrarily chosen threshold
                return;
        }
    }
    // This means that lookupLatinKeysym() will not find anything and latin
    // key shortcuts might not work. This is a bug in the affected desktop
    // environment. Usually can be solved via system settings by adding e.g. 'us'
    // layout to the list of seleced layouts, or by using command line, "setxkbmap
    // -layout rus,en". The position of latin key based layout in the list of the
    // selected layouts is irrelevant. Properly functioning desktop environments
    // handle this behind the scenes, even if no latin key based layout has been
    // explicitly listed in the selected layouts.
    qCDebug(lcXkbcommon, "no keyboard layouts with latin keys present");
}

xkb_keysym_t QXkbCommon::lookupLatinKeysym(xkb_state *state, xkb_keycode_t keycode)
{
    xkb_layout_index_t layout;
    xkb_keysym_t sym = XKB_KEY_NoSymbol;
    xkb_keymap *keymap = xkb_state_get_keymap(state);
    const xkb_layout_index_t layoutCount = xkb_keymap_num_layouts_for_key(keymap, keycode);
    const xkb_layout_index_t currentLayout = xkb_state_key_get_layout(state, keycode);
    // Look at user layouts in the order in which they are defined in system
    // settings to find a latin keysym.
    for (layout = 0; layout < layoutCount; ++layout) {
        if (layout == currentLayout)
            continue;
        const xkb_keysym_t *syms = nullptr;
        xkb_level_index_t level = xkb_state_key_get_level(state, keycode, layout);
        if (xkb_keymap_key_get_syms_by_level(keymap, keycode, layout, level, &syms) != 1)
            continue;
        if (isLatin(syms[0])) {
            sym = syms[0];
            break;
        }
    }

    if (sym == XKB_KEY_NoSymbol)
        return sym;

    xkb_mod_mask_t latchedMods = xkb_state_serialize_mods(state, XKB_STATE_MODS_LATCHED);
    xkb_mod_mask_t lockedMods = xkb_state_serialize_mods(state, XKB_STATE_MODS_LOCKED);

    // Check for uniqueness, consider the following setup:
    // setxkbmap -layout us,ru,us -variant dvorak,, -option 'grp:ctrl_alt_toggle' (set 'ru' as active).
    // In this setup, the user would expect to trigger a ctrl+q shortcut by pressing ctrl+<physical x key>,
    // because "US dvorak" is higher up in the layout settings list. This check verifies that an obtained
    // 'sym' can not be acquired by any other layout higher up in the user's layout list. If it can be acquired
    // then the obtained key is not unique. This prevents ctrl+<physical q key> from generating a ctrl+q
    // shortcut in the above described setup. We don't want ctrl+<physical x key> and ctrl+<physical q key> to
    // generate the same shortcut event in this case.
    const xkb_keycode_t minKeycode = xkb_keymap_min_keycode(keymap);
    const xkb_keycode_t maxKeycode = xkb_keymap_max_keycode(keymap);
    ScopedXKBState queryState(xkb_state_new(keymap));
    for (xkb_layout_index_t prevLayout = 0; prevLayout < layout; ++prevLayout) {
        xkb_state_update_mask(queryState.get(), 0, latchedMods, lockedMods, 0, 0, prevLayout);
        for (xkb_keycode_t code = minKeycode; code < maxKeycode; ++code) {
            xkb_keysym_t prevSym = xkb_state_key_get_one_sym(queryState.get(), code);
            if (prevSym == sym) {
                sym = XKB_KEY_NoSymbol;
                break;
            }
        }
    }

    return sym;
}

void QXkbCommon::setXkbContext(QPlatformInputContext *inputContext, struct xkb_context *context)
{
    if (!inputContext || !context)
        return;

    const char *const inputContextClassName = "QComposeInputContext";
    const char *const normalizedSignature = "setXkbContext(xkb_context*)";

    if (inputContext->objectName() != QLatin1String(inputContextClassName))
        return;

    static const QMetaMethod setXkbContext = [&]() {
        int methodIndex = inputContext->metaObject()->indexOfMethod(normalizedSignature);
        QMetaMethod method = inputContext->metaObject()->method(methodIndex);
        Q_ASSERT(method.isValid());
        if (!method.isValid())
            qCWarning(lcXkbcommon) << normalizedSignature << "not found on" << inputContextClassName;
        return method;
    }();

    if (!setXkbContext.isValid())
        return;

    setXkbContext.invoke(inputContext, Qt::DirectConnection, Q_ARG(struct xkb_context*, context));
}

QT_END_NAMESPACE
