/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Copyright (C) 2020 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QSTRINGCONVERTER_P_H
#define QSTRINGCONVERTER_P_H

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

#include <QtCore/qstring.h>
#include <QtCore/qendian.h>
#include <QtCore/qstringconverter.h>

QT_BEGIN_NAMESPACE

#ifndef __cpp_char8_t
enum char8_t : uchar {};
#endif

struct QUtf8BaseTraits
{
    static const bool isTrusted = false;
    static const bool allowNonCharacters = true;
    static const bool skipAsciiHandling = false;
    static const int Error = -1;
    static const int EndOfString = -2;

    static bool isValidCharacter(uint u)
    { return int(u) >= 0; }

    static void appendByte(uchar *&ptr, uchar b)
    { *ptr++ = b; }

    static void appendByte(char8_t *&ptr, char8_t b)
    { *ptr++ = b; }

    static uchar peekByte(const uchar *ptr, qsizetype n = 0)
    { return ptr[n]; }

    static uchar peekByte(const char8_t *ptr, int n = 0)
    { return ptr[n]; }

    static qptrdiff availableBytes(const uchar *ptr, const uchar *end)
    { return end - ptr; }

    static qptrdiff availableBytes(const char8_t *ptr, const char8_t *end)
    { return end - ptr; }

    static void advanceByte(const uchar *&ptr, qsizetype n = 1)
    { ptr += n; }

    static void advanceByte(const char8_t *&ptr, int n = 1)
    { ptr += n; }

    static void appendUtf16(ushort *&ptr, ushort uc)
    { *ptr++ = uc; }

    static void appendUtf16(char16_t *&ptr, ushort uc)
    { *ptr++ = char16_t(uc); }

    static void appendUcs4(ushort *&ptr, uint uc)
    {
        appendUtf16(ptr, QChar::highSurrogate(uc));
        appendUtf16(ptr, QChar::lowSurrogate(uc));
    }

    static void appendUcs4(char16_t *&ptr, char32_t uc)
    {
        appendUtf16(ptr, QChar::highSurrogate(uc));
        appendUtf16(ptr, QChar::lowSurrogate(uc));
    }

    static ushort peekUtf16(const ushort *ptr, qsizetype n = 0)
    { return ptr[n]; }

    static ushort peekUtf16(const char16_t *ptr, int n = 0)
    { return ptr[n]; }

    static qptrdiff availableUtf16(const ushort *ptr, const ushort *end)
    { return end - ptr; }

    static qptrdiff availableUtf16(const char16_t *ptr, const char16_t *end)
    { return end - ptr; }

    static void advanceUtf16(const ushort *&ptr, qsizetype n = 1)
    { ptr += n; }

    static void advanceUtf16(const char16_t *&ptr, int n = 1)
    { ptr += n; }

    // it's possible to output to UCS-4 too
    static void appendUtf16(uint *&ptr, ushort uc)
    { *ptr++ = uc; }

    static void appendUtf16(char32_t *&ptr, ushort uc)
    { *ptr++ = char32_t(uc); }

    static void appendUcs4(uint *&ptr, uint uc)
    { *ptr++ = uc; }

    static void appendUcs4(char32_t *&ptr, uint uc)
    { *ptr++ = char32_t(uc); }
};

struct QUtf8BaseTraitsNoAscii : public QUtf8BaseTraits
{
    static const bool skipAsciiHandling = true;
};

