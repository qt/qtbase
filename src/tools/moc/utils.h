// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef UTILS_H
#define UTILS_H

#include <QtCore/qglobal.h>
#include <private/qtools_p.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

inline bool is_whitespace(char s)
{
    return (s == ' ' || s == '\t' || s == '\n');
}

inline bool is_space(char s)
{
    return (s == ' ' || s == '\t');
}

inline bool is_ident_start(char s)
{
    using namespace QtMiscUtils;
    return isAsciiLower(s) || isAsciiUpper(s) || s == '_' || s == '$';
}

inline bool is_ident_char(char s)
{
    return QtMiscUtils::isAsciiLetterOrNumber(s) || s == '_' || s == '$';
}

inline bool is_identifier(const char *s, qsizetype len)
{
    if (len < 1)
        return false;
    return std::all_of(s, s + len, is_ident_char);
}

inline const char *skipQuote(const char *data)
{
    while (*data && (*data != '\"')) {
        if (*data == '\\') {
            ++data;
            if (!*data) break;
        }
        ++data;
    }

    if (*data)  //Skip last quote
        ++data;
    return data;
}

QT_END_NAMESPACE

#endif // UTILS_H
