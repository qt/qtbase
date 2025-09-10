// Copyright (C) 2015-2016 Oleksandr Tymoshenko <gonzo@bluezbox.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QBSDKEYBOARD_H
#define QBSDKEYBOARD_H

#include <qobject.h>
#include <QDataStream>
#include <QList>

QT_BEGIN_NAMESPACE

class QSocketNotifier;

struct termios;

enum {
    Bsd_NoKeyMode       = -1
};

namespace QBsdKeyboardMap {
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
        NoFlags    = 0x00,
        IsLetter   = 0x01,
        IsModifier = 0x02
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

inline QDataStream &operator>>(QDataStream &ds, QBsdKeyboardMap::Mapping &m)
{
    return ds >> m.keycode >> m.unicode >> m.qtcode >> m.modifiers >> m.flags >> m.special;
}

class QBsdKeyboardHandler : public QObject
{
    Q_OBJECT

public:
    QBsdKeyboardHandler(const QString &key, const QString &specification);
    ~QBsdKeyboardHandler() override;

    static Qt::KeyboardModifiers toQtModifiers(quint8 mod)
    {
        Qt::KeyboardModifiers qtmod = Qt::NoModifier;

        if (mod & (QBsdKeyboardMap::ModShift | QBsdKeyboardMap::ModShiftL | QBsdKeyboardMap::ModShiftR))
            qtmod |= Qt::ShiftModifier;
        if (mod & (QBsdKeyboardMap::ModControl | QBsdKeyboardMap::ModCtrlL | QBsdKeyboardMap::ModCtrlR))
            qtmod |= Qt::ControlModifier;
        if (mod & QBsdKeyboardMap::ModAlt)
            qtmod |= Qt::AltModifier;

        return qtmod;
    }

protected:
    void switchLed(int led, bool state);
    void processKeycode(quint16 keycode, bool pressed, bool autorepeat);
    void processKeyEvent(int nativecode, int unicode, int qtcode,
                         Qt::KeyboardModifiers modifiers, bool isPress, bool autoRepeat);
    void revertTTYSettings();
    void resetKeymap();
    void readKeyboardData();

private:
    QScopedPointer<QSocketNotifier> m_notifier;
    QScopedPointer<termios> m_kbdOrigTty;
    int m_origKbdMode = Bsd_NoKeyMode;
    int m_fd = -1;
    bool m_shouldClose = false;
    QString m_spec;

    // keymap handling
    quint8 m_modifiers = 0;
    bool m_capsLock = false;
    bool m_numLock = false;
    bool m_scrollLock = false;

    QList<QBsdKeyboardMap::Mapping> m_keymap;
};

QT_END_NAMESPACE

#endif // QBSDKEYBOARD_H
