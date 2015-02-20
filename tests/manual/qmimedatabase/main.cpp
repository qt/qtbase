/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/QMimeDatabase>
#include <QtCore/QMimeType>
#include <QtCore/QCoreApplication>
#include <QtCore/QCommandLineParser>
#include <QtCore/QCommandLineOption>
#include <QtCore/QDebug>

#include <iostream>
#include <algorithm>
#include <iterator>

std::wostream &operator<<(std::wostream &str, const QString &s)
{
    str << s.toStdWString();
    return str;
}

template <class T>
std::wostream &operator<<(std::wostream &str, const QList<T> &l)
{
    std::copy(l.constBegin(), l.constEnd(),
              std::ostream_iterator<T, wchar_t>(str, L" "));
    return str;
}

std::wostream &operator<<(std::wostream &str, const QMimeType &type)
{
    str << "Type    : " << type.name();
    const QStringList aliases = type.aliases();
    if (!aliases.isEmpty())
        str << " (" << aliases << ')';
    str << '\n';

    const QStringList parentMimeTypes = type.parentMimeTypes();
    if (!parentMimeTypes.isEmpty())
        str << "Inherits: " << parentMimeTypes << '\n';

    if (!type.comment().isEmpty())
        str << "Comment : " << type.comment() << '\n';

    const QStringList globPatterns = type.globPatterns();
    if (!globPatterns.isEmpty())
        str << "Patterns: " << globPatterns << '\n';

    if (!type.preferredSuffix().isEmpty()) {
        str << "Suffix  : " << type.preferredSuffix();
        const QStringList suffixes = type.suffixes();
        if (suffixes.size() > 1)
            str << " (" << suffixes << ')';
        str << '\n';
    }

    if (!type.iconName().isEmpty())
        str << "Icon    : " << type.iconName() << '\n';

    return str;
}

bool operator<(const QMimeType &t1, const QMimeType &t2)
{
    return t1.name() < t2.name();
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption matchContentOnlyOption(QStringList() << "c" << "content",
                                              "Use only the file content for determining the mimetype.");
    parser.addOption(matchContentOnlyOption);
    QCommandLineOption matchFileOnlyOption(QStringList() << "f" << "filename-only",
                                           "Whether use the file name only for determining the mimetype. Not used if -c is specified.");
    parser.addOption(matchFileOnlyOption);
    QCommandLineOption dumpAllOption("a", "Dump all mime types.");
    parser.addOption(dumpAllOption);
    parser.addPositionalArgument("file", "The file(s) to analyze.");
    parser.process(app);

    QMimeDatabase::MatchMode matchMode = QMimeDatabase::MatchDefault;
    if (parser.isSet(matchContentOnlyOption))
        matchMode = QMimeDatabase::MatchContent;
    else if (parser.isSet(matchFileOnlyOption))
        matchMode = QMimeDatabase::MatchExtension;

    const bool dumpAll = parser.isSet(dumpAllOption);

    QMimeDatabase mimeDatabase;

    if (dumpAll) {
        QList<QMimeType> mimeTypes = mimeDatabase.allMimeTypes();
        std::stable_sort(mimeTypes.begin(), mimeTypes.end());
        std::wcout << mimeTypes.size() << " mime types found.\n\n";
        foreach (const QMimeType &type, mimeTypes)
            std::wcout << type << '\n';
    } else {
        foreach (const QString &fileName, parser.positionalArguments()) {
            QMimeType data = mimeDatabase.mimeTypeForFile(fileName, matchMode);
            std::wcout << "File    : " << fileName << '\n' << data << '\n';
        }
    }

    return 0;
}
