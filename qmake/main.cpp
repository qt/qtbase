/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the qmake application of the Qt Toolkit.
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

#include "project.h"
#include "property.h"
#include "option.h"
#include "cachekeys.h"
#include "metamakefile.h"
#include <qnamespace.h>
#include <qdebug.h>
#include <qregexp.h>
#include <qdir.h>
#include <qdiriterator.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#if defined(Q_OS_UNIX)
#include <errno.h>
#include <unistd.h>
#endif

#ifdef Q_OS_WIN
#  include <qt_windows.h>
#endif

using namespace QMakeInternal;

QT_BEGIN_NAMESPACE

#ifdef Q_OS_WIN

struct SedSubst {
    QRegExp from;
    QString to;
};
Q_DECLARE_TYPEINFO(SedSubst, Q_MOVABLE_TYPE);

static int doSed(int argc, char **argv)
{
    QVector<SedSubst> substs;
    QList<const char *> inFiles;
    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "-e")) {
            if (++i == argc) {
                fprintf(stderr, "Error: sed option -e requires an argument\n");
                return 3;
            }
            QString cmd = QString::fromLocal8Bit(argv[i]);
            for (int j = 0; j < cmd.length(); j++) {
                QChar c = cmd.at(j);
                if (c.isSpace())
                    continue;
                if (c != QLatin1Char('s')) {
                    fprintf(stderr, "Error: unrecognized sed command '%c'\n", c.toLatin1());
                    return 3;
                }
                QChar sep = ++j < cmd.length() ? cmd.at(j) : QChar();
                Qt::CaseSensitivity matchcase = Qt::CaseSensitive;
                bool escaped = false;
                int phase = 1;
                QStringList phases;
                QString curr;
                while (++j < cmd.length()) {
                    c = cmd.at(j);
                    if (!escaped) {
                        if (c == QLatin1Char(';'))
                            break;
                        if (c == QLatin1Char('\\')) {
                            escaped = true;
                            continue;
                        }
                        if (c == sep) {
                            phase++;
                            phases << curr;
                            curr.clear();
                            continue;
                        }
                    }
                    if (phase == 1
                        && (c == QLatin1Char('+') || c == QLatin1Char('?') || c == QLatin1Char('|')
                            || c == QLatin1Char('{') || c == QLatin1Char('}')
                            || c == QLatin1Char('(') || c == QLatin1Char(')'))) {
                        // translate sed rx to QRegExp
                        escaped ^= 1;
                    }
                    if (escaped) {
                        escaped = false;
                        curr += QLatin1Char('\\');
                    }
                    curr += c;
                }
                if (escaped) {
                    fprintf(stderr, "Error: unterminated escape sequence in sed s command\n");
                    return 3;
                }
                if (phase != 3) {
                    fprintf(stderr, "Error: sed s command requires three arguments (%d, %c, %s)\n", phase, sep.toLatin1(), qPrintable(curr));
                    return 3;
                }
                if (curr.contains(QLatin1Char('i'))) {
                    curr.remove(QLatin1Char('i'));
                    matchcase = Qt::CaseInsensitive;
                }
                if (curr != QLatin1String("g")) {
                    fprintf(stderr, "Error: sed s command supports only g & i options; g is required\n");
                    return 3;
                }
                SedSubst subst;
                subst.from = QRegExp(phases.at(0), matchcase);
                subst.to = phases.at(1);
                subst.to.replace(QLatin1String("\\\\"), QLatin1String("\\")); // QString::replace(rx, sub) groks \1, but not \\.
                substs << subst;
            }
        } else if (argv[i][0] == '-' && argv[i][1] != 0) {
            fprintf(stderr, "Error: unrecognized sed option '%s'\n", argv[i]);
            return 3;
        } else {
            inFiles << argv[i];
        }
    }
    if (inFiles.isEmpty())
        inFiles << "-";
    for (const char *inFile : qAsConst(inFiles)) {
        FILE *f;
        if (!strcmp(inFile, "-")) {
            f = stdin;
        } else if (!(f = fopen(inFile, "rb"))) {
            perror(inFile);
            return 1;
        }
        QTextStream is(f);
        while (!is.atEnd()) {
            QString line = is.readLine();
            for (int i = 0; i < substs.size(); i++)
                line.replace(substs.at(i).from, substs.at(i).to);
            puts(qPrintable(line));
        }
        if (f != stdin)
            fclose(f);
    }
    return 0;
}

