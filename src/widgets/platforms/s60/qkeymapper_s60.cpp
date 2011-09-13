/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "private/qkeymapper_p.h"
#include <private/qcore_symbian_p.h>
#include <e32keys.h>
#include <e32cmn.h>
#include <centralrepository.h>
#include <biditext.h>

QT_BEGIN_NAMESPACE

QKeyMapperPrivate::QKeyMapperPrivate()
{
}

QKeyMapperPrivate::~QKeyMapperPrivate()
{
}

QList<int> QKeyMapperPrivate::possibleKeys(QKeyEvent * /* e */)
{
    QList<int> result;
    return result;
}

void QKeyMapperPrivate::clearMappings()
{
    // stub
}

QString QKeyMapperPrivate::translateKeyEvent(int keySym, Qt::KeyboardModifiers /* modifiers */)
{
    if (keySym >= Qt::Key_Escape) {
        switch (keySym) {
        case Qt::Key_Tab:
            return QString(QChar('\t'));
        case Qt::Key_Return:    // fall through
        case Qt::Key_Enter:
            return QString(QChar('\r'));
        default:
            return QString();
        }
    }

    // Symbian doesn't actually use modifiers, but gives us the character code directly.

    return QString(QChar(keySym));
}

#include <e32keys.h>
struct KeyMapping{
    TKeyCode s60KeyCode;
    TStdScanCode s60ScanCode;
    Qt::Key qtKey;
};

using namespace Qt;

static const KeyMapping keyMapping[] = {
    {EKeyBackspace, EStdKeyBackspace, Key_Backspace},
    {EKeyTab, EStdKeyTab, Key_Tab},
    {EKeyEnter, EStdKeyEnter, Key_Enter},
    {EKeyEscape, EStdKeyEscape, Key_Escape},
    {EKeySpace, EStdKeySpace, Key_Space},
    {EKeyDelete, EStdKeyDelete, Key_Delete},
    {EKeyPrintScreen, EStdKeyPrintScreen, Key_SysReq},
    {EKeyPause, EStdKeyPause, Key_Pause},
    {EKeyHome, EStdKeyHome, Key_Home},
    {EKeyEnd, EStdKeyEnd, Key_End},
    {EKeyPageUp, EStdKeyPageUp, Key_PageUp},
    {EKeyPageDown, EStdKeyPageDown, Key_PageDown},
    {EKeyInsert, EStdKeyInsert, Key_Insert},
    {EKeyLeftArrow, EStdKeyLeftArrow, Key_Left},
    {EKeyRightArrow, EStdKeyRightArrow, Key_Right},
    {EKeyUpArrow, EStdKeyUpArrow, Key_Up},
    {EKeyDownArrow, EStdKeyDownArrow, Key_Down},
    {EKeyLeftShift, EStdKeyLeftShift, Key_Shift},
    {EKeyRightShift, EStdKeyRightShift, Key_Shift},
    {EKeyLeftAlt, EStdKeyLeftAlt, Key_Alt},
    {EKeyRightAlt, EStdKeyRightAlt, Key_AltGr},
    {EKeyLeftCtrl, EStdKeyLeftCtrl, Key_Control},
    {EKeyRightCtrl, EStdKeyRightCtrl, Key_Control},
    {EKeyLeftFunc, EStdKeyLeftFunc, Key_Super_L},
    {EKeyRightFunc, EStdKeyRightFunc, Key_Super_R},
    {EKeyCapsLock, EStdKeyCapsLock, Key_CapsLock},
    {EKeyNumLock, EStdKeyNumLock, Key_NumLock},
    {EKeyScrollLock, EStdKeyScrollLock, Key_ScrollLock},
    {EKeyF1, EStdKeyF1, Key_F1},
    {EKeyF2, EStdKeyF2, Key_F2},
    {EKeyF3, EStdKeyF3, Key_F3},
    {EKeyF4, EStdKeyF4, Key_F4},
    {EKeyF5, EStdKeyF5, Key_F5},
    {EKeyF6, EStdKeyF6, Key_F6},
    {EKeyF7, EStdKeyF7, Key_F7},
    {EKeyF8, EStdKeyF8, Key_F8},
    {EKeyF9, EStdKeyF9, Key_F9},
    {EKeyF10, EStdKeyF10, Key_F10},
    {EKeyF11, EStdKeyF11, Key_F11},
    {EKeyF12, EStdKeyF12, Key_F12},
    {EKeyF13, EStdKeyF13, Key_F13},
    {EKeyF14, EStdKeyF14, Key_F14},
    {EKeyF15, EStdKeyF15, Key_F15},
    {EKeyF16, EStdKeyF16, Key_F16},
    {EKeyF17, EStdKeyF17, Key_F17},
    {EKeyF18, EStdKeyF18, Key_F18},
    {EKeyF19, EStdKeyF19, Key_F19},
    {EKeyF20, EStdKeyF20, Key_F20},
    {EKeyF21, EStdKeyF21, Key_F21},
    {EKeyF22, EStdKeyF22, Key_F22},
    {EKeyF23, EStdKeyF23, Key_F23},
    {EKeyF24, EStdKeyF24, Key_F24},
    {EKeyOff, EStdKeyOff, Key_PowerOff},
//    {EKeyMenu, EStdKeyMenu, Key_Menu}, // Menu is EKeyApplication0
    {EKeyHelp, EStdKeyHelp, Key_Help},
    {EKeyDial, EStdKeyDial, Key_Call},
    {EKeyIncVolume, EStdKeyIncVolume, Key_VolumeUp},
    {EKeyDecVolume, EStdKeyDecVolume, Key_VolumeDown},
    {EKeyDevice0, EStdKeyDevice0, Key_Context1}, // Found by manual testing.
    {EKeyDevice1, EStdKeyDevice1, Key_Context2}, // Found by manual testing.
    {EKeyDevice3, EStdKeyDevice3, Key_Select},
    {EKeyDevice7, EStdKeyDevice7, Key_Camera},  
    {EKeyApplication0, EStdKeyApplication0, Key_Menu}, // Found by manual testing.
    {EKeyApplication1, EStdKeyApplication1, Key_Launch1}, // Found by manual testing.
    {EKeyApplication2, EStdKeyApplication2, Key_MediaPlay}, // Found by manual testing.
    {EKeyApplication3, EStdKeyApplication3, Key_MediaStop}, // Found by manual testing.
    {EKeyApplication4, EStdKeyApplication4, Key_MediaNext}, // Found by manual testing.
    {EKeyApplication5, EStdKeyApplication5, Key_MediaPrevious}, // Found by manual testing.
    {EKeyApplication6, EStdKeyApplication6, Key_Launch6},
    {EKeyApplication7, EStdKeyApplication7, Key_Launch7},
    {EKeyApplication8, EStdKeyApplication8, Key_Launch8},
    {EKeyApplication9, EStdKeyApplication9, Key_Launch9},
    {EKeyApplicationA, EStdKeyApplicationA, Key_LaunchA},
    {EKeyApplicationB, EStdKeyApplicationB, Key_LaunchB},
    {EKeyApplicationC, EStdKeyApplicationC, Key_LaunchC},
    {EKeyApplicationD, EStdKeyApplicationD, Key_LaunchD},
    {EKeyApplicationE, EStdKeyApplicationE, Key_LaunchE},
    {EKeyApplicationF, EStdKeyApplicationF, Key_LaunchF},
    {EKeyApplication19, EStdKeyApplication19, Key_CameraFocus}, 
    {EKeyYes, EStdKeyYes, Key_Yes},
    {EKeyNo, EStdKeyNo, Key_No},
    {TKeyCode(0), TStdScanCode(0), Qt::Key(0)}
};

