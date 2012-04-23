/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the qmake application of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "option.h"
#include "cachekeys.h"
#include <qdir.h>
#include <qregexp.h>
#include <qhash.h>
#include <qdebug.h>
#include <qsettings.h>
#include <stdlib.h>
#include <stdarg.h>

QT_BEGIN_NAMESPACE

//convenience
const char *Option::application_argv0 = 0;
QString Option::prf_ext;
QString Option::prl_ext;
QString Option::libtool_ext;
QString Option::pkgcfg_ext;
QString Option::ui_ext;
QStringList Option::h_ext;
QString Option::cpp_moc_ext;
QStringList Option::cpp_ext;
QStringList Option::c_ext;
QString Option::obj_ext;
QString Option::lex_ext;
QString Option::yacc_ext;
QString Option::pro_ext;
QString Option::dir_sep;
QString Option::dirlist_sep;
QString Option::h_moc_mod;
QString Option::yacc_mod;
QString Option::lex_mod;
QString Option::res_ext;
char Option::field_sep;

//mode
Option::QMAKE_MODE Option::qmake_mode = Option::QMAKE_GENERATE_NOTHING;

//all modes
QString Option::qmake_abslocation;
int Option::warn_level = WarnLogic | WarnDeprecated;
int Option::debug_level = 0;
QFile Option::output;
QString Option::output_dir;
Option::QMAKE_RECURSIVE Option::recursive = Option::QMAKE_RECURSIVE_DEFAULT;
QStringList Option::before_user_vars;
QStringList Option::after_user_vars;
QString Option::user_template;
QString Option::user_template_prefix;
QStringList Option::shellPath;
Option::HOST_MODE Option::host_mode = Option::HOST_UNKNOWN_MODE;

//QMAKE_*_PROPERTY stuff
QStringList Option::prop::properties;

//QMAKE_GENERATE_PROJECT stuff
bool Option::projfile::do_pwd = true;
QStringList Option::projfile::project_dirs;

//QMAKE_GENERATE_MAKEFILE stuff
QString Option::mkfile::qmakespec;
QString Option::mkfile::xqmakespec;
int Option::mkfile::cachefile_depth = -1;
bool Option::mkfile::do_deps = true;
bool Option::mkfile::do_mocs = true;
bool Option::mkfile::do_dep_heuristics = true;
bool Option::mkfile::do_preprocess = false;
bool Option::mkfile::do_stub_makefile = false;
bool Option::mkfile::do_cache = true;
QString Option::mkfile::source_root;
QString Option::mkfile::build_root;
QString Option::mkfile::cachefile;
QStringList Option::mkfile::project_files;
QString Option::mkfile::qmakespec_commandline;
QString Option::mkfile::xqmakespec_commandline;

static Option::QMAKE_MODE default_mode(QString progname)
{
    int s = progname.lastIndexOf(QDir::separator());
    if(s != -1)
        progname = progname.right(progname.length() - (s + 1));
    if(progname == "qmakegen")
        return Option::QMAKE_GENERATE_PROJECT;
    else if(progname == "qt-config")
        return Option::QMAKE_QUERY_PROPERTY;
    return Option::QMAKE_GENERATE_MAKEFILE;
}

static QString detectProjectFile(const QString &path)
{
    QString ret;
    QDir dir(path);
    if(dir.exists(dir.dirName() + Option::pro_ext)) {
        ret = dir.filePath(dir.dirName()) + Option::pro_ext;
    } else { //last try..
        QStringList profiles = dir.entryList(QStringList("*" + Option::pro_ext));
        if(profiles.count() == 1)
            ret = dir.filePath(profiles.at(0));
    }
    return ret;
}

static QString cleanSpec(const QString &spec)
{
    QString ret = QDir::cleanPath(spec);
    if (ret.contains('/')) {
        const QFileInfo specDirInfo(ret);
        if (specDirInfo.exists() && specDirInfo.isDir())
            ret = QDir::cleanPath(specDirInfo.absoluteFilePath());
    }
    return ret;
}

