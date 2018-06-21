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
#include "qxcbxkbcommon.h"

#include <qpa/qwindowsysteminterface.h>
#include <qpa/qplatforminputcontext.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformcursor.h>

#include <QtCore/QMetaEnum>

#include <private/qguiapplication_p.h>

#include <xkbcommon/xkbcommon-keysyms.h>

#if QT_CONFIG(xinput2)
#include <X11/extensions/XI2proto.h>
#undef KeyPress
#undef KeyRelease
#endif
QT_BEGIN_NAMESPACE

static const unsigned int KeyTbl[] = {
    // misc keys

    XKB_KEY_Escape,                  Qt::Key_Escape,
    XKB_KEY_Tab,                     Qt::Key_Tab,
    XKB_KEY_ISO_Left_Tab,            Qt::Key_Backtab,
    XKB_KEY_BackSpace,               Qt::Key_Backspace,
    XKB_KEY_Return,                  Qt::Key_Return,
    XKB_KEY_Insert,                  Qt::Key_Insert,
    XKB_KEY_Delete,                  Qt::Key_Delete,
    XKB_KEY_Clear,                   Qt::Key_Delete,
    XKB_KEY_Pause,                   Qt::Key_Pause,
    XKB_KEY_Print,                   Qt::Key_Print,
    0x1005FF60,                 Qt::Key_SysReq,         // hardcoded Sun SysReq
    0x1007ff00,                 Qt::Key_SysReq,         // hardcoded X386 SysReq

    // cursor movement

    XKB_KEY_Home,                    Qt::Key_Home,
    XKB_KEY_End,                     Qt::Key_End,
    XKB_KEY_Left,                    Qt::Key_Left,
    XKB_KEY_Up,                      Qt::Key_Up,
    XKB_KEY_Right,                   Qt::Key_Right,
    XKB_KEY_Down,                    Qt::Key_Down,
    XKB_KEY_Prior,                   Qt::Key_PageUp,
    XKB_KEY_Next,                    Qt::Key_PageDown,

    // modifiers

    XKB_KEY_Shift_L,                 Qt::Key_Shift,
    XKB_KEY_Shift_R,                 Qt::Key_Shift,
    XKB_KEY_Shift_Lock,              Qt::Key_Shift,
    XKB_KEY_Control_L,               Qt::Key_Control,
    XKB_KEY_Control_R,               Qt::Key_Control,
    XKB_KEY_Meta_L,                  Qt::Key_Meta,
    XKB_KEY_Meta_R,                  Qt::Key_Meta,
    XKB_KEY_Alt_L,                   Qt::Key_Alt,
    XKB_KEY_Alt_R,                   Qt::Key_Alt,
    XKB_KEY_Caps_Lock,               Qt::Key_CapsLock,
    XKB_KEY_Num_Lock,                Qt::Key_NumLock,
    XKB_KEY_Scroll_Lock,             Qt::Key_ScrollLock,
    XKB_KEY_Super_L,                 Qt::Key_Super_L,
    XKB_KEY_Super_R,                 Qt::Key_Super_R,
    XKB_KEY_Menu,                    Qt::Key_Menu,
    XKB_KEY_Hyper_L,                 Qt::Key_Hyper_L,
    XKB_KEY_Hyper_R,                 Qt::Key_Hyper_R,
    XKB_KEY_Help,                    Qt::Key_Help,
    0x1000FF74,                 Qt::Key_Backtab,        // hardcoded HP backtab
    0x1005FF10,                 Qt::Key_F11,            // hardcoded Sun F36 (labeled F11)
    0x1005FF11,                 Qt::Key_F12,            // hardcoded Sun F37 (labeled F12)

    // numeric and function keypad keys

    XKB_KEY_KP_Space,                Qt::Key_Space,
    XKB_KEY_KP_Tab,                  Qt::Key_Tab,
    XKB_KEY_KP_Enter,                Qt::Key_Enter,
    //XKB_KEY_KP_F1,                 Qt::Key_F1,
    //XKB_KEY_KP_F2,                 Qt::Key_F2,
    //XKB_KEY_KP_F3,                 Qt::Key_F3,
    //XKB_KEY_KP_F4,                 Qt::Key_F4,
    XKB_KEY_KP_Home,                 Qt::Key_Home,
    XKB_KEY_KP_Left,                 Qt::Key_Left,
    XKB_KEY_KP_Up,                   Qt::Key_Up,
    XKB_KEY_KP_Right,                Qt::Key_Right,
    XKB_KEY_KP_Down,                 Qt::Key_Down,
    XKB_KEY_KP_Prior,                Qt::Key_PageUp,
    XKB_KEY_KP_Next,                 Qt::Key_PageDown,
    XKB_KEY_KP_End,                  Qt::Key_End,
    XKB_KEY_KP_Begin,                Qt::Key_Clear,
    XKB_KEY_KP_Insert,               Qt::Key_Insert,
    XKB_KEY_KP_Delete,               Qt::Key_Delete,
    XKB_KEY_KP_Equal,                Qt::Key_Equal,
    XKB_KEY_KP_Multiply,             Qt::Key_Asterisk,
    XKB_KEY_KP_Add,                  Qt::Key_Plus,
    XKB_KEY_KP_Separator,            Qt::Key_Comma,
    XKB_KEY_KP_Subtract,             Qt::Key_Minus,
    XKB_KEY_KP_Decimal,              Qt::Key_Period,
    XKB_KEY_KP_Divide,               Qt::Key_Slash,

    // special non-XF86 function keys

    XKB_KEY_Undo,                    Qt::Key_Undo,
    XKB_KEY_Redo,                    Qt::Key_Redo,
    XKB_KEY_Find,                    Qt::Key_Find,
    XKB_KEY_Cancel,                  Qt::Key_Cancel,

    // International input method support keys

    // International & multi-key character composition
    XKB_KEY_ISO_Level3_Shift,        Qt::Key_AltGr,
    XKB_KEY_Multi_key,               Qt::Key_Multi_key,
    XKB_KEY_Codeinput,               Qt::Key_Codeinput,
    XKB_KEY_SingleCandidate,         Qt::Key_SingleCandidate,
    XKB_KEY_MultipleCandidate,       Qt::Key_MultipleCandidate,
    XKB_KEY_PreviousCandidate,       Qt::Key_PreviousCandidate,

    // Misc Functions
    XKB_KEY_Mode_switch,             Qt::Key_Mode_switch,
    XKB_KEY_script_switch,           Qt::Key_Mode_switch,

    // Japanese keyboard support
    XKB_KEY_Kanji,                   Qt::Key_Kanji,
    XKB_KEY_Muhenkan,                Qt::Key_Muhenkan,
    //XKB_KEY_Henkan_Mode,           Qt::Key_Henkan_Mode,
    XKB_KEY_Henkan_Mode,             Qt::Key_Henkan,
    XKB_KEY_Henkan,                  Qt::Key_Henkan,
    XKB_KEY_Romaji,                  Qt::Key_Romaji,
    XKB_KEY_Hiragana,                Qt::Key_Hiragana,
    XKB_KEY_Katakana,                Qt::Key_Katakana,
    XKB_KEY_Hiragana_Katakana,       Qt::Key_Hiragana_Katakana,
    XKB_KEY_Zenkaku,                 Qt::Key_Zenkaku,
    XKB_KEY_Hankaku,                 Qt::Key_Hankaku,
    XKB_KEY_Zenkaku_Hankaku,         Qt::Key_Zenkaku_Hankaku,
    XKB_KEY_Touroku,                 Qt::Key_Touroku,
    XKB_KEY_Massyo,                  Qt::Key_Massyo,
    XKB_KEY_Kana_Lock,               Qt::Key_Kana_Lock,
    XKB_KEY_Kana_Shift,              Qt::Key_Kana_Shift,
    XKB_KEY_Eisu_Shift,              Qt::Key_Eisu_Shift,
    XKB_KEY_Eisu_toggle,             Qt::Key_Eisu_toggle,
    //XKB_KEY_Kanji_Bangou,          Qt::Key_Kanji_Bangou,
    //XKB_KEY_Zen_Koho,              Qt::Key_Zen_Koho,
    //XKB_KEY_Mae_Koho,              Qt::Key_Mae_Koho,
    XKB_KEY_Kanji_Bangou,            Qt::Key_Codeinput,
    XKB_KEY_Zen_Koho,                Qt::Key_MultipleCandidate,
    XKB_KEY_Mae_Koho,                Qt::Key_PreviousCandidate,

    // Korean keyboard support
    XKB_KEY_Hangul,                  Qt::Key_Hangul,
    XKB_KEY_Hangul_Start,            Qt::Key_Hangul_Start,
    XKB_KEY_Hangul_End,              Qt::Key_Hangul_End,
    XKB_KEY_Hangul_Hanja,            Qt::Key_Hangul_Hanja,
    XKB_KEY_Hangul_Jamo,             Qt::Key_Hangul_Jamo,
    XKB_KEY_Hangul_Romaja,           Qt::Key_Hangul_Romaja,
    //XKB_KEY_Hangul_Codeinput,      Qt::Key_Hangul_Codeinput,
    XKB_KEY_Hangul_Codeinput,        Qt::Key_Codeinput,
    XKB_KEY_Hangul_Jeonja,           Qt::Key_Hangul_Jeonja,
    XKB_KEY_Hangul_Banja,            Qt::Key_Hangul_Banja,
    XKB_KEY_Hangul_PreHanja,         Qt::Key_Hangul_PreHanja,
    XKB_KEY_Hangul_PostHanja,        Qt::Key_Hangul_PostHanja,
    //XKB_KEY_Hangul_SingleCandidate,Qt::Key_Hangul_SingleCandidate,
    //XKB_KEY_Hangul_MultipleCandidate,Qt::Key_Hangul_MultipleCandidate,
    //XKB_KEY_Hangul_PreviousCandidate,Qt::Key_Hangul_PreviousCandidate,
    XKB_KEY_Hangul_SingleCandidate,  Qt::Key_SingleCandidate,
    XKB_KEY_Hangul_MultipleCandidate,Qt::Key_MultipleCandidate,
    XKB_KEY_Hangul_PreviousCandidate,Qt::Key_PreviousCandidate,
    XKB_KEY_Hangul_Special,          Qt::Key_Hangul_Special,
    //XKB_KEY_Hangul_switch,         Qt::Key_Hangul_switch,
    XKB_KEY_Hangul_switch,           Qt::Key_Mode_switch,

    // dead keys
    XKB_KEY_dead_grave,              Qt::Key_Dead_Grave,
    XKB_KEY_dead_acute,              Qt::Key_Dead_Acute,
    XKB_KEY_dead_circumflex,         Qt::Key_Dead_Circumflex,
    XKB_KEY_dead_tilde,              Qt::Key_Dead_Tilde,
    XKB_KEY_dead_macron,             Qt::Key_Dead_Macron,
    XKB_KEY_dead_breve,              Qt::Key_Dead_Breve,
    XKB_KEY_dead_abovedot,           Qt::Key_Dead_Abovedot,
    XKB_KEY_dead_diaeresis,          Qt::Key_Dead_Diaeresis,
    XKB_KEY_dead_abovering,          Qt::Key_Dead_Abovering,
    XKB_KEY_dead_doubleacute,        Qt::Key_Dead_Doubleacute,
    XKB_KEY_dead_caron,              Qt::Key_Dead_Caron,
    XKB_KEY_dead_cedilla,            Qt::Key_Dead_Cedilla,
    XKB_KEY_dead_ogonek,             Qt::Key_Dead_Ogonek,
    XKB_KEY_dead_iota,               Qt::Key_Dead_Iota,
    XKB_KEY_dead_voiced_sound,       Qt::Key_Dead_Voiced_Sound,
    XKB_KEY_dead_semivoiced_sound,   Qt::Key_Dead_Semivoiced_Sound,
    XKB_KEY_dead_belowdot,           Qt::Key_Dead_Belowdot,
    XKB_KEY_dead_hook,               Qt::Key_Dead_Hook,
    XKB_KEY_dead_horn,               Qt::Key_Dead_Horn,
    XKB_KEY_dead_stroke,             Qt::Key_Dead_Stroke,
    XKB_KEY_dead_abovecomma,         Qt::Key_Dead_Abovecomma,
    XKB_KEY_dead_abovereversedcomma, Qt::Key_Dead_Abovereversedcomma,
    XKB_KEY_dead_doublegrave,        Qt::Key_Dead_Doublegrave,
    XKB_KEY_dead_belowring,          Qt::Key_Dead_Belowring,
    XKB_KEY_dead_belowmacron,        Qt::Key_Dead_Belowmacron,
    XKB_KEY_dead_belowcircumflex,    Qt::Key_Dead_Belowcircumflex,
    XKB_KEY_dead_belowtilde,         Qt::Key_Dead_Belowtilde,
    XKB_KEY_dead_belowbreve,         Qt::Key_Dead_Belowbreve,
    XKB_KEY_dead_belowdiaeresis,     Qt::Key_Dead_Belowdiaeresis,
    XKB_KEY_dead_invertedbreve,      Qt::Key_Dead_Invertedbreve,
    XKB_KEY_dead_belowcomma,         Qt::Key_Dead_Belowcomma,
    XKB_KEY_dead_currency,           Qt::Key_Dead_Currency,
    XKB_KEY_dead_a,                  Qt::Key_Dead_a,
    XKB_KEY_dead_A,                  Qt::Key_Dead_A,
    XKB_KEY_dead_e,                  Qt::Key_Dead_e,
    XKB_KEY_dead_E,                  Qt::Key_Dead_E,
    XKB_KEY_dead_i,                  Qt::Key_Dead_i,
    XKB_KEY_dead_I,                  Qt::Key_Dead_I,
    XKB_KEY_dead_o,                  Qt::Key_Dead_o,
    XKB_KEY_dead_O,                  Qt::Key_Dead_O,
    XKB_KEY_dead_u,                  Qt::Key_Dead_u,
    XKB_KEY_dead_U,                  Qt::Key_Dead_U,
    XKB_KEY_dead_small_schwa,        Qt::Key_Dead_Small_Schwa,
    XKB_KEY_dead_capital_schwa,      Qt::Key_Dead_Capital_Schwa,
    XKB_KEY_dead_greek,              Qt::Key_Dead_Greek,
    XKB_KEY_dead_lowline,            Qt::Key_Dead_Lowline,
    XKB_KEY_dead_aboveverticalline,  Qt::Key_Dead_Aboveverticalline,
    XKB_KEY_dead_belowverticalline,  Qt::Key_Dead_Belowverticalline,
    XKB_KEY_dead_longsolidusoverlay, Qt::Key_Dead_Longsolidusoverlay,

    // Special keys from X.org - This include multimedia keys,
    // wireless/bluetooth/uwb keys, special launcher keys, etc.
    XKB_KEY_XF86Back,                Qt::Key_Back,
    XKB_KEY_XF86Forward,             Qt::Key_Forward,
    XKB_KEY_XF86Stop,                Qt::Key_Stop,
    XKB_KEY_XF86Refresh,             Qt::Key_Refresh,
    XKB_KEY_XF86Favorites,           Qt::Key_Favorites,
    XKB_KEY_XF86AudioMedia,          Qt::Key_LaunchMedia,
    XKB_KEY_XF86OpenURL,             Qt::Key_OpenUrl,
    XKB_KEY_XF86HomePage,            Qt::Key_HomePage,
    XKB_KEY_XF86Search,              Qt::Key_Search,
    XKB_KEY_XF86AudioLowerVolume,    Qt::Key_VolumeDown,
    XKB_KEY_XF86AudioMute,           Qt::Key_VolumeMute,
    XKB_KEY_XF86AudioRaiseVolume,    Qt::Key_VolumeUp,
    XKB_KEY_XF86AudioPlay,           Qt::Key_MediaPlay,
    XKB_KEY_XF86AudioStop,           Qt::Key_MediaStop,
    XKB_KEY_XF86AudioPrev,           Qt::Key_MediaPrevious,
    XKB_KEY_XF86AudioNext,           Qt::Key_MediaNext,
    XKB_KEY_XF86AudioRecord,         Qt::Key_MediaRecord,
    XKB_KEY_XF86AudioPause,          Qt::Key_MediaPause,
    XKB_KEY_XF86Mail,                Qt::Key_LaunchMail,
    XKB_KEY_XF86MyComputer,          Qt::Key_Launch0,  // ### Qt 6: remap properly
    XKB_KEY_XF86Calculator,          Qt::Key_Launch1,
    XKB_KEY_XF86Memo,                Qt::Key_Memo,
    XKB_KEY_XF86ToDoList,            Qt::Key_ToDoList,
    XKB_KEY_XF86Calendar,            Qt::Key_Calendar,
    XKB_KEY_XF86PowerDown,           Qt::Key_PowerDown,
    XKB_KEY_XF86ContrastAdjust,      Qt::Key_ContrastAdjust,
    XKB_KEY_XF86Standby,             Qt::Key_Standby,
    XKB_KEY_XF86MonBrightnessUp,     Qt::Key_MonBrightnessUp,
    XKB_KEY_XF86MonBrightnessDown,   Qt::Key_MonBrightnessDown,
    XKB_KEY_XF86KbdLightOnOff,       Qt::Key_KeyboardLightOnOff,
    XKB_KEY_XF86KbdBrightnessUp,     Qt::Key_KeyboardBrightnessUp,
    XKB_KEY_XF86KbdBrightnessDown,   Qt::Key_KeyboardBrightnessDown,
    XKB_KEY_XF86PowerOff,            Qt::Key_PowerOff,
    XKB_KEY_XF86WakeUp,              Qt::Key_WakeUp,
    XKB_KEY_XF86Eject,               Qt::Key_Eject,
    XKB_KEY_XF86ScreenSaver,         Qt::Key_ScreenSaver,
    XKB_KEY_XF86WWW,                 Qt::Key_WWW,
    XKB_KEY_XF86Sleep,               Qt::Key_Sleep,
    XKB_KEY_XF86LightBulb,           Qt::Key_LightBulb,
    XKB_KEY_XF86Shop,                Qt::Key_Shop,
    XKB_KEY_XF86History,             Qt::Key_History,
    XKB_KEY_XF86AddFavorite,         Qt::Key_AddFavorite,
    XKB_KEY_XF86HotLinks,            Qt::Key_HotLinks,
    XKB_KEY_XF86BrightnessAdjust,    Qt::Key_BrightnessAdjust,
    XKB_KEY_XF86Finance,             Qt::Key_Finance,
    XKB_KEY_XF86Community,           Qt::Key_Community,
    XKB_KEY_XF86AudioRewind,         Qt::Key_AudioRewind,
    XKB_KEY_XF86BackForward,         Qt::Key_BackForward,
    XKB_KEY_XF86ApplicationLeft,     Qt::Key_ApplicationLeft,
    XKB_KEY_XF86ApplicationRight,    Qt::Key_ApplicationRight,
    XKB_KEY_XF86Book,                Qt::Key_Book,
    XKB_KEY_XF86CD,                  Qt::Key_CD,
    XKB_KEY_XF86Calculater,          Qt::Key_Calculator,
    XKB_KEY_XF86Clear,               Qt::Key_Clear,
    XKB_KEY_XF86ClearGrab,           Qt::Key_ClearGrab,
    XKB_KEY_XF86Close,               Qt::Key_Close,
    XKB_KEY_XF86Copy,                Qt::Key_Copy,
    XKB_KEY_XF86Cut,                 Qt::Key_Cut,
    XKB_KEY_XF86Display,             Qt::Key_Display,
    XKB_KEY_XF86DOS,                 Qt::Key_DOS,
    XKB_KEY_XF86Documents,           Qt::Key_Documents,
    XKB_KEY_XF86Excel,               Qt::Key_Excel,
    XKB_KEY_XF86Explorer,            Qt::Key_Explorer,
    XKB_KEY_XF86Game,                Qt::Key_Game,
    XKB_KEY_XF86Go,                  Qt::Key_Go,
    XKB_KEY_XF86iTouch,              Qt::Key_iTouch,
    XKB_KEY_XF86LogOff,              Qt::Key_LogOff,
    XKB_KEY_XF86Market,              Qt::Key_Market,
    XKB_KEY_XF86Meeting,             Qt::Key_Meeting,
    XKB_KEY_XF86MenuKB,              Qt::Key_MenuKB,
    XKB_KEY_XF86MenuPB,              Qt::Key_MenuPB,
    XKB_KEY_XF86MySites,             Qt::Key_MySites,
    XKB_KEY_XF86New,                 Qt::Key_New,
    XKB_KEY_XF86News,                Qt::Key_News,
    XKB_KEY_XF86OfficeHome,          Qt::Key_OfficeHome,
    XKB_KEY_XF86Open,                Qt::Key_Open,
    XKB_KEY_XF86Option,              Qt::Key_Option,
    XKB_KEY_XF86Paste,               Qt::Key_Paste,
    XKB_KEY_XF86Phone,               Qt::Key_Phone,
    XKB_KEY_XF86Reply,               Qt::Key_Reply,
    XKB_KEY_XF86Reload,              Qt::Key_Reload,
    XKB_KEY_XF86RotateWindows,       Qt::Key_RotateWindows,
    XKB_KEY_XF86RotationPB,          Qt::Key_RotationPB,
    XKB_KEY_XF86RotationKB,          Qt::Key_RotationKB,
    XKB_KEY_XF86Save,                Qt::Key_Save,
    XKB_KEY_XF86Send,                Qt::Key_Send,
    XKB_KEY_XF86Spell,               Qt::Key_Spell,
    XKB_KEY_XF86SplitScreen,         Qt::Key_SplitScreen,
    XKB_KEY_XF86Support,             Qt::Key_Support,
    XKB_KEY_XF86TaskPane,            Qt::Key_TaskPane,
    XKB_KEY_XF86Terminal,            Qt::Key_Terminal,
    XKB_KEY_XF86Tools,               Qt::Key_Tools,
    XKB_KEY_XF86Travel,              Qt::Key_Travel,
    XKB_KEY_XF86Video,               Qt::Key_Video,
    XKB_KEY_XF86Word,                Qt::Key_Word,
    XKB_KEY_XF86Xfer,                Qt::Key_Xfer,
    XKB_KEY_XF86ZoomIn,              Qt::Key_ZoomIn,
    XKB_KEY_XF86ZoomOut,             Qt::Key_ZoomOut,
    XKB_KEY_XF86Away,                Qt::Key_Away,
    XKB_KEY_XF86Messenger,           Qt::Key_Messenger,
    XKB_KEY_XF86WebCam,              Qt::Key_WebCam,
    XKB_KEY_XF86MailForward,         Qt::Key_MailForward,
    XKB_KEY_XF86Pictures,            Qt::Key_Pictures,
    XKB_KEY_XF86Music,               Qt::Key_Music,
    XKB_KEY_XF86Battery,             Qt::Key_Battery,
    XKB_KEY_XF86Bluetooth,           Qt::Key_Bluetooth,
    XKB_KEY_XF86WLAN,                Qt::Key_WLAN,
    XKB_KEY_XF86UWB,                 Qt::Key_UWB,
    XKB_KEY_XF86AudioForward,        Qt::Key_AudioForward,
    XKB_KEY_XF86AudioRepeat,         Qt::Key_AudioRepeat,
    XKB_KEY_XF86AudioRandomPlay,     Qt::Key_AudioRandomPlay,
    XKB_KEY_XF86Subtitle,            Qt::Key_Subtitle,
    XKB_KEY_XF86AudioCycleTrack,     Qt::Key_AudioCycleTrack,
    XKB_KEY_XF86Time,                Qt::Key_Time,
    XKB_KEY_XF86Select,              Qt::Key_Select,
    XKB_KEY_XF86View,                Qt::Key_View,
    XKB_KEY_XF86TopMenu,             Qt::Key_TopMenu,
    XKB_KEY_XF86Red,                 Qt::Key_Red,
    XKB_KEY_XF86Green,               Qt::Key_Green,
    XKB_KEY_XF86Yellow,              Qt::Key_Yellow,
    XKB_KEY_XF86Blue,                Qt::Key_Blue,
    XKB_KEY_XF86Bluetooth,           Qt::Key_Bluetooth,
    XKB_KEY_XF86Suspend,             Qt::Key_Suspend,
    XKB_KEY_XF86Hibernate,           Qt::Key_Hibernate,
    XKB_KEY_XF86TouchpadToggle,      Qt::Key_TouchpadToggle,
    XKB_KEY_XF86TouchpadOn,          Qt::Key_TouchpadOn,
    XKB_KEY_XF86TouchpadOff,         Qt::Key_TouchpadOff,
    XKB_KEY_XF86AudioMicMute,        Qt::Key_MicMute,
    XKB_KEY_XF86Launch0,             Qt::Key_Launch2, // ### Qt 6: remap properly
    XKB_KEY_XF86Launch1,             Qt::Key_Launch3,
    XKB_KEY_XF86Launch2,             Qt::Key_Launch4,
    XKB_KEY_XF86Launch3,             Qt::Key_Launch5,
    XKB_KEY_XF86Launch4,             Qt::Key_Launch6,
    XKB_KEY_XF86Launch5,             Qt::Key_Launch7,
    XKB_KEY_XF86Launch6,             Qt::Key_Launch8,
    XKB_KEY_XF86Launch7,             Qt::Key_Launch9,
    XKB_KEY_XF86Launch8,             Qt::Key_LaunchA,
    XKB_KEY_XF86Launch9,             Qt::Key_LaunchB,
    XKB_KEY_XF86LaunchA,             Qt::Key_LaunchC,
    XKB_KEY_XF86LaunchB,             Qt::Key_LaunchD,
    XKB_KEY_XF86LaunchC,             Qt::Key_LaunchE,
    XKB_KEY_XF86LaunchD,             Qt::Key_LaunchF,
    XKB_KEY_XF86LaunchE,             Qt::Key_LaunchG,
    XKB_KEY_XF86LaunchF,             Qt::Key_LaunchH,

    0,                          0
};

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

