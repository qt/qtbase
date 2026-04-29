// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QPARSE_COMMON_P_H
#define QPARSE_COMMON_P_H

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

#include <QtCore/private/qglobal_p.h>
#include <QtCore/qassert.h>
#include <QtCore/qstringview.h>

QT_BEGIN_NAMESPACE

namespace QtParseCommon {
struct ParsedText
{
    qsizetype startIndex = 0;
    qsizetype endIndex = 0;

    constexpr operator bool() const noexcept { return endIndex > startIndex; }
    constexpr bool isEmpty() const { return endIndex <= startIndex; }
    // size() is a UTF-16 length, not a count of Unicode characters:
    constexpr qsizetype size() const { return endIndex - startIndex; }
    // text must be the same as was passed to whatever's parsing produced this:
    constexpr QStringView used(QStringView text) const
    {
        Q_PRE(0 <= startIndex);
        Q_PRE(startIndex <= endIndex);
        Q_PRE(endIndex <= text.size());
        return text.first(endIndex).sliced(startIndex);
    }
};
}

QT_END_NAMESPACE

#endif // QPARSE_COMMON_P_H