QString project_builtin_regx();
bool usage(const char *a0)
{
    fprintf(stdout, "Usage: %s [mode] [options] [files]\n"
            "\n"
            "QMake has two modes, one mode for generating project files based on\n"
            "some heuristics, and the other for generating makefiles. Normally you\n"
            "shouldn't need to specify a mode, as makefile generation is the default\n"
            "mode for qmake, but you may use this to test qmake on an existing project\n"
            "\n"
            "Mode:\n"
            "  -project       Put qmake into project file generation mode%s\n"
            "                 In this mode qmake interprets files as files to\n"
            "                 be built,\n"
            "                 defaults to %s\n"
            "                 Note: The created .pro file probably will \n"
            "                 need to be edited. For example add the QT variable to \n"
            "                 specify what modules are required.\n"
            "  -makefile      Put qmake into makefile generation mode%s\n"
            "                 In this mode qmake interprets files as project files to\n"
            "                 be processed, if skipped qmake will try to find a project\n"
            "                 file in your current working directory\n"
            "\n"
            "Warnings Options:\n"
            "  -Wnone         Turn off all warnings; specific ones may be re-enabled by\n"
            "                 later -W options\n"
            "  -Wall          Turn on all warnings\n"
            "  -Wparser       Turn on parser warnings\n"
            "  -Wlogic        Turn on logic warnings (on by default)\n"
            "  -Wdeprecated   Turn on deprecation warnings (on by default)\n"
            "\n"
            "Options:\n"
            "   * You can place any variable assignment in options and it will be     *\n"
            "   * processed as if it was in [files]. These assignments will be parsed *\n"
            "   * before [files].                                                     *\n"
            "  -o file        Write output to file\n"
            "  -d             Increase debug level\n"
            "  -t templ       Overrides TEMPLATE as templ\n"
            "  -tp prefix     Overrides TEMPLATE so that prefix is prefixed into the value\n"
            "  -help          This help\n"
            "  -v             Version information\n"
            "  -after         All variable assignments after this will be\n"
            "                 parsed after [files]\n"
            "  -norecursive   Don't do a recursive search\n"
            "  -recursive     Do a recursive search\n"
            "  -set <prop> <value> Set persistent property\n"
            "  -unset <prop>  Unset persistent property\n"
            "  -query <prop>  Query persistent property. Show all if <prop> is empty.\n"
            "  -cache file    Use file as cache           [makefile mode only]\n"
            "  -spec spec     Use spec as QMAKESPEC       [makefile mode only]\n"
            "  -nocache       Don't use a cache file      [makefile mode only]\n"
            "  -nodepend      Don't generate dependencies [makefile mode only]\n"
            "  -nomoc         Don't generate moc targets  [makefile mode only]\n"
            "  -nopwd         Don't look for files in pwd [project mode only]\n"
            ,a0,
            default_mode(a0) == Option::QMAKE_GENERATE_PROJECT  ? " (default)" : "", project_builtin_regx().toLatin1().constData(),
            default_mode(a0) == Option::QMAKE_GENERATE_MAKEFILE ? " (default)" : ""
        );
    return false;
}

