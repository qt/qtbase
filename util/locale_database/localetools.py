#############################################################################
##
## Copyright (C) 2020 The Qt Company Ltd.
## Contact: https://www.qt.io/licensing/
##
## This file is part of the test suite of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:GPL-EXCEPT$
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company. For licensing terms
## and conditions see https://www.qt.io/terms-conditions. For further
## information use the contact form at https://www.qt.io/contact-us.
##
## GNU General Public License Usage
## Alternatively, this file may be used under the terms of the GNU
## General Public License version 3 as published by the Free Software
## Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
## included in the packaging of this file. Please review the following
## information to ensure the GNU General Public License requirements will
## be met: https://www.gnu.org/licenses/gpl-3.0.html.
##
## $QT_END_LICENSE$
##
#############################################################################
"""Utilities shared among the CLDR extraction tools.
Functions:
  unicode2hex() -- converts unicode text to UCS-2 in hex form.
  wrap_list() -- map list to comma-separated string, 20 entries per line.

Classes:
  Error -- A shared error class.
"""

class Error (StandardError):
    __upinit = StandardError.__init__
    def __init__(self, msg, *args):
        self.__upinit(msg, *args)
        self.message = msg
    def __str__(self):
        return self.message

def unicode2hex(s):
    lst = []
    for x in s:
        v = ord(x)
        if v > 0xFFFF:
            # make a surrogate pair
            # copied from qchar.h
            high = (v >> 10) + 0xd7c0
            low = (v % 0x400 + 0xdc00)
            lst.append(hex(high))
            lst.append(hex(low))
        else:
            lst.append(hex(v))
    return lst

def wrap_list(lst):
    def split(lst, size):
        while lst:
            head, lst = lst[:size], lst[size:]
            yield head
    return ",\n".join(", ".join(x) for x in split(lst, 20))
