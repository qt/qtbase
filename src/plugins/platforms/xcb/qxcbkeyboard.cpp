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

#include <X11/keysym.h>

#if QT_CONFIG(xinput2)
#include <X11/extensions/XI2proto.h>
#undef KeyPress
#undef KeyRelease
#endif

#ifndef XK_ISO_Left_Tab
#define XK_ISO_Left_Tab         0xFE20
#endif

#ifndef XK_dead_a
#define XK_dead_a               0xFE80
#endif

#ifndef XK_dead_A
#define XK_dead_A               0xFE81
#endif

#ifndef XK_dead_e
#define XK_dead_e               0xFE82
#endif

#ifndef XK_dead_E
#define XK_dead_E               0xFE83
#endif

#ifndef XK_dead_i
#define XK_dead_i               0xFE84
#endif

#ifndef XK_dead_I
#define XK_dead_I               0xFE85
#endif

#ifndef XK_dead_o
#define XK_dead_o               0xFE86
#endif

#ifndef XK_dead_O
#define XK_dead_O               0xFE87
#endif

#ifndef XK_dead_u
#define XK_dead_u               0xFE88
#endif

#ifndef XK_dead_U
#define XK_dead_U               0xFE89
#endif

#ifndef XK_dead_small_schwa
#define XK_dead_small_schwa     0xFE8A
#endif

#ifndef XK_dead_capital_schwa
#define XK_dead_capital_schwa   0xFE8B
#endif

#ifndef XK_dead_greek
#define XK_dead_greek           0xFE8C
#endif

#ifndef XK_dead_lowline
#define XK_dead_lowline         0xFE90
#endif

#ifndef XK_dead_aboveverticalline
#define XK_dead_aboveverticalline 0xFE91
#endif

#ifndef XK_dead_belowverticalline
#define XK_dead_belowverticalline 0xFE92
#endif

#ifndef XK_dead_longsolidusoverlay
#define XK_dead_longsolidusoverlay 0xFE93
#endif


#ifndef XK_Codeinput
#define XK_Codeinput            0xFF37
#endif

#ifndef XK_Kanji_Bangou
#define XK_Kanji_Bangou         0xFF37 /* same as codeinput */
#endif

// Fix old X libraries
#ifndef XK_KP_Home
#define XK_KP_Home              0xFF95
#endif
#ifndef XK_KP_Left
#define XK_KP_Left              0xFF96
#endif
#ifndef XK_KP_Up
#define XK_KP_Up                0xFF97
#endif
#ifndef XK_KP_Right
#define XK_KP_Right             0xFF98
#endif
#ifndef XK_KP_Down
#define XK_KP_Down              0xFF99
#endif
#ifndef XK_KP_Prior
#define XK_KP_Prior             0xFF9A
#endif
#ifndef XK_KP_Next
#define XK_KP_Next              0xFF9B
#endif
#ifndef XK_KP_End
#define XK_KP_End               0xFF9C
#endif
#ifndef XK_KP_Insert
#define XK_KP_Insert            0xFF9E
#endif
#ifndef XK_KP_Delete
#define XK_KP_Delete            0xFF9F
#endif