int
Option::parseCommandLine(int argc, char **argv, int skip)
{
    QStringList user_configs;

    bool before = true;
    for(int x = skip; x < argc; x++) {
        if(*argv[x] == '-' && strlen(argv[x]) > 1) { /* options */
            QString opt = argv[x] + 1;

            //first param is a mode, or we default
            if(x == 1) {
                bool specified = true;
                if(opt == "project") {
                    Option::recursive = Option::QMAKE_RECURSIVE_YES;
                    Option::qmake_mode = Option::QMAKE_GENERATE_PROJECT;
                } else if(opt == "prl") {
                    Option::mkfile::do_deps = false;
                    Option::mkfile::do_mocs = false;
                    Option::qmake_mode = Option::QMAKE_GENERATE_PRL;
                } else if(opt == "set") {
                    Option::qmake_mode = Option::QMAKE_SET_PROPERTY;
                } else if(opt == "unset") {
                    Option::qmake_mode = Option::QMAKE_UNSET_PROPERTY;
                } else if(opt == "query") {
                    Option::qmake_mode = Option::QMAKE_QUERY_PROPERTY;
                } else if(opt == "makefile") {
                    Option::qmake_mode = Option::QMAKE_GENERATE_MAKEFILE;
                } else {
                    specified = false;
                }
                if(specified)
                    continue;
            }
            //all modes
            if(opt == "o" || opt == "output") {
                Option::output.setFileName(argv[++x]);
            } else if(opt == "after") {
                before = false;
            } else if(opt == "t" || opt == "template") {
                Option::user_template = argv[++x];
            } else if(opt == "tp" || opt == "template_prefix") {
                Option::user_template_prefix = argv[++x];
            } else if(opt == "macx") {
                Option::host_mode = HOST_MACX_MODE;
            } else if(opt == "unix") {
                Option::host_mode = HOST_UNIX_MODE;
            } else if(opt == "win32") {
                Option::host_mode = HOST_WIN_MODE;
            } else if(opt == "d") {
                Option::debug_level++;
            } else if(opt == "version" || opt == "v" || opt == "-version") {
                fprintf(stdout,
                        "QMake version %s\n"
                        "Using Qt version %s in %s\n",
                        qmake_version(), QT_VERSION_STR,
                        QLibraryInfo::location(QLibraryInfo::LibrariesPath).toLatin1().constData());
#ifdef QMAKE_OPENSOURCE_VERSION
                fprintf(stdout, "QMake is Open Source software from Nokia Corporation and/or its subsidiary(-ies).\n");
#endif
                return Option::QMAKE_CMDLINE_BAIL;
            } else if(opt == "h" || opt == "help") {
                return Option::QMAKE_CMDLINE_SHOW_USAGE;
            } else if(opt == "Wall") {
                Option::warn_level |= WarnAll;
            } else if(opt == "Wparser") {
                Option::warn_level |= WarnParser;
            } else if(opt == "Wlogic") {
                Option::warn_level |= WarnLogic;
            } else if(opt == "Wdeprecated") {
                Option::warn_level |= WarnDeprecated;
            } else if(opt == "Wnone") {
                Option::warn_level = WarnNone;
            } else if(opt == "r" || opt == "recursive") {
                Option::recursive = Option::QMAKE_RECURSIVE_YES;
            } else if(opt == "nr" || opt == "norecursive") {
                Option::recursive = Option::QMAKE_RECURSIVE_NO;
            } else if(opt == "config") {
                user_configs += argv[++x];
            } else {
                if(Option::qmake_mode == Option::QMAKE_GENERATE_MAKEFILE ||
                   Option::qmake_mode == Option::QMAKE_GENERATE_PRL) {
                    if(opt == "nodepend" || opt == "nodepends") {
                        Option::mkfile::do_deps = false;
                    } else if(opt == "nomoc") {
                        Option::mkfile::do_mocs = false;
                    } else if(opt == "nocache") {
                        Option::mkfile::do_cache = false;
                    } else if(opt == "createstub") {
                        Option::mkfile::do_stub_makefile = true;
                    } else if(opt == "nodependheuristics") {
                        Option::mkfile::do_dep_heuristics = false;
                    } else if(opt == "E") {
                        fprintf(stderr, "-E is deprecated. Use -d instead.\n");
                        Option::mkfile::do_preprocess = true;
                    } else if(opt == "cache") {
                        Option::mkfile::cachefile = argv[++x];
                    } else if(opt == "platform" || opt == "spec") {
                        Option::mkfile::qmakespec = cleanSpec(argv[++x]);
                        Option::mkfile::qmakespec_commandline = argv[x];
                    } else if (opt == "xplatform" || opt == "xspec") {
                        Option::mkfile::xqmakespec = cleanSpec(argv[++x]);
                        Option::mkfile::xqmakespec_commandline = argv[x];
                    } else {
                        fprintf(stderr, "***Unknown option -%s\n", opt.toLatin1().constData());
                        return Option::QMAKE_CMDLINE_SHOW_USAGE | Option::QMAKE_CMDLINE_ERROR;
                    }
                } else if(Option::qmake_mode == Option::QMAKE_GENERATE_PROJECT) {
                    if(opt == "nopwd") {
                        Option::projfile::do_pwd = false;
                    } else {
                        fprintf(stderr, "***Unknown option -%s\n", opt.toLatin1().constData());
                        return Option::QMAKE_CMDLINE_SHOW_USAGE | Option::QMAKE_CMDLINE_ERROR;
                    }
                }
            }
        } else {
            QString arg = argv[x];
            if(arg.indexOf('=') != -1) {
                if(before)
                    Option::before_user_vars.append(arg);
                else
                    Option::after_user_vars.append(arg);
            } else {
                bool handled = true;
                if(Option::qmake_mode == Option::QMAKE_QUERY_PROPERTY ||
                    Option::qmake_mode == Option::QMAKE_SET_PROPERTY ||
                    Option::qmake_mode == Option::QMAKE_UNSET_PROPERTY) {
                    Option::prop::properties.append(arg);
                } else {
                    QFileInfo fi(arg);
                    if(!fi.makeAbsolute()) //strange
                        arg = fi.filePath();
                    if(Option::qmake_mode == Option::QMAKE_GENERATE_MAKEFILE ||
                       Option::qmake_mode == Option::QMAKE_GENERATE_PRL) {
                        if(fi.isDir()) {
                            QString proj = detectProjectFile(arg);
                            if (!proj.isNull())
                                arg = proj;
                        }
                        Option::mkfile::project_files.append(arg);
                    } else if(Option::qmake_mode == Option::QMAKE_GENERATE_PROJECT) {
                        Option::projfile::project_dirs.append(arg);
                    } else {
                        handled = false;
                    }
                }
                if(!handled) {
                    return Option::QMAKE_CMDLINE_SHOW_USAGE | Option::QMAKE_CMDLINE_ERROR;
                }
            }
        }
    }

    if (!user_configs.isEmpty())
        Option::before_user_vars += "CONFIG += " + user_configs.join(" ");

    if (Option::mkfile::xqmakespec.isEmpty())
        Option::mkfile::xqmakespec = Option::mkfile::qmakespec;

    return Option::QMAKE_CMDLINE_SUCCESS;
}