static int doLink(int argc, char **argv)
{
    bool isSymlink = false;
    bool force = false;
    QList<const char *> inFiles;
    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "-s")) {
            isSymlink = true;
        } else if (!strcmp(argv[i], "-f")) {
            force = true;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Error: unrecognized ln option '%s'\n", argv[i]);
            return 3;
        } else {
            inFiles << argv[i];
        }
    }
    if (inFiles.size() != 2) {
        fprintf(stderr, "Error: this ln requires exactly two file arguments\n");
        return 3;
    }
    if (!isSymlink) {
        fprintf(stderr, "Error: this ln supports faking symlinks only\n");
        return 3;
    }
    QString target = QString::fromLocal8Bit(inFiles[0]);
    QString linkname = QString::fromLocal8Bit(inFiles[1]);

    QDir destdir;
    QFileInfo tfi(target);
    QFileInfo lfi(linkname);
    if (lfi.isDir()) {
        destdir.setPath(linkname);
        lfi.setFile(destdir, tfi.fileName());
    } else {
        destdir.setPath(lfi.path());
    }
    if (!destdir.exists()) {
        fprintf(stderr, "Error: destination directory %s does not exist\n", qPrintable(destdir.path()));
        return 1;
    }
    tfi.setFile(destdir.absoluteFilePath(tfi.filePath()));
    if (!tfi.exists()) {
        fprintf(stderr, "Error: this ln does not support symlinking non-existing targets\n");
        return 3;
    }
    if (tfi.isDir()) {
        fprintf(stderr, "Error: this ln does not support symlinking directories\n");
        return 3;
    }
    if (lfi.exists()) {
        if (!force) {
            fprintf(stderr, "Error: %s exists\n", qPrintable(lfi.filePath()));
            return 1;
        }
        if (!QFile::remove(lfi.filePath())) {
            fprintf(stderr, "Error: cannot overwrite %s\n", qPrintable(lfi.filePath()));
            return 1;
        }
    }
    if (!QFile::copy(tfi.filePath(), lfi.filePath())) {
        fprintf(stderr, "Error: cannot copy %s to %s\n",
                qPrintable(tfi.filePath()), qPrintable(lfi.filePath()));
        return 1;
    }

    return 0;
}

#endif

static bool setFilePermissions(QFile &file, QFileDevice::Permissions permissions)
{
    if (file.setPermissions(permissions))
        return true;
    fprintf(stderr, "Error setting permissions on %s: %s\n",
            qPrintable(file.fileName()), qPrintable(file.errorString()));
    return false;
}

static bool copyFileTimes(QFile &targetFile, const QString &sourceFilePath,
                          bool mustEnsureWritability, QString *errorString)
{
#ifdef Q_OS_WIN
    bool mustRestorePermissions = false;
    QFileDevice::Permissions targetPermissions;
    if (mustEnsureWritability) {
        targetPermissions = targetFile.permissions();
        if (!targetPermissions.testFlag(QFileDevice::WriteUser)) {
            mustRestorePermissions = true;
            if (!setFilePermissions(targetFile, targetPermissions | QFileDevice::WriteUser))
                return false;
        }
    }
#endif
    if (!IoUtils::touchFile(targetFile.fileName(), sourceFilePath, errorString))
        return false;
#ifdef Q_OS_WIN
    if (mustRestorePermissions && !setFilePermissions(targetFile, targetPermissions))
        return false;
#endif
    return true;
}

static int installFile(const QString &source, const QString &target, bool exe = false,
                       bool preservePermissions = false)
{
    QFile sourceFile(source);
    QFile targetFile(target);
    if (targetFile.exists()) {
#ifdef Q_OS_WIN
        targetFile.setPermissions(targetFile.permissions() | QFile::WriteUser);
#endif
        QFile::remove(target);
    } else {
        QDir::root().mkpath(QFileInfo(target).absolutePath());
    }

    if (!sourceFile.copy(target)) {
        fprintf(stderr, "Error copying %s to %s: %s\n", source.toLatin1().constData(), qPrintable(target), qPrintable(sourceFile.errorString()));
        return 3;
    }

    QFileDevice::Permissions targetPermissions = preservePermissions
            ? sourceFile.permissions()
            : (QFileDevice::ReadOwner | QFileDevice::WriteOwner
               | QFileDevice::ReadUser | QFileDevice::WriteUser
               | QFileDevice::ReadGroup | QFileDevice::ReadOther);
    if (exe) {
        targetPermissions |= QFileDevice::ExeOwner | QFileDevice::ExeUser |
                QFileDevice::ExeGroup | QFileDevice::ExeOther;
    }
    if (!setFilePermissions(targetFile, targetPermissions))
        return 3;

    QString error;
    if (!copyFileTimes(targetFile, sourceFile.fileName(), preservePermissions, &error)) {
        fprintf(stderr, "%s", qPrintable(error));
        return 3;
    }

    return 0;
}

