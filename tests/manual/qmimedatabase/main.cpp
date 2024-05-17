// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#undef QT_NO_FOREACH // this file contains unported legacy Q_FOREACH uses

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