int
Option::init(int argc, char **argv)
{
#if defined(Q_OS_MAC)
    Option::host_mode = Option::HOST_MACX_MODE;
#elif defined(Q_OS_UNIX)
    Option::host_mode = Option::HOST_UNIX_MODE;
#else
    Option::host_mode = Option::HOST_WIN_MODE;
#endif
    Option::application_argv0 = 0;
    Option::prf_ext = ".prf";
    Option::pro_ext = ".pro";
#ifdef Q_OS_WIN
    Option::dirlist_sep = ";";
#else
    Option::dirlist_sep = ":";
#endif
    Option::field_sep = ' ';

    if(argc && argv) {
        Option::application_argv0 = argv[0];
        QString argv0 = argv[0];
        if(Option::qmake_mode == Option::QMAKE_GENERATE_NOTHING)
            Option::qmake_mode = default_mode(argv0);
        if(!argv0.isEmpty() && !QFileInfo(argv0).isRelative()) {
            Option::qmake_abslocation = argv0;
        } else if (argv0.contains(QLatin1Char('/'))
#ifdef Q_OS_WIN
		   || argv0.contains(QLatin1Char('\\'))
#endif
	    ) { //relative PWD
            Option::qmake_abslocation = QDir::current().absoluteFilePath(argv0);
        } else { //in the PATH
            QByteArray pEnv = qgetenv("PATH");
            QDir currentDir = QDir::current();
#ifdef Q_OS_WIN
            QStringList paths = QString::fromLocal8Bit(pEnv).split(QLatin1String(";"));
#else
            QStringList paths = QString::fromLocal8Bit(pEnv).split(QLatin1String(":"));
#endif
            for (QStringList::const_iterator p = paths.constBegin(); p != paths.constEnd(); ++p) {
                if ((*p).isEmpty())
                    continue;
                QString candidate = currentDir.absoluteFilePath(*p + QLatin1Char('/') + argv0);
#ifdef Q_OS_WIN
                candidate += ".exe";
#endif
                if (QFile::exists(candidate)) {
                    Option::qmake_abslocation = candidate;
                    break;
                }
            }
        }
        if(!Option::qmake_abslocation.isNull())
            Option::qmake_abslocation = QDir::cleanPath(Option::qmake_abslocation);
        else // This is rather unlikely to ever happen on a modern system ...
            Option::qmake_abslocation = QLibraryInfo::rawLocation(QLibraryInfo::HostBinariesPath,
                                                                  QLibraryInfo::EffectivePaths) +
#ifdef Q_OS_WIN
                    "/qmake.exe";
#else
                    "/qmake";
#endif
    } else {
        Option::qmake_mode = Option::QMAKE_GENERATE_MAKEFILE;
    }

    const QByteArray envflags = qgetenv("QMAKEFLAGS");
    if (!envflags.isNull()) {
        int env_argc = 0, env_size = 0, currlen=0;
        char quote = 0, **env_argv = NULL;
        for (int i = 0; i < envflags.size(); ++i) {
            if (!quote && (envflags.at(i) == '\'' || envflags.at(i) == '"')) {
                quote = envflags.at(i);
            } else if (envflags.at(i) == quote) {
                quote = 0;
            } else if (!quote && envflags.at(i) == ' ') {
                if (currlen && env_argv && env_argv[env_argc]) {
                    env_argv[env_argc][currlen] = '\0';
                    currlen = 0;
                    env_argc++;
                }
            } else {
                if(!env_argv || env_argc > env_size) {
                    env_argv = (char **)realloc(env_argv, sizeof(char *)*(env_size+=10));
                    for(int i2 = env_argc; i2 < env_size; i2++)
                        env_argv[i2] = NULL;
                }
                if(!env_argv[env_argc]) {
                    currlen = 0;
                    env_argv[env_argc] = (char*)malloc(255);
                }
                if(currlen < 255)
                    env_argv[env_argc][currlen++] = envflags.at(i);
            }
        }
        if(env_argv) {
            if(env_argv[env_argc]) {
                env_argv[env_argc][currlen] = '\0';
                currlen = 0;
                env_argc++;
            }
            parseCommandLine(env_argc, env_argv);
            for(int i2 = 0; i2 < env_size; i2++) {
                if(env_argv[i2])
                    free(env_argv[i2]);
            }
            free(env_argv);
        }
    }
    if(argc && argv) {
        int ret = parseCommandLine(argc, argv, 1);
        if(ret != Option::QMAKE_CMDLINE_SUCCESS) {
            if ((ret & Option::QMAKE_CMDLINE_SHOW_USAGE) != 0)
                usage(argv[0]);
            return ret;
            //return ret == QMAKE_CMDLINE_SHOW_USAGE ? usage(argv[0]) : false;
        }
    }

    //last chance for defaults
    if(Option::qmake_mode == Option::QMAKE_GENERATE_MAKEFILE ||
        Option::qmake_mode == Option::QMAKE_GENERATE_PRL) {
        if (Option::mkfile::xqmakespec.isEmpty())
            Option::mkfile::xqmakespec = QString::fromLocal8Bit(qgetenv("XQMAKESPEC").constData());
        if (Option::mkfile::qmakespec.isEmpty()) {
            Option::mkfile::qmakespec = QString::fromLocal8Bit(qgetenv("QMAKESPEC").constData());
            if (Option::mkfile::xqmakespec.isEmpty())
                Option::mkfile::xqmakespec = Option::mkfile::qmakespec;
        }

        //try REALLY hard to do it for them, lazy..
        if(Option::mkfile::project_files.isEmpty()) {
            QString proj = detectProjectFile(qmake_getpwd());
            if(!proj.isNull())
                Option::mkfile::project_files.append(proj);
#ifndef QT_BUILD_QMAKE_LIBRARY
            if(Option::mkfile::project_files.isEmpty()) {
                usage(argv[0]);
                return Option::QMAKE_CMDLINE_ERROR;
            }
#endif
        }
    }

    //defaults for globals
    if (Option::host_mode == Option::HOST_WIN_MODE) {
        Option::dir_sep = "\\";
    } else {
        Option::dir_sep = "/";
    }

    return QMAKE_CMDLINE_SUCCESS;
}