static int installFileOrDirectory(const QString &source, const QString &target,
                                  bool preservePermissions = false)
{
    QFileInfo fi(source);
    if (false) {
#if defined(Q_OS_UNIX)
    } else if (fi.isSymLink()) {
        QString linkTarget;
        if (!IoUtils::readLinkTarget(fi.absoluteFilePath(), &linkTarget)) {
            fprintf(stderr, "Could not read link %s: %s\n", qPrintable(fi.absoluteFilePath()), strerror(errno));
            return 3;
        }
        QFile::remove(target);
        if (::symlink(linkTarget.toLocal8Bit().constData(), target.toLocal8Bit().constData()) < 0) {
            fprintf(stderr, "Could not create link: %s\n", strerror(errno));
            return 3;
        }
#endif
    } else if (fi.isDir()) {
        QDir::current().mkpath(target);

        QDirIterator it(source, QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);
        while (it.hasNext()) {
            it.next();
            const QFileInfo &entry = it.fileInfo();
            const QString &entryTarget = target + QDir::separator() + entry.fileName();

            const int recursionResult = installFileOrDirectory(entry.filePath(), entryTarget, true);
            if (recursionResult != 0)
                return recursionResult;
        }
    } else {
        const int fileCopyResult = installFile(source, target, /*exe*/ false, preservePermissions);
        if (fileCopyResult != 0)
            return fileCopyResult;
    }
    return 0;
}

static int doQInstall(int argc, char **argv)
{
    bool installExecutable = false;
    if (argc == 3 && !strcmp(argv[0], "-exe")) {
        installExecutable = true;
        --argc;
        ++argv;
    }

    if (argc != 2 && !installExecutable) {
        fprintf(stderr, "Error: usage: [-exe] source target\n");
        return 3;
    }

    const QString source = QString::fromLocal8Bit(argv[0]);
    const QString target = QString::fromLocal8Bit(argv[1]);

    if (installExecutable)
        return installFile(source, target, /*exe=*/true);
    return installFileOrDirectory(source, target);
}


static int doInstall(int argc, char **argv)
{
    if (!argc) {
        fprintf(stderr, "Error: -install requires further arguments\n");
        return 3;
    }
#ifdef Q_OS_WIN
    if (!strcmp(argv[0], "sed"))
        return doSed(argc - 1, argv + 1);
    if (!strcmp(argv[0], "ln"))
        return doLink(argc - 1, argv + 1);
#endif
    if (!strcmp(argv[0], "qinstall"))
        return doQInstall(argc - 1, argv + 1);
    fprintf(stderr, "Error: unrecognized -install subcommand '%s'\n", argv[0]);
    return 3;
}


#ifdef Q_OS_WIN

static int dumpMacros(const wchar_t *cmdline)
{
    // from http://stackoverflow.com/questions/3665537/how-to-find-out-cl-exes-built-in-macros
    int argc;
    wchar_t **argv = CommandLineToArgvW(cmdline, &argc);
    if (!argv)
        return 2;
    for (int i = 0; i < argc; ++i) {
        if (argv[i][0] != L'-' || argv[i][1] != 'D')
            continue;

        wchar_t *value = wcschr(argv[i], L'=');
        if (value) {
            *value = 0;
            ++value;
        } else {
            // point to the NUL at the end, so we don't print anything
            value = argv[i] + wcslen(argv[i]);
        }
        wprintf(L"#define %Ls %Ls\n", argv[i] + 2, value);
    }
    return 0;
}

#endif // Q_OS_WIN

/* This is to work around lame implementation on Darwin. It has been noted that the getpwd(3) function
   is much too slow, and called much too often inside of Qt (every fileFixify). With this we use a locally
   cached copy because I can control all the times it is set (because Qt never sets the pwd under me).
*/
static QString pwd;
QString qmake_getpwd()
{
    if(pwd.isNull())
        pwd = QDir::currentPath();
    return pwd;
}
bool qmake_setpwd(const QString &p)
{
    if(QDir::setCurrent(p)) {
        pwd = QDir::currentPath();
        return true;
    }
    return false;
}