// the next lines are taken on 10/2009 from X.org (X11/XF86keysym.h), defining some special
// multimedia keys. They are included here as not every system has them.
#define XF86XK_MonBrightnessUp     0x1008FF02
#define XF86XK_MonBrightnessDown   0x1008FF03
#define XF86XK_KbdLightOnOff       0x1008FF04
#define XF86XK_KbdBrightnessUp     0x1008FF05
#define XF86XK_KbdBrightnessDown   0x1008FF06
#define XF86XK_Standby             0x1008FF10
#define XF86XK_AudioLowerVolume    0x1008FF11
#define XF86XK_AudioMute           0x1008FF12
#define XF86XK_AudioRaiseVolume    0x1008FF13
#define XF86XK_AudioPlay           0x1008FF14
#define XF86XK_AudioStop           0x1008FF15
#define XF86XK_AudioPrev           0x1008FF16
#define XF86XK_AudioNext           0x1008FF17
#define XF86XK_HomePage            0x1008FF18
#define XF86XK_Mail                0x1008FF19
#define XF86XK_Start               0x1008FF1A
#define XF86XK_Search              0x1008FF1B
#define XF86XK_AudioRecord         0x1008FF1C
#define XF86XK_Calculator          0x1008FF1D
#define XF86XK_Memo                0x1008FF1E
#define XF86XK_ToDoList            0x1008FF1F
#define XF86XK_Calendar            0x1008FF20
#define XF86XK_PowerDown           0x1008FF21
#define XF86XK_ContrastAdjust      0x1008FF22
#define XF86XK_Back                0x1008FF26
#define XF86XK_Forward             0x1008FF27
#define XF86XK_Stop                0x1008FF28
#define XF86XK_Refresh             0x1008FF29
#define XF86XK_PowerOff            0x1008FF2A
#define XF86XK_WakeUp              0x1008FF2B
#define XF86XK_Eject               0x1008FF2C
#define XF86XK_ScreenSaver         0x1008FF2D
#define XF86XK_WWW                 0x1008FF2E
#define XF86XK_Sleep               0x1008FF2F
#define XF86XK_Favorites           0x1008FF30
#define XF86XK_AudioPause          0x1008FF31
#define XF86XK_AudioMedia          0x1008FF32
#define XF86XK_MyComputer          0x1008FF33
#define XF86XK_LightBulb           0x1008FF35
#define XF86XK_Shop                0x1008FF36
#define XF86XK_History             0x1008FF37
#define XF86XK_OpenURL             0x1008FF38
#define XF86XK_AddFavorite         0x1008FF39
#define XF86XK_HotLinks            0x1008FF3A
#define XF86XK_BrightnessAdjust    0x1008FF3B
#define XF86XK_Finance             0x1008FF3C
#define XF86XK_Community           0x1008FF3D
#define XF86XK_AudioRewind         0x1008FF3E
#define XF86XK_BackForward         0x1008FF3F
#define XF86XK_Launch0             0x1008FF40
#define XF86XK_Launch1             0x1008FF41
#define XF86XK_Launch2             0x1008FF42
#define XF86XK_Launch3             0x1008FF43
#define XF86XK_Launch4             0x1008FF44
#define XF86XK_Launch5             0x1008FF45
#define XF86XK_Launch6             0x1008FF46
#define XF86XK_Launch7             0x1008FF47
#define XF86XK_Launch8             0x1008FF48
#define XF86XK_Launch9             0x1008FF49
#define XF86XK_LaunchA             0x1008FF4A
#define XF86XK_LaunchB             0x1008FF4B
#define XF86XK_LaunchC             0x1008FF4C
#define XF86XK_LaunchD             0x1008FF4D
#define XF86XK_LaunchE             0x1008FF4E
#define XF86XK_LaunchF             0x1008FF4F
#define XF86XK_ApplicationLeft     0x1008FF50
#define XF86XK_ApplicationRight    0x1008FF51
#define XF86XK_Book                0x1008FF52
#define XF86XK_CD                  0x1008FF53
#define XF86XK_Calculater          0x1008FF54
#define XF86XK_Clear               0x1008FF55
#define XF86XK_ClearGrab           0x1008FE21
#define XF86XK_Close               0x1008FF56
#define XF86XK_Copy                0x1008FF57
#define XF86XK_Cut                 0x1008FF58
#define XF86XK_Display             0x1008FF59
#define XF86XK_DOS                 0x1008FF5A
#define XF86XK_Documents           0x1008FF5B
#define XF86XK_Excel               0x1008FF5C
#define XF86XK_Explorer            0x1008FF5D
#define XF86XK_Game                0x1008FF5E
#define XF86XK_Go                  0x1008FF5F
#define XF86XK_iTouch              0x1008FF60
#define XF86XK_LogOff              0x1008FF61
#define XF86XK_Market              0x1008FF62
#define XF86XK_Meeting             0x1008FF63
#define XF86XK_MenuKB              0x1008FF65
#define XF86XK_MenuPB              0x1008FF66
#define XF86XK_MySites             0x1008FF67
#define XF86XK_New                 0x1008FF68
#define XF86XK_News                0x1008FF69
#define XF86XK_OfficeHome          0x1008FF6A
#define XF86XK_Open                0x1008FF6B
#define XF86XK_Option              0x1008FF6C
#define XF86XK_Paste               0x1008FF6D
#define XF86XK_Phone               0x1008FF6E
#define XF86XK_Reply               0x1008FF72
#define XF86XK_Reload              0x1008FF73
#define XF86XK_RotateWindows       0x1008FF74
#define XF86XK_RotationPB          0x1008FF75
#define XF86XK_RotationKB          0x1008FF76
#define XF86XK_Save                0x1008FF77
#define XF86XK_Send                0x1008FF7B
#define XF86XK_Spell               0x1008FF7C
#define XF86XK_SplitScreen         0x1008FF7D
#define XF86XK_Support             0x1008FF7E
#define XF86XK_TaskPane            0x1008FF7F
#define XF86XK_Terminal            0x1008FF80
#define XF86XK_Tools               0x1008FF81
#define XF86XK_Travel              0x1008FF82
#define XF86XK_Video               0x1008FF87
#define XF86XK_Word                0x1008FF89
#define XF86XK_Xfer                0x1008FF8A
#define XF86XK_ZoomIn              0x1008FF8B
#define XF86XK_ZoomOut             0x1008FF8C
#define XF86XK_Away                0x1008FF8D
#define XF86XK_Messenger           0x1008FF8E
#define XF86XK_WebCam              0x1008FF8F
#define XF86XK_MailForward         0x1008FF90
#define XF86XK_Pictures            0x1008FF91
#define XF86XK_Music               0x1008FF92
#define XF86XK_Battery             0x1008FF93
#define XF86XK_Bluetooth           0x1008FF94
#define XF86XK_WLAN                0x1008FF95
#define XF86XK_UWB                 0x1008FF96
#define XF86XK_AudioForward        0x1008FF97
#define XF86XK_AudioRepeat         0x1008FF98
#define XF86XK_AudioRandomPlay     0x1008FF99
#define XF86XK_Subtitle            0x1008FF9A
#define XF86XK_AudioCycleTrack     0x1008FF9B
#define XF86XK_Time                0x1008FF9F
#define XF86XK_Select              0x1008FFA0
#define XF86XK_View                0x1008FFA1
#define XF86XK_TopMenu             0x1008FFA2
#define XF86XK_Red                 0x1008FFA3
#define XF86XK_Green               0x1008FFA4
#define XF86XK_Yellow              0x1008FFA5
#define XF86XK_Blue                0x1008FFA6
#define XF86XK_Suspend             0x1008FFA7
#define XF86XK_Hibernate           0x1008FFA8
#define XF86XK_TouchpadToggle      0x1008FFA9
#define XF86XK_TouchpadOn          0x1008FFB0
#define XF86XK_TouchpadOff         0x1008FFB1
#define XF86XK_AudioMicMute        0x1008FFB2


