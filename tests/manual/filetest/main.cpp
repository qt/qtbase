// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QDateTime>
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
"          find FILES            list file information recursively\n"
"          stat FILES            print detailed file information\n"
"          mv  SOURCE TARGET     rename files using QFile::rename\n"
"          cp  SOURCE TARGET     copy files using QFile::copy\n"
"          rm  FILE              remove file using QFile::remove\n"
"          rmr DIR               remove directory recursively\n"
"                                using QDir::removeRecursively\n"
"          trash FILES           moves the file or directory to trash\n";

std::ostream &operator<<(std::ostream &o, const QString &str)
{
    return o << qPrintable(str);
}

std::ostream &operator<<(std::ostream &o, const QDateTime &dt)
{
    if (dt.isValid())
        return o << dt.toString(Qt::ISODateWithMs);
    return o << '-';
}

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

static int ls(int argCount, const char **args, int depth = 0, int maxDepth = 1)
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

        if (depth < maxDepth && fi.isDir()) {
            QDir dir(fi.absoluteFilePath());
            const QStringList entries = dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot);
            for (const QString &s : entries) {
                QByteArray encoded = QFile::encodeName(dir.filePath(s));
                const char *ptr = encoded.constData();
                ls(1, &ptr, depth + 1, maxDepth);
            }
        }
    }
    return 0;
}

static int stat(int argCount, char **args)
{
    for (int i = 0 ; i < argCount; ++i) {
        const QFileInfo fi(QFile::decodeName(args[i]));
        std::cout << "Name:\t" << fi.fileName() << std::endl;
        std::cout << "Path:\t" << QDir::toNativeSeparators(fi.path())
                  << " (" << QDir::toNativeSeparators(fi.absolutePath()) << ')' << std::endl;
        std::cout << "Size:\t" << fi.size()
                  << "\tType: "
                  << (fi.isSymLink() && !fi.exists() ? "Broken symlink" :
                      !fi.exists() ? "Non-existent" :
                      fi.isSymLink() ? "Symlink to " : "")
                  << (!fi.exists() ? "" :
                      fi.isFile() ? "Regular file" :
                      fi.isDir() ? "Directory" : "Special node")
                  << std::endl;
        if (fi.isSymLink())
            std::cout << "Target:\t" << fi.symLinkTarget() << std::endl;
        std::cout << "Attrs:  "
                  << (fi.isReadable() ? " readable" : "")
                  << (fi.isWritable() ? " writable" : "")
                  << (fi.isExecutable() ? " executable" : "")
                  << (fi.isHidden() ? " hidden" : "")
                  << (fi.isNativePath() ? " nativepath" : "")
                  << (fi.isRoot() ? " root" : "")
                  << (fi.isBundle() ? " bundle" : "")
                  << std::endl;
        std::cout << "Mode:\t" << permissions(fi) << std::endl;
        std::cout << "Owner:\t" << fi.owner() << " (" << fi.ownerId()
                  << ")\tGroup:\t" << fi.group() << " ("  << fi.groupId() << ')' << std::endl;
        std::cout << "Access:\t" << fi.lastRead() << std::endl;
        std::cout << "Birth:\t" << fi.birthTime() << std::endl;
        std::cout << "Change:\t" << fi.metadataChangeTime() << std::endl;
        std::cout << "Modified: " << fi.lastModified() << std::endl;
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
    QDir dir(QString::fromLocal8Bit(dirName));
    if (!dir.removeRecursively()) {
        qWarning().nospace() << "Failed to remove " << dir.absolutePath();
        return -1;
    }

    return 0;
}

static int trash(const char *filename)
{
    QFile f(QString::fromLocal8Bit(filename));
    if (!f.moveToTrash()) {
        qWarning().nospace() << "Failed to trash " << f.fileName()
                             << ": " << f.errorString();
        return -1;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    Q_UNUSED(a);
    if (argc >= 3 && !qstrcmp(argv[1], "ls"))
        return ls(argc -2, const_cast<const char **>(argv + 2));

    if (argc >= 3 && !qstrcmp(argv[1], "find"))
        return ls(argc -2, const_cast<const char **>(argv + 2), 0, std::numeric_limits<int>::max());

    if (argc >= 3 && !qstrcmp(argv[1], "stat"))
        return stat(argc -2, argv + 2);

    if (argc == 4 && !qstrcmp(argv[1], "mv"))
        return mv(argv[2], argv[3]);

    if (argc == 4 && !qstrcmp(argv[1], "cp"))
        return cp(argv[2], argv[3]);

    if (argc == 3 && !qstrcmp(argv[1], "rm"))
        return rm(argv[2]);

    if (argc == 3 && !qstrcmp(argv[1], "rmr"))
        return rmr(argv[2]);

    if (argc == 3 && !qstrcmp(argv[1], "trash"))
        return trash(argv[2]);

    std::cerr << usage1 << argv[0] << usage2;
    return 0;
}