int runQMake(int argc, char **argv)
{
    qSetGlobalQHashSeed(0);

    // stderr is unbuffered by default, but stdout buffering depends on whether
    // there is a terminal attached. Buffering can make output from stderr and stdout
    // appear out of sync, so force stdout to be unbuffered as well.
    // This is particularly important for things like QtCreator and scripted builds.
    setvbuf(stdout, (char *)NULL, _IONBF, 0);

    // Workaround for inferior/missing command line tools on Windows: make our own!
    if (argc >= 2 && !strcmp(argv[1], "-install"))
        return doInstall(argc - 2, argv + 2);

#ifdef Q_OS_WIN
    {
        // Support running as Visual C++'s compiler
        const wchar_t *cmdline = _wgetenv(L"MSC_CMD_FLAGS");
        if (!cmdline || !*cmdline)
            cmdline = _wgetenv(L"MSC_IDE_FLAGS");
        if (cmdline && *cmdline)
            return dumpMacros(cmdline);
    }
#endif

    QMakeVfs vfs;
    Option::vfs = &vfs;
    QMakeGlobals globals;
    Option::globals = &globals;

    // parse command line
    int ret = Option::init(argc, argv);
    if(ret != Option::QMAKE_CMDLINE_SUCCESS) {
        if ((ret & Option::QMAKE_CMDLINE_ERROR) != 0)
            return 1;
        return 0;
    }

    QString oldpwd = qmake_getpwd();

    Option::output_dir = oldpwd; //for now this is the output dir
    if (!Option::output.fileName().isEmpty() && Option::output.fileName() != "-") {
        // The output 'filename', as given by the -o option, might include one
        // or more directories, so we may need to rebase the output directory.
        QFileInfo fi(Option::output);

        QDir dir(QDir::cleanPath(fi.isDir() ? fi.absoluteFilePath() : fi.absolutePath()));

        // Don't treat Xcode project directory as part of OUT_PWD
        if (dir.dirName().endsWith(QLatin1String(".xcodeproj"))) {
            // Note: we're intentionally not using cdUp(), as the dir may not exist
            dir.setPath(QDir::cleanPath(dir.filePath("..")));
        }

        Option::output_dir = dir.path();
        QString absoluteFilePath = QDir::cleanPath(fi.absoluteFilePath());
        Option::output.setFileName(absoluteFilePath.mid(Option::output_dir.length() + 1));
    }

    QMakeProperty prop;
    if(Option::qmake_mode == Option::QMAKE_QUERY_PROPERTY ||
       Option::qmake_mode == Option::QMAKE_SET_PROPERTY ||
       Option::qmake_mode == Option::QMAKE_UNSET_PROPERTY)
        return prop.exec() ? 0 : 101;
    globals.setQMakeProperty(&prop);

    ProFileCache proFileCache;
    Option::proFileCache = &proFileCache;
    QMakeParser parser(&proFileCache, &vfs, &Option::evalHandler);
    Option::parser = &parser;

    QMakeProject project;
    int exit_val = 0;
    QStringList files;
    if(Option::qmake_mode == Option::QMAKE_GENERATE_PROJECT)
        files << "(*hack*)"; //we don't even use files, but we do the for() body once
    else
        files = Option::mkfile::project_files;
    for(QStringList::Iterator pfile = files.begin(); pfile != files.end(); pfile++) {
        if(Option::qmake_mode == Option::QMAKE_GENERATE_MAKEFILE ||
           Option::qmake_mode == Option::QMAKE_GENERATE_PRL) {
            QString fn = Option::normalizePath(*pfile);
            if(!QFile::exists(fn)) {
                fprintf(stderr, "Cannot find file: %s.\n",
                        QDir::toNativeSeparators(fn).toLatin1().constData());
                exit_val = 2;
                continue;
            }

            //setup pwd properly
            debug_msg(1, "Resetting dir to: %s",
                      QDir::toNativeSeparators(oldpwd).toLatin1().constData());
            qmake_setpwd(oldpwd); //reset the old pwd
            int di = fn.lastIndexOf(QLatin1Char('/'));
            if(di != -1) {
                debug_msg(1, "Changing dir to: %s",
                          QDir::toNativeSeparators(fn.left(di)).toLatin1().constData());
                if(!qmake_setpwd(fn.left(di)))
                    fprintf(stderr, "Cannot find directory: %s\n",
                            QDir::toNativeSeparators(fn.left(di)).toLatin1().constData());
                fn = fn.right(fn.length() - di - 1);
            }

            Option::prepareProject(fn);

            // read project..
            if(!project.read(fn)) {
                fprintf(stderr, "Error processing project file: %s\n",
                        QDir::toNativeSeparators(*pfile).toLatin1().constData());
                exit_val = 3;
                continue;
            }
            if (Option::mkfile::do_preprocess) {
                project.dump();
                continue; //no need to create makefile
            }
        }

        bool success = true;
        MetaMakefileGenerator *mkfile = MetaMakefileGenerator::createMetaGenerator(&project, QString(), false, &success);
        if (!success)
            exit_val = 3;

        if (mkfile && !mkfile->write()) {
            if(Option::qmake_mode == Option::QMAKE_GENERATE_PROJECT)
                fprintf(stderr, "Unable to generate project file.\n");
            else
                fprintf(stderr, "Unable to generate makefile for: %s\n",
                        QDir::toNativeSeparators(*pfile).toLatin1().constData());
            exit_val = 5;
        }
        delete mkfile;
        mkfile = nullptr;
    }
    qmakeClearCaches();
    return exit_val;
}

QT_END_NAMESPACE

int main(int argc, char **argv)
{
    return QT_PREPEND_NAMESPACE(runQMake)(argc, argv);
}
