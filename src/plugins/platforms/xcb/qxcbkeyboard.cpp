/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qxcbkeyboard.h"
#include "qxcbwindow.h"
#include "qxcbscreen.h"
#include "qxlibconvenience.h"
#include <xcb/xcb_keysyms.h>
#include <X11/keysym.h>
#include <qpa/qwindowsysteminterface.h>
#include <QtCore/QTextCodec>
#include <QtCore/QMetaMethod>
#include <private/qguiapplication_p.h>
#include <stdio.h>

#include <qpa/qplatforminputcontext.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformcursor.h>

#ifndef XK_ISO_Left_Tab
#define XK_ISO_Left_Tab         0xFE20
#endif

#ifndef XK_dead_hook
#define XK_dead_hook            0xFE61
#endif

#ifndef XK_dead_horn
#define XK_dead_horn            0xFE62
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
#define XF86XK_AudioLowerVolume	   0x1008FF11
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
#define XF86XK_News                0x1008FF69
#define XF86XK_OfficeHome          0x1008FF6A
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
#define XF86XK_Suspend             0x1008FFA7
#define XF86XK_Hibernate           0x1008FFA8
#define XF86XK_TouchpadToggle      0x1008FFA9
#define XF86XK_TouchpadOn          0x1008FFB0
#define XF86XK_TouchpadOff         0x1008FFB1


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
    XK_Multi_key,		Qt::Key_Multi_key,
    XK_Codeinput,		Qt::Key_Codeinput,
    XK_SingleCandidate,		Qt::Key_SingleCandidate,
    XK_MultipleCandidate,	Qt::Key_MultipleCandidate,
    XK_PreviousCandidate,	Qt::Key_PreviousCandidate,

    // Misc Functions
    XK_Mode_switch,		Qt::Key_Mode_switch,
    XK_script_switch,		Qt::Key_Mode_switch,

    // Japanese keyboard support
    XK_Kanji,			Qt::Key_Kanji,
    XK_Muhenkan,		Qt::Key_Muhenkan,
    //XK_Henkan_Mode,		Qt::Key_Henkan_Mode,
    XK_Henkan_Mode,		Qt::Key_Henkan,
    XK_Henkan,			Qt::Key_Henkan,
    XK_Romaji,			Qt::Key_Romaji,
    XK_Hiragana,		Qt::Key_Hiragana,
    XK_Katakana,		Qt::Key_Katakana,
    XK_Hiragana_Katakana,	Qt::Key_Hiragana_Katakana,
    XK_Zenkaku,			Qt::Key_Zenkaku,
    XK_Hankaku,			Qt::Key_Hankaku,
    XK_Zenkaku_Hankaku,		Qt::Key_Zenkaku_Hankaku,
    XK_Touroku,			Qt::Key_Touroku,
    XK_Massyo,			Qt::Key_Massyo,
    XK_Kana_Lock,		Qt::Key_Kana_Lock,
    XK_Kana_Shift,		Qt::Key_Kana_Shift,
    XK_Eisu_Shift,		Qt::Key_Eisu_Shift,
    XK_Eisu_toggle,		Qt::Key_Eisu_toggle,
    //XK_Kanji_Bangou,		Qt::Key_Kanji_Bangou,
    //XK_Zen_Koho,		Qt::Key_Zen_Koho,
    //XK_Mae_Koho,		Qt::Key_Mae_Koho,
    XK_Kanji_Bangou,		Qt::Key_Codeinput,
    XK_Zen_Koho,		Qt::Key_MultipleCandidate,
    XK_Mae_Koho,		Qt::Key_PreviousCandidate,

