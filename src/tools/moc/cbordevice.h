/****************************************************************************
**
** Copyright (C) 2018 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef CBORDEVICE_H
#define CBORDEVICE_H

#include <memory>
#include <stdio.h>

#define CBOR_API            inline
#define CBOR_PRIVATE_API    inline
#define CBOR_NO_PARSER_API  1
#include <cbor.h>

class CborDevice
{
public:
    CborDevice(FILE *out) : out(out) {}

    void nextItem(const char *comment = nullptr)
    {
        i = 0;
        if (comment)
            fprintf(out, "\n    // %s", comment);
    }

    static CborError callback(void *self, const void *ptr, size_t len, CborEncoderAppendType t)
    {
        auto that = static_cast<CborDevice *>(self);
        auto data = static_cast<const char *>(ptr);
        if (t == CborEncoderAppendCborData) {
            while (len--)
                that->putByte(*data++);
        } else {
            while (len--)
                that->putChar(*data++);
        }
        return CborNoError;
    }

private:
    FILE *out;
    int i = 0;

    void putNewline()
    {
        if ((i++ % 8) == 0)
            fputs("\n   ", out);
    }

    void putByte(uint8_t c)
    {
        putNewline();
        fprintf(out, " 0x%02x, ", c);
    }

    void putChar(char c)
    {
        putNewline();
        if (uchar(c) < 0x20)
            fprintf(out, " '\\x%x',", uint8_t(c));
        else if (uchar(c) >= 0x7f)
            fprintf(out, " uchar('\\x%x'),", uint8_t(c));
        else if (c == '\'' || c == '\\')
            fprintf(out, " '\\%c',", c);
        else
            fprintf(out, " '%c', ", c);
    }
};

#endif // CBORDEVICE_H