namespace QUtf8Functions
{
    /// returns 0 on success; errors can only happen if \a u is a surrogate:
    /// Error if \a u is a low surrogate;
    /// if \a u is a high surrogate, Error if the next isn't a low one,
    /// EndOfString if we run into the end of the string.
    template <typename Traits, typename OutputPtr, typename InputPtr> inline
    int toUtf8(ushort u, OutputPtr &dst, InputPtr &src, InputPtr end)
    {
        if (!Traits::skipAsciiHandling && u < 0x80) {
            // U+0000 to U+007F (US-ASCII) - one byte
            Traits::appendByte(dst, uchar(u));
            return 0;
        } else if (u < 0x0800) {
            // U+0080 to U+07FF - two bytes
            // first of two bytes
            Traits::appendByte(dst, 0xc0 | uchar(u >> 6));
        } else {
            if (!QChar::isSurrogate(u)) {
                // U+0800 to U+FFFF (except U+D800-U+DFFF) - three bytes
                if (!Traits::allowNonCharacters && QChar::isNonCharacter(u))
                    return Traits::Error;

                // first of three bytes
                Traits::appendByte(dst, 0xe0 | uchar(u >> 12));
            } else {
                // U+10000 to U+10FFFF - four bytes
                // need to get one extra codepoint
                if (Traits::availableUtf16(src, end) == 0)
                    return Traits::EndOfString;

                ushort low = Traits::peekUtf16(src);
                if (!QChar::isHighSurrogate(u))
                    return Traits::Error;
                if (!QChar::isLowSurrogate(low))
                    return Traits::Error;

                Traits::advanceUtf16(src);
                uint ucs4 = QChar::surrogateToUcs4(u, low);

                if (!Traits::allowNonCharacters && QChar::isNonCharacter(ucs4))
                    return Traits::Error;

                // first byte
                Traits::appendByte(dst, 0xf0 | (uchar(ucs4 >> 18) & 0xf));

                // second of four bytes
                Traits::appendByte(dst, 0x80 | (uchar(ucs4 >> 12) & 0x3f));

                // for the rest of the bytes
                u = ushort(ucs4);
            }

            // second to last byte
            Traits::appendByte(dst, 0x80 | (uchar(u >> 6) & 0x3f));
        }

        // last byte
        Traits::appendByte(dst, 0x80 | (u & 0x3f));
        return 0;
    }

    inline bool isContinuationByte(uchar b)
    {
        return (b & 0xc0) == 0x80;
    }

    /// returns the number of characters consumed (including \a b) in case of success;
    /// returns negative in case of error: Traits::Error or Traits::EndOfString
    template <typename Traits, typename OutputPtr, typename InputPtr> inline
    qsizetype fromUtf8(uchar b, OutputPtr &dst, InputPtr &src, InputPtr end)
    {
        qsizetype charsNeeded;
        uint min_uc;
        uint uc;

        if (!Traits::skipAsciiHandling && b < 0x80) {
            // US-ASCII
            Traits::appendUtf16(dst, b);
            return 1;
        }

        if (!Traits::isTrusted && Q_UNLIKELY(b <= 0xC1)) {
            // an UTF-8 first character must be at least 0xC0
            // however, all 0xC0 and 0xC1 first bytes can only produce overlong sequences
            return Traits::Error;
        } else if (b < 0xe0) {
            charsNeeded = 2;
            min_uc = 0x80;
            uc = b & 0x1f;
        } else if (b < 0xf0) {
            charsNeeded = 3;
            min_uc = 0x800;
            uc = b & 0x0f;
        } else if (b < 0xf5) {
            charsNeeded = 4;
            min_uc = 0x10000;
            uc = b & 0x07;
        } else {
            // the last Unicode character is U+10FFFF
            // it's encoded in UTF-8 as "\xF4\x8F\xBF\xBF"
            // therefore, a byte higher than 0xF4 is not the UTF-8 first byte
            return Traits::Error;
        }

        qptrdiff bytesAvailable = Traits::availableBytes(src, end);
        if (Q_UNLIKELY(bytesAvailable < charsNeeded - 1)) {
            // it's possible that we have an error instead of just unfinished bytes
            if (bytesAvailable > 0 && !isContinuationByte(Traits::peekByte(src, 0)))
                return Traits::Error;
            if (bytesAvailable > 1 && !isContinuationByte(Traits::peekByte(src, 1)))
                return Traits::Error;
            return Traits::EndOfString;
        }

        // first continuation character
        b = Traits::peekByte(src, 0);
        if (!isContinuationByte(b))
            return Traits::Error;
        uc <<= 6;
        uc |= b & 0x3f;

        if (charsNeeded > 2) {
            // second continuation character
            b = Traits::peekByte(src, 1);
            if (!isContinuationByte(b))
                return Traits::Error;
            uc <<= 6;
            uc |= b & 0x3f;

            if (charsNeeded > 3) {
                // third continuation character
                b = Traits::peekByte(src, 2);
                if (!isContinuationByte(b))
                    return Traits::Error;
                uc <<= 6;
                uc |= b & 0x3f;
            }
        }

        // we've decoded something; safety-check it
        if (!Traits::isTrusted) {
            if (uc < min_uc)
                return Traits::Error;
            if (QChar::isSurrogate(uc) || uc > QChar::LastValidCodePoint)
                return Traits::Error;
            if (!Traits::allowNonCharacters && QChar::isNonCharacter(uc))
                return Traits::Error;
        }

        // write the UTF-16 sequence
        if (!QChar::requiresSurrogates(uc)) {
            // UTF-8 decoded and no surrogates are required
            // detach if necessary
            Traits::appendUtf16(dst, ushort(uc));
        } else {
            // UTF-8 decoded to something that requires a surrogate pair
            Traits::appendUcs4(dst, uc);
        }

        Traits::advanceByte(src, charsNeeded - 1);
        return charsNeeded;
    }
}