#ifdef XK_KOREAN
    // Korean keyboard support
    XK_Hangul,			Qt::Key_Hangul,
    XK_Hangul_Start,		Qt::Key_Hangul_Start,
    XK_Hangul_End,		Qt::Key_Hangul_End,
    XK_Hangul_Hanja,		Qt::Key_Hangul_Hanja,
    XK_Hangul_Jamo,		Qt::Key_Hangul_Jamo,
    XK_Hangul_Romaja,		Qt::Key_Hangul_Romaja,
    //XK_Hangul_Codeinput,	Qt::Key_Hangul_Codeinput,
    XK_Hangul_Codeinput,	Qt::Key_Codeinput,
    XK_Hangul_Jeonja,		Qt::Key_Hangul_Jeonja,
    XK_Hangul_Banja,		Qt::Key_Hangul_Banja,
    XK_Hangul_PreHanja,		Qt::Key_Hangul_PreHanja,
    XK_Hangul_PostHanja,	Qt::Key_Hangul_PostHanja,
    //XK_Hangul_SingleCandidate,Qt::Key_Hangul_SingleCandidate,
    //XK_Hangul_MultipleCandidate,Qt::Key_Hangul_MultipleCandidate,
    //XK_Hangul_PreviousCandidate,Qt::Key_Hangul_PreviousCandidate,
    XK_Hangul_SingleCandidate,	Qt::Key_SingleCandidate,
    XK_Hangul_MultipleCandidate,Qt::Key_MultipleCandidate,
    XK_Hangul_PreviousCandidate,Qt::Key_PreviousCandidate,
    XK_Hangul_Special,		Qt::Key_Hangul_Special,
    //XK_Hangul_switch,		Qt::Key_Hangul_switch,
    XK_Hangul_switch,		Qt::Key_Mode_switch,
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
    XF86XK_News,                Qt::Key_News,
    XF86XK_OfficeHome,          Qt::Key_OfficeHome,
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
    XF86XK_Bluetooth,           Qt::Key_Bluetooth,
    XF86XK_Suspend,             Qt::Key_Suspend,
    XF86XK_Hibernate,           Qt::Key_Hibernate,
    XF86XK_TouchpadToggle,      Qt::Key_TouchpadToggle,
    XF86XK_TouchpadOn,          Qt::Key_TouchpadOn,
    XF86XK_TouchpadOff,         Qt::Key_TouchpadOff,
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

static const unsigned short katakanaKeysymsToUnicode[] = {
    0x0000, 0x3002, 0x300C, 0x300D, 0x3001, 0x30FB, 0x30F2, 0x30A1,
    0x30A3, 0x30A5, 0x30A7, 0x30A9, 0x30E3, 0x30E5, 0x30E7, 0x30C3,
    0x30FC, 0x30A2, 0x30A4, 0x30A6, 0x30A8, 0x30AA, 0x30AB, 0x30AD,
    0x30AF, 0x30B1, 0x30B3, 0x30B5, 0x30B7, 0x30B9, 0x30BB, 0x30BD,
    0x30BF, 0x30C1, 0x30C4, 0x30C6, 0x30C8, 0x30CA, 0x30CB, 0x30CC,
    0x30CD, 0x30CE, 0x30CF, 0x30D2, 0x30D5, 0x30D8, 0x30DB, 0x30DE,
    0x30DF, 0x30E0, 0x30E1, 0x30E2, 0x30E4, 0x30E6, 0x30E8, 0x30E9,
    0x30EA, 0x30EB, 0x30EC, 0x30ED, 0x30EF, 0x30F3, 0x309B, 0x309C
};

static const unsigned short cyrillicKeysymsToUnicode[] = {
    0x0000, 0x0452, 0x0453, 0x0451, 0x0454, 0x0455, 0x0456, 0x0457,
    0x0458, 0x0459, 0x045a, 0x045b, 0x045c, 0x0000, 0x045e, 0x045f,
    0x2116, 0x0402, 0x0403, 0x0401, 0x0404, 0x0405, 0x0406, 0x0407,
    0x0408, 0x0409, 0x040a, 0x040b, 0x040c, 0x0000, 0x040e, 0x040f,
    0x044e, 0x0430, 0x0431, 0x0446, 0x0434, 0x0435, 0x0444, 0x0433,
    0x0445, 0x0438, 0x0439, 0x043a, 0x043b, 0x043c, 0x043d, 0x043e,
    0x043f, 0x044f, 0x0440, 0x0441, 0x0442, 0x0443, 0x0436, 0x0432,
    0x044c, 0x044b, 0x0437, 0x0448, 0x044d, 0x0449, 0x0447, 0x044a,
    0x042e, 0x0410, 0x0411, 0x0426, 0x0414, 0x0415, 0x0424, 0x0413,
    0x0425, 0x0418, 0x0419, 0x041a, 0x041b, 0x041c, 0x041d, 0x041e,
    0x041f, 0x042f, 0x0420, 0x0421, 0x0422, 0x0423, 0x0416, 0x0412,
    0x042c, 0x042b, 0x0417, 0x0428, 0x042d, 0x0429, 0x0427, 0x042a
};

