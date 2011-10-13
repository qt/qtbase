/****************************************************************************
**
** Copyright (C) 2012 Intel Corporation
** Contact: http://www.qt-project.org/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qipaddress_p.h"
#include "private/qlocale_tools_p.h"
#include "qvarlengtharray.h"

QT_BEGIN_NAMESPACE
namespace QIPAddressUtils {

static QString number(quint8 val, int base = 10)
{
    QChar zero(0x30);
    return val ? qulltoa(val, base, zero) : zero;
}

typedef QVarLengthArray<char, 64> Buffer;
static bool checkedToAscii(Buffer &buffer, const QChar *begin, const QChar *end)
{
    const ushort *const ubegin = reinterpret_cast<const ushort *>(begin);
    const ushort *const uend = reinterpret_cast<const ushort *>(end);
    const ushort *src = ubegin;

    buffer.resize(uend - ubegin + 1);
    char *dst = buffer.data();

    while (src != uend) {
        if (*src >= 0x7f)
            return false;
        *dst++ = *src++;
    }
    *dst = '\0';
    return true;
}

bool parseIp4(IPv4Address &address, const QChar *begin, const QChar *end)
{
    Q_ASSERT(begin != end);
    Buffer buffer;
    if (!checkedToAscii(buffer, begin, end))
        return false;

    int dotCount = 0;
    address = 0;
    const char *ptr = buffer.data();
    while (dotCount < 4) {
        const char *endptr;
        bool ok;
        quint64 ll = qstrtoull(ptr, &endptr, 0, &ok);
        quint32 x = ll;
        if (!ok || endptr == ptr || ll != x)
            return false;

        if (*endptr == '.' || dotCount == 3) {
            if (x & ~0xff)
                return false;
            address <<= 8;
        } else if (dotCount == 2) {
            if (x & ~0xffff)
                return false;
            address <<= 16;
        } else if (dotCount == 1) {
            if (x & ~0xffffff)
                return false;
            address <<= 24;
        }
        address |= x;

        if (dotCount == 3 && *endptr != '\0')
            return false;
        else if (dotCount == 3 || *endptr == '\0')
            return true;
        ++dotCount;
        ptr = endptr + 1;
    }
    return false;
}

void toString(QString &appendTo, IPv4Address address)
{
    // reconstructing is easy
    // use the fast operator% that pre-calculates the size
    appendTo += number(address >> 24)
                % QLatin1Char('.')
                % number(address >> 16)
                % QLatin1Char('.')
                % number(address >> 8)
                % QLatin1Char('.')
                % number(address);
}

}
QT_END_NAMESPACE
