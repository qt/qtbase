/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the config.tests of the Qt Toolkit.
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

#include <xcb/xcb.h>

// This is needed to make Qt compile together with XKB. xkb.h is using a variable
// which is called 'explicit', this is a reserved keyword in c++ */
#define explicit dont_use_cxx_explicit
#include <xcb/xkb.h>
#undef explicit

int main(int, char **)
{
    int primaryScreen = 0;

    xcb_connection_t *connection = xcb_connect("", &primaryScreen);

    // This takes more arguments in xcb-xkb < 1.10.
    xcb_xkb_get_kbd_by_name_unchecked(NULL, 0, 0, 0, 0);

    // This won't compile unless libxcb >= 1.5 which defines XCB_ATOM_PRIMARY.
    int xcbAtomPrimary = XCB_ATOM_PRIMARY;

    return 0;
}
