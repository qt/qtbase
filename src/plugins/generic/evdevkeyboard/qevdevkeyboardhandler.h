/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QEVDEVKEYBOARDHANDLER_H
#define QEVDEVKEYBOARDHANDLER_H

#include <qobject.h>
#include <QTimer>
#include <QDataStream>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

namespace QEvdevKeyboardMap {
    const quint32 FileMagic = 0x514d4150; // 'QMAP'

    struct Mapping {
        quint16 keycode;
        quint16 unicode;
        quint32 qtcode;
        quint8 modifiers;
        quint8 flags;
        quint16 special;

    };

    enum Flags {
        IsDead     = 0x01,
        IsLetter   = 0x02,
        IsModifier = 0x04,
        IsSystem   = 0x08
    };

    enum System {
        SystemConsoleFirst    = 0x0100,
        SystemConsoleMask     = 0x007f,
        SystemConsoleLast     = 0x017f,
        SystemConsolePrevious = 0x0180,
        SystemConsoleNext     = 0x0181,
        SystemReboot          = 0x0200,
        SystemZap             = 0x0300
    };

    struct Composing {
        quint16 first;
        quint16 second;
        quint16 result;
    };

    enum Modifiers {
        ModPlain   = 0x00,
        ModShift   = 0x01,
        ModAltGr   = 0x02,
        ModControl = 0x04,
        ModAlt     = 0x08,
        ModShiftL  = 0x10,
        ModShiftR  = 0x20,
        ModCtrlL   = 0x40,
        ModCtrlR   = 0x80
        // ModCapsShift = 0x100, // not supported!
    };
}

inline QDataStream &operator>>(QDataStream &ds, QEvdevKeyboardMap::Mapping &m)
{
    return ds >> m.keycode >> m.unicode >> m.qtcode >> m.modifiers >> m.flags >> m.special;
}

inline QDataStream &operator<<(QDataStream &ds, const QEvdevKeyboardMap::Mapping &m)
{
    return ds << m.keycode << m.unicode << m.qtcode << m.modifiers << m.flags << m.special;
}

inline QDataStream &operator>>(QDataStream &ds, QEvdevKeyboardMap::Composing &c)
{
    return ds >> c.first >> c.second >> c.result;
}

inline QDataStream &operator<<(QDataStream &ds, const QEvdevKeyboardMap::Composing &c)
{
    return ds << c.first << c.second << c.result;
}


class QEvdevKeyboardHandler : public QObject
{
    Q_OBJECT
public:
    QEvdevKeyboardHandler(int deviceDescriptor, bool disableZap, bool enableCompose, const QString &keymapFile);
    ~QEvdevKeyboardHandler();

    enum KeycodeAction {
        None               = 0,

        CapsLockOff        = 0x01000000,
        CapsLockOn         = 0x01000001,
        NumLockOff         = 0x02000000,
        NumLockOn          = 0x02000001,
        ScrollLockOff      = 0x03000000,
        ScrollLockOn       = 0x03000001,

        Reboot             = 0x04000000,

        PreviousConsole    = 0x05000000,
        NextConsole        = 0x05000001,
        SwitchConsoleFirst = 0x06000000,
        SwitchConsoleLast  = 0x0600007f,
        SwitchConsoleMask  = 0x0000007f
    };

    static QEvdevKeyboardHandler *createLinuxInputKeyboardHandler(const QString &key, const QString &specification);

    static Qt::KeyboardModifiers toQtModifiers(quint8 mod)
    {
        Qt::KeyboardModifiers qtmod = Qt::NoModifier;

        if (mod & (QEvdevKeyboardMap::ModShift | QEvdevKeyboardMap::ModShiftL | QEvdevKeyboardMap::ModShiftR))
            qtmod |= Qt::ShiftModifier;
        if (mod & (QEvdevKeyboardMap::ModControl | QEvdevKeyboardMap::ModCtrlL | QEvdevKeyboardMap::ModCtrlR))
            qtmod |= Qt::ControlModifier;
        if (mod & QEvdevKeyboardMap::ModAlt)
            qtmod |= Qt::AltModifier;

        return qtmod;
    }

private slots:
    void readKeycode();
    KeycodeAction processKeycode(quint16 keycode, bool pressed, bool autorepeat);

private:
    void unloadKeymap();
    bool loadKeymap(const QString &file);
    void processKeyEvent(int unicode, int keycode, Qt::KeyboardModifiers modifiers, bool isPress, bool autoRepeat);
    void switchLed(int, bool);

    int m_fd;

    // keymap handling
    quint8 m_modifiers;
    quint8 m_locks[3];
    int m_composing;
    quint16 m_dead_unicode;

    bool m_no_zap;
    bool m_do_compose;

    const QEvdevKeyboardMap::Mapping *m_keymap;
    int m_keymap_size;
    const QEvdevKeyboardMap::Composing *m_keycompose;
    int m_keycompose_size;

    static const QEvdevKeyboardMap::Mapping s_keymap_default[];
    static const QEvdevKeyboardMap::Composing s_keycompose_default[];
};


QT_END_NAMESPACE

QT_END_HEADER

#endif // QEVDEVKEYBOARDHANDLER_H