int QKeyMapperPrivate::mapS60KeyToQt(TUint s60key)
{
    int res = Qt::Key_unknown;
    for (int i = 0; keyMapping[i].s60KeyCode != 0; i++) {
        if (keyMapping[i].s60KeyCode == s60key) {
            res = keyMapping[i].qtKey;
            break;
        }
    }
    return res;
}

int QKeyMapperPrivate::mapS60ScanCodesToQt(TUint s60scanCode)
{
    int res = Qt::Key_unknown;
    for (int i = 0; keyMapping[i].s60KeyCode != 0; i++) {
        if (keyMapping[i].s60ScanCode == s60scanCode) {
            res = keyMapping[i].qtKey;
            break;
        }
    }
    return res;
}

int QKeyMapperPrivate::mapQtToS60Key(int qtKey)
{
    int res = KErrUnknown;
    for (int i = 0; keyMapping[i].s60KeyCode != 0; i++) {
        if (keyMapping[i].qtKey == qtKey) {
            res = keyMapping[i].s60KeyCode;
            break;
        }
    }
    return res;
}

int QKeyMapperPrivate::mapQtToS60ScanCodes(int qtKey)
{
    int res = KErrUnknown;
    for (int i = 0; keyMapping[i].s60KeyCode != 0; i++) {
        if (keyMapping[i].qtKey == qtKey) {
            res = keyMapping[i].s60ScanCode;
            break;
        }
    }
    return res;
}

void QKeyMapperPrivate::updateInputLanguage()
{
#ifdef Q_WS_S60
    TInt err;
    CRepository *repo;
    const TUid KCRUidAknFep = TUid::Uid(0x101F876D);
    const TUint32 KAknFepInputTxtLang = 0x00000005;
    TRAP(err, repo = CRepository::NewL(KCRUidAknFep));
    if (err != KErrNone)
        return;

    TInt symbianLang;
    err = repo->Get(KAknFepInputTxtLang, symbianLang);
    delete repo;
    if (err != KErrNone)
        return;

    QString qtLang = QString::fromAscii(qt_symbianLocaleName(symbianLang));
    keyboardInputLocale = QLocale(qtLang);
    keyboardInputDirection = (TBidiText::ScriptDirectionality(TLanguage(symbianLang)) == TBidiText::ERightToLeft)
            ? Qt::RightToLeft : Qt::LeftToRight;
#else
    keyboardInputLocale = QLocale();
    keyboardInputDirection = Qt::LeftToRight;
#endif
}

QT_END_NAMESPACE
