// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QTBASE_DEPFILE_SHARED_H
#define QTBASE_DEPFILE_SHARED_H

#include <QByteArray>
#include <QString>
#include <QFile>

// Escape characters in given path. Dependency paths are Make-style, not NMake/Jom style.
// The paths can also be consumed by Ninja.
// "$" replaced by "$$"
// "#" replaced by "\#"
// " " replaced by "\ "
// "\#" replaced by "\\#"
// "\ " replaced by "\\\ "
//
// The escape rules are according to what clang / llvm escapes when generating a Make-style
// dependency file.
// Is a template function, because input param can be either a QString or a QByteArray.
template <typename T> struct CharType;
template <> struct CharType<QString> { using type = QLatin1Char; };
template <> struct CharType<QByteArray> { using type = char; };
template <typename StringType>
StringType escapeDependencyPath(const StringType &path)
{
    using CT = typename CharType<StringType>::type;
    StringType escapedPath;
    int size = path.size();
    escapedPath.reserve(size);
    for (int i = 0; i < size; ++i) {
        if (path[i] == CT('$')) {
            escapedPath.append(CT('$'));
        } else if (path[i] == CT('#')) {
            escapedPath.append(CT('\\'));
        } else if (path[i] == CT(' ')) {
            escapedPath.append(CT('\\'));
            int backwards_it = i - 1;
            while (backwards_it > 0 && path[backwards_it] == CT('\\')) {
                escapedPath.append(CT('\\'));
                --backwards_it;
            }
        }
        escapedPath.append(path[i]);
    }
    return escapedPath;
}

static inline QByteArray escapeAndEncodeDependencyPath(const QString &path)
{
    return QFile::encodeName(escapeDependencyPath(path));
}

#endif // QTBASE_DEPFILE_SHARED_H
