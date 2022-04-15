/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#ifndef SHELLQUOTE_SHARED_H
#define SHELLQUOTE_SHARED_H

#include <QDir>
#include <QRegularExpression>
#include <QString>

// Copy-pasted from qmake/library/ioutil.cpp
inline static bool hasSpecialChars(const QString &arg, const uchar (&iqm)[16])
{
    for (int x = arg.length() - 1; x >= 0; --x) {
        ushort c = arg.unicode()[x].unicode();
        if ((c < sizeof(iqm) * 8) && (iqm[c / 8] & (1 << (c & 7))))
            return true;
    }
    return false;
}

static QString shellQuoteUnix(const QString &arg)
{
    // Chars that should be quoted (TM). This includes:
    static const uchar iqm[] = {
        0xff, 0xff, 0xff, 0xff, 0xdf, 0x07, 0x00, 0xd8,
        0x00, 0x00, 0x00, 0x38, 0x01, 0x00, 0x00, 0x78
    }; // 0-32 \'"$`<>|;&(){}*?#!~[]

    if (!arg.length())
        return QLatin1String("\"\"");

    QString ret(arg);
    if (hasSpecialChars(ret, iqm)) {
        ret.replace(QLatin1Char('\''), QLatin1String("'\\''"));
        ret.prepend(QLatin1Char('\''));
        ret.append(QLatin1Char('\''));
    }
    return ret;
}

static QString shellQuoteWin(const QString &arg)
{
    // Chars that should be quoted (TM). This includes:
    // - control chars & space
    // - the shell meta chars "&()<>^|
    // - the potential separators ,;=
    static const uchar iqm[] = {
        0xff, 0xff, 0xff, 0xff, 0x45, 0x13, 0x00, 0x78,
        0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x10
    };

    if (!arg.length())
        return QLatin1String("\"\"");

    QString ret(arg);
    if (hasSpecialChars(ret, iqm)) {
        // Quotes are escaped and their preceding backslashes are doubled.
        // It's impossible to escape anything inside a quoted string on cmd
        // level, so the outer quoting must be "suspended".
        ret.replace(QRegularExpression(QLatin1String("(\\\\*)\"")), QLatin1String("\"\\1\\1\\^\"\""));
        // The argument must not end with a \ since this would be interpreted
        // as escaping the quote -- rather put the \ behind the quote: e.g.
        // rather use "foo"\ than "foo\"
        int i = ret.length();
        while (i > 0 && ret.at(i - 1) == QLatin1Char('\\'))
            --i;
        ret.insert(i, QLatin1Char('"'));
        ret.prepend(QLatin1Char('"'));
    }
    return ret;
}

static QString shellQuote(const QString &arg)
{
    if (QDir::separator() == QLatin1Char('\\'))
        return shellQuoteWin(arg);
    else
        return shellQuoteUnix(arg);
}

#endif // SHELLQUOTE_SHARED_H