enum DataEndianness
{
    DetectEndianness,
    BigEndianness,
    LittleEndianness
};

struct QUtf8
{
    Q_CORE_EXPORT static QChar *convertToUnicode(QChar *buffer, QByteArrayView in) noexcept;
    static QString convertToUnicode(QByteArrayView in);
    Q_CORE_EXPORT static QString convertToUnicode(QByteArrayView in, QStringConverter::State *state);
    static QChar *convertToUnicode(QChar *out, QByteArrayView in, QStringConverter::State *state);
    Q_CORE_EXPORT static QByteArray convertFromUnicode(QStringView in);
    Q_CORE_EXPORT static QByteArray convertFromUnicode(QStringView in, QStringConverterBase::State *state);
    static char *convertFromUnicode(char *out, QStringView in, QStringConverter::State *state);
    struct ValidUtf8Result {
        bool isValidUtf8;
        bool isValidAscii;
    };
    static ValidUtf8Result isValidUtf8(QByteArrayView in);
    static int compareUtf8(QByteArrayView utf8, QStringView utf16) noexcept;
    static int compareUtf8(QByteArrayView utf8, QLatin1String s);
};

struct QUtf16
{
    Q_CORE_EXPORT static QString convertToUnicode(QByteArrayView, QStringConverter::State *, DataEndianness = DetectEndianness);
    static QChar *convertToUnicode(QChar *out, QByteArrayView, QStringConverter::State *state, DataEndianness endian);
    Q_CORE_EXPORT static QByteArray convertFromUnicode(QStringView, QStringConverter::State *, DataEndianness = DetectEndianness);
    static char *convertFromUnicode(char *out, QStringView in, QStringConverter::State *state, DataEndianness endian);
};

struct QUtf32
{
    static QChar *convertToUnicode(QChar *out, QByteArrayView, QStringConverter::State *state, DataEndianness endian);
    Q_CORE_EXPORT static QString convertToUnicode(QByteArrayView, QStringConverter::State *, DataEndianness = DetectEndianness);
    Q_CORE_EXPORT static QByteArray convertFromUnicode(QStringView, QStringConverter::State *, DataEndianness = DetectEndianness);
    static char *convertFromUnicode(char *out, QStringView in, QStringConverter::State *state, DataEndianness endian);
};

struct Q_CORE_EXPORT QLocal8Bit
{
#if !defined(Q_OS_WIN) || defined(QT_BOOTSTRAPPED)
    static QString convertToUnicode(QByteArrayView in, QStringConverter::State *state)
    { return QUtf8::convertToUnicode(in, state); }
    static QByteArray convertFromUnicode(QStringView in, QStringConverter::State *state)
    { return QUtf8::convertFromUnicode(in, state); }
#else
    static QString convertToUnicode(QByteArrayView, QStringConverter::State *);
    static QByteArray convertFromUnicode(QStringView, QStringConverter::State *);
#endif
};

QT_END_NAMESPACE

#endif // QSTRINGCONVERTER_P_H