void Option::prepareProject(const QString &pfile)
{
    QString srcpath = (pfile != "-")
            ? QDir::cleanPath(QFileInfo(pfile).absolutePath()) : qmake_getpwd();
    if (srcpath != output_dir) {
        if (!srcpath.endsWith(QLatin1Char('/')))
            srcpath += QLatin1Char('/');
        QString dstpath = output_dir;
        if (!dstpath.endsWith(QLatin1Char('/')))
            dstpath += QLatin1Char('/');
        int srcLen = srcpath.length();
        int dstLen = dstpath.length();
        int lastSl = -1;
        while (++lastSl, srcpath.at(--srcLen) == dstpath.at(--dstLen))
            if (srcpath.at(srcLen) == QLatin1Char('/'))
                lastSl = 0;
        mkfile::source_root = srcpath.left(srcLen + lastSl);
        mkfile::build_root = dstpath.left(dstLen + lastSl);
    } else {
        mkfile::source_root.clear();
    }
}

bool Option::postProcessProject(QMakeProject *project)
{
    Option::cpp_ext = project->values("QMAKE_EXT_CPP");
    Option::h_ext = project->values("QMAKE_EXT_H");
    Option::c_ext = project->values("QMAKE_EXT_C");
    Option::res_ext = project->first("QMAKE_EXT_RES");
    Option::pkgcfg_ext = project->first("QMAKE_EXT_PKGCONFIG");
    Option::libtool_ext = project->first("QMAKE_EXT_LIBTOOL");
    Option::prl_ext = project->first("QMAKE_EXT_PRL");
    Option::ui_ext = project->first("QMAKE_EXT_UI");
    Option::cpp_moc_ext = project->first("QMAKE_EXT_CPP_MOC");
    Option::lex_ext = project->first("QMAKE_EXT_LEX");
    Option::yacc_ext = project->first("QMAKE_EXT_YACC");
    Option::obj_ext = project->first("QMAKE_EXT_OBJ");
    Option::h_moc_mod = project->first("QMAKE_H_MOD_MOC");
    Option::lex_mod = project->first("QMAKE_MOD_LEX");
    Option::yacc_mod = project->first("QMAKE_MOD_YACC");
    Option::dir_sep = project->first("QMAKE_DIR_SEP");
    Option::shellPath = project->values("QMAKE_SH");
    return true;
}

