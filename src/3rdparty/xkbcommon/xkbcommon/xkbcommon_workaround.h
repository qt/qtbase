/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef XKBCOMMON_WORKAROUND_H
#define XKBCOMMON_WORKAROUND_H

// Function utf32_to_utf8() is borrowed from the libxkbcommon library,
// file keysym-utf.c. The workaround should be removed once the fix from
// https://bugs.freedesktop.org/show_bug.cgi?id=56780 gets released.
static int utf32_to_utf8(uint32_t unichar, char *buffer)
{
    int count, shift, length;
    uint8_t head;

    if (unichar <= 0x007f) {
        buffer[0] = unichar;
        buffer[1] = '\0';
        return 2;
    }
    else if (unichar <= 0x07FF) {
        length = 2;
        head = 0xc0;
    }
    else if (unichar <= 0xffff) {
        length = 3;
        head = 0xe0;
    }
    else if (unichar <= 0x1fffff) {
        length = 4;
        head = 0xf0;
    }
    else if (unichar <= 0x3ffffff) {
        length = 5;
        head = 0xf8;
    }
    else {
        length = 6;
        head = 0xfc;
    }

    for (count = length - 1, shift = 0; count > 0; count--, shift += 6)
        buffer[count] = 0x80 | ((unichar >> shift) & 0x3f);

    buffer[0] = head | ((unichar >> shift) & 0x3f);
    buffer[length] = '\0';

    return length + 1;
}

static bool needWorkaround(uint32_t sym)
{
    /* patch encoding botch */
    if (sym == XKB_KEY_KP_Space)
        return true;

    /* special keysyms */
    if ((sym >= XKB_KEY_BackSpace && sym <= XKB_KEY_Clear) ||
        (sym >= XKB_KEY_KP_Multiply && sym <= XKB_KEY_KP_9) ||
        sym == XKB_KEY_Return || sym == XKB_KEY_Escape ||
        sym == XKB_KEY_Delete || sym == XKB_KEY_KP_Tab ||
        sym == XKB_KEY_KP_Enter || sym == XKB_KEY_KP_Equal)
        return true;

    return false;
}

#endif // XKBCOMMON_WORKAROUND_H