// end of XF86keysyms.h

QT_BEGIN_NAMESPACE

// keyboard mapping table
static const unsigned int KeyTbl[] = {

    // misc keys

    XK_Escape,                  Qt::Key_Escape,
    XK_Tab,                     Qt::Key_Tab,
    XK_ISO_Left_Tab,            Qt::Key_Backtab,
    XK_BackSpace,               Qt::Key_Backspace,
    XK_Return,                  Qt::Key_Return,
    XK_Insert,                  Qt::Key_Insert,
    XK_Delete,                  Qt::Key_Delete,
    XK_Clear,                   Qt::Key_Delete,
    XK_Pause,                   Qt::Key_Pause,
    XK_Print,                   Qt::Key_Print,
    0x1005FF60,                 Qt::Key_SysReq,         // hardcoded Sun SysReq
    0x1007ff00,                 Qt::Key_SysReq,         // hardcoded X386 SysReq

    // cursor movement

    XK_Home,                    Qt::Key_Home,
    XK_End,                     Qt::Key_End,
    XK_Left,                    Qt::Key_Left,
    XK_Up,                      Qt::Key_Up,
    XK_Right,                   Qt::Key_Right,
    XK_Down,                    Qt::Key_Down,
    XK_Prior,                   Qt::Key_PageUp,
    XK_Next,                    Qt::Key_PageDown,

    // modifiers

    XK_Shift_L,                 Qt::Key_Shift,
    XK_Shift_R,                 Qt::Key_Shift,
    XK_Shift_Lock,              Qt::Key_Shift,
    XK_Control_L,               Qt::Key_Control,
    XK_Control_R,               Qt::Key_Control,
    XK_Meta_L,                  Qt::Key_Meta,
    XK_Meta_R,                  Qt::Key_Meta,
    XK_Alt_L,                   Qt::Key_Alt,
    XK_Alt_R,                   Qt::Key_Alt,
    XK_Caps_Lock,               Qt::Key_CapsLock,
    XK_Num_Lock,                Qt::Key_NumLock,
    XK_Scroll_Lock,             Qt::Key_ScrollLock,
    XK_Super_L,                 Qt::Key_Super_L,
    XK_Super_R,                 Qt::Key_Super_R,
    XK_Menu,                    Qt::Key_Menu,
    XK_Hyper_L,                 Qt::Key_Hyper_L,
    XK_Hyper_R,                 Qt::Key_Hyper_R,
    XK_Help,                    Qt::Key_Help,
    0x1000FF74,                 Qt::Key_Backtab,        // hardcoded HP backtab
    0x1005FF10,                 Qt::Key_F11,            // hardcoded Sun F36 (labeled F11)
    0x1005FF11,                 Qt::Key_F12,            // hardcoded Sun F37 (labeled F12)

    // numeric and function keypad keys

    XK_KP_Space,                Qt::Key_Space,
    XK_KP_Tab,                  Qt::Key_Tab,
    XK_KP_Enter,                Qt::Key_Enter,
    //XK_KP_F1,                 Qt::Key_F1,
    //XK_KP_F2,                 Qt::Key_F2,
    //XK_KP_F3,                 Qt::Key_F3,
    //XK_KP_F4,                 Qt::Key_F4,
    XK_KP_Home,                 Qt::Key_Home,
    XK_KP_Left,                 Qt::Key_Left,
    XK_KP_Up,                   Qt::Key_Up,
    XK_KP_Right,                Qt::Key_Right,
    XK_KP_Down,                 Qt::Key_Down,
    XK_KP_Prior,                Qt::Key_PageUp,
    XK_KP_Next,                 Qt::Key_PageDown,
    XK_KP_End,                  Qt::Key_End,
    XK_KP_Begin,                Qt::Key_Clear,
    XK_KP_Insert,               Qt::Key_Insert,
    XK_KP_Delete,               Qt::Key_Delete,
    XK_KP_Equal,                Qt::Key_Equal,
    XK_KP_Multiply,             Qt::Key_Asterisk,
    XK_KP_Add,                  Qt::Key_Plus,
    XK_KP_Separator,            Qt::Key_Comma,
    XK_KP_Subtract,             Qt::Key_Minus,
    XK_KP_Decimal,              Qt::Key_Period,
    XK_KP_Divide,               Qt::Key_Slash,

    // International input method support keys

    // International & multi-key character composition
    XK_ISO_Level3_Shift,        Qt::Key_AltGr,
    XK_Multi_key,               Qt::Key_Multi_key,
    XK_Codeinput,               Qt::Key_Codeinput,
    XK_SingleCandidate,         Qt::Key_SingleCandidate,
    XK_MultipleCandidate,       Qt::Key_MultipleCandidate,
    XK_PreviousCandidate,       Qt::Key_PreviousCandidate,

    // Misc Functions
    XK_Mode_switch,             Qt::Key_Mode_switch,
    XK_script_switch,           Qt::Key_Mode_switch,

    // Japanese keyboard support
    XK_Kanji,                   Qt::Key_Kanji,
    XK_Muhenkan,                Qt::Key_Muhenkan,
    //XK_Henkan_Mode,           Qt::Key_Henkan_Mode,
    XK_Henkan_Mode,             Qt::Key_Henkan,
    XK_Henkan,                  Qt::Key_Henkan,
    XK_Romaji,                  Qt::Key_Romaji,
    XK_Hiragana,                Qt::Key_Hiragana,
    XK_Katakana,                Qt::Key_Katakana,
    XK_Hiragana_Katakana,       Qt::Key_Hiragana_Katakana,
    XK_Zenkaku,                 Qt::Key_Zenkaku,
    XK_Hankaku,                 Qt::Key_Hankaku,
    XK_Zenkaku_Hankaku,         Qt::Key_Zenkaku_Hankaku,
    XK_Touroku,                 Qt::Key_Touroku,
    XK_Massyo,                  Qt::Key_Massyo,
    XK_Kana_Lock,               Qt::Key_Kana_Lock,
    XK_Kana_Shift,              Qt::Key_Kana_Shift,
    XK_Eisu_Shift,              Qt::Key_Eisu_Shift,
    XK_Eisu_toggle,             Qt::Key_Eisu_toggle,
    //XK_Kanji_Bangou,          Qt::Key_Kanji_Bangou,
    //XK_Zen_Koho,              Qt::Key_Zen_Koho,
    //XK_Mae_Koho,              Qt::Key_Mae_Koho,
    XK_Kanji_Bangou,            Qt::Key_Codeinput,
    XK_Zen_Koho,                Qt::Key_MultipleCandidate,
    XK_Mae_Koho,                Qt::Key_PreviousCandidate,

#ifdef XK_KOREAN
    // Korean keyboard support
    XK_Hangul,                  Qt::Key_Hangul,
    XK_Hangul_Start,            Qt::Key_Hangul_Start,
    XK_Hangul_End,              Qt::Key_Hangul_End,
    XK_Hangul_Hanja,            Qt::Key_Hangul_Hanja,
    XK_Hangul_Jamo,             Qt::Key_Hangul_Jamo,
    XK_Hangul_Romaja,           Qt::Key_Hangul_Romaja,
    //XK_Hangul_Codeinput,      Qt::Key_Hangul_Codeinput,
    XK_Hangul_Codeinput,        Qt::Key_Codeinput,
    XK_Hangul_Jeonja,           Qt::Key_Hangul_Jeonja,
    XK_Hangul_Banja,            Qt::Key_Hangul_Banja,
    XK_Hangul_PreHanja,         Qt::Key_Hangul_PreHanja,
    XK_Hangul_PostHanja,        Qt::Key_Hangul_PostHanja,
    //XK_Hangul_SingleCandidate,Qt::Key_Hangul_SingleCandidate,
    //XK_Hangul_MultipleCandidate,Qt::Key_Hangul_MultipleCandidate,
    //XK_Hangul_PreviousCandidate,Qt::Key_Hangul_PreviousCandidate,
    XK_Hangul_SingleCandidate,  Qt::Key_SingleCandidate,
    XK_Hangul_MultipleCandidate,Qt::Key_MultipleCandidate,
    XK_Hangul_PreviousCandidate,Qt::Key_PreviousCandidate,
    XK_Hangul_Special,          Qt::Key_Hangul_Special,
    //XK_Hangul_switch,         Qt::Key_Hangul_switch,
    XK_Hangul_switch,           Qt::Key_Mode_switch,
#endif  // XK_KOREAN

    // dead keys
    XK_dead_grave,              Qt::Key_Dead_Grave,
    XK_dead_acute,              Qt::Key_Dead_Acute,
    XK_dead_circumflex,         Qt::Key_Dead_Circumflex,
    XK_dead_tilde,              Qt::Key_Dead_Tilde,
    XK_dead_macron,             Qt::Key_Dead_Macron,
    XK_dead_breve,              Qt::Key_Dead_Breve,
    XK_dead_abovedot,           Qt::Key_Dead_Abovedot,
    XK_dead_diaeresis,          Qt::Key_Dead_Diaeresis,
    XK_dead_abovering,          Qt::Key_Dead_Abovering,
    XK_dead_doubleacute,        Qt::Key_Dead_Doubleacute,
    XK_dead_caron,              Qt::Key_Dead_Caron,
    XK_dead_cedilla,            Qt::Key_Dead_Cedilla,
    XK_dead_ogonek,             Qt::Key_Dead_Ogonek,
    XK_dead_iota,               Qt::Key_Dead_Iota,
    XK_dead_voiced_sound,       Qt::Key_Dead_Voiced_Sound,
    XK_dead_semivoiced_sound,   Qt::Key_Dead_Semivoiced_Sound,
    XK_dead_belowdot,           Qt::Key_Dead_Belowdot,
    XK_dead_hook,               Qt::Key_Dead_Hook,
    XK_dead_horn,               Qt::Key_Dead_Horn,
    XK_dead_stroke,             Qt::Key_Dead_Stroke,
    XK_dead_abovecomma,         Qt::Key_Dead_Abovecomma,
    XK_dead_abovereversedcomma, Qt::Key_Dead_Abovereversedcomma,
    XK_dead_doublegrave,        Qt::Key_Dead_Doublegrave,
    XK_dead_belowring,          Qt::Key_Dead_Belowring,
    XK_dead_belowmacron,        Qt::Key_Dead_Belowmacron,
    XK_dead_belowcircumflex,    Qt::Key_Dead_Belowcircumflex,
    XK_dead_belowtilde,         Qt::Key_Dead_Belowtilde,
    XK_dead_belowbreve,         Qt::Key_Dead_Belowbreve,
    XK_dead_belowdiaeresis,     Qt::Key_Dead_Belowdiaeresis,
    XK_dead_invertedbreve,      Qt::Key_Dead_Invertedbreve,
    XK_dead_belowcomma,         Qt::Key_Dead_Belowcomma,
    XK_dead_currency,           Qt::Key_Dead_Currency,
    XK_dead_a,                  Qt::Key_Dead_a,
    XK_dead_A,                  Qt::Key_Dead_A,
    XK_dead_e,                  Qt::Key_Dead_e,
    XK_dead_E,                  Qt::Key_Dead_E,
    XK_dead_i,                  Qt::Key_Dead_i,
    XK_dead_I,                  Qt::Key_Dead_I,
    XK_dead_o,                  Qt::Key_Dead_o,
    XK_dead_O,                  Qt::Key_Dead_O,
    XK_dead_u,                  Qt::Key_Dead_u,
    XK_dead_U,                  Qt::Key_Dead_U,
    XK_dead_small_schwa,        Qt::Key_Dead_Small_Schwa,
    XK_dead_capital_schwa,      Qt::Key_Dead_Capital_Schwa,
    XK_dead_greek,              Qt::Key_Dead_Greek,
    XK_dead_lowline,            Qt::Key_Dead_Lowline,
    XK_dead_aboveverticalline,  Qt::Key_Dead_Aboveverticalline,
    XK_dead_belowverticalline,  Qt::Key_Dead_Belowverticalline,
    XK_dead_longsolidusoverlay, Qt::Key_Dead_Longsolidusoverlay,

    // Special keys from X.org - This include multimedia keys,
        // wireless/bluetooth/uwb keys, special launcher keys, etc.
    XF86XK_Back,                Qt::Key_Back,
    XF86XK_Forward,             Qt::Key_Forward,
    XF86XK_Stop,                Qt::Key_Stop,
    XF86XK_Refresh,             Qt::Key_Refresh,
    XF86XK_Favorites,           Qt::Key_Favorites,
    XF86XK_AudioMedia,          Qt::Key_LaunchMedia,
    XF86XK_OpenURL,             Qt::Key_OpenUrl,
    XF86XK_HomePage,            Qt::Key_HomePage,
    XF86XK_Search,              Qt::Key_Search,
    XF86XK_AudioLowerVolume,    Qt::Key_VolumeDown,
    XF86XK_AudioMute,           Qt::Key_VolumeMute,
    XF86XK_AudioRaiseVolume,    Qt::Key_VolumeUp,
    XF86XK_AudioPlay,           Qt::Key_MediaPlay,
    XF86XK_AudioStop,           Qt::Key_MediaStop,
    XF86XK_AudioPrev,           Qt::Key_MediaPrevious,
    XF86XK_AudioNext,           Qt::Key_MediaNext,
    XF86XK_AudioRecord,         Qt::Key_MediaRecord,
    XF86XK_AudioPause,          Qt::Key_MediaPause,
    XF86XK_Mail,                Qt::Key_LaunchMail,
    XF86XK_MyComputer,          Qt::Key_Launch0,  // ### Qt 6: remap properly
    XF86XK_Calculator,          Qt::Key_Launch1,
    XF86XK_Memo,                Qt::Key_Memo,
    XF86XK_ToDoList,            Qt::Key_ToDoList,
    XF86XK_Calendar,            Qt::Key_Calendar,
    XF86XK_PowerDown,           Qt::Key_PowerDown,
    XF86XK_ContrastAdjust,      Qt::Key_ContrastAdjust,
    XF86XK_Standby,             Qt::Key_Standby,
    XF86XK_MonBrightnessUp,     Qt::Key_MonBrightnessUp,
    XF86XK_MonBrightnessDown,   Qt::Key_MonBrightnessDown,
    XF86XK_KbdLightOnOff,       Qt::Key_KeyboardLightOnOff,
    XF86XK_KbdBrightnessUp,     Qt::Key_KeyboardBrightnessUp,
    XF86XK_KbdBrightnessDown,   Qt::Key_KeyboardBrightnessDown,
    XF86XK_PowerOff,            Qt::Key_PowerOff,
    XF86XK_WakeUp,              Qt::Key_WakeUp,
    XF86XK_Eject,               Qt::Key_Eject,
    XF86XK_ScreenSaver,         Qt::Key_ScreenSaver,
    XF86XK_WWW,                 Qt::Key_WWW,
    XF86XK_Sleep,               Qt::Key_Sleep,
    XF86XK_LightBulb,           Qt::Key_LightBulb,
    XF86XK_Shop,                Qt::Key_Shop,
    XF86XK_History,             Qt::Key_History,
    XF86XK_AddFavorite,         Qt::Key_AddFavorite,
    XF86XK_HotLinks,            Qt::Key_HotLinks,
    XF86XK_BrightnessAdjust,    Qt::Key_BrightnessAdjust,
    XF86XK_Finance,             Qt::Key_Finance,
    XF86XK_Community,           Qt::Key_Community,
    XF86XK_AudioRewind,         Qt::Key_AudioRewind,
    XF86XK_BackForward,         Qt::Key_BackForward,
    XF86XK_ApplicationLeft,     Qt::Key_ApplicationLeft,
    XF86XK_ApplicationRight,    Qt::Key_ApplicationRight,
    XF86XK_Book,                Qt::Key_Book,
    XF86XK_CD,                  Qt::Key_CD,
    XF86XK_Calculater,          Qt::Key_Calculator,
    XF86XK_Clear,               Qt::Key_Clear,
    XF86XK_ClearGrab,           Qt::Key_ClearGrab,
    XF86XK_Close,               Qt::Key_Close,
    XF86XK_Copy,                Qt::Key_Copy,
    XF86XK_Cut,                 Qt::Key_Cut,
    XF86XK_Display,             Qt::Key_Display,
    XF86XK_DOS,                 Qt::Key_DOS,
    XF86XK_Documents,           Qt::Key_Documents,
    XF86XK_Excel,               Qt::Key_Excel,
    XF86XK_Explorer,            Qt::Key_Explorer,
    XF86XK_Game,                Qt::Key_Game,
    XF86XK_Go,                  Qt::Key_Go,
    XF86XK_iTouch,              Qt::Key_iTouch,
    XF86XK_LogOff,              Qt::Key_LogOff,
    XF86XK_Market,              Qt::Key_Market,
    XF86XK_Meeting,             Qt::Key_Meeting,
    XF86XK_MenuKB,              Qt::Key_MenuKB,
    XF86XK_MenuPB,              Qt::Key_MenuPB,
    XF86XK_MySites,             Qt::Key_MySites,
    XF86XK_New,                 Qt::Key_New,
    XF86XK_News,                Qt::Key_News,
    XF86XK_OfficeHome,          Qt::Key_OfficeHome,
    XF86XK_Open,                Qt::Key_Open,
    XF86XK_Option,              Qt::Key_Option,
    XF86XK_Paste,               Qt::Key_Paste,
    XF86XK_Phone,               Qt::Key_Phone,
    XF86XK_Reply,               Qt::Key_Reply,
    XF86XK_Reload,              Qt::Key_Reload,
    XF86XK_RotateWindows,       Qt::Key_RotateWindows,
    XF86XK_RotationPB,          Qt::Key_RotationPB,
    XF86XK_RotationKB,          Qt::Key_RotationKB,
    XF86XK_Save,                Qt::Key_Save,
    XF86XK_Send,                Qt::Key_Send,
    XF86XK_Spell,               Qt::Key_Spell,
    XF86XK_SplitScreen,         Qt::Key_SplitScreen,
    XF86XK_Support,             Qt::Key_Support,
    XF86XK_TaskPane,            Qt::Key_TaskPane,
    XF86XK_Terminal,            Qt::Key_Terminal,
    XF86XK_Tools,               Qt::Key_Tools,
    XF86XK_Travel,              Qt::Key_Travel,
    XF86XK_Video,               Qt::Key_Video,
    XF86XK_Word,                Qt::Key_Word,
    XF86XK_Xfer,                Qt::Key_Xfer,
    XF86XK_ZoomIn,              Qt::Key_ZoomIn,
    XF86XK_ZoomOut,             Qt::Key_ZoomOut,
    XF86XK_Away,                Qt::Key_Away,
    XF86XK_Messenger,           Qt::Key_Messenger,
    XF86XK_WebCam,              Qt::Key_WebCam,
    XF86XK_MailForward,         Qt::Key_MailForward,
    XF86XK_Pictures,            Qt::Key_Pictures,
    XF86XK_Music,               Qt::Key_Music,
    XF86XK_Battery,             Qt::Key_Battery,
    XF86XK_Bluetooth,           Qt::Key_Bluetooth,
    XF86XK_WLAN,                Qt::Key_WLAN,
    XF86XK_UWB,                 Qt::Key_UWB,
    XF86XK_AudioForward,        Qt::Key_AudioForward,
    XF86XK_AudioRepeat,         Qt::Key_AudioRepeat,
    XF86XK_AudioRandomPlay,     Qt::Key_AudioRandomPlay,
    XF86XK_Subtitle,            Qt::Key_Subtitle,
    XF86XK_AudioCycleTrack,     Qt::Key_AudioCycleTrack,
    XF86XK_Time,                Qt::Key_Time,
    XF86XK_Select,              Qt::Key_Select,
    XF86XK_View,                Qt::Key_View,
    XF86XK_TopMenu,             Qt::Key_TopMenu,
    XF86XK_Red,                 Qt::Key_Red,
    XF86XK_Green,               Qt::Key_Green,
    XF86XK_Yellow,              Qt::Key_Yellow,
    XF86XK_Blue,                Qt::Key_Blue,
    XF86XK_Bluetooth,           Qt::Key_Bluetooth,
    XF86XK_Suspend,             Qt::Key_Suspend,
    XF86XK_Hibernate,           Qt::Key_Hibernate,
    XF86XK_TouchpadToggle,      Qt::Key_TouchpadToggle,
    XF86XK_TouchpadOn,          Qt::Key_TouchpadOn,
    XF86XK_TouchpadOff,         Qt::Key_TouchpadOff,
    XF86XK_AudioMicMute,        Qt::Key_MicMute,
    XF86XK_Launch0,             Qt::Key_Launch2, // ### Qt 6: remap properly
    XF86XK_Launch1,             Qt::Key_Launch3,
    XF86XK_Launch2,             Qt::Key_Launch4,
    XF86XK_Launch3,             Qt::Key_Launch5,
    XF86XK_Launch4,             Qt::Key_Launch6,
    XF86XK_Launch5,             Qt::Key_Launch7,
    XF86XK_Launch6,             Qt::Key_Launch8,
    XF86XK_Launch7,             Qt::Key_Launch9,
    XF86XK_Launch8,             Qt::Key_LaunchA,
    XF86XK_Launch9,             Qt::Key_LaunchB,
    XF86XK_LaunchA,             Qt::Key_LaunchC,
    XF86XK_LaunchB,             Qt::Key_LaunchD,
    XF86XK_LaunchC,             Qt::Key_LaunchE,
    XF86XK_LaunchD,             Qt::Key_LaunchF,
    XF86XK_LaunchE,             Qt::Key_LaunchG,
    XF86XK_LaunchF,             Qt::Key_LaunchH,

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

struct xkb_keymap *QXcbKeyboard::keymapFromCore()
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

        KeysymModifierMap keysymMods(keysymsToModifiers());
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

void QXcbKeyboard::updateKeymap()
{
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
        m_xkbKeymap.reset(keymapFromCore());
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
        const xkb_state_component newState
                = xkb_state_update_mask(m_xkbState.get(),
                                  state->baseMods,
                                  state->latchedMods,
                                  state->lockedMods,
                                  state->baseGroup,
                                  state->latchedGroup,
                                  state->lockedGroup);

        if ((newState & XKB_STATE_LAYOUT_EFFECTIVE) == XKB_STATE_LAYOUT_EFFECTIVE) {
            //qWarning("TODO: Support KeyboardLayoutChange on QPA (QTBUG-27681)");
        }
    }
}
#endif