static const unsigned short greekKeysymsToUnicode[] = {
    0x0000, 0x0386, 0x0388, 0x0389, 0x038a, 0x03aa, 0x0000, 0x038c,
    0x038e, 0x03ab, 0x0000, 0x038f, 0x0000, 0x0000, 0x0385, 0x2015,
    0x0000, 0x03ac, 0x03ad, 0x03ae, 0x03af, 0x03ca, 0x0390, 0x03cc,
    0x03cd, 0x03cb, 0x03b0, 0x03ce, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397,
    0x0398, 0x0399, 0x039a, 0x039b, 0x039c, 0x039d, 0x039e, 0x039f,
    0x03a0, 0x03a1, 0x03a3, 0x0000, 0x03a4, 0x03a5, 0x03a6, 0x03a7,
    0x03a8, 0x03a9, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x03b1, 0x03b2, 0x03b3, 0x03b4, 0x03b5, 0x03b6, 0x03b7,
    0x03b8, 0x03b9, 0x03ba, 0x03bb, 0x03bc, 0x03bd, 0x03be, 0x03bf,
    0x03c0, 0x03c1, 0x03c3, 0x03c2, 0x03c4, 0x03c5, 0x03c6, 0x03c7,
    0x03c8, 0x03c9, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
};

static const unsigned short technicalKeysymsToUnicode[] = {
    0x0000, 0x23B7, 0x250C, 0x2500, 0x2320, 0x2321, 0x2502, 0x23A1,
    0x23A3, 0x23A4, 0x23A6, 0x239B, 0x239D, 0x239E, 0x23A0, 0x23A8,
    0x23AC, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x2264, 0x2260, 0x2265, 0x222B,
    0x2234, 0x221D, 0x221E, 0x0000, 0x0000, 0x2207, 0x0000, 0x0000,
    0x223C, 0x2243, 0x0000, 0x0000, 0x0000, 0x21D4, 0x21D2, 0x2261,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x221A, 0x0000,
    0x0000, 0x0000, 0x2282, 0x2283, 0x2229, 0x222A, 0x2227, 0x2228,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x2202,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0192, 0x0000,
    0x0000, 0x0000, 0x0000, 0x2190, 0x2191, 0x2192, 0x2193, 0x0000
};

static const unsigned short specialKeysymsToUnicode[] = {
    0x25C6, 0x2592, 0x2409, 0x240C, 0x240D, 0x240A, 0x0000, 0x0000,
    0x2424, 0x240B, 0x2518, 0x2510, 0x250C, 0x2514, 0x253C, 0x23BA,
    0x23BB, 0x2500, 0x23BC, 0x23BD, 0x251C, 0x2524, 0x2534, 0x252C,
    0x2502, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
};

static const unsigned short publishingKeysymsToUnicode[] = {
    0x0000, 0x2003, 0x2002, 0x2004, 0x2005, 0x2007, 0x2008, 0x2009,
    0x200a, 0x2014, 0x2013, 0x0000, 0x0000, 0x0000, 0x2026, 0x2025,
    0x2153, 0x2154, 0x2155, 0x2156, 0x2157, 0x2158, 0x2159, 0x215a,
    0x2105, 0x0000, 0x0000, 0x2012, 0x2329, 0x0000, 0x232a, 0x0000,
    0x0000, 0x0000, 0x0000, 0x215b, 0x215c, 0x215d, 0x215e, 0x0000,
    0x0000, 0x2122, 0x2613, 0x0000, 0x25c1, 0x25b7, 0x25cb, 0x25af,
    0x2018, 0x2019, 0x201c, 0x201d, 0x211e, 0x0000, 0x2032, 0x2033,
    0x0000, 0x271d, 0x0000, 0x25ac, 0x25c0, 0x25b6, 0x25cf, 0x25ae,
    0x25e6, 0x25ab, 0x25ad, 0x25b3, 0x25bd, 0x2606, 0x2022, 0x25aa,
    0x25b2, 0x25bc, 0x261c, 0x261e, 0x2663, 0x2666, 0x2665, 0x0000,
    0x2720, 0x2020, 0x2021, 0x2713, 0x2717, 0x266f, 0x266d, 0x2642,
    0x2640, 0x260e, 0x2315, 0x2117, 0x2038, 0x201a, 0x201e, 0x0000
};

