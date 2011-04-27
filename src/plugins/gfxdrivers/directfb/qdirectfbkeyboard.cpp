/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdirectfbkeyboard.h"

#ifndef QT_NO_QWS_DIRECTFB

#include "qdirectfbscreen.h"
#include <qobject.h>
#include <qsocketnotifier.h>
#include <qhash.h>

#include <directfb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

QT_BEGIN_NAMESPACE

class KeyMap : public QHash<DFBInputDeviceKeySymbol, Qt::Key>
{
public:
    KeyMap();
};

Q_GLOBAL_STATIC(KeyMap, keymap);

class QDirectFBKeyboardHandlerPrivate : public QObject
{
    Q_OBJECT
public:
    QDirectFBKeyboardHandlerPrivate(QDirectFBKeyboardHandler *handler);
    ~QDirectFBKeyboardHandlerPrivate();

    void suspend();
    void resume();

private:
    QDirectFBKeyboardHandler *handler;
    IDirectFBEventBuffer *eventBuffer;
    QSocketNotifier *keyboardNotifier;
    DFBEvent event;
    int bytesRead;
    int lastUnicode, lastKeycode;
    Qt::KeyboardModifiers lastModifiers;
private Q_SLOTS:
    void readKeyboardData();
};

QDirectFBKeyboardHandlerPrivate::QDirectFBKeyboardHandlerPrivate(QDirectFBKeyboardHandler *h)
    : handler(h), eventBuffer(0), keyboardNotifier(0), bytesRead(0),
      lastUnicode(0), lastKeycode(0), lastModifiers(0)
{
    Q_ASSERT(qt_screen);

    IDirectFB *fb = QDirectFBScreen::instance()->dfb();
    if (!fb) {
        qCritical("QDirectFBKeyboardHandler: DirectFB not initialized");
        return;
    }

    DFBResult result;
    result = fb->CreateInputEventBuffer(fb, DICAPS_KEYS, DFB_TRUE,
                                        &eventBuffer);
    if (result != DFB_OK) {
        DirectFBError("QDirectFBKeyboardHandler: "
                      "Unable to create input event buffer", result);
        return;
    }

    int fd;
    result = eventBuffer->CreateFileDescriptor(eventBuffer, &fd);
    if (result != DFB_OK) {
        DirectFBError("QDirectFBKeyboardHandler: "
                      "Unable to create file descriptor", result);
        return;
    }

    int flags = ::fcntl(fd, F_GETFL, 0);
    ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    memset(&event, 0, sizeof(event));

    keyboardNotifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
    connect(keyboardNotifier, SIGNAL(activated(int)),
            this, SLOT(readKeyboardData()));
    resume();
}

void QDirectFBKeyboardHandlerPrivate::suspend()
{
    keyboardNotifier->setEnabled(false);
}

void QDirectFBKeyboardHandlerPrivate::resume()
{
    eventBuffer->Reset(eventBuffer);
    keyboardNotifier->setEnabled(true);
}

QDirectFBKeyboardHandlerPrivate::~QDirectFBKeyboardHandlerPrivate()
{
    if (eventBuffer)
        eventBuffer->Release(eventBuffer);
}

