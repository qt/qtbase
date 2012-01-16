/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef OPENKODEKEYTRANSLATOR_H
#define OPENKODEKEYTRANSLATOR_H

#ifdef KD_ATX_keyboard

#include <KD/ATX_keyboard.h>

QT_BEGIN_NAMESPACE

Qt::Key keyTranslator( int key )
{
    switch (key) {
// KD_KEY_ACCEPT_ATX:
// KD_KEY_AGAIN_ATX:
// KD_KEY_ALLCANDIDATES_ATX
// KD_KEY_ALPHANUMERIC_ATX
    case KD_KEY_ALT_ATX:
        return Qt::Key_Alt;
    case KD_KEY_ALTGRAPH_ATX:
        return Qt::Key_AltGr;
// KD_KEY_APPS_ATX
// KD_KEY_ATTN_ATX
// KD_KEY_BROWSERBACK_ATX
// KD_KEY_BROWSERFAVORITES_ATX
// KD_KEY_BROWSERFORWARD_ATX
// KD_KEY_BROWSERHOME_ATX
// KD_KEY_BROWSERREFRESH_ATX
// KD_KEY_BROWSERSEARCH_ATX
// KD_KEY_BROWSERSTOP_ATX
    case KD_KEY_CAPSLOCK_ATX:
        return Qt::Key_CapsLock;
    case KD_KEY_CLEAR_ATX:
        return Qt::Key_Clear;
    case KD_KEY_CODEINPUT_ATX:
        return Qt::Key_Codeinput;
// KD_KEY_COMPOSE_ATX
    case KD_KEY_CONTROL_ATX:
        return Qt::Key_Control;
// KD_KEY_CRSEL_ATX
// KD_KEY_CONVERT_ATX
    case KD_KEY_COPY_ATX:
        return Qt::Key_Copy;
    case KD_KEY_CUT_ATX:
        return Qt::Key_Cut;
    case KD_KEY_DOWN_ATX:
        return Qt::Key_Down;
    case KD_KEY_END_ATX:
        return Qt::Key_End;
    case KD_KEY_ENTER_ATX:
        return Qt::Key_Enter;
// KD_KEY_ERASEEOF_ATX
// KD_KEY_EXECUTE_ATX
// KD_KEY_EXSEL_ATX
    case KD_KEY_F1_ATX:
        return Qt::Key_F1;
    case KD_KEY_F2_ATX:
        return Qt::Key_F2;
    case KD_KEY_F3_ATX:
        return Qt::Key_F3;
    case KD_KEY_F4_ATX:
        return Qt::Key_F4;
    case KD_KEY_F5_ATX:
        return Qt::Key_F5;
    case KD_KEY_F6_ATX:
        return Qt::Key_F6;
    case KD_KEY_F7_ATX:
        return Qt::Key_F7;
    case KD_KEY_F8_ATX:
        return Qt::Key_F8;
    case KD_KEY_F9_ATX:
        return Qt::Key_F9;
    case KD_KEY_F10_ATX:
        return Qt::Key_F10;
    case KD_KEY_F11_ATX:
        return Qt::Key_F11;
    case KD_KEY_F12_ATX:
        return Qt::Key_F12;
    case KD_KEY_F13_ATX:
        return Qt::Key_F13;
    case KD_KEY_F14_ATX:
        return Qt::Key_F14;
    case KD_KEY_F15_ATX:
        return Qt::Key_F15;
    case KD_KEY_F16_ATX:
        return Qt::Key_F16;
    case KD_KEY_F17_ATX:
        return Qt::Key_F17;
    case KD_KEY_F18_ATX:
        return Qt::Key_F18;
    case KD_KEY_F19_ATX:
        return Qt::Key_F19;
    case KD_KEY_F20_ATX:
        return Qt::Key_F20;
    case KD_KEY_F21_ATX:
        return Qt::Key_F21;
    case KD_KEY_F22_ATX:
        return Qt::Key_F22;
    case KD_KEY_F23_ATX:
        return Qt::Key_F23;
    case KD_KEY_F24_ATX:
        return Qt::Key_F24;
// KD_KEY_FINALMODE_ATX
// KD_KEY_FIND_ATX
// KD_KEY_FULLWIDTH_ATX
// KD_KEY_HALFWIDTH_ATX
    case KD_KEY_HANGULMODE_ATX:
        return Qt::Key_Hangul;
// KD_KEY_HANJAMODE_ATX
    case KD_KEY_HELP_ATX:
        return Qt::Key_Help;
    case KD_KEY_HIRAGANA_ATX:
        return Qt::Key_Hiragana;
    case KD_KEY_HOME_ATX:
        return Qt::Key_Home;
    case KD_KEY_INSERT_ATX:
        return Qt::Key_Insert;
// KD_KEY_JAPANESEHIRAGANA_ATX:
// KD_KEY_JAPANESEKATAKANA_ATX
// KD_KEY_JAPANESEROMAJI_ATX
// KD_KEY_JUNJAMODE_ATX
    case KD_KEY_KANAMODE_ATX:
        return Qt::Key_Kana_Lock; //?
    case KD_KEY_KANJIMODE_ATX:
        return Qt::Key_Kanji;
// KD_KEY_KATAKANA_ATX
// KD_KEY_LAUNCHAPPLICATION1_ATX
// KD_KEY_LAUNCHAPPLICATION2_ATX
    case KD_KEY_LAUNCHMAIL_ATX:
        return Qt::Key_MailForward;
    case KD_KEY_LEFT_ATX:
        return Qt::Key_Left;
    case KD_KEY_META_ATX:
        return Qt::Key_Meta;
    case KD_KEY_MEDIANEXTTRACK_ATX:
        return Qt::Key_MediaNext;
    case KD_KEY_MEDIAPLAYPAUSE_ATX:
        return Qt::Key_MediaPause;
    case KD_KEY_MEDIAPREVIOUSTRACK_ATX:
        return Qt::Key_MediaPrevious;
    case KD_KEY_MEDIASTOP_ATX:
        return Qt::Key_MediaStop;
    case KD_KEY_MODECHANGE_ATX:
        return Qt::Key_Mode_switch;
// KD_KEY_NONCONVERT_ATX
    case KD_KEY_NUMLOCK_ATX:
        return Qt::Key_NumLock;
    case KD_KEY_PAGEDOWN_ATX:
        return Qt::Key_PageDown;
    case KD_KEY_PAGEUP_ATX:
        return Qt::Key_PageUp;
    case KD_KEY_PASTE_ATX:
        return Qt::Key_Paste;
    case KD_KEY_PAUSE_ATX:
        return Qt::Key_Pause;
    case KD_KEY_PLAY_ATX:
        return Qt::Key_Play;
// KD_KEY_PREVIOUSCANDIDATE_ATX
    case KD_KEY_PRINTSCREEN_ATX:
        return Qt::Key_Print;
// case KD_KEY_PROCESS_ATX
// case KD_KEY_PROPS_ATX
    case KD_KEY_RIGHT_ATX:
        return Qt::Key_Right;
// KD_KEY_ROMANCHARACTERS_ATX
    case KD_KEY_SCROLL_ATX:
        return Qt::Key_ScrollLock;
    case KD_KEY_SELECT_ATX:
        return Qt::Key_Select;
// KD_KEY_SELECTMEDIA_ATX
    case KD_KEY_SHIFT_ATX:
        return Qt::Key_Shift;
    case KD_KEY_STOP_ATX:
        return Qt::Key_Stop;
    case KD_KEY_UP_ATX:
        return Qt::Key_Up;
// KD_KEY_UNDO_ATX
    case KD_KEY_VOLUMEDOWN_ATX:
        return Qt::Key_VolumeDown;
    case KD_KEY_VOLUMEMUTE_ATX:
        return Qt::Key_VolumeMute;
    case KD_KEY_VOLUMEUP_ATX:
        return Qt::Key_VolumeUp;
    case KD_KEY_WIN_ATX:
        return Qt::Key_Meta;
    case KD_KEY_ZOOM_ATX:
        return Qt::Key_Zoom;
    case 0x8:
        return Qt::Key_Backspace;
    case 0x1b:
        return Qt::Key_Escape;
    case 0x9:
        return Qt::Key_Tab;

    default:
        break;
    }

    return Qt::Key_Escape;
}

QT_END_NAMESPACE
#endif //KD_ATX_keyboard
#endif // OPENKODEKEYTRANSLATOR_H