void QXcbKeyboard::updateXKBStateFromState(struct xkb_state *kb_state, quint16 state)
{
    const quint32 modsDepressed = xkb_state_serialize_mods(kb_state, XKB_STATE_MODS_DEPRESSED);
    const quint32 modsLatched = xkb_state_serialize_mods(kb_state, XKB_STATE_MODS_LATCHED);
    const quint32 modsLocked = xkb_state_serialize_mods(kb_state, XKB_STATE_MODS_LOCKED);
    const quint32 xkbMask = xkbModMask(state);

    const quint32 latched = modsLatched & xkbMask;
    const quint32 locked = modsLocked & xkbMask;
    quint32 depressed = modsDepressed & xkbMask;
    // set modifiers in depressed if they don't appear in any of the final masks
    depressed |= ~(depressed | latched | locked) & xkbMask;

    const xkb_state_component newState
            = xkb_state_update_mask(kb_state,
                            depressed,
                            latched,
                            locked,
                            0,
                            0,
                            (state >> 13) & 3); // bits 13 and 14 report the state keyboard group

    if ((newState & XKB_STATE_LAYOUT_EFFECTIVE) == XKB_STATE_LAYOUT_EFFECTIVE) {
        //qWarning("TODO: Support KeyboardLayoutChange on QPA (QTBUG-27681)");
    }
}