QString
Option::fixString(QString string, uchar flags)
{
    //const QString orig_string = string;
    static QHash<FixStringCacheKey, QString> *cache = 0;
    if(!cache) {
        cache = new QHash<FixStringCacheKey, QString>;
        qmakeAddCacheClear(qmakeDeleteCacheClear<QHash<FixStringCacheKey, QString> >, (void**)&cache);
    }
    FixStringCacheKey cacheKey(string, flags);

    QHash<FixStringCacheKey, QString>::const_iterator it = cache->constFind(cacheKey);

    if (it != cache->constEnd()) {
        //qDebug() << "Fix (cached) " << orig_string << "->" << it.value();
        return it.value();
    }

    //fix the environment variables
    if(flags & Option::FixEnvVars) {
        int rep;
        static QRegExp reg_var("\\$\\(.*\\)");
        reg_var.setMinimal(true);
        while((rep = reg_var.indexIn(string)) != -1)
            string.replace(rep, reg_var.matchedLength(),
                           QString::fromLocal8Bit(qgetenv(string.mid(rep + 2, reg_var.matchedLength() - 3).toLatin1().constData()).constData()));
    }

    //canonicalize it (and treat as a path)
    if(flags & Option::FixPathCanonicalize) {
#if 0
        string = QFileInfo(string).canonicalFilePath();
#endif
        string = QDir::cleanPath(string);
    }

    if(string.length() > 2 && string[0].isLetter() && string[1] == QLatin1Char(':'))
        string[0] = string[0].toLower();

    bool localSep = (flags & Option::FixPathToLocalSeparators) != 0;
    bool targetSep = (flags & Option::FixPathToTargetSeparators) != 0;
    bool normalSep = (flags & Option::FixPathToNormalSeparators) != 0;

    // either none or only one active flag
    Q_ASSERT(localSep + targetSep + normalSep <= 1);
    //fix separators
    if (flags & Option::FixPathToNormalSeparators) {
        string = string.replace('\\', '/');
    } else if (flags & Option::FixPathToLocalSeparators) {
#if defined(Q_OS_WIN32)
        string = string.replace('/', '\\');
#else
        string = string.replace('\\', '/');
#endif
    } else if(flags & Option::FixPathToTargetSeparators) {
        string = string.replace('/', Option::dir_sep).replace('\\', Option::dir_sep);
    }

    if ((string.startsWith("\"") && string.endsWith("\"")) ||
        (string.startsWith("\'") && string.endsWith("\'")))
        string = string.mid(1, string.length()-2);

    //cache
    //qDebug() << "Fix" << orig_string << "->" << string;
    cache->insert(cacheKey, string);
    return string;
}

