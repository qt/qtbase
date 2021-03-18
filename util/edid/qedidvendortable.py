#!/usr/bin/env python3
#############################################################################
##
## Copyright (C) 2017 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
## Contact: https://www.qt.io/licensing/
##
## This file is part of the plugins of the Qt Toolkit.
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

import urllib.request

# The original source for this data used to be
# 'https://git.fedorahosted.org/cgit/hwdata.git/plain/pnp.ids'
# which is discontinued. For now there seems to be a fork at:
url = 'https://github.com/vcrhonek/hwdata/raw/master/pnp.ids'

copyright = """/****************************************************************************
**
** Copyright (C) 2017 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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
"""

notice = """/*
 * This lookup table was generated from {}
 *
 * Do not change this file directly, instead edit the
 * qtbase/util/edid/qedidvendortable.py script and regenerate this file.
 */""".format(url)

header = """
#ifndef QEDIDVENDORTABLE_P_H
#define QEDIDVENDORTABLE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

QT_BEGIN_NAMESPACE

typedef struct VendorTable {
    const char id[4];
    const char name[%d];
} VendorTable;

static const struct VendorTable q_edidVendorTable[] = {"""

footer = """};

QT_END_NAMESPACE

#endif // QEDIDVENDORTABLE_P_H"""

vendors = {}

max_vendor_length = 0

response = urllib.request.urlopen(url)
data = response.read().decode('utf-8')
for line in data.split('\n'):
    l = line.split()
    if line.startswith('#'):
        continue
    elif len(l) == 0:
        continue
    else:
        pnp_id = l[0].upper()
        vendors[pnp_id] = ' '.join(l[1:])
        if len(vendors[pnp_id]) > max_vendor_length:
            max_vendor_length = len(vendors[pnp_id])

print(copyright)
print(notice)
print(header % (max_vendor_length + 1))
for pnp_id in sorted(vendors.keys()):
    print('    { "%s", "%s" },' % (pnp_id, vendors[pnp_id]))
print(footer)