Qt::KeyboardModifiers QXcbKeyboard::translateModifiers(int s) const
{
    Qt::KeyboardModifiers ret = 0;
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
    xkbcommon_XConvertCase(unshifted, &xlower, &xupper);

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

#if QT_CONFIG(xkb)
    if (connection()->hasXKB()) {
        m_xkbKeymap.reset(xkb_x11_keymap_new_from_device(m_xkbContext.get(), xcb_connection(),
                                                         core_device_id, XKB_KEYMAP_COMPILE_NO_FLAGS));
        if (m_xkbKeymap)
            m_xkbState.reset(xkb_x11_state_new_from_device(m_xkbKeymap.get(), xcb_connection(), core_device_id));
    } else {
#endif
        m_xkbKeymap.reset(keymapFromCore(keysymMods));
        if (m_xkbKeymap)
            m_xkbState.reset(xkb_state_new(m_xkbKeymap.get()));
#if QT_CONFIG(xkb)
    }
#endif

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

    checkForLatinLayout();
}

#if QT_CONFIG(xkb)
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
#endif

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

#if QT_CONFIG(xinput2)
void QXcbKeyboard::updateXKBStateFromXI(void *modInfo, void *groupInfo)
{
    if (m_config && !connection()->hasXKB()) {
        xXIModifierInfo *mods = static_cast<xXIModifierInfo *>(modInfo);
        xXIGroupInfo *group = static_cast<xXIGroupInfo *>(groupInfo);
        const xkb_state_component changedComponents
                = xkb_state_update_mask(m_xkbState.get(),
                                        mods->base_mods,
                                        mods->latched_mods,
                                        mods->locked_mods,
                                        group->base_group,
                                        group->latched_group,
                                        group->locked_group);

        handleStateChanges(changedComponents);
    }
}
#endif

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

