// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSTRINGALGORITHMS_P_H
#define QSTRINGALGORITHMS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of internal files.  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

#include "qstring.h"
#include "qlocale_p.h"      // for ascii_isspace

QT_BEGIN_NAMESPACE

template <typename StringType> struct QStringAlgorithms
{
    typedef typename StringType::value_type Char;
    typedef typename StringType::size_type size_type;
    typedef typename std::remove_cv<StringType>::type NakedStringType;
    static const bool isConst = std::is_const<StringType>::value;

    static inline bool isSpace(char ch) { return ascii_isspace(ch); }
    static inline bool isSpace(QChar ch) { return ch.isSpace(); }

    // Surrogate pairs are not handled in either of the functions below. That is
    // not a problem because there are no space characters (Zs, Zl, Zp) outside the
    // Basic Multilingual Plane.

    static inline StringType trimmed_helper_inplace(NakedStringType &str, const Char *begin, const Char *end)
    {
        // in-place trimming:
        Char *data = const_cast<Char *>(str.cbegin());
        if (begin != data)
            memmove(data, begin, (end - begin) * sizeof(Char));
        str.resize(end - begin);
        return std::move(str);
    }

    static inline StringType trimmed_helper_inplace(const NakedStringType &, const Char *, const Char *)
    {
        // can't happen
        Q_UNREACHABLE_RETURN(StringType());
    }

    struct TrimPositions {
        const Char *begin;
        const Char *end;
    };
    // Returns {begin, end} where:
    // - "begin" refers to the first non-space character
    // - if there is a sequence of one or more space chacaters at the end,
    //   "end" refers to the first character in that sequence, otherwise
    //   "end" is str.cend()
    [[nodiscard]] static TrimPositions trimmed_helper_positions(const StringType &str)
    {
        const Char *begin = str.cbegin();
        const Char *end = str.cend();
        // skip white space from end
        while (begin < end && isSpace(end[-1]))
            --end;
        // skip white space from start
        while (begin < end && isSpace(*begin))
            begin++;
        return {begin, end};
    }

    static inline StringType trimmed_helper(StringType &str)
    {
        const auto [begin, end] = trimmed_helper_positions(str);
        if (begin == str.cbegin() && end == str.cend())
            return str;
        if (!isConst && str.isDetached())
            return trimmed_helper_inplace(str, begin, end);
        return StringType(begin, end - begin);
    }

    static inline StringType simplified_helper(StringType &str)
    {
        if (str.isEmpty())
            return str;
        const Char *src = str.cbegin();
        const Char *end = str.cend();
        NakedStringType result = isConst || !str.isDetached() ?
                                     StringType(str.size(), Qt::Uninitialized) :
                                     std::move(str);

        Char *dst = const_cast<Char *>(result.cbegin());
        Char *ptr = dst;
        bool unmodified = true;
        while (true) {
            while (src != end && isSpace(*src))
                ++src;
            while (src != end && !isSpace(*src))
                *ptr++ = *src++;
            if (src == end)
                break;
            if (*src != QChar::Space)
                unmodified = false;
            *ptr++ = QChar::Space;
        }
        if (ptr != dst && ptr[-1] == QChar::Space)
            --ptr;

        qsizetype newlen = ptr - dst;
        if (isConst && newlen == str.size() && unmodified) {
            // nothing happened, return the original
            return str;
        }
        result.resize(newlen);
        return result;
    }
};

QT_END_NAMESPACE

#endif // QSTRINGALGORITHMS_P_H