static const unsigned short aplKeysymsToUnicode[] = {
    0x0000, 0x0000, 0x0000, 0x003c, 0x0000, 0x0000, 0x003e, 0x0000,
    0x2228, 0x2227, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x00af, 0x0000, 0x22a5, 0x2229, 0x230a, 0x0000, 0x005f, 0x0000,
    0x0000, 0x0000, 0x2218, 0x0000, 0x2395, 0x0000, 0x22a4, 0x25cb,
    0x0000, 0x0000, 0x0000, 0x2308, 0x0000, 0x0000, 0x222a, 0x0000,
    0x2283, 0x0000, 0x2282, 0x0000, 0x22a2, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x22a3, 0x0000, 0x0000, 0x0000
};

static const unsigned short koreanKeysymsToUnicode[] = {
    0x0000, 0x3131, 0x3132, 0x3133, 0x3134, 0x3135, 0x3136, 0x3137,
    0x3138, 0x3139, 0x313a, 0x313b, 0x313c, 0x313d, 0x313e, 0x313f,
    0x3140, 0x3141, 0x3142, 0x3143, 0x3144, 0x3145, 0x3146, 0x3147,
    0x3148, 0x3149, 0x314a, 0x314b, 0x314c, 0x314d, 0x314e, 0x314f,
    0x3150, 0x3151, 0x3152, 0x3153, 0x3154, 0x3155, 0x3156, 0x3157,
    0x3158, 0x3159, 0x315a, 0x315b, 0x315c, 0x315d, 0x315e, 0x315f,
    0x3160, 0x3161, 0x3162, 0x3163, 0x11a8, 0x11a9, 0x11aa, 0x11ab,
    0x11ac, 0x11ad, 0x11ae, 0x11af, 0x11b0, 0x11b1, 0x11b2, 0x11b3,
    0x11b4, 0x11b5, 0x11b6, 0x11b7, 0x11b8, 0x11b9, 0x11ba, 0x11bb,
    0x11bc, 0x11bd, 0x11be, 0x11bf, 0x11c0, 0x11c1, 0x11c2, 0x316d,
    0x3171, 0x3178, 0x317f, 0x3181, 0x3184, 0x3186, 0x318d, 0x318e,
    0x11eb, 0x11f0, 0x11f9, 0x0000, 0x0000, 0x0000, 0x0000, 0x20a9
};

static QChar keysymToUnicode(unsigned char byte3, unsigned char byte4)
{
    switch (byte3) {
    case 0x04:
        // katakana
        if (byte4 > 0xa0 && byte4 < 0xe0)
            return QChar(katakanaKeysymsToUnicode[byte4 - 0xa0]);
        else if (byte4 == 0x7e)
            return QChar(0x203e); // Overline
        break;
    case 0x06:
        // russian, use lookup table
        if (byte4 > 0xa0)
            return QChar(cyrillicKeysymsToUnicode[byte4 - 0xa0]);
        break;
    case 0x07:
        // greek
        if (byte4 > 0xa0)
            return QChar(greekKeysymsToUnicode[byte4 - 0xa0]);
        break;
    case 0x08:
        // technical
        if (byte4 > 0xa0)
            return QChar(technicalKeysymsToUnicode[byte4 - 0xa0]);
        break;
    case 0x09:
        // special
        if (byte4 >= 0xe0)
            return QChar(specialKeysymsToUnicode[byte4 - 0xe0]);
        break;
    case 0x0a:
        // publishing
        if (byte4 > 0xa0)
            return QChar(publishingKeysymsToUnicode[byte4 - 0xa0]);
        break;
    case 0x0b:
        // APL
        if (byte4 > 0xa0)
            return QChar(aplKeysymsToUnicode[byte4 - 0xa0]);
        break;
    case 0x0e:
        // Korean
        if (byte4 > 0xa0)
            return QChar(koreanKeysymsToUnicode[byte4 - 0xa0]);
        break;
    default:
        break;
    }
    return QChar(0x0);
}

