/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the qmake application of the Qt Toolkit.
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

#include "project.h"
#include "property.h"
#include "option.h"
#include "cachekeys.h"
#include "metamakefile.h"
#include <qnamespace.h>
#include <qdebug.h>
#include <qregexp.h>
#include <qdir.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

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
                subst.to.replace("\\\\", "\\"); // QString::replace(rx, sub) groks \1, but not \\.
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
    foreach (const char *inFile, inFiles) {
        FILE *f;
        if (!strcmp(inFile, "-")) {
            f = stdin;
        } else if (!(f = fopen(inFile, "r"))) {
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

static int doInstall(int argc, char **argv)
{
    if (!argc) {
        fprintf(stderr, "Error: -install requires further arguments\n");
        return 3;
    }
    if (!strcmp(argv[0], "sed"))
        return doSed(argc - 1, argv + 1);
    if (!strcmp(argv[0], "ln"))
        return doLink(argc - 1, argv + 1);
    fprintf(stderr, "Error: unrecognized -install subcommand '%s'\n", argv[0]);
    return 3;
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
    // stderr is unbuffered by default, but stdout buffering depends on whether
    // there is a terminal attached. Buffering can make output from stderr and stdout
    // appear out of sync, so force stdout to be unbuffered as well.
    // This is particularly important for things like QtCreator and scripted builds.
    setvbuf(stdout, (char *)NULL, _IONBF, 0);

#ifdef Q_OS_WIN
    // Workaround for inferior/missing command line tools on Windows: make our own!
    if (argc >= 2 && !strcmp(argv[1], "-install"))
        return doInstall(argc - 2, argv + 2);
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
    if(Option::output.fileName() != "-") {
        QFileInfo fi(Option::output);
        QString dir;
        if(fi.isDir()) {
            dir = fi.filePath();
        } else {
            QString tmp_dir = fi.path();
            if(!tmp_dir.isEmpty() && QFile::exists(tmp_dir))
                dir = tmp_dir;
        }
#ifdef Q_OS_MAC
        if (fi.fileName().endsWith(QLatin1String(".pbxproj"))
            && dir.endsWith(QLatin1String(".xcodeproj")))
            dir += QStringLiteral("/..");
#endif
        if(!dir.isNull() && dir != ".")
            Option::output_dir = dir;
        if (QDir::isRelativePath(Option::output_dir)) {
            Option::output.setFileName(fi.fileName());
            Option::output_dir.prepend(oldpwd + QLatin1Char('/'));
        }
        Option::output_dir = QDir::cleanPath(Option::output_dir);
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
        mkfile = NULL;
    }
    qmakeClearCaches();
    return exit_val;
}

QT_END_NAMESPACE

int main(int argc, char **argv)
{
    return QT_PREPEND_NAMESPACE(runQMake)(argc, argv);
}
