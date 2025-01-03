#!/usr/bin/env python3
# Copyright (C) 2017 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import urllib.request

# The original source for this data used to be
# 'https://git.fedorahosted.org/cgit/hwdata.git/plain/pnp.ids'
# which is discontinued. For now there seems to be a fork at:
url = 'https://github.com/vcrhonek/hwdata/raw/master/pnp.ids'

copyright = """
// Copyright (C) 2017 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
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

#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

struct VendorTable {
    const char id[4];
    const char name[%d];
};

static const VendorTable q_edidVendorTable[] = {"""

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