Qt::KeyboardModifiers QXcbKeyboard::translateModifiers(int s)
{
    Qt::KeyboardModifiers ret = 0;
    if (s & XCB_MOD_MASK_SHIFT)
        ret |= Qt::ShiftModifier;
    if (s & XCB_MOD_MASK_CONTROL)
        ret |= Qt::ControlModifier;
    if (s & m_alt_mask)
        ret |= Qt::AltModifier;
    if (s & m_meta_mask)
        ret |= Qt::MetaModifier;
    return ret;
}

int QXcbKeyboard::translateKeySym(uint key) const
{
    int code = -1;
    int i = 0;                                // any other keys
    while (KeyTbl[i]) {
        if (key == KeyTbl[i]) {
            code = (int)KeyTbl[i+1];
            break;
        }
        i += 2;
    }
    if (m_meta_mask) {
        // translate Super/Hyper keys to Meta if we're using them as the MetaModifier
        if (m_meta_mask == m_super_mask && (code == Qt::Key_Super_L || code == Qt::Key_Super_R)) {
            code = Qt::Key_Meta;
        } else if (m_meta_mask == m_hyper_mask && (code == Qt::Key_Hyper_L || code == Qt::Key_Hyper_R)) {
            code = Qt::Key_Meta;
        }
    }
    return code;
}

QString QXcbKeyboard::translateKeySym(xcb_keysym_t keysym, uint xmodifiers,
                                      int &code, Qt::KeyboardModifiers &modifiers,
                                      QByteArray &chars, int &count)
{
    // all keysyms smaller than 0xff00 are actally keys that can be mapped to unicode chars

    QTextCodec *mapper = QTextCodec::codecForLocale();
    QChar converted;

    if (/*count == 0 &&*/ keysym < 0xff00) {
        unsigned char byte3 = (unsigned char)(keysym >> 8);
        int mib = -1;
        switch(byte3) {
        case 0: // Latin 1
        case 1: // Latin 2
        case 2: //latin 3
        case 3: // latin4
            mib = byte3 + 4; break;
        case 5: // arabic
            mib = 82; break;
        case 12: // Hebrew
            mib = 85; break;
        case 13: // Thai
            mib = 2259; break;
        case 4: // kana
        case 6: // cyrillic
        case 7: // greek
        case 8: // technical, no mapping here at the moment
        case 9: // Special
        case 10: // Publishing
        case 11: // APL
        case 14: // Korean, no mapping
            mib = -1; // manual conversion
            mapper= 0;
#if !defined(QT_NO_XIM)
            converted = keysymToUnicode(byte3, keysym & 0xff);
#endif
        case 0x20:
            // currency symbols
            if (keysym >= 0x20a0 && keysym <= 0x20ac) {
                mib = -1; // manual conversion
                mapper = 0;
                converted = (uint)keysym;
            }
            break;
        default:
            break;
        }
        if (mib != -1) {
            mapper = QTextCodec::codecForMib(mib);
            if (chars.isEmpty())
                chars.resize(1);
            chars[0] = (unsigned char) (keysym & 0xff); // get only the fourth bit for conversion later
            count = 1;
        }
    } else if (keysym >= 0x1000000 && keysym <= 0x100ffff) {
        converted = (ushort) (keysym - 0x1000000);
        mapper = 0;
    }
    if (count < (int)chars.size()-1)
        chars[count] = '\0';

    QString text;
    if (!mapper && converted.unicode() != 0x0) {
        text = converted;
    } else if (!chars.isEmpty()) {
        // convert chars (8bit) to text (unicode).
        if (mapper)
            text = mapper->toUnicode(chars.data(), count, 0);
        if (text.isEmpty()) {
            // no mapper, or codec couldn't convert to unicode (this
            // can happen when running in the C locale or with no LANG
            // set). try converting from latin-1
            text = QString::fromLatin1(chars);
        }
    }

    modifiers = translateModifiers(xmodifiers);

    // Commentary in X11/keysymdef says that X codes match ASCII, so it
    // is safe to use the locale functions to process X codes in ISO8859-1.
    //
    // This is mainly for compatibility - applications should not use the
    // Qt keycodes between 128 and 255, but should rather use the
    // QKeyEvent::text().
    //
    if (keysym < 128 || (keysym < 256 && (!mapper || mapper->mibEnum()==4))) {
        // upper-case key, if known
        code = isprint((int)keysym) ? toupper((int)keysym) : 0;
    } else if (keysym >= XK_F1 && keysym <= XK_F35) {
        // function keys
        code = Qt::Key_F1 + ((int)keysym - XK_F1);
    } else if (keysym >= XK_KP_Space && keysym <= XK_KP_9) {
        if (keysym >= XK_KP_0) {
            // numeric keypad keys
            code = Qt::Key_0 + ((int)keysym - XK_KP_0);
        } else {
            code = translateKeySym(keysym);
        }
        modifiers |= Qt::KeypadModifier;
    } else if (text.length() == 1 && text.unicode()->unicode() > 0x1f && text.unicode()->unicode() != 0x7f && !(keysym >= XK_dead_grave && keysym <= XK_dead_horn)) {
        code = text.unicode()->toUpper().unicode();
    } else {
        // any other keys
        code = translateKeySym(keysym);

        if (code == Qt::Key_Tab && (modifiers & Qt::ShiftModifier)) {
            // map shift+tab to shift+backtab, QShortcutMap knows about it
            // and will handle it.
            code = Qt::Key_Backtab;
            text = QString();
        }
    }

    return text;
}

