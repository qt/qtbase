// Copyright (C) 2018 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