static bool isLatin(xkb_keysym_t sym)
{
    return ((sym >= 'a' && sym <= 'z') || (sym >= 'A' && sym <= 'Z'));
}

void QXcbKeyboard::checkForLatinLayout() const
{
    const xkb_layout_index_t layoutCount = xkb_keymap_num_layouts(m_xkbKeymap.get());
    const xcb_keycode_t minKeycode = xkb_keymap_min_keycode(m_xkbKeymap.get());
    const xcb_keycode_t maxKeycode = xkb_keymap_max_keycode(m_xkbKeymap.get());

    const xkb_keysym_t *keysyms = nullptr;
    int nrLatinKeys = 0;
    for (xkb_layout_index_t layout = 0; layout < layoutCount; ++layout) {
        for (xcb_keycode_t code = minKeycode; code < maxKeycode; ++code) {
            xkb_keymap_key_get_syms_by_level(m_xkbKeymap.get(), code, layout, 0, &keysyms);
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
    qCWarning(lcQpaKeyboard, "no keyboard layouts with latin keys present");
}

xkb_keysym_t QXcbKeyboard::lookupLatinKeysym(xkb_keycode_t keycode) const
{
    xkb_layout_index_t layout;
    xkb_keysym_t sym = XKB_KEY_NoSymbol;
    const xkb_layout_index_t layoutCount = xkb_keymap_num_layouts_for_key(m_xkbKeymap.get(), keycode);
    const xkb_layout_index_t currentLayout = xkb_state_key_get_layout(m_xkbState.get(), keycode);
    // Look at user layouts in the order in which they are defined in system
    // settings to find a latin keysym.
    for (layout = 0; layout < layoutCount; ++layout) {
        if (layout == currentLayout)
            continue;
        const xkb_keysym_t *syms;
        xkb_level_index_t level = xkb_state_key_get_level(m_xkbState.get(), keycode, layout);
        if (xkb_keymap_key_get_syms_by_level(m_xkbKeymap.get(), keycode, layout, level, &syms) != 1)
            continue;
        if (isLatin(syms[0])) {
            sym = syms[0];
            break;
        }
    }

    if (sym == XKB_KEY_NoSymbol)
        return sym;

    xkb_mod_mask_t latchedMods = xkb_state_serialize_mods(m_xkbState.get(), XKB_STATE_MODS_LATCHED);
    xkb_mod_mask_t lockedMods = xkb_state_serialize_mods(m_xkbState.get(), XKB_STATE_MODS_LOCKED);

    // Check for uniqueness, consider the following setup:
    // setxkbmap -layout us,ru,us -variant dvorak,, -option 'grp:ctrl_alt_toggle' (set 'ru' as active).
    // In this setup, the user would expect to trigger a ctrl+q shortcut by pressing ctrl+<physical x key>,
    // because "US dvorak" is higher up in the layout settings list. This check verifies that an obtained
    // 'sym' can not be acquired by any other layout higher up in the user's layout list. If it can be acquired
    // then the obtained key is not unique. This prevents ctrl+<physical q key> from generating a ctrl+q
    // shortcut in the above described setup. We don't want ctrl+<physical x key> and ctrl+<physical q key> to
    // generate the same shortcut event in this case.
    const xcb_keycode_t minKeycode = xkb_keymap_min_keycode(m_xkbKeymap.get());
    const xcb_keycode_t maxKeycode = xkb_keymap_max_keycode(m_xkbKeymap.get());
    ScopedXKBState state(xkb_state_new(m_xkbKeymap.get()));
    for (xkb_layout_index_t prevLayout = 0; prevLayout < layout; ++prevLayout) {
        xkb_state_update_mask(state.get(), 0, latchedMods, lockedMods, 0, 0, prevLayout);
        for (xcb_keycode_t code = minKeycode; code < maxKeycode; ++code) {
            xkb_keysym_t prevSym = xkb_state_key_get_one_sym(state.get(), code);
            if (prevSym == sym) {
                sym = XKB_KEY_NoSymbol;
                break;
            }
        }
    }

    return sym;
}

static const char *qtKeyName(int qtKey)
{
    int keyEnumIndex = qt_getQtMetaObject()->indexOfEnumerator("Key");
    QMetaEnum keyEnum = qt_getQtMetaObject()->enumerator(keyEnumIndex);
    return keyEnum.valueToKey(qtKey);
}

QList<int> QXcbKeyboard::possibleKeys(const QKeyEvent *event) const
{
    // turn off the modifier bits which doesn't participate in shortcuts
    Qt::KeyboardModifiers notNeeded = Qt::KeypadModifier | Qt::GroupSwitchModifier;
    Qt::KeyboardModifiers modifiers = event->modifiers() &= ~notNeeded;
    // create a fresh kb state and test against the relevant modifier combinations
    struct xkb_state *kb_state = xkb_state_new(m_xkbKeymap.get());
    if (!kb_state) {
        qWarning("QXcbKeyboard: failed to compile xkb keymap!");
        return QList<int>();
    }
    // get kb state from the master xkb_state and update the temporary kb_state
    xkb_layout_index_t lockedLayout = xkb_state_serialize_layout(m_xkbState.get(), XKB_STATE_LAYOUT_LOCKED);
    xkb_mod_mask_t latchedMods = xkb_state_serialize_mods(m_xkbState.get(), XKB_STATE_MODS_LATCHED);
    xkb_mod_mask_t lockedMods = xkb_state_serialize_mods(m_xkbState.get(), XKB_STATE_MODS_LOCKED);
    xkb_mod_mask_t depressedMods = xkb_state_serialize_mods(m_xkbState.get(), XKB_STATE_MODS_DEPRESSED);

    xkb_state_update_mask(kb_state, depressedMods, latchedMods, lockedMods, 0, 0, lockedLayout);
    quint32 keycode = event->nativeScanCode();
    // handle shortcuts for level three and above
    xkb_layout_index_t layoutIndex = xkb_state_key_get_layout(kb_state, keycode);
    xkb_level_index_t levelIndex = 0;
    if (layoutIndex != XKB_LAYOUT_INVALID) {
        levelIndex = xkb_state_key_get_level(kb_state, keycode, layoutIndex);
        if (levelIndex == XKB_LEVEL_INVALID)
            levelIndex = 0;
    }
    if (levelIndex <= 1)
        xkb_state_update_mask(kb_state, 0, latchedMods, lockedMods, 0, 0, lockedLayout);

    xkb_keysym_t sym = xkb_state_key_get_one_sym(kb_state, keycode);
    if (sym == XKB_KEY_NoSymbol) {
        xkb_state_unref(kb_state);
        return QList<int>();
    }

    QList<int> result;
    int baseQtKey = keysymToQtKey(sym, modifiers, kb_state, keycode);
    if (baseQtKey)
        result += (baseQtKey + modifiers);

    xkb_mod_index_t shiftMod = xkb_keymap_mod_get_index(m_xkbKeymap.get(), "Shift");
    xkb_mod_index_t altMod = xkb_keymap_mod_get_index(m_xkbKeymap.get(), "Alt");
    xkb_mod_index_t controlMod = xkb_keymap_mod_get_index(m_xkbKeymap.get(), "Control");
    xkb_mod_index_t metaMod = xkb_keymap_mod_get_index(m_xkbKeymap.get(), "Meta");

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
                sym = lookupLatinKeysym(keycode);
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
                xkb_state_update_mask(kb_state, depressed, latchedMods, lockedMods, 0, 0, lockedLayout);
                sym = xkb_state_key_get_one_sym(kb_state, keycode);
            }
            if (sym == XKB_KEY_NoSymbol)
                continue;

            Qt::KeyboardModifiers mods = modifiers & ~neededMods;
            qtKey = keysymToQtKey(sym, mods, kb_state, keycode);
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
    xkb_state_unref(kb_state);
    return result;
}

int QXcbKeyboard::keysymToQtKey(xcb_keysym_t keysym, Qt::KeyboardModifiers modifiers,
                                struct xkb_state *state, xcb_keycode_t code) const
{
    int qtKey = 0;

    // lookup from direct mapping
    if (keysym >= XKB_KEY_F1 && keysym <= XKB_KEY_F35) {
        // function keys
        qtKey = Qt::Key_F1 + (keysym - XKB_KEY_F1);
    } else if (keysym >= XKB_KEY_KP_0 && keysym <= XKB_KEY_KP_9) {
        // numeric keypad keys
        qtKey = Qt::Key_0 + (keysym - XKB_KEY_KP_0);
    } else if (isLatin(keysym)) {
        qtKey = xkbcommon_xkb_keysym_to_upper(keysym);
    } else {
        // check if we have a direct mapping
        int i = 0;
        while (KeyTbl[i]) {
            if (keysym == KeyTbl[i]) {
                qtKey = KeyTbl[i + 1];
                break;
            }
            i += 2;
        }
    }

    QString text;
    bool fromUnicode = qtKey == 0;
    if (fromUnicode) { // lookup from unicode
        if (modifiers & Qt::ControlModifier) {
            // Control modifier changes the text to ASCII control character, therefore we
            // can't use this text to map keysym to a qt key. We can use the same keysym
            // (it is not affectd by transformation) to obtain untransformed text. For details
            // see "Appendix A. Default Symbol Transformations" in the XKB specification.
            text = lookupStringNoKeysymTransformations(keysym);
        } else {
            text = lookupString(state, code);
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
    }

    if (rmod_masks.meta) {
        // translate Super/Hyper keys to Meta if we're using them as the MetaModifier
        if (rmod_masks.meta == rmod_masks.super && (qtKey == Qt::Key_Super_L
                                                 || qtKey == Qt::Key_Super_R)) {
            qtKey = Qt::Key_Meta;
        } else if (rmod_masks.meta == rmod_masks.hyper && (qtKey == Qt::Key_Hyper_L
                                                        || qtKey == Qt::Key_Hyper_R)) {
            qtKey = Qt::Key_Meta;
        }
    }

    if (Q_UNLIKELY(lcQpaKeyboard().isDebugEnabled())) {
        char keysymName[64];
        xkb_keysym_get_name(keysym, keysymName, sizeof(keysymName));
        QString keysymInHex = QString(QStringLiteral("0x%1")).arg(keysym, 0, 16);
        if (qtKeyName(qtKey)) {
            qCDebug(lcQpaKeyboard).nospace() << "keysym: " << keysymName << "("
                << keysymInHex << ") mapped to Qt::" << qtKeyName(qtKey) << " | text: " << text
                << " | qt key: " << qtKey << " mapped from unicode number: " << fromUnicode;
        } else {
            qCDebug(lcQpaKeyboard).nospace() << "no Qt::Key for keysym: " << keysymName
                << "(" << keysymInHex << ") | text: " << text << " | qt key: " << qtKey;
        }
    }

    return qtKey;
}

QXcbKeyboard::QXcbKeyboard(QXcbConnection *connection)
    : QXcbObject(connection)
{
#if QT_CONFIG(xkb)
    core_device_id = 0;
    if (connection->hasXKB()) {
        core_device_id = xkb_x11_get_core_keyboard_device_id(xcb_connection());
        if (core_device_id == -1) {
            qWarning("Qt: couldn't get core keyboard device info");
            return;
        }
    } else {
#endif
        m_key_symbols = xcb_key_symbols_alloc(xcb_connection());
#if QT_CONFIG(xkb)
    }
#endif
    updateKeymap();
}

QXcbKeyboard::~QXcbKeyboard()
{
    if (m_key_symbols)
        xcb_key_symbols_free(m_key_symbols);
}

void QXcbKeyboard::updateVModMapping()
{
#if QT_CONFIG(xkb)
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
        vmod_name = 0;

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
#endif
}

void QXcbKeyboard::updateVModToRModMapping()
{
#if QT_CONFIG(xkb)
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
#endif
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
}

class KeyChecker
{
public:
    KeyChecker(xcb_window_t window, xcb_keycode_t code, xcb_timestamp_t time, quint16 state)
        : m_window(window)
        , m_code(code)
        , m_time(time)
        , m_state(state)
        , m_error(false)
        , m_release(true)
    {
    }

    bool checkEvent(xcb_generic_event_t *ev)
    {
        if (m_error || !ev)
            return false;

        int type = ev->response_type & ~0x80;
        if (type != XCB_KEY_PRESS && type != XCB_KEY_RELEASE)
            return false;

        xcb_key_press_event_t *event = (xcb_key_press_event_t *)ev;

        if (event->event != m_window || event->detail != m_code || event->state != m_state) {
            m_error = true;
            return false;
        }

        if (type == XCB_KEY_PRESS) {
            m_error = !m_release || event->time - m_time > 10;
            return !m_error;
        }

        if (m_release) {
            m_error = true;
            return false;
        }

        m_release = true;
        m_time = event->time;

        return false;
    }

    bool release() const { return m_release; }
    xcb_timestamp_t time() const { return m_time; }

private:
    xcb_window_t m_window;
    xcb_keycode_t m_code;
    xcb_timestamp_t m_time;
    quint16 m_state;

    bool m_error;
    bool m_release;
};

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


    ScopedXKBState sendEventState;
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
    QString string = lookupString(xkbState, code);

    Qt::KeyboardModifiers modifiers = translateModifiers(state);
    if (sym >= XKB_KEY_KP_Space && sym <= XKB_KEY_KP_9)
        modifiers |= Qt::KeypadModifier;

    // Note 1: All standard key sequences on linux (as defined in platform theme)
    // that use a latin character also contain a control modifier, which is why
    // checking for Qt::ControlModifier is sufficient here. It is possible to
    // override QPlatformTheme::keyBindings() and provide custom sequences for
    // QKeySequence::StandardKey. Custom sequences probably should respect this
    // convention (alternatively, we could test against other modifiers here).
    // Note 2: The possibleKeys() shorcut mechanism is not affected by this value
    // adjustment and does its own thing.
    xcb_keysym_t latinKeysym = XKB_KEY_NoSymbol;
    if (modifiers & Qt::ControlModifier) {
        // With standard shortcuts we should prefer a latin character, this is
        // in checks like "event == QKeySequence::Copy".
        if (!isLatin(sym))
            latinKeysym = lookupLatinKeysym(code);
    }

    int qtcode = keysymToQtKey(latinKeysym != XKB_KEY_NoSymbol ? latinKeysym : sym,
                               modifiers, xkbState, code);

    bool isAutoRepeat = false;
    if (type == QEvent::KeyPress) {
        if (m_autorepeat_code == code) {
            isAutoRepeat = true;
            m_autorepeat_code = 0;
        }
    } else {
        // look ahead for auto-repeat
        KeyChecker checker(source->xcb_window(), code, time, state);
        xcb_generic_event_t *event = connection()->checkEvent(checker);
        if (event) {
            isAutoRepeat = true;
            free(event);
        }
        m_autorepeat_code = isAutoRepeat ? code : 0;
    }

    bool filtered = false;
    QPlatformInputContext *inputContext = QGuiApplicationPrivate::platformIntegration()->inputContext();
    if (inputContext) {
        QKeyEvent event(type, qtcode, modifiers, code, sym, state, string, isAutoRepeat, string.length());
        event.setTimestamp(time);
        filtered = inputContext->filterEvent(&event);
    }

    QWindow *window = targetWindow->window();
    if (!filtered) {
#ifndef QT_NO_CONTEXTMENU
        if (type == QEvent::KeyPress && qtcode == Qt::Key_Menu) {
            const QPoint globalPos = window->screen()->handle()->cursor()->pos();
            const QPoint pos = window->mapFromGlobal(globalPos);
            QWindowSystemInterface::handleContextMenuEvent(window, false, pos, globalPos, modifiers);
        }
#endif // QT_NO_CONTEXTMENU
        QWindowSystemInterface::handleExtendedKeyEvent(window, time, type, qtcode, modifiers,
                                                       code, sym, state, string, isAutoRepeat);
    }

    if (isAutoRepeat && type == QEvent::KeyRelease) {
        // since we removed it from the event queue using checkEvent we need to send the key press here
        filtered = false;
        if (inputContext) {
            QKeyEvent event(QEvent::KeyPress, qtcode, modifiers, code, sym, state, string, isAutoRepeat, string.length());
            event.setTimestamp(time);
            filtered = inputContext->filterEvent(&event);
        }
        if (!filtered)
            QWindowSystemInterface::handleExtendedKeyEvent(window, time, QEvent::KeyPress, qtcode, modifiers,
                                                           code, sym, state, string, isAutoRepeat);
    }
}

QString QXcbKeyboard::lookupString(struct xkb_state *state, xcb_keycode_t code) const
{
    QVarLengthArray<char, 32> chars(32);
    const int size = xkb_state_key_get_utf8(state, code, chars.data(), chars.size());
    if (Q_UNLIKELY(size + 1 > chars.size())) { // +1 for NUL
        chars.resize(size + 1);
        xkb_state_key_get_utf8(state, code, chars.data(), chars.size());
    }
    return QString::fromUtf8(chars.constData(), size);
}

QString QXcbKeyboard::lookupStringNoKeysymTransformations(xkb_keysym_t keysym) const
{
    QVarLengthArray<char, 32> chars(32);
    const int size = xkb_keysym_to_utf8(keysym, chars.data(), chars.size());
    if (Q_UNLIKELY(size > chars.size())) {
        chars.resize(size);
        xkb_keysym_to_utf8(keysym, chars.data(), chars.size());
    }
    return QString::fromUtf8(chars.constData(), size);
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