QXcbKeyboard::QXcbKeyboard(QXcbConnection *connection)
    : QXcbObject(connection)
    , m_autorepeat_code(0)
{
    m_key_symbols = xcb_key_symbols_alloc(xcb_connection());
    setupModifiers();
}

QXcbKeyboard::~QXcbKeyboard()
{
    xcb_key_symbols_free(m_key_symbols);
}

void QXcbKeyboard::setupModifiers()
{
    m_alt_mask = 0;
    m_super_mask = 0;
    m_hyper_mask = 0;
    m_meta_mask = 0;
    m_mode_switch_mask = 0;
    m_num_lock_mask = 0;
    m_caps_lock_mask = 0;

    xcb_generic_error_t *error = 0;
    xcb_connection_t *conn = xcb_connection();
    xcb_get_modifier_mapping_cookie_t modMapCookie = xcb_get_modifier_mapping(conn);
    xcb_get_modifier_mapping_reply_t *modMapReply =
        xcb_get_modifier_mapping_reply(conn, modMapCookie, &error);
    if (error) {
        qWarning("QXcbKeyboard: failed to get modifier mapping");
        free(error);
        return;
    }

    // for Alt and Meta L and R are the same
    static const xcb_keysym_t symbols[] = {
        XK_Alt_L, XK_Meta_L, XK_Super_L, XK_Super_R,
        XK_Hyper_L, XK_Hyper_R, XK_Num_Lock, XK_Mode_switch, XK_Caps_Lock,
    };
    static const size_t numSymbols = sizeof symbols / sizeof *symbols;

    // Figure out the modifier mapping, ICCCM 6.6
    xcb_keycode_t* modKeyCodes[numSymbols];
    for (size_t i = 0; i < numSymbols; ++i)
        modKeyCodes[i] = xcb_key_symbols_get_keycode(m_key_symbols, symbols[i]);

    xcb_keycode_t *modMap = xcb_get_modifier_mapping_keycodes(modMapReply);
    const int w = modMapReply->keycodes_per_modifier;
    for (size_t i = 0; i < numSymbols; ++i) {
        for (int bit = 0; bit < 8; ++bit) {
            uint mask = 1 << bit;
            for (int x = 0; x < w; ++x) {
                xcb_keycode_t keyCode = modMap[x + bit * w];
                xcb_keycode_t *itk = modKeyCodes[i];
                while (itk && *itk != XCB_NO_SYMBOL)
                    if (*itk++ == keyCode)
                        setMask(symbols[i], mask);
            }
        }
    }

    for (size_t i = 0; i < numSymbols; ++i)
        free(modKeyCodes[i]);
    free(modMapReply);
}