const char *qmake_version()
{
    static char *ret = NULL;
    if(ret)
        return ret;
    ret = (char *)malloc(15);
    qmakeAddCacheClear(qmakeFreeCacheClear, (void**)&ret);
#if defined(_MSC_VER) && _MSC_VER >= 1400
    sprintf_s(ret, 15, "%d.%02d%c", QMAKE_VERSION_MAJOR, QMAKE_VERSION_MINOR, 'a' + QMAKE_VERSION_PATCH);
#else
    sprintf(ret, "%d.%02d%c", QMAKE_VERSION_MAJOR, QMAKE_VERSION_MINOR, 'a' + QMAKE_VERSION_PATCH);
#endif
    return ret;
}

void debug_msg_internal(int level, const char *fmt, ...)
{
    if(Option::debug_level < level)
        return;
    fprintf(stderr, "DEBUG %d: ", level);
    {
        va_list ap;
        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
    }
    fprintf(stderr, "\n");
}

void warn_msg(QMakeWarn type, const char *fmt, ...)
{
    if(!(Option::warn_level & type))
        return;
    fprintf(stderr, "WARNING: ");
    {
        va_list ap;
        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
    }
    fprintf(stderr, "\n");
}

class QMakeCacheClearItem {
private:
    qmakeCacheClearFunc func;
    void **data;
public:
    QMakeCacheClearItem(qmakeCacheClearFunc f, void **d) : func(f), data(d) { }
    ~QMakeCacheClearItem() {
        (*func)(*data);
        *data = 0;
    }
};
static QList<QMakeCacheClearItem*> cache_items;

void
qmakeClearCaches()
{
    qDeleteAll(cache_items);
    cache_items.clear();
}

void
qmakeAddCacheClear(qmakeCacheClearFunc func, void **data)
{
    cache_items.append(new QMakeCacheClearItem(func, data));
}

QString qmake_libraryInfoFile()
{
    if(!Option::qmake_abslocation.isEmpty())
        return QDir(QFileInfo(Option::qmake_abslocation).absolutePath()).filePath("qt.conf");
    return QString();
}

QT_END_NAMESPACE
