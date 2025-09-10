// Copyright (C) 2014 Robin Burchell <robin.burchell@viroteck.net>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTUIO_P_H
#define QTUIO_P_H

QT_BEGIN_NAMESPACE

inline bool qt_readOscString(const QByteArray &source, QByteArray &dest, quint32 &pos)
{
    int end = source.indexOf('\0', pos);
    if (end < 0) {
        pos = source.size();
        dest = QByteArray();
        return false;
    }

    dest = source.mid(pos, end - pos);

    // Skip additional NULL bytes at the end of the string to make sure the
    // total number of bits a multiple of 32 bits ("OSC-string" in the
    // specification).
    end += 4 - ((end - pos) % 4);

    pos = end;
    return true;
}

QT_END_NAMESPACE

#endif