void QXcbKeyboard::setMask(uint sym, uint mask)
{
    if (m_alt_mask == 0
        && m_meta_mask != mask
        && m_super_mask != mask
        && m_hyper_mask != mask
        && (sym == XK_Alt_L || sym == XK_Alt_R))
        m_alt_mask = mask;

    if (m_meta_mask == 0
        && m_alt_mask != mask
        && m_super_mask != mask
        && m_hyper_mask != mask
        && (sym == XK_Meta_L || sym == XK_Meta_R))
        m_meta_mask = mask;

    if (m_super_mask == 0
        && m_alt_mask != mask
        && m_meta_mask != mask
        && m_hyper_mask != mask
        && (sym == XK_Super_L || sym == XK_Super_R))
        m_super_mask = mask;

    if (m_hyper_mask == 0
        && m_alt_mask != mask
        && m_meta_mask != mask
        && m_super_mask != mask
        && (sym == XK_Hyper_L || sym == XK_Hyper_R))
        m_hyper_mask = mask;

    if (m_mode_switch_mask == 0
        && m_alt_mask != mask
        && m_meta_mask != mask
        && m_super_mask != mask
        && m_hyper_mask != mask
        && sym == XK_Mode_switch)
        m_mode_switch_mask = mask;

    if (m_num_lock_mask == 0 && sym == XK_Num_Lock)
        m_num_lock_mask = mask;

    if (m_caps_lock_mask == 0 && sym == XK_Caps_Lock)
        m_caps_lock_mask = mask;
}

// #define XCB_KEYBOARD_DEBUG

class KeyChecker
{
public:
    KeyChecker(xcb_window_t window, xcb_keycode_t code, xcb_timestamp_t time)
        : m_window(window)
        , m_code(code)
        , m_time(time)
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