void QDirectFBKeyboardHandlerPrivate::readKeyboardData()
{
    if(!qt_screen)
        return;

    for (;;) {
        // GetEvent returns DFB_UNSUPPORTED after CreateFileDescriptor().
        // This seems stupid and I really hope it's a bug which will be fixed.

        // DFBResult ret = eventBuffer->GetEvent(eventBuffer, &event);

        char *buf = reinterpret_cast<char*>(&event);
        int ret = ::read(keyboardNotifier->socket(),
                         buf + bytesRead, sizeof(DFBEvent) - bytesRead);
        if (ret == -1) {
            if (errno != EAGAIN)
                qWarning("QDirectFBKeyboardHandlerPrivate::readKeyboardData(): %s",
                         strerror(errno));
            return;
        }

        Q_ASSERT(ret >= 0);
        bytesRead += ret;
        if (bytesRead < int(sizeof(DFBEvent)))
            break;
        bytesRead = 0;

        Q_ASSERT(event.clazz == DFEC_INPUT);

        const DFBInputEvent input = event.input;

        Qt::KeyboardModifiers modifiers = Qt::NoModifier;

        // Not implemented:
        // if (input.modifiers & DIMM_SUPER)
        // if (input.modifiers & DIMM_HYPER)

        if (!(input.flags & DIEF_KEYSYMBOL) ||
            !(input.flags & DIEF_KEYID) ||
            !(input.type & (DIET_KEYPRESS|DIET_KEYRELEASE)))
        {
            static bool first = true;
            if (first) {
                qWarning("QDirectFBKeyboardHandler - Getting unexpected non-keyboard related events");
                first = false;
            }
            break;
        }

        if (input.flags & DIEF_MODIFIERS) {
            if (input.modifiers & DIMM_SHIFT)
                modifiers |= Qt::ShiftModifier;
            if (input.modifiers & DIMM_CONTROL)
                modifiers |= Qt::ControlModifier;
            if (input.modifiers & DIMM_ALT)
                modifiers |= Qt::AltModifier;
            if (input.modifiers & DIMM_ALTGR)
                modifiers |= Qt::AltModifier;
            if (input.modifiers & DIMM_META)
                modifiers |= Qt::MetaModifier;
        }


        const bool press = input.type & DIET_KEYPRESS;
        DFBInputDeviceKeySymbol symbol = input.key_symbol;
        int unicode = -1;
        int keycode = 0;

        keycode = keymap()->value(symbol);
        if (DFB_KEY_TYPE(symbol) == DIKT_UNICODE)
            unicode = symbol;

        if (unicode != -1 || keycode != 0) {
            bool autoRepeat = false;
            if (press) {
                if (unicode == lastUnicode && keycode == lastKeycode && modifiers == lastModifiers) {
                    autoRepeat = true;
                } else {
                    lastUnicode = unicode;
                    lastKeycode = keycode;
                    lastModifiers = modifiers;
                }
            } else {
                lastUnicode = lastKeycode = -1;
                lastModifiers = 0;
            }
            if (autoRepeat) {
                handler->processKeyEvent(unicode, keycode,
                                         modifiers, false, autoRepeat);

            }

            handler->processKeyEvent(unicode, keycode,
                                     modifiers, press, autoRepeat);
        }
    }
}

QDirectFBKeyboardHandler::QDirectFBKeyboardHandler(const QString &device)
    : QWSKeyboardHandler()
{
    Q_UNUSED(device);
    d = new QDirectFBKeyboardHandlerPrivate(this);
}

QDirectFBKeyboardHandler::~QDirectFBKeyboardHandler()
{
    delete d;
}

