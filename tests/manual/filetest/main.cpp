/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

#include <QDebug>
#include <QCoreApplication>
#include <QFileInfo>
#include <QFile>
#include <QDir>

#include <iostream>
#include <string>

static const char usage1[] =
"\nTests various file functionality in Qt\n\n"
"Usage: ";
static const char usage2[] =" [KEYWORD] [ARGUMENTS]\n\n"
"Keywords: ls  FILES             list file information\n"
"          mv  SOURCE TARGET     rename files using QFile::rename\n"
"          cp  SOURCE TARGET     copy files using QFile::copy\n"
"          rm  FILE              remove file using QFile::remove\n"
"          rmr DIR               remove directory recursively\n"
"                                using QDir::removeRecursively\n";

static inline std::string permissions(QFile::Permissions permissions)
{
    std::string result(10, '-');
    if (permissions & QFile::ReadOwner)
        result[1] = 'r';
    if (permissions & QFile::WriteOwner)
        result[2] = 'w';
    if (permissions & QFile::ExeOwner)
        result[3] = 'x';
    if (permissions & QFile::ReadGroup)
        result[4] = 'r';
    if (permissions & QFile::WriteGroup)
        result[5] = 'w';
    if (permissions & QFile::ExeGroup)
        result[6] = 'x';
    if (permissions & QFile::ReadOther)
        result[7] = 'r';
    if (permissions & QFile::WriteOther)
        result[8] = 'w';
    if (permissions & QFile::ExeOther)
        result[9] = 'x';
    return result;
}

static inline std::string permissions(const QFileInfo &fi)
{
    std::string result = permissions(fi.permissions());
    if (fi.isSymLink())
        result[0] = 'l';
    else if (fi.isDir())
        result[0] = 'd';
    return result;
}

static int ls(int argCount, char **args)
{
    for (int i = 0 ; i < argCount; ++i) {
        const QFileInfo fi(QString::fromLocal8Bit(args[i]));
        std::cout << QDir::toNativeSeparators(fi.absoluteFilePath()).toStdString() << ' ' << fi.size()
                  << ' ' << permissions(fi);
        if (fi.exists())
            std::cout << " [exists]";
        if (fi.isFile())
            std::cout << " [file]";
        if (fi.isSymLink()) {
            std::cout << " [symlink to "
                      << QDir::toNativeSeparators(fi.symLinkTarget()).toStdString() << ']';
        }
        if (fi.isDir())
            std::cout << " [dir]";

        std::cout << std::endl;
    }
    return 0;
}

static int mv(const char *sourceFileName, const char *targetFileName)
{
    QFile sourceFile(QString::fromLocal8Bit(sourceFileName));
    if (!sourceFile.rename(QString::fromLocal8Bit(targetFileName))) {
        qWarning().nospace() << sourceFile.errorString();
        return -1;
    }
    return 0;
}

static int cp(const char *sourceFileName, const char *targetFileName)
{
    QFile sourceFile(QString::fromLocal8Bit(sourceFileName));
    if (!sourceFile.copy(QString::fromLocal8Bit(targetFileName))) {
        qWarning().nospace() << sourceFile.errorString();
        return -1;
    }
    return 0;
}

static int rm(const char *fileName)
{
    QFile file(QString::fromLocal8Bit(fileName));
    if (!file.remove()) {
        qWarning().nospace() << file.errorString();
        return -1;
    }
    return 0;
}

static int rmr(const char *dirName)
{
#if QT_VERSION < 0x050000
    Q_UNUSED(dirName)
    return 1;
#else
    QDir dir(QString::fromLocal8Bit(dirName));
    if (!dir.removeRecursively()) {
        qWarning().nospace() << "Failed to remove " << dir.absolutePath();
        return -1;
    }

    return 0;
#endif
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    Q_UNUSED(a)
    if (argc >= 3 && !qstrcmp(argv[1], "ls"))
        return ls(argc -2, argv + 2);

    if (argc == 4 && !qstrcmp(argv[1], "mv"))
        return mv(argv[2], argv[3]);

    if (argc == 4 && !qstrcmp(argv[1], "cp"))
        return cp(argv[2], argv[3]);

    if (argc == 3 && !qstrcmp(argv[1], "rm"))
        return rm(argv[2]);

    if (argc == 3 && !qstrcmp(argv[1], "rmr"))
        return rmr(argv[2]);

    std::cerr << usage1 << argv[0] << usage2;
    return 0;
}