        if (event->event != m_window || event->detail != m_code) {
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

    bool m_error;
    bool m_release;
};

void QXcbKeyboard::handleKeyEvent(QWindow *window, QEvent::Type type, xcb_keycode_t code,
                                  quint16 state, xcb_timestamp_t time)
{
    Q_XCB_NOOP(connection());
#ifdef XCB_KEYBOARD_DEBUG
    printf("key code: %d, state: %d, syms: ", code, state);
    for (int i = 0; i <= 5; ++i) {
        printf("%d ", xcb_key_symbols_get_keysym(m_key_symbols, code, i));
    }
    printf("\n");
#endif

    QByteArray chars;
    xcb_keysym_t sym = lookupString(window, state, code, type, &chars);
    QPlatformInputContext *inputContext = QGuiApplicationPrivate::platformIntegration()->inputContext();
    QMetaMethod method;

    if (inputContext) {
        int methodIndex = inputContext->metaObject()->indexOfMethod("x11FilterEvent(uint,uint,uint,bool)");
        if (methodIndex != -1)
            method = inputContext->metaObject()->method(methodIndex);
    }

    if (method.isValid()) {
        bool retval = false;
        method.invoke(inputContext, Qt::DirectConnection,
                      Q_RETURN_ARG(bool, retval),
                      Q_ARG(uint, sym),
                      Q_ARG(uint, code),
                      Q_ARG(uint, state),
                      Q_ARG(bool, type == QEvent::KeyPress));
        if (retval)
            return;
    }

    Qt::KeyboardModifiers modifiers;
    int qtcode = 0;
    int count = chars.count();
    QString string = translateKeySym(sym, state, qtcode, modifiers, chars, count);

    bool isAutoRepeat = false;

    if (type == QEvent::KeyPress) {
        if (m_autorepeat_code == code) {
            isAutoRepeat = true;
            m_autorepeat_code = 0;
        }
    } else {
        // look ahead for auto-repeat
        KeyChecker checker(((QXcbWindow *)window->handle())->xcb_window(), code, time);
        xcb_generic_event_t *event = connection()->checkEvent(checker);
        if (event) {
            isAutoRepeat = true;
            free(event);
        }
        m_autorepeat_code = isAutoRepeat ? code : 0;
    }

    bool filtered = false;
    if (inputContext) {
        QKeyEvent event(type, qtcode, modifiers, code, sym, state, string.left(count), isAutoRepeat, count);
        event.setTimestamp(time);
        filtered = inputContext->filterEvent(&event);
    }

    if (!filtered) {
        if (type == QEvent::KeyPress && qtcode == Qt::Key_Menu) {
            const QPoint globalPos = window->screen()->handle()->cursor()->pos();
            const QPoint pos = window->mapFromGlobal(globalPos);
            QWindowSystemInterface::handleContextMenuEvent(window, false, pos, globalPos, modifiers);
        }
        QWindowSystemInterface::handleExtendedKeyEvent(window, time, type, qtcode, modifiers,
                                                       code, sym, state, string.left(count), isAutoRepeat);
    }

    if (isAutoRepeat && type == QEvent::KeyRelease) {
        // since we removed it from the event queue using checkEvent we need to send the key press here
        filtered = false;
        if (method.isValid()) {
            method.invoke(inputContext, Qt::DirectConnection,
                          Q_RETURN_ARG(bool, filtered),
                          Q_ARG(uint, sym),
                          Q_ARG(uint, code),
                          Q_ARG(uint, state),
                          Q_ARG(bool, true));
        }

        if (!filtered && inputContext) {
            QKeyEvent event(QEvent::KeyPress, qtcode, modifiers, code, sym, state, string.left(count), isAutoRepeat, count);
            event.setTimestamp(time);
            filtered = inputContext->filterEvent(&event);
        }
        if (!filtered)
            QWindowSystemInterface::handleExtendedKeyEvent(window, time, QEvent::KeyPress, qtcode, modifiers,
                                                           code, sym, state, string.left(count), isAutoRepeat);
    }
}

xcb_keysym_t QXcbKeyboard::lookupString(QWindow *window, uint state, xcb_keycode_t code,
                                        QEvent::Type type, QByteArray *chars)
{
#ifdef XCB_USE_XLIB
    xcb_window_t xWindow = static_cast<QXcbWindow *>(window->handle())->xcb_window();
    xcb_window_t root = connection()->screens().at(0)->root();
    void *xDisplay = connection()->xlib_display();
    int xType = (type == QEvent::KeyRelease ? 3 : 2);
    return q_XLookupString(xDisplay, xWindow, root, state, code, xType, chars);
#else

    // No XLookupString available. The following is really incomplete...

    int col = state & XCB_MOD_MASK_SHIFT ? 1 : 0;
    const int altGrOffset = 4;
    if (state & 128)
        col += altGrOffset;
    xcb_keysym_t sym = xcb_key_symbols_get_keysym(m_key_symbols, code, col);
    if (sym == XCB_NO_SYMBOL)
        sym = xcb_key_symbols_get_keysym(m_key_symbols, code, col ^ 0x1);
    if (state & XCB_MOD_MASK_LOCK && sym <= 0x7f && isprint(sym)) {
        if (isupper(sym))
            sym = tolower(sym);
        else
            sym = toupper(sym);
    }
    return sym;

#endif
}

void QXcbKeyboard::handleKeyPressEvent(QXcbWindow *window, const xcb_key_press_event_t *event)
{
    window->updateNetWmUserTime(event->time);
    handleKeyEvent(window->window(), QEvent::KeyPress, event->detail, event->state, event->time);
}

void QXcbKeyboard::handleKeyReleaseEvent(QXcbWindow *window, const xcb_key_release_event_t *event)
{
    handleKeyEvent(window->window(), QEvent::KeyRelease, event->detail, event->state, event->time);
}

void QXcbKeyboard::handleMappingNotifyEvent(const xcb_mapping_notify_event_t *event)
{
    xcb_refresh_keyboard_mapping(m_key_symbols, const_cast<xcb_mapping_notify_event_t *>(event));
    setupModifiers();
}

QT_END_NAMESPACE