KeyMap::KeyMap()
{
    insert(DIKS_BACKSPACE             , Qt::Key_Backspace);
    insert(DIKS_TAB                   , Qt::Key_Tab);
    insert(DIKS_RETURN                , Qt::Key_Return);
    insert(DIKS_ESCAPE                , Qt::Key_Escape);
    insert(DIKS_DELETE                , Qt::Key_Delete);

    insert(DIKS_CURSOR_LEFT           , Qt::Key_Left);
    insert(DIKS_CURSOR_RIGHT          , Qt::Key_Right);
    insert(DIKS_CURSOR_UP             , Qt::Key_Up);
    insert(DIKS_CURSOR_DOWN           , Qt::Key_Down);
    insert(DIKS_INSERT                , Qt::Key_Insert);
    insert(DIKS_HOME                  , Qt::Key_Home);
    insert(DIKS_END                   , Qt::Key_End);
    insert(DIKS_PAGE_UP               , Qt::Key_PageUp);
    insert(DIKS_PAGE_DOWN             , Qt::Key_PageDown);
    insert(DIKS_PRINT                 , Qt::Key_Print);
    insert(DIKS_PAUSE                 , Qt::Key_Pause);
    insert(DIKS_SELECT                , Qt::Key_Select);
    insert(DIKS_GOTO                  , Qt::Key_OpenUrl);
    insert(DIKS_CLEAR                 , Qt::Key_Clear);
    insert(DIKS_MENU                  , Qt::Key_Menu);
    insert(DIKS_HELP                  , Qt::Key_Help);

    insert(DIKS_INTERNET              , Qt::Key_HomePage);
    insert(DIKS_MAIL                  , Qt::Key_LaunchMail);
    insert(DIKS_FAVORITES             , Qt::Key_Favorites);

    insert(DIKS_BACK                  , Qt::Key_Back);
    insert(DIKS_FORWARD               , Qt::Key_Forward);
    insert(DIKS_VOLUME_UP             , Qt::Key_VolumeUp);
    insert(DIKS_VOLUME_DOWN           , Qt::Key_VolumeDown);
    insert(DIKS_MUTE                  , Qt::Key_VolumeMute);
    insert(DIKS_PLAYPAUSE             , Qt::Key_Pause);
    insert(DIKS_PLAY                  , Qt::Key_MediaPlay);
    insert(DIKS_STOP                  , Qt::Key_MediaStop);
    insert(DIKS_RECORD                , Qt::Key_MediaRecord);
    insert(DIKS_PREVIOUS              , Qt::Key_MediaPrevious);
    insert(DIKS_NEXT                  , Qt::Key_MediaNext);

    insert(DIKS_F1                    , Qt::Key_F1);
    insert(DIKS_F2                    , Qt::Key_F2);
    insert(DIKS_F3                    , Qt::Key_F3);
    insert(DIKS_F4                    , Qt::Key_F4);
    insert(DIKS_F5                    , Qt::Key_F5);
    insert(DIKS_F6                    , Qt::Key_F6);
    insert(DIKS_F7                    , Qt::Key_F7);
    insert(DIKS_F8                    , Qt::Key_F8);
    insert(DIKS_F9                    , Qt::Key_F9);
    insert(DIKS_F10                   , Qt::Key_F10);
    insert(DIKS_F11                   , Qt::Key_F11);
    insert(DIKS_F12                   , Qt::Key_F12);

    insert(DIKS_SHIFT                 , Qt::Key_Shift);
    insert(DIKS_CONTROL               , Qt::Key_Control);
    insert(DIKS_ALT                   , Qt::Key_Alt);
    insert(DIKS_ALTGR                 , Qt::Key_AltGr);

    insert(DIKS_META                  , Qt::Key_Meta);
    insert(DIKS_SUPER                 , Qt::Key_Super_L); // ???
    insert(DIKS_HYPER                 , Qt::Key_Hyper_L); // ???

    insert(DIKS_CAPS_LOCK             , Qt::Key_CapsLock);
    insert(DIKS_NUM_LOCK              , Qt::Key_NumLock);
    insert(DIKS_SCROLL_LOCK           , Qt::Key_ScrollLock);

    insert(DIKS_DEAD_ABOVEDOT         , Qt::Key_Dead_Abovedot);
    insert(DIKS_DEAD_ABOVERING        , Qt::Key_Dead_Abovering);
    insert(DIKS_DEAD_ACUTE            , Qt::Key_Dead_Acute);
    insert(DIKS_DEAD_BREVE            , Qt::Key_Dead_Breve);
    insert(DIKS_DEAD_CARON            , Qt::Key_Dead_Caron);
    insert(DIKS_DEAD_CEDILLA          , Qt::Key_Dead_Cedilla);
    insert(DIKS_DEAD_CIRCUMFLEX       , Qt::Key_Dead_Circumflex);
    insert(DIKS_DEAD_DIAERESIS        , Qt::Key_Dead_Diaeresis);
    insert(DIKS_DEAD_DOUBLEACUTE      , Qt::Key_Dead_Doubleacute);
    insert(DIKS_DEAD_GRAVE            , Qt::Key_Dead_Grave);
    insert(DIKS_DEAD_IOTA             , Qt::Key_Dead_Iota);
    insert(DIKS_DEAD_MACRON           , Qt::Key_Dead_Macron);
    insert(DIKS_DEAD_OGONEK           , Qt::Key_Dead_Ogonek);
    insert(DIKS_DEAD_SEMIVOICED_SOUND , Qt::Key_Dead_Semivoiced_Sound);
    insert(DIKS_DEAD_TILDE            , Qt::Key_Dead_Tilde);
    insert(DIKS_DEAD_VOICED_SOUND     , Qt::Key_Dead_Voiced_Sound);
    insert(DIKS_SPACE                 , Qt::Key_Space);
    insert(DIKS_EXCLAMATION_MARK      , Qt::Key_Exclam);
    insert(DIKS_QUOTATION             , Qt::Key_QuoteDbl);
    insert(DIKS_NUMBER_SIGN           , Qt::Key_NumberSign);
    insert(DIKS_DOLLAR_SIGN           , Qt::Key_Dollar);
    insert(DIKS_PERCENT_SIGN          , Qt::Key_Percent);
    insert(DIKS_AMPERSAND             , Qt::Key_Ampersand);
    insert(DIKS_APOSTROPHE            , Qt::Key_Apostrophe);
    insert(DIKS_PARENTHESIS_LEFT      , Qt::Key_ParenLeft);
    insert(DIKS_PARENTHESIS_RIGHT     , Qt::Key_ParenRight);
    insert(DIKS_ASTERISK              , Qt::Key_Asterisk);
    insert(DIKS_PLUS_SIGN             , Qt::Key_Plus);
    insert(DIKS_COMMA                 , Qt::Key_Comma);
    insert(DIKS_MINUS_SIGN            , Qt::Key_Minus);
    insert(DIKS_PERIOD                , Qt::Key_Period);
    insert(DIKS_SLASH                 , Qt::Key_Slash);
    insert(DIKS_0                     , Qt::Key_0);
    insert(DIKS_1                     , Qt::Key_1);
    insert(DIKS_2                     , Qt::Key_2);
    insert(DIKS_3                     , Qt::Key_3);
    insert(DIKS_4                     , Qt::Key_4);
    insert(DIKS_5                     , Qt::Key_5);
    insert(DIKS_6                     , Qt::Key_6);
    insert(DIKS_7                     , Qt::Key_7);
    insert(DIKS_8                     , Qt::Key_8);
    insert(DIKS_9                     , Qt::Key_9);
    insert(DIKS_COLON                 , Qt::Key_Colon);
    insert(DIKS_SEMICOLON             , Qt::Key_Semicolon);
    insert(DIKS_LESS_THAN_SIGN        , Qt::Key_Less);
    insert(DIKS_EQUALS_SIGN           , Qt::Key_Equal);
    insert(DIKS_GREATER_THAN_SIGN     , Qt::Key_Greater);
    insert(DIKS_QUESTION_MARK         , Qt::Key_Question);
    insert(DIKS_AT                    , Qt::Key_At);
    insert(DIKS_CAPITAL_A             , Qt::Key_A);
    insert(DIKS_CAPITAL_B             , Qt::Key_B);
    insert(DIKS_CAPITAL_C             , Qt::Key_C);
    insert(DIKS_CAPITAL_D             , Qt::Key_D);
    insert(DIKS_CAPITAL_E             , Qt::Key_E);
    insert(DIKS_CAPITAL_F             , Qt::Key_F);
    insert(DIKS_CAPITAL_G             , Qt::Key_G);
    insert(DIKS_CAPITAL_H             , Qt::Key_H);
    insert(DIKS_CAPITAL_I             , Qt::Key_I);
    insert(DIKS_CAPITAL_J             , Qt::Key_J);
    insert(DIKS_CAPITAL_K             , Qt::Key_K);
    insert(DIKS_CAPITAL_L             , Qt::Key_L);
    insert(DIKS_CAPITAL_M             , Qt::Key_M);
    insert(DIKS_CAPITAL_N             , Qt::Key_N);
    insert(DIKS_CAPITAL_O             , Qt::Key_O);
    insert(DIKS_CAPITAL_P             , Qt::Key_P);
    insert(DIKS_CAPITAL_Q             , Qt::Key_Q);
    insert(DIKS_CAPITAL_R             , Qt::Key_R);
    insert(DIKS_CAPITAL_S             , Qt::Key_S);
    insert(DIKS_CAPITAL_T             , Qt::Key_T);
    insert(DIKS_CAPITAL_U             , Qt::Key_U);
    insert(DIKS_CAPITAL_V             , Qt::Key_V);
    insert(DIKS_CAPITAL_W             , Qt::Key_W);
    insert(DIKS_CAPITAL_X             , Qt::Key_X);
    insert(DIKS_CAPITAL_Y             , Qt::Key_Y);
    insert(DIKS_CAPITAL_Z             , Qt::Key_Z);
    insert(DIKS_SQUARE_BRACKET_LEFT   , Qt::Key_BracketLeft);
    insert(DIKS_BACKSLASH             , Qt::Key_Backslash);
    insert(DIKS_SQUARE_BRACKET_RIGHT  , Qt::Key_BracketRight);
    insert(DIKS_CIRCUMFLEX_ACCENT     , Qt::Key_AsciiCircum);
    insert(DIKS_UNDERSCORE            , Qt::Key_Underscore);
    insert(DIKS_SMALL_A               , Qt::Key_A);
    insert(DIKS_SMALL_B               , Qt::Key_B);
    insert(DIKS_SMALL_C               , Qt::Key_C);
    insert(DIKS_SMALL_D               , Qt::Key_D);
    insert(DIKS_SMALL_E               , Qt::Key_E);
    insert(DIKS_SMALL_F               , Qt::Key_F);
    insert(DIKS_SMALL_G               , Qt::Key_G);
    insert(DIKS_SMALL_H               , Qt::Key_H);
    insert(DIKS_SMALL_I               , Qt::Key_I);
    insert(DIKS_SMALL_J               , Qt::Key_J);
    insert(DIKS_SMALL_K               , Qt::Key_K);
    insert(DIKS_SMALL_L               , Qt::Key_L);
    insert(DIKS_SMALL_M               , Qt::Key_M);
    insert(DIKS_SMALL_N               , Qt::Key_N);
    insert(DIKS_SMALL_O               , Qt::Key_O);
    insert(DIKS_SMALL_P               , Qt::Key_P);
    insert(DIKS_SMALL_Q               , Qt::Key_Q);
    insert(DIKS_SMALL_R               , Qt::Key_R);
    insert(DIKS_SMALL_S               , Qt::Key_S);
    insert(DIKS_SMALL_T               , Qt::Key_T);
    insert(DIKS_SMALL_U               , Qt::Key_U);
    insert(DIKS_SMALL_V               , Qt::Key_V);
    insert(DIKS_SMALL_W               , Qt::Key_W);
    insert(DIKS_SMALL_X               , Qt::Key_X);
    insert(DIKS_SMALL_Y               , Qt::Key_Y);
    insert(DIKS_SMALL_Z               , Qt::Key_Z);
    insert(DIKS_CURLY_BRACKET_LEFT    , Qt::Key_BraceLeft);
    insert(DIKS_VERTICAL_BAR          , Qt::Key_Bar);
    insert(DIKS_CURLY_BRACKET_RIGHT   , Qt::Key_BraceRight);
    insert(DIKS_TILDE                 , Qt::Key_AsciiTilde);
}

QT_END_NAMESPACE
#include "qdirectfbkeyboard.moc"
#endif // QT_NO_QWS_DIRECTFB
