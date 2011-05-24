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

#ifndef QWSKEYBOARD_P_H
#define QWSKEYBOARD_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QDataStream>

QT_BEGIN_NAMESPACE

namespace QWSKeyboard {
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
        IsSystem   = 0x08,
    };

    enum System {
        SystemConsoleFirst    = 0x0100,
        SystemConsoleMask     = 0x007f,
        SystemConsoleLast     = 0x017f,
        SystemConsolePrevious = 0x0180,
        SystemConsoleNext     = 0x0181,
        SystemReboot          = 0x0200,
        SystemZap             = 0x0300,
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
        ModCtrlR   = 0x80,
        // ModCapsShift = 0x100, // not supported!
    };
};

#ifndef QT_NO_DATASTREAM
inline QDataStream &operator>>(QDataStream &ds, QWSKeyboard::Mapping &m)
{
    return ds >> m.keycode >> m.unicode >> m.qtcode >> m.modifiers >> m.flags >> m.special;
}

inline QDataStream &operator<<(QDataStream &ds, const QWSKeyboard::Mapping &m)
{
    return ds << m.keycode << m.unicode << m.qtcode << m.modifiers << m.flags << m.special;
}

inline QDataStream &operator>>(QDataStream &ds, QWSKeyboard::Composing &c)
{
    return ds >> c.first >> c.second >> c.result;
}

inline QDataStream &operator<<(QDataStream &ds, const QWSKeyboard::Composing &c)
{
    return ds << c.first << c.second << c.result;
}
#endif // QT_NO_DATASTREAM

QT_END_NAMESPACE

#endif // QWSKEYBOARD_H