void QXcbKeyboard::updateXKBStateFromCore(quint16 state)
{
    if (m_config && !connection()->hasXKB()) {
        updateXKBStateFromState(m_xkbState.get(), state);
    }
}

#if QT_CONFIG(xinput2)
void QXcbKeyboard::updateXKBStateFromXI(void *modInfo, void *groupInfo)
{
    if (m_config && !connection()->hasXKB()) {
        xXIModifierInfo *mods = static_cast<xXIModifierInfo *>(modInfo);
        xXIGroupInfo *group = static_cast<xXIGroupInfo *>(groupInfo);
        const xkb_state_component newState = xkb_state_update_mask(m_xkbState.get(),
                                                                   mods->base_mods,
                                                                   mods->latched_mods,
                                                                   mods->locked_mods,
                                                                   group->base_group,
                                                                   group->latched_group,
                                                                   group->locked_group);

        if ((newState & XKB_STATE_LAYOUT_EFFECTIVE) == XKB_STATE_LAYOUT_EFFECTIVE) {
            //qWarning("TODO: Support KeyboardLayoutChange on QPA (QTBUG-27681)");
        }
    }
}
#endif

quint32 QXcbKeyboard::xkbModMask(quint16 state)
{
    quint32 xkb_mask = 0;

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
    const xcb_keycode_t minKeycode = connection()->setup()->min_keycode;
    const xcb_keycode_t maxKeycode = connection()->setup()->max_keycode;

    ScopedXKBState state(xkb_state_new(m_xkbKeymap.get()));
    for (xkb_layout_index_t layout = 0; layout < layoutCount; ++layout) {
        xkb_state_update_mask(state.get(), 0, 0, 0, 0, 0, layout);
        for (xcb_keycode_t code = minKeycode; code < maxKeycode; ++code) {
            xkb_keysym_t sym = xkb_state_key_get_one_sym(state.get(), code);
            // if layout can produce any of these latin letters (chosen
            // arbitrarily) then it must be a latin key based layout
            if (sym == XK_q || sym == XK_a || sym == XK_e)
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
    const xcb_keycode_t minKeycode = connection()->setup()->min_keycode;
    const xcb_keycode_t maxKeycode = connection()->setup()->max_keycode;
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
        updateVModMapping();
        updateVModToRModMapping();
        core_device_id = xkb_x11_get_core_keyboard_device_id(xcb_connection());
        if (core_device_id == -1) {
            qWarning("Qt: couldn't get core keyboard device info");
            return;
        }
    } else {
#endif
        m_key_symbols = xcb_key_symbols_alloc(xcb_connection());
        updateModifiers();
#if QT_CONFIG(xkb)
    }
#endif
    updateKeymap();
}

QXcbKeyboard::~QXcbKeyboard()
{
    if (!connection()->hasXKB())
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

    resolveMaskConflicts();
#endif
}

// Small helper: set modifier bit, if modifier position is valid
static inline void applyModifier(uint *mask, int modifierBit)
{
    if (modifierBit >= 0 && modifierBit < 8)
        *mask |= 1 << modifierBit;
}

void QXcbKeyboard::updateModifiers()
{
    memset(&rmod_masks, 0, sizeof(rmod_masks));

    // Compute X modifier bits for Qt modifiers
    KeysymModifierMap keysymMods(keysymsToModifiers());
    applyModifier(&rmod_masks.alt,   keysymMods.value(XK_Alt_L,       -1));
    applyModifier(&rmod_masks.alt,   keysymMods.value(XK_Alt_R,       -1));
    applyModifier(&rmod_masks.meta,  keysymMods.value(XK_Meta_L,      -1));
    applyModifier(&rmod_masks.meta,  keysymMods.value(XK_Meta_R,      -1));
    applyModifier(&rmod_masks.altgr, keysymMods.value(XK_Mode_switch, -1));
    applyModifier(&rmod_masks.super, keysymMods.value(XK_Super_L,     -1));
    applyModifier(&rmod_masks.super, keysymMods.value(XK_Super_R,     -1));
    applyModifier(&rmod_masks.hyper, keysymMods.value(XK_Hyper_L,     -1));
    applyModifier(&rmod_masks.hyper, keysymMods.value(XK_Hyper_R,     -1));

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
        XK_Alt_L, XK_Meta_L, XK_Mode_switch, XK_Super_L, XK_Super_R,
        XK_Hyper_L, XK_Hyper_R
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
                                  quint16 state, xcb_timestamp_t time)
{
    if (!m_config)
        return;

    QXcbWindow *source = connection()->platformWindowFromId(sourceWindow);
    QXcbWindow *targetWindow = connection()->focusWindow() ? connection()->focusWindow() : source;
    if (!targetWindow || !source)
        return;
    if (type == QEvent::KeyPress)
        targetWindow->updateNetWmUserTime(time);

    // Have a temporary keyboard state filled in from state
    // this way we allow for synthetic events to have different state
    // from the current state i.e. you can have Alt+Ctrl pressed
    // and receive a synthetic key event that has neither Alt nor Ctrl pressed
    ScopedXKBState xkbState(xkb_state_new(m_xkbKeymap.get()));
    if (!xkbState)
        return;
    updateXKBStateFromState(xkbState.get(), state);

    xcb_keysym_t sym = xkb_state_key_get_one_sym(xkbState.get(), code);
    QString string = lookupString(xkbState.get(), code);

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
                               modifiers, xkbState.get(), code);

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

void QXcbKeyboard::handleKeyPressEvent(const xcb_key_press_event_t *event)
{
    handleKeyEvent(event->event, QEvent::KeyPress, event->detail, event->state, event->time);
}

void QXcbKeyboard::handleKeyReleaseEvent(const xcb_key_release_event_t *event)
{
    handleKeyEvent(event->event, QEvent::KeyRelease, event->detail, event->state, event->time);
}

void QXcbKeyboard::handleMappingNotifyEvent(const void *event)
{
    updateKeymap();
    if (connection()->hasXKB()) {
        updateVModMapping();
        updateVModToRModMapping();
    } else {
        void *ev = const_cast<void *>(event);
        xcb_refresh_keyboard_mapping(m_key_symbols, static_cast<xcb_mapping_notify_event_t *>(ev));
        updateModifiers();
    }
}

QT_END_NAMESPACE
