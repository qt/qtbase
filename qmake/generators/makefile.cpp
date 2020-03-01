/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "makefile.h"
#include "option.h"
#include "cachekeys.h"
#include "meta.h"

#include <ioutils.h>

#include <qdir.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qregexp.h>
#include <qhash.h>
#include <qdebug.h>
#include <qbuffer.h>
#include <qdatetime.h>

#if defined(Q_OS_UNIX)
#include <unistd.h>
#else
#include <io.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

using namespace QMakeInternal;

bool MakefileGenerator::canExecute(const QStringList &cmdline, int *a) const
{
    int argv0 = -1;
    for(int i = 0; i < cmdline.count(); ++i) {
        if(!cmdline.at(i).contains('=')) {
            argv0 = i;
            break;
        }
    }
    if(a)
        *a = argv0;
    if(argv0 != -1) {
        const QString c = Option::normalizePath(cmdline.at(argv0));
        if(exists(c))
            return true;
    }
    return false;
}

QString MakefileGenerator::mkdir_p_asstring(const QString &dir, bool escape) const
{
    return "@" + makedir.arg(
        escape ? escapeFilePath(Option::fixPathToTargetOS(dir, false, false)) : dir);
}

bool MakefileGenerator::mkdir(const QString &in_path) const
{
    QString path = Option::normalizePath(in_path);
    if(QFile::exists(path))
        return true;

    return QDir().mkpath(path);
}

void
MakefileGenerator::verifyCompilers()
{
    ProValueMap &v = project->variables();
    ProStringList &quc = v["QMAKE_EXTRA_COMPILERS"];
    for(int i = 0; i < quc.size(); ) {
        bool error = false;
        const ProString &comp = quc.at(i);
        const ProKey okey(comp + ".output");
        if (v[okey].isEmpty()) {
            const ProKey ofkey(comp + ".output_function");
            if (!v[ofkey].isEmpty()) {
                v[okey].append("${QMAKE_FUNC_FILE_IN_" + v[ofkey].first() + "}");
            } else {
                error = true;
                warn_msg(WarnLogic, "Compiler: %s: No output file specified", comp.toLatin1().constData());
            }
        } else if (v[ProKey(comp + ".input")].isEmpty()) {
            error = true;
            warn_msg(WarnLogic, "Compiler: %s: No input variable specified", comp.toLatin1().constData());
        }
        if(error)
            quc.removeAt(i);
        else
            ++i;
    }
}

void
MakefileGenerator::initOutPaths()
{
    ProValueMap &v = project->variables();
    //for shadow builds
    if(!v.contains("QMAKE_ABSOLUTE_SOURCE_PATH")) {
        if (Option::globals->do_cache && !project->cacheFile().isEmpty() &&
           v.contains("QMAKE_ABSOLUTE_SOURCE_ROOT")) {
            QString root = v["QMAKE_ABSOLUTE_SOURCE_ROOT"].first().toQString();
            root = QDir::fromNativeSeparators(root);
            if(!root.isEmpty()) {
                QFileInfo fi = fileInfo(project->cacheFile());
                if(!fi.makeAbsolute()) {
                    QString cache_r = fi.path(), pwd = Option::output_dir;
                    if(pwd.startsWith(cache_r) && !pwd.startsWith(root)) {
                        pwd = root + pwd.mid(cache_r.length());
                        if(exists(pwd))
                            v.insert("QMAKE_ABSOLUTE_SOURCE_PATH", ProStringList(pwd));
                    }
                }
            }
        }
    }
    if(!v["QMAKE_ABSOLUTE_SOURCE_PATH"].isEmpty()) {
        ProString &asp = v["QMAKE_ABSOLUTE_SOURCE_PATH"].first();
        asp = QDir::fromNativeSeparators(asp.toQString());
        if(asp.isEmpty() || asp == Option::output_dir) //if they're the same, why bother?
            v["QMAKE_ABSOLUTE_SOURCE_PATH"].clear();
    }

    //some builtin directories
    if(project->isEmpty("PRECOMPILED_DIR") && !project->isEmpty("OBJECTS_DIR"))
        v["PRECOMPILED_DIR"] = v["OBJECTS_DIR"];
    static const char * const dirs[] = { "OBJECTS_DIR", "DESTDIR",
                                         "SUBLIBS_DIR", "DLLDESTDIR",
                                         "PRECOMPILED_DIR", nullptr };
    for (int x = 0; dirs[x]; x++) {
        const ProKey dkey(dirs[x]);
        if (v[dkey].isEmpty())
            continue;
        const ProString orig_path = v[dkey].first();

        ProString &pathRef = v[dkey].first();
        pathRef = fileFixify(pathRef.toQString(), FileFixifyFromOutdir);

        if (!pathRef.endsWith(Option::dir_sep))
            pathRef += Option::dir_sep;

        if (noIO() || (project->first("TEMPLATE") == "subdirs"))
            continue;

        QString path = project->first(dkey).toQString(); //not to be changed any further
        path = fileFixify(path, FileFixifyBackwards);
        debug_msg(3, "Fixed output_dir %s (%s) into %s", dirs[x],
                  orig_path.toLatin1().constData(), path.toLatin1().constData());
        if(!mkdir(path))
            warn_msg(WarnLogic, "%s: Cannot access directory '%s'", dirs[x],
                     path.toLatin1().constData());
    }

    //out paths from the extra compilers
    const ProStringList &quc = project->values("QMAKE_EXTRA_COMPILERS");
    for (ProStringList::ConstIterator it = quc.begin(); it != quc.end(); ++it) {
        QString tmp_out = project->first(ProKey(*it + ".output")).toQString();
        if(tmp_out.isEmpty())
            continue;
        const ProStringList &tmp = project->values(ProKey(*it + ".input"));
        for (ProStringList::ConstIterator it2 = tmp.begin(); it2 != tmp.end(); ++it2) {
            ProStringList &inputs = project->values((*it2).toKey());
            for (ProStringList::Iterator input = inputs.begin(); input != inputs.end(); ++input) {
                QString finp = fileFixify((*input).toQString(), FileFixifyFromOutdir);
                QString path = replaceExtraCompilerVariables(tmp_out, finp, QString(), NoShell);
                path = Option::normalizePath(path);
                int slash = path.lastIndexOf('/');
                if(slash != -1) {
                    path = path.left(slash);
                    // Make out path only if it does not contain makefile variables
                    if(!path.contains("${"))
                        if(path != "." &&
                           !mkdir(fileFixify(path, FileFixifyBackwards)))
                            warn_msg(WarnLogic, "%s: Cannot access directory '%s'",
                                     (*it).toLatin1().constData(), path.toLatin1().constData());
                }
            }
        }
    }

    if(!v["DESTDIR"].isEmpty()) {
        QDir d(v["DESTDIR"].first().toQString());
        if (Option::normalizePath(d.absolutePath()) == Option::normalizePath(Option::output_dir))
            v.remove("DESTDIR");
    }
}

QMakeProject
*MakefileGenerator::projectFile() const
{
    return project;
}

void
MakefileGenerator::setProjectFile(QMakeProject *p)
{
    if(project)
        return;
    project = p;
    if (project->isActiveConfig("win32"))
        target_mode = TARG_WIN_MODE;
    else if (project->isActiveConfig("mac"))
        target_mode = TARG_MAC_MODE;
    else
        target_mode = TARG_UNIX_MODE;
    init();
    bool linkPrl = (Option::qmake_mode == Option::QMAKE_GENERATE_MAKEFILE)
                   && project->isActiveConfig("link_prl");
    bool mergeLflags = !project->isActiveConfig("no_smart_library_merge")
                       && !project->isActiveConfig("no_lflags_merge");
    findLibraries(linkPrl, mergeLflags);
}

ProStringList
MakefileGenerator::findFilesInVPATH(ProStringList l, uchar flags, const QString &vpath_var)
{
    ProStringList vpath;
    const ProValueMap &v = project->variables();
    for(int val_it = 0; val_it < l.count(); ) {
        bool remove_file = false;
        ProString &val = l[val_it];
        if(!val.isEmpty()) {
            QString qval = val.toQString();
            QString file = fixEnvVariables(qval);
            if (file.isEmpty()) {
                ++val_it;
                continue;
            }
            if(!(flags & VPATH_NoFixify))
                file = fileFixify(file, FileFixifyBackwards);

            if(exists(file)) {
                ++val_it;
                continue;
            }
            bool found = false;
            if (QDir::isRelativePath(qval)) {
                if(vpath.isEmpty()) {
                    if(!vpath_var.isEmpty())
                        vpath = v[ProKey(vpath_var)];
                    vpath += v["VPATH"] + v["QMAKE_ABSOLUTE_SOURCE_PATH"];
                    if(Option::output_dir != qmake_getpwd())
                        vpath << Option::output_dir;
                }
                for (ProStringList::Iterator vpath_it = vpath.begin();
                    vpath_it != vpath.end(); ++vpath_it) {
                    QString real_dir = Option::normalizePath((*vpath_it).toQString());
                    if (exists(real_dir + '/' + val)) {
                        ProString dir = (*vpath_it);
                        if(!dir.endsWith(Option::dir_sep))
                            dir += Option::dir_sep;
                        val = dir + val;
                        if(!(flags & VPATH_NoFixify))
                            val = fileFixify(val.toQString());
                        found = true;
                        debug_msg(1, "Found file through vpath %s -> %s",
                                  file.toLatin1().constData(), val.toLatin1().constData());
                        break;
                    }
                }
            }
            if(!found) {
                QString dir, regex = val.toQString(), real_dir;
                if(regex.lastIndexOf(Option::dir_sep) != -1) {
                    dir = regex.left(regex.lastIndexOf(Option::dir_sep) + 1);
                    real_dir = dir;
                    if(!(flags & VPATH_NoFixify))
                        real_dir = fileFixify(real_dir, FileFixifyBackwards) + '/';
                    regex.remove(0, dir.length());
                }
                if(real_dir.isEmpty() || exists(real_dir)) {
                    QStringList files = QDir(real_dir).entryList(QStringList(regex),
                                                QDir::NoDotAndDotDot | QDir::AllEntries);
                    if(files.isEmpty()) {
                        debug_msg(1, "%s:%d Failure to find %s in vpath (%s)",
                                  __FILE__, __LINE__, val.toLatin1().constData(),
                                  vpath.join(QString("::")).toLatin1().constData());
                        if(flags & VPATH_RemoveMissingFiles)
                            remove_file = true;
                        else if(flags & VPATH_WarnMissingFiles)
                            warn_msg(WarnLogic, "Failure to find: %s", val.toLatin1().constData());
                    } else {
                        l.removeAt(val_it);
                        QString a;
                        for(int i = (int)files.count()-1; i >= 0; i--) {
                            a = real_dir + files[i];
                            if(!(flags & VPATH_NoFixify))
                                a = fileFixify(a);
                            l.insert(val_it, a);
                        }
                    }
                } else {
                    debug_msg(1, "%s:%d Cannot match %s%s, as %s does not exist.",
                              __FILE__, __LINE__, real_dir.toLatin1().constData(),
                              regex.toLatin1().constData(), real_dir.toLatin1().constData());
                    if(flags & VPATH_RemoveMissingFiles)
                        remove_file = true;
                    else if(flags & VPATH_WarnMissingFiles)
                        warn_msg(WarnLogic, "Failure to find: %s", val.toLatin1().constData());
                }
            }
        }
        if(remove_file)
            l.removeAt(val_it);
        else
            ++val_it;
    }
    return l;
}

void
MakefileGenerator::initCompiler(const MakefileGenerator::Compiler &comp)
{
    ProValueMap &v = project->variables();
    ProStringList &l = v[ProKey(comp.variable_in)];
    // find all the relevant file inputs
    if(!init_compiler_already.contains(comp.variable_in)) {
        init_compiler_already.insert(comp.variable_in, true);
        if(!noIO())
            l = findFilesInVPATH(l, (comp.flags & Compiler::CompilerRemoveNoExist) ?
                                 VPATH_RemoveMissingFiles : VPATH_WarnMissingFiles, "VPATH_" + comp.variable_in);
    }
}

void
MakefileGenerator::init()
{
    verifyCompilers();
    initOutPaths();

    ProValueMap &v = project->variables();

    v["QMAKE_BUILTIN_COMPILERS"] = ProStringList() << "C" << "CXX";

    v["QMAKE_LANGUAGE_C"] = ProString("c");
    v["QMAKE_LANGUAGE_CXX"] = ProString("c++");
    v["QMAKE_LANGUAGE_OBJC"] = ProString("objective-c");
    v["QMAKE_LANGUAGE_OBJCXX"] = ProString("objective-c++");

    if (v["TARGET"].isEmpty())
        warn_msg(WarnLogic, "TARGET is empty");

    makedir = v["QMAKE_MKDIR_CMD"].join(' ');
    chkexists = v["QMAKE_CHK_EXISTS"].join(' ');
    if (makedir.isEmpty()) { // Backwards compat with Qt < 5.0.2 specs
        if (isWindowsShell()) {
            makedir = "if not exist %1 mkdir %1 & if not exist %1 exit 1";
            chkexists = "if not exist %1";
        } else {
            makedir = "test -d %1 || mkdir -p %1";
            chkexists = "test -e %1 ||";
        }
    }

    if (v["QMAKE_CC_O_FLAG"].isEmpty())
        v["QMAKE_CC_O_FLAG"].append("-o ");

    if (v["QMAKE_LINK_O_FLAG"].isEmpty())
        v["QMAKE_LINK_O_FLAG"].append("-o ");

    setSystemIncludes(v["QMAKE_DEFAULT_INCDIRS"]);

    ProStringList &incs = project->values("INCLUDEPATH");
    if (!project->isActiveConfig("no_include_pwd")) {
        if (Option::output_dir != qmake_getpwd()) {
            // Pretend that the build dir is the source dir for #include purposes,
            // consistently with the "transparent shadow builds" strategy. This is
            // also consistent with #include "foo.h" falling back to #include <foo.h>
            // behavior if it doesn't find the file in the source dir.
            incs.prepend(Option::output_dir);
        }
        // This makes #include <foo.h> work if the header lives in the source dir.
        // The benefit of that is questionable, as generally the user should use the
        // correct include style, and extra compilers that put stuff in the source dir
        // should add the dir themselves.
        // More importantly, it makes #include "foo.h" work with MSVC when shadow-building,
        // as this compiler looks files up relative to %CD%, not the source file's parent.
        incs.prepend(qmake_getpwd());
    }
    incs.append(project->specDir());

    const auto platform = v["QMAKE_PLATFORM"];
    resolveDependenciesInFrameworks = platform.contains("darwin");

    const char * const cacheKeys[] = { "_QMAKE_STASH_", "_QMAKE_SUPER_CACHE_", nullptr };
    for (int i = 0; cacheKeys[i]; ++i) {
        if (v[cacheKeys[i]].isEmpty())
            continue;
        const ProString &file = v[cacheKeys[i]].first();
        if (file.isEmpty())
            continue;

        QFileInfo fi(fileInfo(file.toQString()));

        // If the file lives in the output dir we treat it as 'owned' by
        // the current project, so it should be distcleaned by it as well.
        if (fi.path() == Option::output_dir)
            v["QMAKE_DISTCLEAN"].append(fi.fileName());
    }

    ProStringList &quc = v["QMAKE_EXTRA_COMPILERS"];

    //make sure the COMPILERS are in the correct input/output chain order
    for(int comp_out = 0, jump_count = 0; comp_out < quc.size(); ++comp_out) {
    continue_compiler_chain:
        if(jump_count > quc.size()) //just to avoid an infinite loop here
            break;
        const ProKey vokey(quc.at(comp_out) + ".variable_out");
        if (v.contains(vokey)) {
            const ProStringList &outputs = v.value(vokey);
            for(int out = 0; out < outputs.size(); ++out) {
                for(int comp_in = 0; comp_in < quc.size(); ++comp_in) {
                    if(comp_in == comp_out)
                        continue;
                    const ProKey ikey(quc.at(comp_in) + ".input");
                    if (v.contains(ikey)) {
                        const ProStringList &inputs = v.value(ikey);
                        for(int in = 0; in < inputs.size(); ++in) {
                            if(inputs.at(in) == outputs.at(out) && comp_out > comp_in) {
                                ++jump_count;
                                //move comp_out to comp_in and continue the compiler chain
                                // quc.move(comp_out, comp_in);
                                quc.insert(comp_in, quc.value(comp_out));
                                // comp_out > comp_in, so the insertion did move everything up
                                quc.remove(comp_out + 1);
                                comp_out = comp_in;
                                goto continue_compiler_chain;
                            }
                        }
                    }
                }
            }
        }
    }

    if(!project->isEmpty("QMAKE_SUBSTITUTES")) {
        const ProStringList &subs = v["QMAKE_SUBSTITUTES"];
        for(int i = 0; i < subs.size(); ++i) {
            QString sub = subs.at(i).toQString();
            QString inn = sub + ".input", outn = sub + ".output";
            const ProKey innkey(inn), outnkey(outn);
            if (v.contains(innkey) || v.contains(outnkey)) {
                if (!v.contains(innkey) || !v.contains(outnkey)) {
                    warn_msg(WarnLogic, "Substitute '%s' has only one of .input and .output",
                             sub.toLatin1().constData());
                    continue;
                }
                const ProStringList &tinn = v[innkey], &toutn = v[outnkey];
                if (tinn.length() != 1) {
                    warn_msg(WarnLogic, "Substitute '%s.input' does not have exactly one value",
                             sub.toLatin1().constData());
                    continue;
                }
                if (toutn.length() != 1) {
                    warn_msg(WarnLogic, "Substitute '%s.output' does not have exactly one value",
                             sub.toLatin1().constData());
                    continue;
                }
                inn = fileFixify(tinn.first().toQString(), FileFixifyToIndir);
                outn = fileFixify(toutn.first().toQString(), FileFixifyBackwards);
            } else {
                inn = fileFixify(sub, FileFixifyToIndir);
                if (!QFile::exists(inn)) {
                    // random insanity for backwards compat: .in file specified with absolute out dir
                    inn = fileFixify(sub);
                }
                if(!inn.endsWith(".in")) {
                    warn_msg(WarnLogic, "Substitute '%s' does not end with '.in'",
                             inn.toLatin1().constData());
                    continue;
                }
                outn = fileFixify(inn.left(inn.length() - 3), FileFixifyBackwards);
            }

            const ProKey confign(sub + ".CONFIG");
            bool verbatim  = false;
            if (v.contains(confign))
                verbatim = v[confign].contains(QLatin1String("verbatim"));

            QFile in(inn);
            if (in.open(QFile::ReadOnly)) {
                QByteArray contentBytes;
                if (verbatim) {
                    contentBytes = in.readAll();
                } else {
                    QString contents;
                    QStack<int> state;
                    enum { IN_CONDITION, MET_CONDITION, PENDING_CONDITION };
                    for (int count = 1; !in.atEnd(); ++count) {
                        QString line = QString::fromLatin1(in.readLine());
                        if (line.startsWith("!!IF ")) {
                            if (state.isEmpty() || state.top() == IN_CONDITION) {
                                QString test = line.mid(5, line.length()-(5+1));
                                if (project->test(test, inn, count))
                                    state.push(IN_CONDITION);
                                else
                                    state.push(PENDING_CONDITION);
                            } else {
                                state.push(MET_CONDITION);
                            }
                        } else if (line.startsWith("!!ELIF ")) {
                            if (state.isEmpty()) {
                                warn_msg(WarnLogic, "(%s:%d): Unexpected else condition",
                                        in.fileName().toLatin1().constData(), count);
                            } else if (state.top() == PENDING_CONDITION) {
                                QString test = line.mid(7, line.length()-(7+1));
                                if (project->test(test, inn, count))  {
                                    state.pop();
                                    state.push(IN_CONDITION);
                                }
                            } else if (state.top() == IN_CONDITION) {
                                state.pop();
                                state.push(MET_CONDITION);
                            }
                        } else if (line.startsWith("!!ELSE")) {
                            if (state.isEmpty()) {
                                warn_msg(WarnLogic, "(%s:%d): Unexpected else condition",
                                        in.fileName().toLatin1().constData(), count);
                            } else if (state.top() == PENDING_CONDITION) {
                                state.pop();
                                state.push(IN_CONDITION);
                            } else if (state.top() == IN_CONDITION) {
                                state.pop();
                                state.push(MET_CONDITION);
                            }
                        } else if (line.startsWith("!!ENDIF")) {
                            if (state.isEmpty())
                                warn_msg(WarnLogic, "(%s:%d): Unexpected endif",
                                        in.fileName().toLatin1().constData(), count);
                            else
                                state.pop();
                        } else if (state.isEmpty() || state.top() == IN_CONDITION) {
                            contents += project->expand(line, in.fileName(), count);
                        }
                    }
                    contentBytes = contents.toLatin1();
                }
                QFile out(outn);
                QFileInfo outfi(out);
                if (out.exists() && out.open(QFile::ReadOnly)) {
                    QByteArray old = out.readAll();
                    if (contentBytes == old) {
                        v["QMAKE_INTERNAL_INCLUDED_FILES"].append(in.fileName());
                        v["QMAKE_DISTCLEAN"].append(outfi.absoluteFilePath());
                        continue;
                    }
                    out.close();
                    if(!out.remove()) {
                        warn_msg(WarnLogic, "Cannot clear substitute '%s'",
                                 out.fileName().toLatin1().constData());
                        continue;
                    }
                }
                mkdir(outfi.absolutePath());
                if(out.open(QFile::WriteOnly)) {
                    v["QMAKE_INTERNAL_INCLUDED_FILES"].append(in.fileName());
                    v["QMAKE_DISTCLEAN"].append(outfi.absoluteFilePath());
                    out.write(contentBytes);
                } else {
                    warn_msg(WarnLogic, "Cannot open substitute for output '%s'",
                             out.fileName().toLatin1().constData());
                }
            } else {
                warn_msg(WarnLogic, "Cannot open substitute for input '%s'",
                         in.fileName().toLatin1().constData());
            }
        }
    }

    int x;

    //build up a list of compilers
    QVector<Compiler> compilers;
    {
        const char *builtins[] = { "OBJECTS", "SOURCES", "PRECOMPILED_HEADER", nullptr };
        for(x = 0; builtins[x]; ++x) {
            Compiler compiler;
            compiler.variable_in = builtins[x];
            compiler.flags = Compiler::CompilerBuiltin;
            compiler.type = QMakeSourceFileInfo::TYPE_C;
            if(!strcmp(builtins[x], "OBJECTS"))
                compiler.flags |= Compiler::CompilerNoCheckDeps;
            compilers.append(compiler);
        }
        for (ProStringList::ConstIterator it = quc.cbegin(); it != quc.cend(); ++it) {
            const ProStringList &inputs = v[ProKey(*it + ".input")];
            for(x = 0; x < inputs.size(); ++x) {
                Compiler compiler;
                compiler.variable_in = inputs.at(x).toQString();
                compiler.flags = Compiler::CompilerNoFlags;
                const ProStringList &config = v[ProKey(*it + ".CONFIG")];
                if (config.indexOf("ignore_no_exist") != -1)
                    compiler.flags |= Compiler::CompilerRemoveNoExist;
                if (config.indexOf("no_dependencies") != -1)
                    compiler.flags |= Compiler::CompilerNoCheckDeps;
                if (config.indexOf("add_inputs_as_makefile_deps") != -1)
                    compiler.flags |= Compiler::CompilerAddInputsAsMakefileDeps;

                const ProKey dkey(*it + ".dependency_type");
                ProString dep_type;
                if (!project->isEmpty(dkey))
                    dep_type = project->first(dkey);
                if (dep_type.isEmpty())
                    compiler.type = QMakeSourceFileInfo::TYPE_UNKNOWN;
                else if(dep_type == "TYPE_UI")
                    compiler.type = QMakeSourceFileInfo::TYPE_UI;
                else
                    compiler.type = QMakeSourceFileInfo::TYPE_C;
                compilers.append(compiler);
            }
        }
    }
    { //do the path fixifying
        ProStringList paths;
        for(x = 0; x < compilers.count(); ++x) {
            if(!paths.contains(compilers.at(x).variable_in))
                paths << compilers.at(x).variable_in;
        }
        paths << "INCLUDEPATH" << "QMAKE_INTERNAL_INCLUDED_FILES" << "PRECOMPILED_HEADER";
        for(int y = 0; y < paths.count(); y++) {
            ProStringList &l = v[paths[y].toKey()];
            for (ProStringList::Iterator it = l.begin(); it != l.end(); ++it) {
                if((*it).isEmpty())
                    continue;
                QString fn = (*it).toQString();
                if (exists(fn))
                    (*it) = fileFixify(fn);
            }
        }
    }

    if(noIO() || !doDepends() || project->isActiveConfig("GNUmake"))
        QMakeSourceFileInfo::setDependencyMode(QMakeSourceFileInfo::NonRecursive);
    for(x = 0; x < compilers.count(); ++x)
        initCompiler(compilers.at(x));

    //merge actual compiler outputs into their variable_out. This is done last so that
    //files are already properly fixified.
    for (ProStringList::Iterator it = quc.begin(); it != quc.end(); ++it) {
        const ProKey ikey(*it + ".input");
        const ProKey vokey(*it + ".variable_out");
        const ProStringList &config = project->values(ProKey(*it + ".CONFIG"));
        const ProString &tmp_out = project->first(ProKey(*it + ".output"));
        if(tmp_out.isEmpty())
            continue;
        if (config.indexOf("combine") != -1) {
            const ProStringList &compilerInputs = project->values(ikey);
            // Don't generate compiler output if it doesn't have input.
            if (compilerInputs.isEmpty() || project->values(compilerInputs.first().toKey()).isEmpty())
                continue;
            if(tmp_out.indexOf("$") == -1) {
                if(!verifyExtraCompiler((*it), QString())) //verify
                    continue;
                QString out = fileFixify(tmp_out.toQString(), FileFixifyFromOutdir);
                bool pre_dep = (config.indexOf("target_predeps") != -1);
                if (v.contains(vokey)) {
                    const ProStringList &var_out = v.value(vokey);
                    for(int i = 0; i < var_out.size(); ++i) {
                        ProKey v = var_out.at(i).toKey();
                        if(v == QLatin1String("SOURCES"))
                            v = "GENERATED_SOURCES";
                        else if(v == QLatin1String("OBJECTS"))
                            pre_dep = false;
                        ProStringList &list = project->values(v);
                        if(!list.contains(out))
                            list.append(out);
                    }
                } else if (config.indexOf("no_link") == -1) {
                    ProStringList &list = project->values("OBJECTS");
                    pre_dep = false;
                    if(!list.contains(out))
                        list.append(out);
                } else {
                        ProStringList &list = project->values("UNUSED_SOURCES");
                        if(!list.contains(out))
                            list.append(out);
                }
                if(pre_dep) {
                    ProStringList &list = project->values("PRE_TARGETDEPS");
                    if(!list.contains(out))
                        list.append(out);
                }
            }
        } else {
            const ProStringList &tmp = project->values(ikey);
            for (ProStringList::ConstIterator it2 = tmp.begin(); it2 != tmp.end(); ++it2) {
                const ProStringList &inputs = project->values((*it2).toKey());
                for (ProStringList::ConstIterator input = inputs.constBegin(); input != inputs.constEnd(); ++input) {
                    if((*input).isEmpty())
                        continue;
                    QString inpf = (*input).toQString();
                    if (!verifyExtraCompiler((*it).toQString(), inpf)) //verify
                        continue;
                    QString out = replaceExtraCompilerVariables(tmp_out.toQString(), inpf, QString(), NoShell);
                    out = fileFixify(out, FileFixifyFromOutdir);
                    bool pre_dep = (config.indexOf("target_predeps") != -1);
                    if (v.contains(vokey)) {
                        const ProStringList &var_out = project->values(vokey);
                        for(int i = 0; i < var_out.size(); ++i) {
                            ProKey v = var_out.at(i).toKey();
                            if(v == QLatin1String("SOURCES"))
                                v = "GENERATED_SOURCES";
                            else if(v == QLatin1String("OBJECTS"))
                                pre_dep = false;
                            ProStringList &list = project->values(v);
                            if(!list.contains(out))
                                list.append(out);
                        }
                    } else if (config.indexOf("no_link") == -1) {
                        pre_dep = false;
                        ProStringList &list = project->values("OBJECTS");
                        if(!list.contains(out))
                            list.append(out);
                    } else {
                        ProStringList &list = project->values("UNUSED_SOURCES");
                        if(!list.contains(out))
                            list.append(out);
                    }
                    if(pre_dep) {
                        ProStringList &list = project->values("PRE_TARGETDEPS");
                        if(!list.contains(out))
                            list.append(out);
                    }
                }
            }
        }
    }

    //handle dependencies
    depHeuristicsCache.clear();
    if(!noIO()) {
        // dependency paths
        ProStringList incDirs = v["DEPENDPATH"] + v["QMAKE_ABSOLUTE_SOURCE_PATH"];
        if(project->isActiveConfig("depend_includepath"))
            incDirs += v["INCLUDEPATH"];
        QVector<QMakeLocalFileName> deplist;
        deplist.reserve(incDirs.size());
        for (ProStringList::Iterator it = incDirs.begin(); it != incDirs.end(); ++it)
            deplist.append(QMakeLocalFileName((*it).toQString()));
        QMakeSourceFileInfo::setDependencyPaths(deplist);
        debug_msg(1, "Dependency Directories: %s",
                  incDirs.join(QString(" :: ")).toLatin1().constData());

        //add to dependency engine
        for(x = 0; x < compilers.count(); ++x) {
            const MakefileGenerator::Compiler &comp = compilers.at(x);
            if(!(comp.flags & Compiler::CompilerNoCheckDeps)) {
                const ProKey ikey(comp.variable_in);
                addSourceFiles(v[ikey], QMakeSourceFileInfo::SEEK_DEPS,
                               (QMakeSourceFileInfo::SourceFileType)comp.type);

                if (comp.flags & Compiler::CompilerAddInputsAsMakefileDeps) {
                    ProStringList &l = v[ikey];
                    for (int i=0; i < l.size(); ++i) {
                        if(v["QMAKE_INTERNAL_INCLUDED_FILES"].indexOf(l.at(i)) == -1)
                            v["QMAKE_INTERNAL_INCLUDED_FILES"].append(l.at(i));
                    }
                }
            }
        }
    }

    processSources(); //remove anything in SOURCES which is included (thus it need not be linked in)

    //all sources and generated sources must be turned into objects at some point (the one builtin compiler)
    v["OBJECTS"] += createObjectList(v["SOURCES"]) + createObjectList(v["GENERATED_SOURCES"]);

    //Translation files
    if(!project->isEmpty("TRANSLATIONS")) {
        ProStringList &trf = project->values("TRANSLATIONS");
        for (ProStringList::Iterator it = trf.begin(); it != trf.end(); ++it)
            (*it) = Option::fixPathToTargetOS((*it).toQString());
    }

    //fix up the target deps
    static const char * const fixpaths[] = { "PRE_TARGETDEPS", "POST_TARGETDEPS", nullptr };
    for (int path = 0; fixpaths[path]; path++) {
        ProStringList &l = v[fixpaths[path]];
        for (ProStringList::Iterator val_it = l.begin(); val_it != l.end(); ++val_it) {
            if(!(*val_it).isEmpty())
                (*val_it) = Option::fixPathToTargetOS((*val_it).toQString(), false, false);
        }
    }

    //extra depends
    if(!project->isEmpty("DEPENDS")) {
        ProStringList &l = v["DEPENDS"];
        for (ProStringList::Iterator it = l.begin(); it != l.end(); ++it) {
            const ProStringList &files = v[ProKey(*it + ".file")] + v[ProKey(*it + ".files")]; //why do I support such evil things?
            for (ProStringList::ConstIterator file_it = files.begin(); file_it != files.end(); ++file_it) {
                QStringList &out_deps = findDependencies((*file_it).toQString());
                const ProStringList &in_deps = v[ProKey(*it + ".depends")]; //even more evilness..
                for (ProStringList::ConstIterator dep_it = in_deps.begin(); dep_it != in_deps.end(); ++dep_it) {
                    QString dep = (*dep_it).toQString();
                    if (exists(dep)) {
                        out_deps.append(dep);
                    } else {
                        QString dir, regex = Option::normalizePath(dep);
                        if (regex.lastIndexOf('/') != -1) {
                            dir = regex.left(regex.lastIndexOf('/') + 1);
                            regex.remove(0, dir.length());
                        }
                        QStringList files = QDir(dir).entryList(QStringList(regex));
                        if(files.isEmpty()) {
                            warn_msg(WarnLogic, "Dependency for [%s]: Not found %s", (*file_it).toLatin1().constData(),
                                     dep.toLatin1().constData());
                        } else {
                            for(int i = 0; i < files.count(); i++)
                                out_deps.append(dir + files[i]);
                        }
                    }
                }
            }
        }
    }

    // escape qmake command
    project->values("QMAKE_QMAKE") =
            ProStringList(escapeFilePath(Option::fixPathToTargetOS(Option::globals->qmake_abslocation, false)));
}

bool
MakefileGenerator::processPrlFile(QString &file, bool baseOnly)
{
    QString f = fileFixify(file, FileFixifyBackwards);
    // Explicitly given full .prl name
    if (!baseOnly && f.endsWith(Option::prl_ext))
        return processPrlFileCore(file, QStringRef(), f);
    // Explicitly given or derived (from -l) base name
    if (processPrlFileCore(file, QStringRef(), f + Option::prl_ext))
        return true;
    if (!baseOnly) {
        // Explicitly given full library name
        int off = qMax(f.lastIndexOf('/'), f.lastIndexOf('\\')) + 1;
        int ext = f.midRef(off).lastIndexOf('.');
        if (ext != -1)
            return processPrlFileBase(file, f.midRef(off), f.leftRef(off + ext), off);
    }
    return false;
}

bool
MakefileGenerator::processPrlFileBase(QString &origFile, const QStringRef &origName,
                                      const QStringRef &fixedBase, int /*slashOff*/)
{
    return processPrlFileCore(origFile, origName, fixedBase + Option::prl_ext);
}

bool
MakefileGenerator::processPrlFileCore(QString &origFile, const QStringRef &origName,
                                      const QString &fixedFile)
{
    const QString meta_file = QMakeMetaInfo::checkLib(fixedFile);
    if (meta_file.isEmpty())
        return false;
    QMakeMetaInfo libinfo;
    debug_msg(1, "Processing PRL file: %s", meta_file.toLatin1().constData());
    if (!libinfo.readLib(meta_file)) {
        fprintf(stderr, "Error processing meta file %s\n", meta_file.toLatin1().constData());
        return false;
    }
    if (project->isActiveConfig("no_read_prl_qmake")) {
        debug_msg(2, "Ignored meta file %s", meta_file.toLatin1().constData());
        return false;
    }
    ProString tgt = libinfo.first("QMAKE_PRL_TARGET");
    if (tgt.isEmpty()) {
        fprintf(stderr, "Error: %s does not define QMAKE_PRL_TARGET\n",
                        meta_file.toLatin1().constData());
        return false;
    }
    if (!tgt.contains('.') && !libinfo.values("QMAKE_PRL_CONFIG").contains("lib_bundle")) {
        fprintf(stderr, "Error: %s defines QMAKE_PRL_TARGET without extension\n",
                        meta_file.toLatin1().constData());
        return false;
    }
    if (origName.isEmpty()) {
        // We got a .prl file as input, replace it with an actual library.
        int off = qMax(origFile.lastIndexOf('/'), origFile.lastIndexOf('\\')) + 1;
        debug_msg(1, "  Replacing library reference %s with %s",
                     origFile.mid(off).toLatin1().constData(),
                     tgt.toQString().toLatin1().constData());
        origFile.replace(off, 1000, tgt.toQString());
    } else if (tgt != origName) {
        // We got an actual library as input, and found the wrong .prl for it.
        debug_msg(2, "Mismatched meta file %s (want %s, got %s)",
                     meta_file.toLatin1().constData(),
                     origName.toLatin1().constData(), tgt.toLatin1().constData());
        return false;
    }
    project->values("QMAKE_CURRENT_PRL_LIBS") = libinfo.values("QMAKE_PRL_LIBS");
    ProStringList &defs = project->values("DEFINES");
    const ProStringList &prl_defs = project->values("PRL_EXPORT_DEFINES");
    for (const ProString &def : libinfo.values("QMAKE_PRL_DEFINES"))
        if (!defs.contains(def) && prl_defs.contains(def))
            defs.append(def);
    QString mf = fileFixify(meta_file);
    if (!project->values("QMAKE_PRL_INTERNAL_FILES").contains(mf))
       project->values("QMAKE_PRL_INTERNAL_FILES").append(mf);
    if (!project->values("QMAKE_INTERNAL_INCLUDED_FILES").contains(mf))
       project->values("QMAKE_INTERNAL_INCLUDED_FILES").append(mf);
    return true;
}

void
MakefileGenerator::filterIncludedFiles(const char *var)
{
    ProStringList &inputs = project->values(var);
    auto isIncluded = [this](const ProString &input) {
        return QMakeSourceFileInfo::included(input.toQString()) > 0;
    };
    inputs.erase(std::remove_if(inputs.begin(), inputs.end(),
                                isIncluded),
                 inputs.end());
}

static QString
qv(const ProString &val)
{
    return ' ' + QMakeEvaluator::quoteValue(val);
}

static QString
qv(const ProStringList &val)
{
    QString ret;
    for (const ProString &v : val)
        ret += qv(v);
    return ret;
}

void
MakefileGenerator::writePrlFile(QTextStream &t)
{
    QString bdir = Option::output_dir;
    if(bdir.isEmpty())
        bdir = qmake_getpwd();
    t << "QMAKE_PRL_BUILD_DIR =" << qv(bdir) << Qt::endl;

    t << "QMAKE_PRO_INPUT =" << qv(project->projectFile().section('/', -1)) << Qt::endl;

    if(!project->isEmpty("QMAKE_ABSOLUTE_SOURCE_PATH"))
        t << "QMAKE_PRL_SOURCE_DIR =" << qv(project->first("QMAKE_ABSOLUTE_SOURCE_PATH")) << Qt::endl;
    t << "QMAKE_PRL_TARGET =" << qv(project->first("LIB_TARGET")) << Qt::endl;
    if(!project->isEmpty("PRL_EXPORT_DEFINES"))
        t << "QMAKE_PRL_DEFINES =" << qv(project->values("PRL_EXPORT_DEFINES")) << Qt::endl;
    if(!project->isEmpty("PRL_EXPORT_CFLAGS"))
        t << "QMAKE_PRL_CFLAGS =" << qv(project->values("PRL_EXPORT_CFLAGS")) << Qt::endl;
    if(!project->isEmpty("PRL_EXPORT_CXXFLAGS"))
        t << "QMAKE_PRL_CXXFLAGS =" << qv(project->values("PRL_EXPORT_CXXFLAGS")) << Qt::endl;
    if(!project->isEmpty("CONFIG"))
        t << "QMAKE_PRL_CONFIG =" << qv(project->values("CONFIG")) << Qt::endl;
    if(!project->isEmpty("TARGET_VERSION_EXT"))
        t << "QMAKE_PRL_VERSION = " << project->first("TARGET_VERSION_EXT") << Qt::endl;
    else if(!project->isEmpty("VERSION"))
        t << "QMAKE_PRL_VERSION = " << project->first("VERSION") << Qt::endl;
    if(project->isActiveConfig("staticlib") || project->isActiveConfig("explicitlib")) {
        ProStringList libs;
        if (!project->isActiveConfig("staticlib"))
            libs << "LIBS" << "QMAKE_LIBS";
        else
            libs << "LIBS" << "LIBS_PRIVATE" << "QMAKE_LIBS" << "QMAKE_LIBS_PRIVATE";
        t << "QMAKE_PRL_LIBS =";
        for (ProStringList::Iterator it = libs.begin(); it != libs.end(); ++it)
            t << qv(project->values((*it).toKey()));
        t << Qt::endl;

        t << "QMAKE_PRL_LIBS_FOR_CMAKE = ";
        QString sep;
        for (ProStringList::Iterator it = libs.begin(); it != libs.end(); ++it) {
            t << sep << project->values((*it).toKey()).join(';').replace('\\', "\\\\");
            sep = ';';
        }
        t << Qt::endl;
    }
}

bool
MakefileGenerator::writeProjectMakefile()
{
    QTextStream t(&Option::output);

    //header
    writeHeader(t);

    QList<SubTarget*> targets;
    {
        ProStringList builds = project->values("BUILDS");
        targets.reserve(builds.size());
        for (ProStringList::Iterator it = builds.begin(); it != builds.end(); ++it) {
            SubTarget *st = new SubTarget;
            targets.append(st);
            st->makefile = "$(MAKEFILE)." + (*it);
            st->name = (*it).toQString();
            const ProKey tkey(*it + ".target");
            st->target = (project->isEmpty(tkey) ? (*it) : project->first(tkey)).toQString();
        }
    }
    if(project->isActiveConfig("build_all")) {
        t << "first: all\n";
        QList<SubTarget*>::Iterator it;

        //install
        t << "install: ";
        for (SubTarget *s : qAsConst(targets))
            t << s->target << '-';
        t << "install " << Qt::endl;

        //uninstall
        t << "uninstall: ";
        for(it = targets.begin(); it != targets.end(); ++it)
            t << (*it)->target << "-uninstall ";
        t << Qt::endl;
    } else {
        t << "first: " << targets.first()->target << Qt::endl
          << "install: " << targets.first()->target << "-install\n"
          << "uninstall: " << targets.first()->target << "-uninstall\n";
    }

    writeSubTargets(t, targets, SubTargetsNoFlags);
    if(!project->isActiveConfig("no_autoqmake")) {
        QString mkf = escapeDependencyPath(fileFixify(Option::output.fileName()));
        for(QList<SubTarget*>::Iterator it = targets.begin(); it != targets.end(); ++it)
            t << escapeDependencyPath((*it)->makefile) << ": " << mkf << Qt::endl;
    }
    qDeleteAll(targets);
    return true;
}

bool
MakefileGenerator::write()
{
    if(!project)
        return false;
    writePrlFile();
    if(Option::qmake_mode == Option::QMAKE_GENERATE_MAKEFILE || //write makefile
       Option::qmake_mode == Option::QMAKE_GENERATE_PROJECT) {
        QTextStream t(&Option::output);
        if(!writeMakefile(t)) {
#if 1
            warn_msg(WarnLogic, "Unable to generate output for: %s [TEMPLATE %s]",
                     Option::output.fileName().toLatin1().constData(),
                     project->first("TEMPLATE").toLatin1().constData());
            if(Option::output.exists())
                Option::output.remove();
#endif
        }
    }
    return true;
}

QString
MakefileGenerator::prlFileName(bool fixify)
{
    QString ret = project->first("PRL_TARGET") + Option::prl_ext;
    if(fixify) {
        if(!project->isEmpty("DESTDIR"))
            ret.prepend(project->first("DESTDIR").toQString());
        ret = fileFixify(ret, FileFixifyBackwards);
    }
    return ret;
}

void
MakefileGenerator::writePrlFile()
{
    if((Option::qmake_mode == Option::QMAKE_GENERATE_MAKEFILE ||
            Option::qmake_mode == Option::QMAKE_GENERATE_PRL)
       && project->values("QMAKE_FAILED_REQUIREMENTS").isEmpty()
       && project->isActiveConfig("create_prl")
       && (project->first("TEMPLATE") == "lib"
       || project->first("TEMPLATE") == "vclib"
       || project->first("TEMPLATE") == "aux")
       && (!project->isActiveConfig("plugin") || project->isActiveConfig("static"))) { //write prl file
        QString local_prl = prlFileName();
        QString prl = fileFixify(local_prl);
        mkdir(fileInfo(local_prl).path());
        QFile ft(local_prl);
        if(ft.open(QIODevice::WriteOnly)) {
            project->values("ALL_DEPS").append(prl);
            project->values("QMAKE_INTERNAL_PRL_FILE").append(prl);
            project->values("QMAKE_DISTCLEAN").append(prl);
            QTextStream t(&ft);
            writePrlFile(t);
        }
    }
}

void
MakefileGenerator::writeObj(QTextStream &t, const char *src)
{
    const ProStringList &srcl = project->values(src);
    const ProStringList objl = createObjectList(srcl);

    ProStringList::ConstIterator oit = objl.begin();
    ProStringList::ConstIterator sit = srcl.begin();
    QLatin1String stringSrc("$src");
    QLatin1String stringObj("$obj");
    for(;sit != srcl.end() && oit != objl.end(); ++oit, ++sit) {
        if((*sit).isEmpty())
            continue;

        QString srcf = (*sit).toQString();
        QString dstf = (*oit).toQString();
        t << escapeDependencyPath(dstf) << ": " << escapeDependencyPath(srcf)
          << " " << finalizeDependencyPaths(findDependencies(srcf)).join(" \\\n\t\t");

        ProKey comp;
        for (const ProString &compiler : project->values("QMAKE_BUILTIN_COMPILERS")) {
            // Unfortunately we were not consistent about the C++ naming
            ProString extensionSuffix = compiler;
            if (extensionSuffix == "CXX")
                extensionSuffix = ProString("CPP");

            // Nor the C naming
            ProString compilerSuffix = compiler;
            if (compilerSuffix == "C")
                compilerSuffix = ProString("CC");

            for (const ProString &extension : project->values(ProKey("QMAKE_EXT_" + extensionSuffix))) {
                if ((*sit).endsWith(extension)) {
                    comp = ProKey("QMAKE_RUN_" + compilerSuffix);
                    break;
                }
            }

            if (!comp.isNull())
                break;
        }

        if (comp.isEmpty())
            comp = "QMAKE_RUN_CC";
        if (!project->isEmpty(comp)) {
            QString p = var(comp);
            p.replace(stringSrc, escapeFilePath(srcf));
            p.replace(stringObj, escapeFilePath(dstf));
            t << "\n\t" << p;
        }
        t << Qt::endl << Qt::endl;
    }
}

QString
MakefileGenerator::filePrefixRoot(const QString &root, const QString &path)
{
    QString ret(path);
    if(path.length() > 2 && path[1] == ':') //c:\foo
        ret.insert(2, root);
    else
        ret.prepend(root);
    while (ret.endsWith('\\'))
        ret.chop(1);
    return ret;
}

void
MakefileGenerator::writeInstalls(QTextStream &t, bool noBuild)
{
    QString rm_dir_contents("-$(DEL_FILE)");
    if (!isWindowsShell()) //ick
        rm_dir_contents = "-$(DEL_FILE) -r";

    QString all_installs, all_uninstalls;
    QSet<QString> made_dirs, removed_dirs;
    const ProStringList &l = project->values("INSTALLS");
    for (ProStringList::ConstIterator it = l.begin(); it != l.end(); ++it) {
        const ProKey pvar(*it + ".path");
        const ProStringList &installConfigValues = project->values(ProKey(*it + ".CONFIG"));
        if (installConfigValues.indexOf("no_path") == -1 &&
            installConfigValues.indexOf("dummy_install") == -1 &&
           project->values(pvar).isEmpty()) {
            warn_msg(WarnLogic, "%s is not defined: install target not created\n", pvar.toLatin1().constData());
            continue;
        }

        bool do_default = true;
        const QString root = installRoot();
        QString dst;
        if (installConfigValues.indexOf("no_path") == -1 &&
            installConfigValues.indexOf("dummy_install") == -1) {
            dst = fileFixify(project->first(pvar).toQString(), FileFixifyAbsolute, false);
            if(!dst.endsWith(Option::dir_sep))
                dst += Option::dir_sep;
        }

        QStringList tmp, inst, uninst;
        //other
        ProStringList tmp2 = project->values(ProKey(*it + ".extra"));
        if (tmp2.isEmpty())
            tmp2 = project->values(ProKey(*it + ".commands")); //to allow compatible name
        if (!tmp2.isEmpty()) {
            do_default = false;
            inst << tmp2.join(' ');
        }
        //masks
        tmp2 = findFilesInVPATH(project->values(ProKey(*it + ".files")), VPATH_NoFixify);
        tmp = fileFixify(tmp2.toQStringList(), FileFixifyAbsolute);
        if(!tmp.isEmpty()) {
            do_default = false;
            QString base_path = project->first(ProKey(*it + ".base")).toQString();
            if (!base_path.isEmpty()) {
                base_path = Option::fixPathToTargetOS(base_path, false, true);
                if (!base_path.endsWith(Option::dir_sep))
                    base_path += Option::dir_sep;
            }
            for(QStringList::Iterator wild_it = tmp.begin(); wild_it != tmp.end(); ++wild_it) {
                QString wild = Option::fixPathToTargetOS((*wild_it), false, false);
                QString dirstr = qmake_getpwd(), filestr = wild;
                int slsh = filestr.lastIndexOf(Option::dir_sep);
                if(slsh != -1) {
                    dirstr = filestr.left(slsh+1);
                    filestr.remove(0, slsh+1);
                }
                if(!dirstr.endsWith(Option::dir_sep))
                    dirstr += Option::dir_sep;
                QString dst_dir = dst;
                if (!base_path.isEmpty()) {
                    if (!dirstr.startsWith(base_path)) {
                        warn_msg(WarnLogic, "File %s in install rule %s does not start with base %s",
                                            qPrintable(wild), qPrintable((*it).toQString()),
                                            qPrintable(base_path));
                    } else {
                        QString dir_sfx = dirstr.mid(base_path.length());
                        dst_dir += dir_sfx;
                        if (!dir_sfx.isEmpty() && !made_dirs.contains(dir_sfx)) {
                            made_dirs.insert(dir_sfx);
                            QString tmp_dst = fileFixify(dst_dir, FileFixifyAbsolute, false);
                            tmp_dst.chop(1);
                            inst << mkdir_p_asstring(filePrefixRoot(root, tmp_dst));
                            for (int i = dst.length(); i < dst_dir.length(); i++) {
                                if (dst_dir.at(i) == Option::dir_sep) {
                                    QString subd = dst_dir.left(i);
                                    if (!removed_dirs.contains(subd)) {
                                        removed_dirs.insert(subd);
                                        tmp_dst = fileFixify(subd, FileFixifyAbsolute, false);
                                        uninst << "-$(DEL_DIR) "
                                                  + escapeFilePath(filePrefixRoot(root, tmp_dst));
                                    }
                                }
                            }
                        }
                    }
                }
                bool is_target = (wild == fileFixify(var("TARGET"), FileFixifyAbsolute));
                const bool noStrip = installConfigValues.contains("nostrip");
                if(is_target || exists(wild)) { //real file or target
                    QFileInfo fi(fileInfo(wild));
                    QString dst_file = filePrefixRoot(root, dst_dir);
                    if (!dst_file.endsWith(Option::dir_sep))
                        dst_file += Option::dir_sep;
                    dst_file += fi.fileName();
                    QString cmd;
                    if (is_target || (!fi.isDir() && fi.isExecutable()))
                       cmd = QLatin1String("$(QINSTALL_PROGRAM)");
                    else
                       cmd = QLatin1String("$(QINSTALL)");
                    cmd += " " + escapeFilePath(wild) + " " + escapeFilePath(dst_file);
                    inst << cmd;
                    if (!noStrip && !project->isActiveConfig("debug_info") && !project->isActiveConfig("nostrip") &&
                       !fi.isDir() && fi.isExecutable() && !project->isEmpty("QMAKE_STRIP"))
                        inst << QString("-") + var("QMAKE_STRIP") + " " +
                                  escapeFilePath(filePrefixRoot(root, fileFixify(dst_dir + filestr, FileFixifyAbsolute, false)));
                    uninst.append(rm_dir_contents + " " + escapeFilePath(filePrefixRoot(root, fileFixify(dst_dir + filestr, FileFixifyAbsolute, false))));
                    continue;
                }
                QString local_dirstr = Option::normalizePath(dirstr);
                QStringList files = QDir(local_dirstr).entryList(QStringList(filestr),
                                            QDir::NoDotAndDotDot | QDir::AllEntries);
                if (installConfigValues.contains("no_check_exist") && files.isEmpty()) {
                    QString dst_file = filePrefixRoot(root, dst_dir);
                    if (!dst_file.endsWith(Option::dir_sep))
                        dst_file += Option::dir_sep;
                    dst_file += filestr;
                    QString cmd;
                    if (installConfigValues.contains("executable"))
                        cmd = QLatin1String("$(QINSTALL_PROGRAM)");
                    else
                        cmd = QLatin1String("$(QINSTALL)");
                    cmd += " " + escapeFilePath(wild) + " " + escapeFilePath(dst_file);
                    inst << cmd;
                    uninst.append(rm_dir_contents + " " + escapeFilePath(filePrefixRoot(root, fileFixify(dst_dir + filestr, FileFixifyAbsolute, false))));
                }
                for(int x = 0; x < files.count(); x++) {
                    QString file = files[x];
                    uninst.append(rm_dir_contents + " " + escapeFilePath(filePrefixRoot(root, fileFixify(dst_dir + file, FileFixifyAbsolute, false))));
                    QFileInfo fi(fileInfo(dirstr + file));
                    QString dst_file = filePrefixRoot(root, fileFixify(dst_dir, FileFixifyAbsolute, false));
                    if (!dst_file.endsWith(Option::dir_sep))
                        dst_file += Option::dir_sep;
                    dst_file += fi.fileName();
                    QString cmd = QLatin1String("$(QINSTALL) ") +
                                  escapeFilePath(dirstr + file) + " " + escapeFilePath(dst_file);
                    inst << cmd;
                    if (!noStrip && !project->isActiveConfig("debug_info") && !project->isActiveConfig("nostrip") &&
                       !fi.isDir() && fi.isExecutable() && !project->isEmpty("QMAKE_STRIP"))
                        inst << QString("-") + var("QMAKE_STRIP") + " " +
                                  escapeFilePath(filePrefixRoot(root, fileFixify(dst_dir + file, FileFixifyAbsolute, false)));
                }
            }
        }
        QString target;
        //default?
        if (do_default)
            target = defaultInstall((*it).toQString());
        else
            target = inst.join("\n\t");
        QString puninst = project->values(ProKey(*it + ".uninstall")).join(' ');
        if (!puninst.isEmpty())
            uninst << puninst;

        if (!target.isEmpty() || installConfigValues.indexOf("dummy_install") != -1) {
            if (noBuild || installConfigValues.indexOf("no_build") != -1)
                t << "install_" << (*it) << ":";
            else if(project->isActiveConfig("build_all"))
                t << "install_" << (*it) << ": all";
            else
                t << "install_" << (*it) << ": first";
            const ProStringList &deps = project->values(ProKey(*it + ".depends"));
            if(!deps.isEmpty()) {
                for (ProStringList::ConstIterator dep_it = deps.begin(); dep_it != deps.end(); ++dep_it) {
                    QString targ = var(ProKey(*dep_it + ".target"));
                    if(targ.isEmpty())
                        targ = (*dep_it).toQString();
                    t << " " << escapeDependencyPath(targ);
                }
            }
            t << " FORCE\n\t";
            const ProStringList &dirs = project->values(pvar);
            for (ProStringList::ConstIterator pit = dirs.begin(); pit != dirs.end(); ++pit) {
                QString tmp_dst = fileFixify((*pit).toQString(), FileFixifyAbsolute, false);
                t << mkdir_p_asstring(filePrefixRoot(root, tmp_dst)) << "\n\t";
            }
            t << target << Qt::endl << Qt::endl;
            if(!uninst.isEmpty()) {
                t << "uninstall_" << (*it) << ": FORCE";
                for (int i = uninst.size(); --i >= 0; )
                    t << "\n\t" << uninst.at(i);
                t << "\n\t-$(DEL_DIR) " << escapeFilePath(filePrefixRoot(root, dst)) << " \n\n";
            }
            t << Qt::endl;

            if (installConfigValues.indexOf("no_default_install") == -1) {
                all_installs += QString("install_") + (*it) + " ";
                if(!uninst.isEmpty())
                    all_uninstalls += "uninstall_" + (*it) + " ";
            }
        }   else {
            debug_msg(1, "no definition for install %s: install target not created",(*it).toLatin1().constData());
        }
    }
    t << "install:" << depVar("INSTALLDEPS") << ' ' << all_installs
      << " FORCE\n\nuninstall: " << all_uninstalls << depVar("UNINSTALLDEPS")
      << " FORCE\n\n";
}

QString
MakefileGenerator::var(const ProKey &var) const
{
    return val(project->values(var));
}

QString
MakefileGenerator::fileVar(const ProKey &var) const
{
    return val(escapeFilePaths(project->values(var)));
}

QString
MakefileGenerator::fileVarList(const ProKey &var) const
{
    return valList(escapeFilePaths(project->values(var)));
}

QString
MakefileGenerator::depVar(const ProKey &var) const
{
    return val(escapeDependencyPaths(project->values(var)));
}

QString
MakefileGenerator::val(const ProStringList &varList) const
{
    return valGlue(varList, "", " ", "");
}

QString
MakefileGenerator::val(const QStringList &varList) const
{
    return valGlue(varList, "", " ", "");
}

QString
MakefileGenerator::varGlue(const ProKey &var, const QString &before, const QString &glue, const QString &after) const
{
    return valGlue(project->values(var), before, glue, after);
}

QString
MakefileGenerator::fileVarGlue(const ProKey &var, const QString &before, const QString &glue, const QString &after) const
{
    return valGlue(escapeFilePaths(project->values(var)), before, glue, after);
}

QString
MakefileGenerator::fixFileVarGlue(const ProKey &var, const QString &before, const QString &glue, const QString &after) const
{
    ProStringList varList;
    const auto values = project->values(var);
    varList.reserve(values.size());
    for (const ProString &val : values)
        varList << escapeFilePath(Option::fixPathToTargetOS(val.toQString()));
    return valGlue(varList, before, glue, after);
}

QString
MakefileGenerator::valGlue(const ProStringList &varList, const QString &before, const QString &glue, const QString &after) const
{
    QString ret;
    for (ProStringList::ConstIterator it = varList.begin(); it != varList.end(); ++it) {
        if (!(*it).isEmpty()) {
            if (!ret.isEmpty())
                ret += glue;
            ret += (*it).toQString();
        }
    }
    return ret.isEmpty() ? QString("") : before + ret + after;
}

QString
MakefileGenerator::valGlue(const QStringList &varList, const QString &before, const QString &glue, const QString &after) const
{
    QString ret;
    for(QStringList::ConstIterator it = varList.begin(); it != varList.end(); ++it) {
        if(!(*it).isEmpty()) {
            if(!ret.isEmpty())
                ret += glue;
            ret += (*it);
        }
    }
    return ret.isEmpty() ? QString("") : before + ret + after;
}


QString
MakefileGenerator::varList(const ProKey &var) const
{
    return valList(project->values(var));
}

QString
MakefileGenerator::valList(const ProStringList &varList) const
{
    return valGlue(varList, "", " \\\n\t\t", "");
}

QString
MakefileGenerator::valList(const QStringList &varList) const
{
    return valGlue(varList, "", " \\\n\t\t", "");
}

ProStringList
MakefileGenerator::createObjectList(const ProStringList &sources)
{
    ProStringList ret;
    QString objdir;
    if(!project->values("OBJECTS_DIR").isEmpty())
        objdir = project->first("OBJECTS_DIR").toQString();
    for (ProStringList::ConstIterator it = sources.begin(); it != sources.end(); ++it) {
        QString sfn = (*it).toQString();
        QFileInfo fi(fileInfo(Option::normalizePath(sfn)));
        QString dir;
        if (project->isActiveConfig("object_parallel_to_source")) {
            // The source paths are relative to the output dir, but we need source-relative paths
            QString sourceRelativePath = fileFixify(sfn, FileFixifyBackwards);

            if (sourceRelativePath.startsWith(".." + Option::dir_sep))
                sourceRelativePath = fileFixify(sourceRelativePath, FileFixifyAbsolute);

            if (QDir::isAbsolutePath(sourceRelativePath))
                sourceRelativePath.remove(0, sourceRelativePath.indexOf(Option::dir_sep) + 1);

            dir = objdir; // We still respect OBJECTS_DIR

            int lastDirSepPosition = sourceRelativePath.lastIndexOf(Option::dir_sep);
            if (lastDirSepPosition != -1)
                dir += sourceRelativePath.leftRef(lastDirSepPosition + 1);

            if (!noIO()) {
                // Ensure that the final output directory of each object exists
                QString outRelativePath = fileFixify(dir, FileFixifyBackwards);
                if (!outRelativePath.isEmpty() && !mkdir(outRelativePath))
                    warn_msg(WarnLogic, "Cannot create directory '%s'", outRelativePath.toLatin1().constData());
            }
        } else {
            dir = objdir;
        }
        ret.append(dir + fi.completeBaseName() + Option::obj_ext);
    }
    return ret;
}

ReplaceExtraCompilerCacheKey::ReplaceExtraCompilerCacheKey(
        const QString &v, const QStringList &i, const QStringList &o, MakefileGenerator::ReplaceFor s)
{
    static QString doubleColon = QLatin1String("::");

    hash = 0;
    pwd = qmake_getpwd();
    var = v;
    {
        QStringList il = i;
        il.sort();
        in = il.join(doubleColon);
    }
    {
        QStringList ol = o;
        ol.sort();
        out = ol.join(doubleColon);
    }
    forShell = s;
}

bool ReplaceExtraCompilerCacheKey::operator==(const ReplaceExtraCompilerCacheKey &f) const
{
    return (hashCode() == f.hashCode() &&
            f.forShell == forShell &&
            f.in == in &&
            f.out == out &&
            f.var == var &&
            f.pwd == pwd);
}


QString
MakefileGenerator::replaceExtraCompilerVariables(
        const QString &orig_var, const QStringList &in, const QStringList &out, ReplaceFor forShell)
{
    //lazy cache
    ReplaceExtraCompilerCacheKey cacheKey(orig_var, in, out, forShell);
    QString cacheVal = extraCompilerVariablesCache.value(cacheKey);
    if(!cacheVal.isNull())
        return cacheVal;

    //do the work
    QString ret = orig_var;
    QRegExp reg_var("\\$\\{.*\\}");
    reg_var.setMinimal(true);
    for(int rep = 0; (rep = reg_var.indexIn(ret, rep)) != -1; ) {
        QStringList val;
        const ProString var(ret.mid(rep + 2, reg_var.matchedLength() - 3));
        bool filePath = false;
        if(val.isEmpty() && var.startsWith(QLatin1String("QMAKE_VAR_"))) {
            const ProKey varname = var.mid(10).toKey();
            val += project->values(varname).toQStringList();
        }
        if(val.isEmpty() && var.startsWith(QLatin1String("QMAKE_VAR_FIRST_"))) {
            const ProKey varname = var.mid(16).toKey();
            val += project->first(varname).toQString();
        }

        if(val.isEmpty() && !in.isEmpty()) {
            if(var.startsWith(QLatin1String("QMAKE_FUNC_FILE_IN_"))) {
                filePath = true;
                const ProKey funcname = var.mid(19).toKey();
                val += project->expand(funcname, QList<ProStringList>() << ProStringList(in));
            } else if(var == QLatin1String("QMAKE_FILE_BASE") || var == QLatin1String("QMAKE_FILE_IN_BASE")) {
                filePath = true;
                for(int i = 0; i < in.size(); ++i) {
                    QFileInfo fi(fileInfo(Option::normalizePath(in.at(i))));
                    QString base = fi.completeBaseName();
                    if(base.isNull())
                        base = fi.fileName();
                    val += base;
                }
            } else if (var == QLatin1String("QMAKE_FILE_EXT") || var == QLatin1String("QMAKE_FILE_IN_EXT")) {
                filePath = true;
                for(int i = 0; i < in.size(); ++i) {
                    QFileInfo fi(fileInfo(Option::normalizePath(in.at(i))));
                    QString ext;
                    // Ensure complementarity with QMAKE_FILE_BASE
                    int baseLen = fi.completeBaseName().length();
                    if(baseLen == 0)
                        ext = fi.fileName();
                    else
                        ext = fi.fileName().remove(0, baseLen);
                    val += ext;
                }
            } else if (var == QLatin1String("QMAKE_FILE_IN_NAME")) {
                filePath = true;
                for (int i = 0; i < in.size(); ++i)
                    val += fileInfo(Option::normalizePath(in.at(i))).fileName();
            } else if(var == QLatin1String("QMAKE_FILE_PATH") || var == QLatin1String("QMAKE_FILE_IN_PATH")) {
                filePath = true;
                for(int i = 0; i < in.size(); ++i)
                    val += fileInfo(Option::normalizePath(in.at(i))).path();
            } else if(var == QLatin1String("QMAKE_FILE_NAME") || var == QLatin1String("QMAKE_FILE_IN")) {
                filePath = true;
                for(int i = 0; i < in.size(); ++i)
                    val += fileInfo(Option::normalizePath(in.at(i))).filePath();

            }
        }
        if(val.isEmpty() && !out.isEmpty()) {
            if(var.startsWith(QLatin1String("QMAKE_FUNC_FILE_OUT_"))) {
                filePath = true;
                const ProKey funcname = var.mid(20).toKey();
                val += project->expand(funcname, QList<ProStringList>() << ProStringList(out));
            } else if (var == QLatin1String("QMAKE_FILE_OUT_PATH")) {
                filePath = true;
                for (int i = 0; i < out.size(); ++i)
                    val += fileInfo(Option::normalizePath(out.at(i))).path();
            } else if(var == QLatin1String("QMAKE_FILE_OUT")) {
                filePath = true;
                for(int i = 0; i < out.size(); ++i)
                    val += fileInfo(Option::normalizePath(out.at(i))).filePath();
            } else if(var == QLatin1String("QMAKE_FILE_OUT_BASE")) {
                filePath = true;
                for(int i = 0; i < out.size(); ++i) {
                    QFileInfo fi(fileInfo(Option::normalizePath(out.at(i))));
                    QString base = fi.completeBaseName();
                    if(base.isNull())
                        base = fi.fileName();
                    val += base;
                }
            }
        }
        if(val.isEmpty() && var.startsWith(QLatin1String("QMAKE_FUNC_"))) {
            const ProKey funcname = var.mid(11).toKey();
            val += project->expand(funcname, QList<ProStringList>() << ProStringList(in) << ProStringList(out));
        }

        if(!val.isEmpty()) {
            QString fullVal;
            if (filePath && forShell != NoShell) {
                for(int i = 0; i < val.size(); ++i) {
                    if(!fullVal.isEmpty())
                        fullVal += " ";
                    if (forShell == LocalShell)
                        fullVal += IoUtils::shellQuote(Option::fixPathToLocalOS(val.at(i), false));
                    else
                        fullVal += escapeFilePath(Option::fixPathToTargetOS(val.at(i), false));
                }
            } else {
                fullVal = val.join(' ');
            }
            ret.replace(rep, reg_var.matchedLength(), fullVal);
            rep += fullVal.length();
        } else {
            rep += reg_var.matchedLength();
        }
    }

    //cache the value
    extraCompilerVariablesCache.insert(cacheKey, ret);
    return ret;
}

bool
MakefileGenerator::verifyExtraCompiler(const ProString &comp, const QString &file_unfixed)
{
    if(noIO())
        return false;
    const QString file = Option::normalizePath(file_unfixed);

    const ProStringList &config = project->values(ProKey(comp + ".CONFIG"));
    if (config.indexOf("moc_verify") != -1) {
        if(!file.isNull()) {
            QMakeSourceFileInfo::addSourceFile(file, QMakeSourceFileInfo::SEEK_MOCS);
            if(!mocable(file)) {
                return false;
            } else {
                project->values("MOCABLES").append(file);
            }
        }
    } else if (config.indexOf("function_verify") != -1) {
        ProString tmp_out = project->first(ProKey(comp + ".output"));
        if(tmp_out.isEmpty())
            return false;
        ProStringList verify_function = project->values(ProKey(comp + ".verify_function"));
        if(verify_function.isEmpty())
            return false;

        for(int i = 0; i < verify_function.size(); ++i) {
            bool invert = false;
            ProString verify = verify_function.at(i);
            if(verify.at(0) == QLatin1Char('!')) {
                invert = true;
                verify = verify.mid(1);
            }

            if (config.indexOf("combine") != -1) {
                bool pass = project->test(verify.toKey(), QList<ProStringList>() << ProStringList(tmp_out) << ProStringList(file));
                if(invert)
                    pass = !pass;
                if(!pass)
                    return false;
            } else {
                const ProStringList &tmp = project->values(ProKey(comp + ".input"));
                for (ProStringList::ConstIterator it = tmp.begin(); it != tmp.end(); ++it) {
                    const ProStringList &inputs = project->values((*it).toKey());
                    for (ProStringList::ConstIterator input = inputs.begin(); input != inputs.end(); ++input) {
                        if((*input).isEmpty())
                            continue;
                        QString inpf = (*input).toQString();
                        QString in = fileFixify(inpf);
                        if(in == file) {
                            bool pass = project->test(verify.toKey(),
                                                      QList<ProStringList>() << ProStringList(replaceExtraCompilerVariables(tmp_out.toQString(), inpf, QString(), NoShell)) <<
                                                      ProStringList(file));
                            if(invert)
                                pass = !pass;
                            if(!pass)
                                return false;
                            break;
                        }
                    }
                }
            }
        }
    } else if (config.indexOf("verify") != -1) {
        QString tmp_out = project->first(ProKey(comp + ".output")).toQString();
        if(tmp_out.isEmpty())
            return false;
        const QString tmp_cmd = project->values(ProKey(comp + ".commands")).join(' ');
        if (config.indexOf("combine") != -1) {
            QString cmd = replaceExtraCompilerVariables(tmp_cmd, QString(), tmp_out, LocalShell);
            if(system(cmd.toLatin1().constData()))
                return false;
        } else {
            const ProStringList &tmp = project->values(ProKey(comp + ".input"));
            for (ProStringList::ConstIterator it = tmp.begin(); it != tmp.end(); ++it) {
                const ProStringList &inputs = project->values((*it).toKey());
                for (ProStringList::ConstIterator input = inputs.begin(); input != inputs.end(); ++input) {
                    if((*input).isEmpty())
                        continue;
                    QString inpf = (*input).toQString();
                    QString in = fileFixify(inpf);
                    if(in == file) {
                        QString out = replaceExtraCompilerVariables(tmp_out, inpf, QString(), NoShell);
                        QString cmd = replaceExtraCompilerVariables(tmp_cmd, in, out, LocalShell);
                        if(system(cmd.toLatin1().constData()))
                            return false;
                        break;
                    }
                }
            }
        }
    }
    return true;
}

void
MakefileGenerator::writeExtraTargets(QTextStream &t)
{
    const ProStringList &qut = project->values("QMAKE_EXTRA_TARGETS");
    for (ProStringList::ConstIterator it = qut.begin(); it != qut.end(); ++it) {
        QString targ = var(ProKey(*it + ".target")),
                 cmd = var(ProKey(*it + ".commands")), deps;
        if(targ.isEmpty())
            targ = (*it).toQString();
        const ProStringList &deplist = project->values(ProKey(*it + ".depends"));
        for (ProStringList::ConstIterator dep_it = deplist.begin(); dep_it != deplist.end(); ++dep_it) {
            QString dep = var(ProKey(*dep_it + ".target"));
            if(dep.isEmpty())
                dep = (*dep_it).toQString();
            deps += " " + escapeDependencyPath(dep);
        }
        const ProStringList &config = project->values(ProKey(*it + ".CONFIG"));
        if (config.indexOf("fix_target") != -1)
            targ = fileFixify(targ, FileFixifyFromOutdir);
        if (config.indexOf("phony") != -1)
            deps += QLatin1String(" FORCE");
        t << escapeDependencyPath(targ) << ":" << deps;
        if(!cmd.isEmpty())
            t << "\n\t" << cmd;
        t << Qt::endl << Qt::endl;
    }
}

static QStringList splitDeps(const QString &indeps, bool lineMode)
{
    if (!lineMode)
        return indeps.simplified().split(' ');
    QStringList deps = indeps.split('\n', Qt::SkipEmptyParts);
#ifdef Q_OS_WIN
    for (auto &dep : deps) {
        if (dep.endsWith(QLatin1Char('\r')))
            dep.chop(1);
    }
#endif
    return deps;
}

QString MakefileGenerator::resolveDependency(const QDir &outDir, const QString &file)
{
    const QVector<QMakeLocalFileName> &depdirs = QMakeSourceFileInfo::dependencyPaths();
    for (const auto &depdir : depdirs) {
        const QString &local = depdir.local();
        QString lf = outDir.absoluteFilePath(local + '/' + file);
        if (exists(lf))
            return lf;

        if (resolveDependenciesInFrameworks) {
            // Given a file like "QtWidgets/QWidget", try to resolve it
            // as framework header "QtWidgets.framework/Headers/QWidget".
            int cut = file.indexOf('/');
            if (cut < 0 || cut + 1 >= file.size())
                continue;
            QStringRef framework = file.leftRef(cut);
            QStringRef include = file.midRef(cut + 1);
            if (local.endsWith('/' + framework + ".framework/Headers")) {
                lf = outDir.absoluteFilePath(local + '/' + include);
                if (exists(lf))
                    return lf;
            }
        }
    }
    return {};
}

void MakefileGenerator::callExtraCompilerDependCommand(const ProString &extraCompiler,
                                                       const QString &tmp_dep_cmd,
                                                       const QString &inpf,
                                                       const QString &tmp_out,
                                                       bool dep_lines,
                                                       QStringList *deps,
                                                       bool existingDepsOnly,
                                                       bool checkCommandAvailability)
{
    char buff[256];
    QString dep_cmd = replaceExtraCompilerVariables(tmp_dep_cmd, inpf, tmp_out, LocalShell);
    if (checkCommandAvailability && !canExecute(dep_cmd))
        return;
    dep_cmd = QLatin1String("cd ")
            + IoUtils::shellQuote(Option::fixPathToLocalOS(Option::output_dir, false))
            + QLatin1String(" && ")
            + fixEnvVariables(dep_cmd);
    if (FILE *proc = QT_POPEN(dep_cmd.toLatin1().constData(), QT_POPEN_READ)) {
        QByteArray depData;
        while (int read_in = feof(proc) ? 0 : (int)fread(buff, 1, 255, proc))
            depData.append(buff, read_in);
        QT_PCLOSE(proc);
        const QString indeps = QString::fromLocal8Bit(depData);
        if (indeps.isEmpty())
            return;
        QDir outDir(Option::output_dir);
        QStringList dep_cmd_deps = splitDeps(indeps, dep_lines);
        for (int i = 0; i < dep_cmd_deps.count(); ++i) {
            QString &file = dep_cmd_deps[i];
            const QString absFile = outDir.absoluteFilePath(file);
            if (absFile == file) {
                // already absolute; don't do any checks.
            } else if (exists(absFile)) {
                file = absFile;
            } else {
                const QString localFile = resolveDependency(outDir, file);
                if (localFile.isEmpty()) {
                    if (exists(file)) {
                        warn_msg(WarnDeprecated, ".depend_command for extra compiler %s"
                                 " prints paths relative to source directory",
                                 extraCompiler.toLatin1().constData());
                    } else if (existingDepsOnly) {
                        file.clear();
                    } else {
                        file = absFile;  // fallback for generated resources
                    }
                } else {
                    file = localFile;
                }
            }
            if (!file.isEmpty())
                file = fileFixify(file);
        }
        deps->append(dep_cmd_deps);
    }
}

void
MakefileGenerator::writeExtraCompilerTargets(QTextStream &t)
{
    QString clean_targets;
    const ProStringList &quc = project->values("QMAKE_EXTRA_COMPILERS");
    for (ProStringList::ConstIterator it = quc.begin(); it != quc.end(); ++it) {
        const ProStringList &config = project->values(ProKey(*it + ".CONFIG"));
        QString tmp_out = fileFixify(project->first(ProKey(*it + ".output")).toQString(),
                                     FileFixifyFromOutdir);
        const QString tmp_cmd = project->values(ProKey(*it + ".commands")).join(' ');
        const QString tmp_dep_cmd = project->values(ProKey(*it + ".depend_command")).join(' ');
        const bool dep_lines = (config.indexOf("dep_lines") != -1);
        const ProStringList &vars = project->values(ProKey(*it + ".variables"));
        if(tmp_out.isEmpty() || tmp_cmd.isEmpty())
            continue;
        ProStringList tmp_inputs;
        {
            const ProStringList &comp_inputs = project->values(ProKey(*it + ".input"));
            for (ProStringList::ConstIterator it2 = comp_inputs.begin(); it2 != comp_inputs.end(); ++it2) {
                const ProStringList &tmp = project->values((*it2).toKey());
                for (ProStringList::ConstIterator input = tmp.begin(); input != tmp.end(); ++input) {
                    if (verifyExtraCompiler((*it), (*input).toQString()))
                        tmp_inputs.append((*input));
                }
            }
        }

        t << "compiler_" << (*it) << "_make_all:";
        if (config.indexOf("combine") != -1) {
            // compilers with a combined input only have one output
            QString input = project->first(ProKey(*it + ".output")).toQString();
            t << ' ' << escapeDependencyPath(fileFixify(
                    replaceExtraCompilerVariables(tmp_out, input, QString(), NoShell),
                    FileFixifyFromOutdir));
        } else {
            for (ProStringList::ConstIterator input = tmp_inputs.cbegin(); input != tmp_inputs.cend(); ++input) {
                t << ' ' << escapeDependencyPath(fileFixify(
                        replaceExtraCompilerVariables(tmp_out, (*input).toQString(), QString(), NoShell),
                        FileFixifyFromOutdir));
            }
        }
        t << Qt::endl;

        if (config.indexOf("no_clean") == -1) {
            QStringList raw_clean = project->values(ProKey(*it + ".clean")).toQStringList();
            if (raw_clean.isEmpty())
                raw_clean << tmp_out;
            QString tmp_clean;
            for (const QString &rc : qAsConst(raw_clean))
                tmp_clean += ' ' + escapeFilePath(Option::fixPathToTargetOS(rc));
            QString tmp_clean_cmds = project->values(ProKey(*it + ".clean_commands")).join(' ');
            if(!tmp_inputs.isEmpty())
                clean_targets += QString("compiler_" + (*it) + "_clean ");
            t << "compiler_" << (*it) << "_clean:";
            bool wrote_clean_cmds = false, wrote_clean = false;
            if(tmp_clean_cmds.isEmpty()) {
                wrote_clean_cmds = true;
            } else if(tmp_clean_cmds.indexOf("${QMAKE_") == -1) {
                t << "\n\t" << tmp_clean_cmds;
                wrote_clean_cmds = true;
            }
            if(tmp_clean.indexOf("${QMAKE_") == -1) {
                t << "\n\t-$(DEL_FILE)" << tmp_clean;
                wrote_clean = true;
            }
            if(!wrote_clean_cmds || !wrote_clean) {
                QStringList cleans;
                const QString del_statement("-$(DEL_FILE)");
                if(!wrote_clean) {
                    QStringList dels;
                    for (ProStringList::ConstIterator input = tmp_inputs.cbegin(); input != tmp_inputs.cend(); ++input) {
                        QString tinp = (*input).toQString();
                        QString out = replaceExtraCompilerVariables(tmp_out, tinp, QString(), NoShell);
                        for (const QString &rc : qAsConst(raw_clean)) {
                            dels << ' ' + escapeFilePath(fileFixify(
                                    replaceExtraCompilerVariables(rc, tinp, out, NoShell),
                                    FileFixifyFromOutdir));
                        }
                    }
                    if(project->isActiveConfig("no_delete_multiple_files")) {
                        cleans = dels;
                    } else {
                        QString files;
                        const int commandlineLimit = 2047; // NT limit, expanded
                        for (const QString &file : qAsConst(dels)) {
                            if(del_statement.length() + files.length() +
                               qMax(fixEnvVariables(file).length(), file.length()) > commandlineLimit) {
                                cleans.append(files);
                                files.clear();
                            }
                            files += file;
                        }
                        if(!files.isEmpty())
                            cleans.append(files);
                    }
                }
                if(!cleans.isEmpty())
                    t << valGlue(cleans, "\n\t" + del_statement, "\n\t" + del_statement, "");
                if(!wrote_clean_cmds) {
                    for (ProStringList::ConstIterator input = tmp_inputs.cbegin(); input != tmp_inputs.cend(); ++input) {
                        QString tinp = (*input).toQString();
                        t << "\n\t" << replaceExtraCompilerVariables(tmp_clean_cmds, tinp,
                                         replaceExtraCompilerVariables(tmp_out, tinp, QString(), NoShell), TargetShell);
                    }
                }
            }
            t << Qt::endl;
        }
        const bool existingDepsOnly = config.contains("dep_existing_only");
        QStringList tmp_dep = project->values(ProKey(*it + ".depends")).toQStringList();
        if (config.indexOf("combine") != -1) {
            if (tmp_out.contains(QRegExp("(^|[^$])\\$\\{QMAKE_(?!VAR_)"))) {
                warn_msg(WarnLogic, "QMAKE_EXTRA_COMPILERS(%s) with combine has variable output.",
                         (*it).toLatin1().constData());
                continue;
            }
            QStringList deps, inputs;
            if(!tmp_dep.isEmpty())
                deps += fileFixify(tmp_dep, FileFixifyFromOutdir);
            for (ProStringList::ConstIterator input = tmp_inputs.cbegin(); input != tmp_inputs.cend(); ++input) {
                QString inpf = (*input).toQString();
                deps += findDependencies(inpf);
                inputs += Option::fixPathToTargetOS(inpf, false);
                if(!tmp_dep_cmd.isEmpty() && doDepends()) {
                    callExtraCompilerDependCommand(*it, tmp_dep_cmd, inpf,
                                                   tmp_out, dep_lines, &deps, existingDepsOnly);
                }
            }
            for(int i = 0; i < inputs.size(); ) {
                if(tmp_out == inputs.at(i))
                    inputs.removeAt(i);
                else
                    ++i;
            }
            for(int i = 0; i < deps.size(); ) {
                if(tmp_out == deps.at(i))
                    deps.removeAt(i);
                else
                    ++i;
            }
            if (inputs.isEmpty())
                continue;

            QString out = replaceExtraCompilerVariables(tmp_out, QString(), QString(), NoShell);
            QString cmd = replaceExtraCompilerVariables(tmp_cmd, inputs, QStringList() << out, TargetShell);
            t << escapeDependencyPath(fileFixify(out, FileFixifyFromOutdir)) << ":";
            // compiler.CONFIG+=explicit_dependencies means that ONLY compiler.depends gets to cause Makefile dependencies
            if (config.indexOf("explicit_dependencies") != -1) {
                t << " " << valList(escapeDependencyPaths(fileFixify(tmp_dep, FileFixifyFromOutdir)));
            } else {
                t << " " << valList(escapeDependencyPaths(inputs)) << " " << valList(finalizeDependencyPaths(deps));
            }
            t << "\n\t" << cmd << Qt::endl << Qt::endl;
            continue;
        }
        for (ProStringList::ConstIterator input = tmp_inputs.cbegin(); input != tmp_inputs.cend(); ++input) {
            QString inpf = (*input).toQString();
            QStringList deps;
            deps << fileFixify(inpf, FileFixifyFromOutdir);
            deps += findDependencies(inpf);
            QString out = fileFixify(replaceExtraCompilerVariables(tmp_out, inpf, QString(), NoShell),
                                     FileFixifyFromOutdir);
            if(!tmp_dep.isEmpty()) {
                QStringList pre_deps = fileFixify(tmp_dep, FileFixifyFromOutdir);
                for(int i = 0; i < pre_deps.size(); ++i)
                   deps << fileFixify(replaceExtraCompilerVariables(pre_deps.at(i), inpf, out, NoShell),
                                      FileFixifyFromOutdir);
            }
            QString cmd = replaceExtraCompilerVariables(tmp_cmd, inpf, out, TargetShell);
            // NOTE: The var -> QMAKE_COMP_var replace feature is unsupported, do not use!
            for (ProStringList::ConstIterator it3 = vars.constBegin(); it3 != vars.constEnd(); ++it3)
                cmd.replace("$(" + (*it3) + ")", "$(QMAKE_COMP_" + (*it3)+")");
            if(!tmp_dep_cmd.isEmpty() && doDepends()) {
                callExtraCompilerDependCommand(*it, tmp_dep_cmd, inpf,
                                               tmp_out, dep_lines, &deps, existingDepsOnly);
                //use the depend system to find includes of these included files
                QStringList inc_deps;
                for(int i = 0; i < deps.size(); ++i) {
                    const QString dep = fileFixify(deps.at(i), FileFixifyFromOutdir | FileFixifyAbsolute);
                    if(QFile::exists(dep)) {
                        SourceFileType type = TYPE_UNKNOWN;
                        if(type == TYPE_UNKNOWN) {
                            for(QStringList::Iterator cit = Option::c_ext.begin();
                                cit != Option::c_ext.end(); ++cit) {
                                if(dep.endsWith((*cit))) {
                                   type = TYPE_C;
                                   break;
                                }
                            }
                        }
                        if(type == TYPE_UNKNOWN) {
                            for(QStringList::Iterator cppit = Option::cpp_ext.begin();
                                cppit != Option::cpp_ext.end(); ++cppit) {
                                if(dep.endsWith((*cppit))) {
                                    type = TYPE_C;
                                    break;
                                }
                            }
                        }
                        if(type == TYPE_UNKNOWN) {
                            for(QStringList::Iterator hit = Option::h_ext.begin();
                                type == TYPE_UNKNOWN && hit != Option::h_ext.end(); ++hit) {
                                if(dep.endsWith((*hit))) {
                                    type = TYPE_C;
                                    break;
                                }
                            }
                        }
                        if(type != TYPE_UNKNOWN) {
                            if(!QMakeSourceFileInfo::containsSourceFile(dep, type))
                                QMakeSourceFileInfo::addSourceFile(dep, type);
                            inc_deps += QMakeSourceFileInfo::dependencies(dep);
                        }
                    }
                }
                deps += fileFixify(inc_deps, FileFixifyFromOutdir);
            }
            for(int i = 0; i < deps.size(); ) {
                QString &dep = deps[i];
                if(out == dep)
                    deps.removeAt(i);
                else
                    ++i;
            }
            t << escapeDependencyPath(out) << ": " << valList(finalizeDependencyPaths(deps)) << "\n\t"
              << cmd << Qt::endl << Qt::endl;
        }
    }
    t << "compiler_clean: " << clean_targets << Qt::endl << Qt::endl;
}

void
MakefileGenerator::writeExtraCompilerVariables(QTextStream &t)
{
    bool first = true;
    const ProStringList &quc = project->values("QMAKE_EXTRA_COMPILERS");
    for (ProStringList::ConstIterator it = quc.begin(); it != quc.end(); ++it) {
        const ProStringList &vars = project->values(ProKey(*it + ".variables"));
        for (ProStringList::ConstIterator varit = vars.begin(); varit != vars.end(); ++varit) {
            if(first) {
                t << "\n####### Custom Compiler Variables\n";
                first = false;
            }
            t << "QMAKE_COMP_" << (*varit) << " = "
              << valList(project->values((*varit).toKey())) << Qt::endl;
        }
    }
    if(!first)
        t << Qt::endl;
}

void
MakefileGenerator::writeExtraVariables(QTextStream &t)
{
    t << Qt::endl;

    ProStringList outlist;
    const ProValueMap &vars = project->variables();
    const ProStringList &exports = project->values("QMAKE_EXTRA_VARIABLES");
    for (ProStringList::ConstIterator exp_it = exports.begin(); exp_it != exports.end(); ++exp_it) {
        QRegExp rx((*exp_it).toQString(), Qt::CaseInsensitive, QRegExp::Wildcard);
        for (ProValueMap::ConstIterator it = vars.begin(); it != vars.end(); ++it) {
            if (rx.exactMatch(it.key().toQString()))
                outlist << ("EXPORT_" + it.key() + " = " + it.value().join(' '));
        }
    }
    if (!outlist.isEmpty()) {
        t << "####### Custom Variables\n";
        t << outlist.join('\n') << Qt::endl << Qt::endl;
    }
}

// This is a more powerful alternative to the above function.
// It's meant to be internal, as one can make quite a mess with it.
void
MakefileGenerator::writeExportedVariables(QTextStream &t)
{
    const auto &vars = project->values("QMAKE_EXPORTED_VARIABLES");
    if (vars.isEmpty())
        return;
    for (const auto &exp : vars) {
        const ProString &name = project->first(ProKey(exp + ".name"));
        const ProString &value = project->first(ProKey(exp + ".value"));
        if (!value.isEmpty())
            t << name << " = " << value << Qt::endl;
        else
            t << name << " =\n";
    }
    t << Qt::endl;
}

bool
MakefileGenerator::writeDummyMakefile(QTextStream &t)
{
    if (project->values("QMAKE_FAILED_REQUIREMENTS").isEmpty())
        return false;
    t << "QMAKE    = " << var("QMAKE_QMAKE") << Qt::endl;
    const ProStringList &qut = project->values("QMAKE_EXTRA_TARGETS");
    for (ProStringList::ConstIterator it = qut.begin(); it != qut.end(); ++it)
        t << *it << " ";
    t << "first all clean install distclean uninstall qmake_all:\n\t"
      << "@echo \"Some of the required modules ("
      << var("QMAKE_FAILED_REQUIREMENTS") << ") are not available.\"\n\t"
      << "@echo \"Skipped.\"\n\n";
    writeMakeQmake(t);
    t << "FORCE:\n\n";
    return true;
}

bool
MakefileGenerator::writeMakefile(QTextStream &t)
{
    t << "####### Compile\n\n";
    writeObj(t, "SOURCES");
    writeObj(t, "GENERATED_SOURCES");

    t << "####### Install\n\n";
    writeInstalls(t);

    t << "FORCE:\n\n";
    return true;
}

void
MakefileGenerator::writeDefaultVariables(QTextStream &t)
{
    t << "QMAKE         = " << var("QMAKE_QMAKE") << Qt::endl;
    t << "DEL_FILE      = " << var("QMAKE_DEL_FILE") << Qt::endl;
    t << "CHK_DIR_EXISTS= " << var("QMAKE_CHK_DIR_EXISTS") << Qt::endl;
    t << "MKDIR         = " << var("QMAKE_MKDIR") << Qt::endl;
    t << "COPY          = " << var("QMAKE_COPY") << Qt::endl;
    t << "COPY_FILE     = " << var("QMAKE_COPY_FILE") << Qt::endl;
    t << "COPY_DIR      = " << var("QMAKE_COPY_DIR") << Qt::endl;
    t << "INSTALL_FILE  = " << var("QMAKE_INSTALL_FILE") << Qt::endl;
    t << "INSTALL_PROGRAM = " << var("QMAKE_INSTALL_PROGRAM") << Qt::endl;
    t << "INSTALL_DIR   = " << var("QMAKE_INSTALL_DIR") << Qt::endl;
    t << "QINSTALL      = " << var("QMAKE_QMAKE") << " -install qinstall" << Qt::endl;
    t << "QINSTALL_PROGRAM = " << var("QMAKE_QMAKE") << " -install qinstall -exe" << Qt::endl;
    t << "DEL_FILE      = " << var("QMAKE_DEL_FILE") << Qt::endl;
    t << "SYMLINK       = " << var("QMAKE_SYMBOLIC_LINK") << Qt::endl;
    t << "DEL_DIR       = " << var("QMAKE_DEL_DIR") << Qt::endl;
    t << "MOVE          = " << var("QMAKE_MOVE") << Qt::endl;
}

QString MakefileGenerator::buildArgs(bool withExtra)
{
    QString ret;

    for (const QString &arg : qAsConst(Option::globals->qmake_args))
        ret += " " + shellQuote(arg);
    if (withExtra && !Option::globals->qmake_extra_args.isEmpty()) {
        ret += " --";
        for (const QString &arg : qAsConst(Option::globals->qmake_extra_args))
            ret += " " + shellQuote(arg);
    }
    return ret;
}

//could get stored argv, but then it would have more options than are
//probably necesary this will try to guess the bare minimum..
QString MakefileGenerator::fullBuildArgs()
{
    QString ret;

    //output
    QString ofile = fileFixify(Option::output.fileName());
    if(!ofile.isEmpty() && ofile != project->first("QMAKE_MAKEFILE"))
        ret += " -o " + escapeFilePath(ofile);

    //inputs
    ret += " " + escapeFilePath(fileFixify(project->projectFile()));

    // general options and arguments
    ret += buildArgs(true);

    return ret;
}

void
MakefileGenerator::writeHeader(QTextStream &t)
{
    t << "#############################################################################\n";
    t << "# Makefile for building: " << escapeFilePath(var("TARGET")) << Qt::endl;
    t << "# Generated by qmake (" QMAKE_VERSION_STR ") (Qt " QT_VERSION_STR ")\n";
    t << "# Project:  " << fileFixify(project->projectFile()) << Qt::endl;
    t << "# Template: " << var("TEMPLATE") << Qt::endl;
    if(!project->isActiveConfig("build_pass"))
        t << "# Command: " << var("QMAKE_QMAKE") << fullBuildArgs() << Qt::endl;
    t << "#############################################################################\n";
    t << Qt::endl;
    QString ofile = Option::fixPathToTargetOS(Option::output.fileName());
    if (ofile.lastIndexOf(Option::dir_sep) != -1)
        ofile.remove(0, ofile.lastIndexOf(Option::dir_sep) +1);
    t << "MAKEFILE      = " << escapeFilePath(ofile) << Qt::endl << Qt::endl;
    t << "EQ            = =\n\n";
}

QList<MakefileGenerator::SubTarget*>
MakefileGenerator::findSubDirsSubTargets() const
{
    QList<SubTarget*> targets;
    {
        const ProStringList &subdirs = project->values("SUBDIRS");
        for(int subdir = 0; subdir < subdirs.size(); ++subdir) {
            ProString ofile = subdirs[subdir];
            QString oname = ofile.toQString();
            QString fixedSubdir = oname;
            fixedSubdir.replace(QRegExp("[^a-zA-Z0-9_]"),"-");

            SubTarget *st = new SubTarget;
            st->name = oname;
            targets.append(st);

            bool fromFile = false;
            const ProKey fkey(fixedSubdir + ".file");
            const ProKey skey(fixedSubdir + ".subdir");
            if (!project->isEmpty(fkey)) {
                if (!project->isEmpty(skey))
                    warn_msg(WarnLogic, "Cannot assign both file and subdir for subdir %s",
                             subdirs[subdir].toLatin1().constData());
                ofile = project->first(fkey);
                fromFile = true;
            } else if (!project->isEmpty(skey)) {
                ofile = project->first(skey);
                fromFile = false;
            } else {
                fromFile = ofile.endsWith(Option::pro_ext);
            }
            QString file = Option::fixPathToTargetOS(ofile.toQString());

            if(fromFile) {
                int slsh = file.lastIndexOf(Option::dir_sep);
                if(slsh != -1) {
                    st->in_directory = file.left(slsh+1);
                    st->profile = file.mid(slsh+1);
                } else {
                    st->profile = file;
                }
            } else {
                if (!file.isEmpty() && !project->isActiveConfig("subdir_first_pro")) {
                    const QString baseName = file.section(Option::dir_sep, -1);
                    if (baseName.isEmpty()) {
                        warn_msg(WarnLogic, "Ignoring invalid SUBDIRS entry %s",
                                 subdirs[subdir].toLatin1().constData());
                        continue;
                    }
                    st->profile = baseName + Option::pro_ext;
                }
                st->in_directory = file;
            }
            while(st->in_directory.endsWith(Option::dir_sep))
                st->in_directory.chop(1);
            if(fileInfo(st->in_directory).isRelative())
                st->out_directory = st->in_directory;
            else
                st->out_directory = fileFixify(st->in_directory, FileFixifyBackwards);
            const ProKey mkey(fixedSubdir + ".makefile");
            if (!project->isEmpty(mkey)) {
                st->makefile = project->first(mkey).toQString();
            } else {
                st->makefile = "Makefile";
                if(!st->profile.isEmpty()) {
                    QString basename = st->in_directory;
                    int new_slsh = basename.lastIndexOf(Option::dir_sep);
                    if(new_slsh != -1)
                        basename = basename.mid(new_slsh+1);
                    if(st->profile != basename + Option::pro_ext)
                        st->makefile += "." + st->profile.left(st->profile.length() - Option::pro_ext.length());
                }
            }
            const ProKey dkey(fixedSubdir + ".depends");
            if (!project->isEmpty(dkey)) {
                const ProStringList &depends = project->values(dkey);
                for(int depend = 0; depend < depends.size(); ++depend) {
                    bool found = false;
                    for(int subDep = 0; subDep < subdirs.size(); ++subDep) {
                        if(subdirs[subDep] == depends.at(depend)) {
                            QString subName = subdirs[subDep].toQString();
                            QString fixedSubDep = subName;
                            fixedSubDep.replace(QRegExp("[^a-zA-Z0-9_]"),"-");
                            const ProKey dtkey(fixedSubDep + ".target");
                            if (!project->isEmpty(dtkey)) {
                                st->depends += project->first(dtkey);
                            } else {
                                QString d = Option::fixPathToTargetOS(subName);
                                const ProKey dfkey(fixedSubDep + ".file");
                                if (!project->isEmpty(dfkey)) {
                                    d = project->first(dfkey).toQString();
                                } else {
                                    const ProKey dskey(fixedSubDep + ".subdir");
                                    if (!project->isEmpty(dskey))
                                        d = project->first(dskey).toQString();
                                }
                                st->depends += "sub-" + d.replace(QRegExp("[^a-zA-Z0-9_]"),"-");
                            }
                            found = true;
                            break;
                        }
                    }
                    if(!found) {
                        QString depend_str = depends.at(depend).toQString();
                        st->depends += depend_str.replace(QRegExp("[^a-zA-Z0-9_]"),"-");
                    }
                }
            }
            const ProKey tkey(fixedSubdir + ".target");
            if (!project->isEmpty(tkey)) {
                st->target = project->first(tkey).toQString();
            } else {
                st->target = "sub-" + file;
                st->target.replace(QRegExp("[^a-zA-Z0-9_]"), "-");
            }
        }
    }
    return targets;
}

void
MakefileGenerator::writeSubDirs(QTextStream &t)
{
    QList<SubTarget*> targets = findSubDirsSubTargets();
    t << "first: make_first\n";
    int flags = SubTargetInstalls;
    if(project->isActiveConfig("ordered"))
        flags |= SubTargetOrdered;
    writeSubTargets(t, targets, flags);
    qDeleteAll(targets);
}

void MakefileGenerator::writeSubMakeCall(QTextStream &t, const QString &callPrefix,
                                         const QString &makeArguments)
{
    t << callPrefix << "$(MAKE)" << makeArguments << Qt::endl;
}

void
MakefileGenerator::writeSubTargetCall(QTextStream &t,
        const QString &in_directory, const QString &in, const QString &out_directory, const QString &out,
        const QString &out_directory_cdin, const QString &makefilein)
{
    QString pfx;
    if (!in.isEmpty()) {
        if (!in_directory.isEmpty())
            t << "\n\t" << mkdir_p_asstring(out_directory);
        pfx = "( " + chkexists.arg(out) +
              + " $(QMAKE) -o " + out + ' ' + in + buildArgs(false)
              + " ) && ";
    }
    writeSubMakeCall(t, out_directory_cdin + pfx, makefilein);
}

static void chopEndLines(QString *s)
{
    while (!s->isEmpty()) {
        const ushort c = s->at(s->size() - 1).unicode();
        if (c != '\n' && c != '\r')
            break;
        s->chop(1);
    }
}

void
MakefileGenerator::writeSubTargets(QTextStream &t, QList<MakefileGenerator::SubTarget*> targets, int flags)
{
    // blasted includes
    const ProStringList &qeui = project->values("QMAKE_EXTRA_INCLUDES");
    for (ProStringList::ConstIterator qeui_it = qeui.begin(); qeui_it != qeui.end(); ++qeui_it)
        t << "include " << (*qeui_it) << Qt::endl;

    if (!(flags & SubTargetSkipDefaultVariables)) {
        writeDefaultVariables(t);
        t << "SUBTARGETS    = ";     // subtargets are sub-directory
        for(int target = 0; target < targets.size(); ++target)
            t << " \\\n\t\t" << targets.at(target)->target;
        t << Qt::endl << Qt::endl;
    }
    writeExtraVariables(t);

    QStringList targetSuffixes;
    const QString abs_source_path = project->first("QMAKE_ABSOLUTE_SOURCE_PATH").toQString();
    if (!(flags & SubTargetSkipDefaultTargets)) {
        targetSuffixes << "make_first" << "all" << "clean" << "distclean"
                       << QString((flags & SubTargetInstalls) ? "install_subtargets" : "install")
                       << QString((flags & SubTargetInstalls) ? "uninstall_subtargets" : "uninstall");
    }

    struct SequentialInstallData
    {
        QString targetPrefix;
        QString commands;
        QTextStream commandsStream;
        SequentialInstallData() : commandsStream(&commands) {}
    };
    std::unique_ptr<SequentialInstallData> sequentialInstallData;
    bool dont_recurse = project->isActiveConfig("dont_recurse");

    // generate target rules
    for(int target = 0; target < targets.size(); ++target) {
        SubTarget *subtarget = targets.at(target);
        QString in_directory = subtarget->in_directory;
        if(!in_directory.isEmpty() && !in_directory.endsWith(Option::dir_sep))
            in_directory += Option::dir_sep;
        QString out_directory = subtarget->out_directory;
        if(!out_directory.isEmpty() && !out_directory.endsWith(Option::dir_sep))
            out_directory += Option::dir_sep;
        if(!abs_source_path.isEmpty() && out_directory.startsWith(abs_source_path))
            out_directory = Option::output_dir + out_directory.mid(abs_source_path.length());

        QString out_directory_cdin = out_directory.isEmpty() ? "\n\t"
                                                             : "\n\tcd " + escapeFilePath(out_directory) + " && ";
        QString makefilein = " -f " + escapeFilePath(subtarget->makefile);

        //qmake it
        QString out;
        QString in;
        if(!subtarget->profile.isEmpty()) {
            out = subtarget->makefile;
            in = escapeFilePath(fileFixify(in_directory + subtarget->profile, FileFixifyAbsolute));
            if(out.startsWith(in_directory))
                out = out.mid(in_directory.length());
            out = escapeFilePath(out);
            t << subtarget->target << "-qmake_all: ";
            if (flags & SubTargetOrdered) {
                if (target)
                    t << targets.at(target - 1)->target << "-qmake_all";
            } else {
                if (!subtarget->depends.isEmpty())
                    t << valGlue(subtarget->depends, QString(), "-qmake_all ", "-qmake_all");
            }
            t << " FORCE\n\t";
            if(!in_directory.isEmpty()) {
                t << mkdir_p_asstring(out_directory)
                  << out_directory_cdin;
            }
            t << "$(QMAKE) -o " << out << ' ' << in << buildArgs(false);
            if (!dont_recurse)
                writeSubMakeCall(t, out_directory_cdin, makefilein + " qmake_all");
            else
                t << Qt::endl;
        }

        { //actually compile
            t << subtarget->target << ":";
            auto extraDeps = extraSubTargetDependencies();
            if (!extraDeps.isEmpty())
                t << " " << valList(extraDeps);
            if(!subtarget->depends.isEmpty())
                t << " " << valList(subtarget->depends);
            t << " FORCE";
            writeSubTargetCall(t, in_directory, in, out_directory, out,
                               out_directory_cdin, makefilein);
        }

        for(int suffix = 0; suffix < targetSuffixes.size(); ++suffix) {
            QString s = targetSuffixes.at(suffix);
            if(s == "install_subtargets")
                s = "install";
            else if(s == "uninstall_subtargets")
                s = "uninstall";
            else if(s == "make_first")
                s = QString();

            if (project->isActiveConfig("build_all") && s == "install") {
                if (!sequentialInstallData)
                    sequentialInstallData.reset(new SequentialInstallData);
                sequentialInstallData->targetPrefix += subtarget->target + '-';
                writeSubTargetCall(sequentialInstallData->commandsStream, in_directory, in,
                                   out_directory, out, out_directory_cdin,
                                   makefilein + " " + s);
                chopEndLines(&sequentialInstallData->commands);
            }

            if(flags & SubTargetOrdered) {
                t << subtarget->target << "-" << targetSuffixes.at(suffix) << "-ordered:";
                if(target)
                    t << " " << targets.at(target-1)->target << "-" << targetSuffixes.at(suffix) << "-ordered ";
                t << " FORCE";
                writeSubTargetCall(t, in_directory, in, out_directory, out,
                                   out_directory_cdin, makefilein + " " + s);
            }
            t << subtarget->target << "-" << targetSuffixes.at(suffix) << ":";
            if(!subtarget->depends.isEmpty())
                t << " " << valGlue(subtarget->depends, QString(), "-" + targetSuffixes.at(suffix) + " ",
                                    "-"+targetSuffixes.at(suffix));
            t << " FORCE";
            writeSubTargetCall(t, in_directory, in, out_directory, out,
                               out_directory_cdin, makefilein + " " + s);
        }
    }
    t << Qt::endl;

    if (sequentialInstallData) {
        t << sequentialInstallData->targetPrefix << "install: FORCE"
          << sequentialInstallData->commands << Qt::endl << Qt::endl;
    }

    if (!(flags & SubTargetSkipDefaultTargets)) {
        writeMakeQmake(t, true);

        t << "qmake_all:";
        if(!targets.isEmpty()) {
            for(QList<SubTarget*>::Iterator it = targets.begin(); it != targets.end(); ++it) {
                if(!(*it)->profile.isEmpty())
                    t << " " << (*it)->target << "-qmake_all";
            }
        }
        t << " FORCE\n\n";
    }

    for(int s = 0; s < targetSuffixes.size(); ++s) {
        QString suffix = targetSuffixes.at(s);
        if(!(flags & SubTargetInstalls) && suffix.endsWith("install"))
            continue;

        t << suffix << ":";
        for(int target = 0; target < targets.size(); ++target) {
            SubTarget *subTarget = targets.at(target);
            const ProStringList &config = project->values(ProKey(subTarget->name + ".CONFIG"));
            if (suffix == "make_first"
                && config.indexOf("no_default_target") != -1) {
                continue;
            }
            if((suffix == "install_subtargets" || suffix == "uninstall_subtargets")
                && config.indexOf("no_default_install") != -1) {
                continue;
            }
            QString targetRule = subTarget->target + "-" + suffix;
            if(flags & SubTargetOrdered)
                targetRule += "-ordered";
            t << " " << targetRule;
        }
        if(suffix == "all" || suffix == "make_first")
            t << ' ' << depVar("ALL_DEPS");
        if(suffix == "clean")
            t << ' ' << depVar("CLEAN_DEPS");
        else if (suffix == "distclean")
            t << ' ' << depVar("DISTCLEAN_DEPS");
        t << " FORCE\n";
        if(suffix == "clean") {
            t << fixFileVarGlue("QMAKE_CLEAN", "\t-$(DEL_FILE) ", "\n\t-$(DEL_FILE) ", "\n");
        } else if(suffix == "distclean") {
            QString ofile = fileFixify(Option::output.fileName());
            if(!ofile.isEmpty())
                t << "\t-$(DEL_FILE) " << escapeFilePath(ofile) << Qt::endl;
            t << fixFileVarGlue("QMAKE_DISTCLEAN", "\t-$(DEL_FILE) ", " ", "\n");
        }
    }

    // user defined targets
    const ProStringList &qut = project->values("QMAKE_EXTRA_TARGETS");
    for (ProStringList::ConstIterator qut_it = qut.begin(); qut_it != qut.end(); ++qut_it) {
        const ProStringList &config = project->values(ProKey(*qut_it + ".CONFIG"));
        QString targ = var(ProKey(*qut_it + ".target")),
                 cmd = var(ProKey(*qut_it + ".commands")), deps;
        if(targ.isEmpty())
            targ = (*qut_it).toQString();
        t << Qt::endl;

        const ProStringList &deplist = project->values(ProKey(*qut_it + ".depends"));
        for (ProStringList::ConstIterator dep_it = deplist.begin(); dep_it != deplist.end(); ++dep_it) {
            QString dep = var(ProKey(*dep_it + ".target"));
            if(dep.isEmpty())
                dep = (*dep_it).toQString();
            deps += ' ' + escapeDependencyPath(Option::fixPathToTargetOS(dep, false));
        }
        if (config.indexOf("recursive") != -1) {
            QSet<QString> recurse;
            const ProKey rkey(*qut_it + ".recurse");
            if (project->isSet(rkey)) {
                const QStringList values = project->values(rkey).toQStringList();
                recurse = QSet<QString>(values.begin(), values.end());
            } else {
                for(int target = 0; target < targets.size(); ++target)
                    recurse.insert(targets.at(target)->name);
            }
            for(int target = 0; target < targets.size(); ++target) {
                SubTarget *subtarget = targets.at(target);
                QString in_directory = subtarget->in_directory;
                if(!in_directory.isEmpty() && !in_directory.endsWith(Option::dir_sep))
                    in_directory += Option::dir_sep;
                QString out_directory = subtarget->out_directory;
                if(!out_directory.isEmpty() && !out_directory.endsWith(Option::dir_sep))
                    out_directory += Option::dir_sep;
                if(!abs_source_path.isEmpty() && out_directory.startsWith(abs_source_path))
                    out_directory = Option::output_dir + out_directory.mid(abs_source_path.length());

                if(!recurse.contains(subtarget->name))
                    continue;

                QString out_directory_cdin = out_directory.isEmpty() ? "\n\t"
                                                                     : "\n\tcd " + escapeFilePath(out_directory) + " && ";
                QString makefilein = " -f " + escapeFilePath(subtarget->makefile);

                QString out;
                QString in;
                if (!subtarget->profile.isEmpty()) {
                    out = subtarget->makefile;
                    in = escapeFilePath(fileFixify(in_directory + subtarget->profile, FileFixifyAbsolute));
                    if (out.startsWith(in_directory))
                        out = out.mid(in_directory.length());
                    out = escapeFilePath(out);
                }

                //write the rule/depends
                if(flags & SubTargetOrdered) {
                    const QString dep = subtarget->target + "-" + (*qut_it) + "_ordered";
                    t << dep << ":";
                    if(target)
                        t << " " << targets.at(target-1)->target << "-" << (*qut_it) << "_ordered ";
                    deps += " " + dep;
                } else {
                    const QString dep = subtarget->target + "-" + (*qut_it);
                    t << dep << ":";
                    if(!subtarget->depends.isEmpty())
                        t << " " << valGlue(subtarget->depends, QString(), "-" + (*qut_it) + " ", "-" + (*qut_it));
                    deps += " " + dep;
                }

                QString sub_targ = targ;
                const ProKey rtkey(*qut_it + ".recurse_target");
                if (project->isSet(rtkey))
                    sub_targ = project->first(rtkey).toQString();

                //write the commands
                writeSubTargetCall(t, in_directory, in, out_directory, out,
                                   out_directory_cdin, makefilein + " " + sub_targ);
            }
        }
        if (config.indexOf("phony") != -1)
            deps += " FORCE";
        t << escapeDependencyPath(Option::fixPathToTargetOS(targ, false)) << ":" << deps << "\n";
        if(!cmd.isEmpty())
            t << "\t" << cmd << Qt::endl;
    }

    if(flags & SubTargetInstalls) {
        project->values("INSTALLDEPS")   += "install_subtargets";
        project->values("UNINSTALLDEPS") += "uninstall_subtargets";
        writeInstalls(t, true);
    }
    t << "FORCE:\n\n";
}

void
MakefileGenerator::writeMakeQmake(QTextStream &t, bool noDummyQmakeAll)
{
    QString ofile = fileFixify(Option::output.fileName());
    if(project->isEmpty("QMAKE_FAILED_REQUIREMENTS") && !project->isEmpty("QMAKE_INTERNAL_PRL_FILE")) {
        QStringList files = escapeFilePaths(fileFixify(Option::mkfile::project_files));
        t << escapeDependencyPath(project->first("QMAKE_INTERNAL_PRL_FILE").toQString()) << ": \n\t"
          << "@$(QMAKE) -prl " << files.join(' ') << ' ' << buildArgs(true) << Qt::endl;
    }

        QString qmake = "$(QMAKE)" + fullBuildArgs();
        if(!ofile.isEmpty() && !project->isActiveConfig("no_autoqmake")) {
            t << escapeDependencyPath(ofile) << ": "
              << escapeDependencyPath(fileFixify(project->projectFile())) << " ";
            if (Option::globals->do_cache) {
                if (!project->confFile().isEmpty())
                    t <<  escapeDependencyPath(fileFixify(project->confFile())) << " ";
                if (!project->cacheFile().isEmpty())
                    t <<  escapeDependencyPath(fileFixify(project->cacheFile())) << " ";
            }
            if(!specdir().isEmpty()) {
                if (exists(Option::normalizePath(specdir() + "/qmake.conf")))
                    t << escapeDependencyPath(specdir() + Option::dir_sep + "qmake.conf") << " ";
            }
            const ProStringList &included = escapeDependencyPaths(project->values("QMAKE_INTERNAL_INCLUDED_FILES"));
            t << included.join(QString(" \\\n\t\t")) << "\n\t"
              << qmake << Qt::endl;
            const ProStringList &extraCommands = project->values("QMAKE_MAKE_QMAKE_EXTRA_COMMANDS");
            if (!extraCommands.isEmpty())
                t << "\t" << extraCommands.join(QString("\n\t")) << Qt::endl;
            for(int include = 0; include < included.size(); ++include) {
                const ProString &i = included.at(include);
                if(!i.isEmpty())
                    t << i << ":\n";
            }
        }
        if(project->first("QMAKE_ORIG_TARGET") != "qmake") {
            t << "qmake: FORCE\n\t@" << qmake << Qt::endl << Qt::endl;
            if (!noDummyQmakeAll)
                t << "qmake_all: FORCE\n\n";
        }
}

QFileInfo
MakefileGenerator::fileInfo(QString file) const
{
    static QHash<FileInfoCacheKey, QFileInfo> *cache = nullptr;
    static QFileInfo noInfo = QFileInfo();
    if(!cache) {
        cache = new QHash<FileInfoCacheKey, QFileInfo>;
        qmakeAddCacheClear(qmakeDeleteCacheClear<QHash<FileInfoCacheKey, QFileInfo> >, (void**)&cache);
    }
    FileInfoCacheKey cacheKey(file);
    QFileInfo value = cache->value(cacheKey, noInfo);
    if (value != noInfo)
        return value;

    QFileInfo fi(file);
    if (fi.exists())
        cache->insert(cacheKey, fi);
    return fi;
}

MakefileGenerator::LibFlagType
MakefileGenerator::parseLibFlag(const ProString &flag, ProString *arg)
{
    if (flag.startsWith("-L")) {
        *arg = flag.mid(2);
        return LibFlagPath;
    }
    if (flag.startsWith("-l")) {
        *arg = flag.mid(2);
        return LibFlagLib;
    }
    if (flag.startsWith('-'))
        return LibFlagOther;
    return LibFlagFile;
}

ProStringList
MakefileGenerator::fixLibFlags(const ProKey &var)
{
    const ProStringList &in = project->values(var);
    ProStringList ret;

    ret.reserve(in.length());
    for (const ProString &v : in)
        ret << fixLibFlag(v);
    return ret;
}

ProString MakefileGenerator::fixLibFlag(const ProString &)
{
    qFatal("MakefileGenerator::fixLibFlag() called");
    return ProString();
}

ProString
MakefileGenerator::escapeFilePath(const ProString &path) const
{
    return ProString(escapeFilePath(path.toQString()));
}

QStringList
MakefileGenerator::escapeFilePaths(const QStringList &paths) const
{
    QStringList ret;
    for(int i = 0; i < paths.size(); ++i)
        ret.append(escapeFilePath(paths.at(i)));
    return ret;
}

ProStringList
MakefileGenerator::escapeFilePaths(const ProStringList &paths) const
{
    ProStringList ret;
    const int size = paths.size();
    ret.reserve(size);
    for (int i = 0; i < size; ++i)
        ret.append(escapeFilePath(paths.at(i)));
    return ret;
}

QString
MakefileGenerator::escapeDependencyPath(const QString &path) const
{
    QString ret = path;
    if (!ret.isEmpty()) {
        // Unix make semantics, to be inherited by unix and mingw generators.
#ifdef Q_OS_UNIX
        // When running on Unix, we need to escape colons (which may appear
        // anywhere in a path, and would be mis-parsed as dependency separators).
        static const QRegExp criticalChars(QStringLiteral("([\t :#])"));
#else
        // MinGW make has a hack for colons which denote drive letters, and no
        // other colons may appear in paths. And escaping colons actually breaks
        // the make from the Android SDK.
        static const QRegExp criticalChars(QStringLiteral("([\t #])"));
#endif
        ret.replace(criticalChars, QStringLiteral("\\\\1"));
        ret.replace(QLatin1Char('='), QStringLiteral("$(EQ)"));
        debug_msg(2, "escapeDependencyPath: %s -> %s", path.toLatin1().constData(), ret.toLatin1().constData());
    }
    return ret;
}

ProString
MakefileGenerator::escapeDependencyPath(const ProString &path) const
{
    return ProString(escapeDependencyPath(path.toQString()));
}

QStringList
MakefileGenerator::escapeDependencyPaths(const QStringList &paths) const
{
    QStringList ret;
    const int size = paths.size();
    ret.reserve(size);
    for (int i = 0; i < size; ++i)
        ret.append(escapeDependencyPath(paths.at(i)));
    return ret;
}

ProStringList
MakefileGenerator::escapeDependencyPaths(const ProStringList &paths) const
{
    ProStringList ret;
    const int size = paths.size();
    ret.reserve(size);
    for (int i = 0; i < size; ++i)
        ret.append(escapeDependencyPath(paths.at(i).toQString()));
    return ret;
}

QStringList
MakefileGenerator::finalizeDependencyPaths(const QStringList &paths) const
{
    QStringList ret;
    const int size = paths.size();
    ret.reserve(size);
    for (int i = 0; i < size; ++i)
        ret.append(escapeDependencyPath(Option::fixPathToTargetOS(paths.at(i), false)));
    return ret;
}

QStringList
MakefileGenerator::fileFixify(const QStringList &files, FileFixifyTypes fix, bool canon) const
{
    if(files.isEmpty())
        return files;
    QStringList ret;
    for(QStringList::ConstIterator it = files.begin(); it != files.end(); ++it) {
        if(!(*it).isEmpty())
            ret << fileFixify((*it), fix, canon);
    }
    return ret;
}

QString
MakefileGenerator::fileFixify(const QString &file, FileFixifyTypes fix, bool canon) const
{
    if(file.isEmpty())
        return file;
    QString ret = file;

    //do the fixin'
    QString orig_file = ret;
    if(ret.startsWith(QLatin1Char('~'))) {
        if(ret.startsWith(QLatin1String("~/")))
            ret = QDir::homePath() + ret.mid(1);
        else
            warn_msg(WarnLogic, "Unable to expand ~ in %s", ret.toLatin1().constData());
    }
    if ((fix & FileFixifyAbsolute)
            || (!(fix & FileFixifyRelative) && project->isActiveConfig("no_fixpath"))) {
        if ((fix & FileFixifyAbsolute) && QDir::isRelativePath(ret)) {
            QString pwd = !(fix & FileFixifyFromOutdir) ? project->projectDir() : Option::output_dir;
            {
                QFileInfo in_fi(fileInfo(pwd));
                if (in_fi.exists())
                    pwd = in_fi.canonicalFilePath();
            }
            if (!pwd.endsWith(QLatin1Char('/')))
                pwd += QLatin1Char('/');
            ret.prepend(pwd);
        }
        ret = Option::fixPathToTargetOS(ret, false, canon);
    } else { //fix it..
        QString out_dir = (fix & FileFixifyToIndir) ? project->projectDir() : Option::output_dir;
        QString in_dir  = !(fix & FileFixifyFromOutdir) ? project->projectDir() : Option::output_dir;
        {
            QFileInfo in_fi(fileInfo(in_dir));
            if(in_fi.exists())
                in_dir = in_fi.canonicalFilePath();
            QFileInfo out_fi(fileInfo(out_dir));
            if(out_fi.exists())
                out_dir = out_fi.canonicalFilePath();
        }

        QString qfile(Option::normalizePath(ret));
        QFileInfo qfileinfo(fileInfo(qfile));
        if(out_dir != in_dir || !qfileinfo.isRelative()) {
            if(qfileinfo.isRelative()) {
                ret = in_dir + "/" + qfile;
                qfileinfo.setFile(ret);
            }
            ret = Option::fixPathToTargetOS(ret, false, canon);
            QString match_dir = Option::fixPathToTargetOS(out_dir, false, canon);
            if(ret == match_dir) {
                ret = "";
            } else if(ret.startsWith(match_dir + Option::dir_sep)) {
                ret = ret.mid(match_dir.length() + Option::dir_sep.length());
            } else {
                //figure out the depth
                int depth = 4;
                if(Option::qmake_mode == Option::QMAKE_GENERATE_MAKEFILE ||
                   Option::qmake_mode == Option::QMAKE_GENERATE_PRL) {
                    if(project && !project->isEmpty("QMAKE_PROJECT_DEPTH"))
                        depth = project->first("QMAKE_PROJECT_DEPTH").toInt();
                    else if(Option::mkfile::cachefile_depth != -1)
                        depth = Option::mkfile::cachefile_depth;
                }
                //calculate how much can be removed
                QString dot_prefix;
                for(int i = 1; i <= depth; i++) {
                    int sl = match_dir.lastIndexOf(Option::dir_sep);
                    if(sl == -1)
                        break;
                    match_dir = match_dir.left(sl);
                    if(match_dir.isEmpty())
                        break;
                    if(ret.startsWith(match_dir + Option::dir_sep)) {
                        //concat
                        int remlen = ret.length() - (match_dir.length() + 1);
                        if(remlen < 0)
                            remlen = 0;
                        ret = ret.right(remlen);
                        //prepend
                        for(int o = 0; o < i; o++)
                            dot_prefix += ".." + Option::dir_sep;
                        break;
                    }
                }
                ret.prepend(dot_prefix);
            }
        } else {
            ret = Option::fixPathToTargetOS(ret, false, canon);
        }
    }
    if(ret.isEmpty())
        ret = ".";
    debug_msg(3, "Fixed[%d,%d] %s :: to :: %s [%s::%s]",
              int(fix), canon, orig_file.toLatin1().constData(), ret.toLatin1().constData(),
              qmake_getpwd().toLatin1().constData(), Option::output_dir.toLatin1().constData());
    return ret;
}

QMakeLocalFileName
MakefileGenerator::fixPathForFile(const QMakeLocalFileName &file, bool forOpen)
{
    if(forOpen)
        return QMakeLocalFileName(fileFixify(file.real(), FileFixifyBackwards));
    return QMakeLocalFileName(fileFixify(file.real()));
}

QFileInfo
MakefileGenerator::findFileInfo(const QMakeLocalFileName &file)
{
    return fileInfo(file.local());
}

QMakeLocalFileName
MakefileGenerator::findFileForDep(const QMakeLocalFileName &dep, const QMakeLocalFileName &file)
{
    QMakeLocalFileName ret;
    if(!project->isEmpty("SKIP_DEPENDS")) {
        bool found = false;
        const ProStringList &nodeplist = project->values("SKIP_DEPENDS");
        for (ProStringList::ConstIterator it = nodeplist.begin();
            it != nodeplist.end(); ++it) {
            QRegExp regx((*it).toQString());
            if(regx.indexIn(dep.local()) != -1) {
                found = true;
                break;
            }
        }
        if(found)
            return ret;
    }

    ret = QMakeSourceFileInfo::findFileForDep(dep, file);
    if(!ret.isNull())
        return ret;

    //these are some "hacky" heuristics it will try to do on an include
    //however these can be turned off at runtime, I'm not sure how
    //reliable these will be, most likely when problems arise turn it off
    //and see if they go away..
    if(Option::mkfile::do_dep_heuristics) {
        if(depHeuristicsCache.contains(dep.real()))
            return depHeuristicsCache[dep.real()];

        if(Option::output_dir != qmake_getpwd()
           && QDir::isRelativePath(dep.real())) { //is it from the shadow tree
            QVector<QMakeLocalFileName> depdirs = QMakeSourceFileInfo::dependencyPaths();
            depdirs.prepend(fileInfo(file.real()).absoluteDir().path());
            QString pwd = qmake_getpwd();
            if(pwd.at(pwd.length()-1) != '/')
                pwd += '/';
            for(int i = 0; i < depdirs.count(); i++) {
                QString dir = depdirs.at(i).real();
                if(!QDir::isRelativePath(dir) && dir.startsWith(pwd))
                    dir = dir.mid(pwd.length());
                if(QDir::isRelativePath(dir)) {
                    if(!dir.endsWith(Option::dir_sep))
                        dir += Option::dir_sep;
                    QString shadow = fileFixify(dir + dep.local(), FileFixifyBackwards);
                    if(exists(shadow)) {
                        ret = QMakeLocalFileName(shadow);
                        goto found_dep_from_heuristic;
                    }
                }
            }
        }
        { //is it from an EXTRA_TARGET
            const QString dep_basename = dep.local().section('/', -1);
            const ProStringList &qut = project->values("QMAKE_EXTRA_TARGETS");
            for (ProStringList::ConstIterator it = qut.begin(); it != qut.end(); ++it) {
                QString targ = var(ProKey(*it + ".target"));
                if(targ.isEmpty())
                    targ = (*it).toQString();
                QString out = Option::fixPathToTargetOS(targ);
                if(out == dep.real() || out.section(Option::dir_sep, -1) == dep_basename) {
                    ret = QMakeLocalFileName(out);
                    goto found_dep_from_heuristic;
                }
            }
        }
        { //is it from an EXTRA_COMPILER
            const QString dep_basename = dep.local().section('/', -1);
            const ProStringList &quc = project->values("QMAKE_EXTRA_COMPILERS");
            for (ProStringList::ConstIterator it = quc.begin(); it != quc.end(); ++it) {
                const ProString &tmp_out = project->first(ProKey(*it + ".output"));
                if(tmp_out.isEmpty())
                    continue;
                const ProStringList &tmp = project->values(ProKey(*it + ".input"));
                for (ProStringList::ConstIterator it2 = tmp.begin(); it2 != tmp.end(); ++it2) {
                    const ProStringList &inputs = project->values((*it2).toKey());
                    for (ProStringList::ConstIterator input = inputs.begin(); input != inputs.end(); ++input) {
                        QString out = Option::fixPathToTargetOS(
                                replaceExtraCompilerVariables(tmp_out.toQString(), (*input).toQString(), QString(), NoShell));
                        if (out == dep.real() || out.section(Option::dir_sep, -1) == dep_basename) {
                            ret = QMakeLocalFileName(fileFixify(out, FileFixifyBackwards));
                            goto found_dep_from_heuristic;
                        }
                    }
                }
            }
        }
    found_dep_from_heuristic:
        depHeuristicsCache.insert(dep.real(), ret);
    }
    return ret;
}

QStringList
&MakefileGenerator::findDependencies(const QString &file)
{
    const QString fixedFile = fileFixify(file);
    if(!dependsCache.contains(fixedFile)) {
#if 1
        QStringList deps = QMakeSourceFileInfo::dependencies(file);
        if(file != fixedFile)
            deps += QMakeSourceFileInfo::dependencies(fixedFile);
#else
        QStringList deps = QMakeSourceFileInfo::dependencies(fixedFile);
#endif
        dependsCache.insert(fixedFile, deps);
    }
    return dependsCache[fixedFile];
}

QString
MakefileGenerator::specdir()
{
    if (spec.isEmpty())
        spec = fileFixify(project->specDir());
    return spec;
}

bool
MakefileGenerator::openOutput(QFile &file, const QString &build) const
{
    debug_msg(3, "asked to open output file '%s' in %s",
        qPrintable(file.fileName()), qPrintable(Option::output_dir));

    if (file.fileName().isEmpty()) {
        file.setFileName(!project->isEmpty("MAKEFILE")
                         ? project->first("MAKEFILE").toQString() : "Makefile");
    }

    file.setFileName(QDir(Option::output_dir).absoluteFilePath(file.fileName()));

    if (!build.isEmpty())
        file.setFileName(file.fileName() + "." + build);

    if (project->isEmpty("QMAKE_MAKEFILE"))
        project->values("QMAKE_MAKEFILE").append(file.fileName());

    // Make required directories. Note that we do this based on the
    // filename, not Option::output_dir, as the filename may include
    // generator specific directories not included in output_dir.
    int slsh = file.fileName().lastIndexOf('/');
    if (slsh != -1)
        mkdir(file.fileName().left(slsh));

    debug_msg(3, "opening output file %s", qPrintable(file.fileName()));
    return file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);
}

QString
MakefileGenerator::pkgConfigFileName(bool fixify)
{
    QString ret = project->first("QMAKE_PKGCONFIG_FILE").toQString();
    if (ret.isEmpty()) {
        ret = project->first("TARGET").toQString();
        int slsh = ret.lastIndexOf(Option::dir_sep);
        if (slsh != -1)
            ret = ret.right(ret.length() - slsh - 1);
        if (ret.startsWith("lib"))
            ret = ret.mid(3);
        int dot = ret.indexOf('.');
        if (dot != -1)
            ret = ret.left(dot);
    }
    ret += Option::pkgcfg_ext;
    QString subdir = project->first("QMAKE_PKGCONFIG_DESTDIR").toQString();
    if(!subdir.isEmpty()) {
        // initOutPaths() appends dir_sep, but just to be safe..
        if (!subdir.endsWith(Option::dir_sep))
            ret.prepend(Option::dir_sep);
        ret.prepend(subdir);
    }
    if(fixify) {
        if(QDir::isRelativePath(ret) && !project->isEmpty("DESTDIR"))
            ret.prepend(project->first("DESTDIR").toQString());
        ret = fileFixify(ret, FileFixifyBackwards);
    }
    return ret;
}

QString
MakefileGenerator::pkgConfigPrefix() const
{
    if(!project->isEmpty("QMAKE_PKGCONFIG_PREFIX"))
        return project->first("QMAKE_PKGCONFIG_PREFIX").toQString();
    return project->propertyValue(ProKey("QT_INSTALL_PREFIX")).toQString();
}

QString
MakefileGenerator::pkgConfigFixPath(QString path) const
{
    QString prefix = pkgConfigPrefix();
    if(path.startsWith(prefix))
        path.replace(prefix, QLatin1String("${prefix}"));
    return path;
}

void
MakefileGenerator::writePkgConfigFile()
{
    QString fname = pkgConfigFileName();
    mkdir(fileInfo(fname).path());
    QFile ft(fname);
    if(!ft.open(QIODevice::WriteOnly))
        return;
    QString ffname(fileFixify(fname));
    project->values("ALL_DEPS").append(ffname);
    project->values("QMAKE_DISTCLEAN").append(ffname);
    QTextStream t(&ft);

    QString prefix = pkgConfigPrefix();
    QString libDir = project->first("QMAKE_PKGCONFIG_LIBDIR").toQString();
    if(libDir.isEmpty())
        libDir = prefix + "/lib";
    QString includeDir = project->first("QMAKE_PKGCONFIG_INCDIR").toQString();
    if(includeDir.isEmpty())
        includeDir = prefix + "/include";

    t << "prefix=" << prefix << Qt::endl;
    t << "exec_prefix=${prefix}\n"
      << "libdir=" << pkgConfigFixPath(libDir) << "\n"
      << "includedir=" << pkgConfigFixPath(includeDir) << Qt::endl;
    t << Qt::endl;

    //extra PKGCONFIG variables
    const ProStringList &pkgconfig_vars = project->values("QMAKE_PKGCONFIG_VARIABLES");
    for(int i = 0; i < pkgconfig_vars.size(); ++i) {
        const ProString &var = project->first(ProKey(pkgconfig_vars.at(i) + ".name"));
        QString val = project->values(ProKey(pkgconfig_vars.at(i) + ".value")).join(' ');
        if(var.isEmpty())
            continue;
        if(val.isEmpty()) {
            const ProStringList &var_vars = project->values(ProKey(pkgconfig_vars.at(i) + ".variable"));
            for(int v = 0; v < var_vars.size(); ++v) {
                const ProStringList &vars = project->values(var_vars.at(v).toKey());
                for(int var = 0; var < vars.size(); ++var) {
                    if(!val.isEmpty())
                        val += " ";
                    val += pkgConfigFixPath(vars.at(var).toQString());
                }
            }
        }
        if (!val.isEmpty())
            t << var << "=" << val << Qt::endl;
    }

    t << Qt::endl;

    QString name = project->first("QMAKE_PKGCONFIG_NAME").toQString();
    if(name.isEmpty()) {
        name = project->first("QMAKE_ORIG_TARGET").toQString().toLower();
        name.replace(0, 1, name[0].toUpper());
    }
    t << "Name: " << name << Qt::endl;
    QString desc = project->values("QMAKE_PKGCONFIG_DESCRIPTION").join(' ');
    if(desc.isEmpty()) {
        if(name.isEmpty()) {
            desc = project->first("QMAKE_ORIG_TARGET").toQString().toLower();
            desc.replace(0, 1, desc[0].toUpper());
        } else {
            desc = name;
        }
        if(project->first("TEMPLATE") == "lib") {
            if(project->isActiveConfig("plugin"))
               desc += " Plugin";
            else
               desc += " Library";
        } else if(project->first("TEMPLATE") == "app") {
            desc += " Application";
        }
    }
    t << "Description: " << desc << Qt::endl;
    ProString version = project->first("QMAKE_PKGCONFIG_VERSION");
    if (version.isEmpty())
        version = project->first("VERSION");
    if (!version.isEmpty())
        t << "Version: " << version << Qt::endl;

    if (project->first("TEMPLATE") == "lib") {
        // libs
        t << "Libs: ";
        QString pkgConfiglibName;
        if (target_mode == TARG_MAC_MODE && project->isActiveConfig("lib_bundle")) {
            if (libDir != QLatin1String("/Library/Frameworks"))
                t << "-F${libdir} ";
            ProString bundle;
            if (!project->isEmpty("QMAKE_FRAMEWORK_BUNDLE_NAME"))
                bundle = project->first("QMAKE_FRAMEWORK_BUNDLE_NAME");
            else
                bundle = project->first("TARGET");
            int suffix = bundle.lastIndexOf(".framework");
            if (suffix != -1)
                bundle = bundle.left(suffix);
            t << "-framework ";
            pkgConfiglibName = bundle.toQString();
        } else {
            if (!project->values("QMAKE_DEFAULT_LIBDIRS").contains(libDir))
                t << "-L${libdir} ";
            pkgConfiglibName = "-l" + project->first("QMAKE_ORIG_TARGET");
            if (project->isActiveConfig("shared"))
                pkgConfiglibName += project->first("TARGET_VERSION_EXT").toQString();
        }
        t << shellQuote(pkgConfiglibName) << " \n";

        if (project->isActiveConfig("staticlib")) {
            ProStringList libs;
            libs << "LIBS";  // FIXME: this should not be conditional on staticlib
            libs << "LIBS_PRIVATE";
            libs << "QMAKE_LIBS";  // FIXME: this should not be conditional on staticlib
            libs << "QMAKE_LIBS_PRIVATE";
            libs << "QMAKE_LFLAGS_THREAD"; //not sure about this one, but what about things like -pthread?
            t << "Libs.private:";
            for (ProStringList::ConstIterator it = libs.cbegin(); it != libs.cend(); ++it)
                t << ' ' << fixLibFlags((*it).toKey()).join(' ');
            t << Qt::endl;
        }
    }

    // flags
    // ### too many
    t << "Cflags: "
        // << var("QMAKE_CXXFLAGS") << " "
      << varGlue("PRL_EXPORT_DEFINES","-D"," -D"," ")
      << varGlue("PRL_EXPORT_CXXFLAGS", "", " ", " ")
      << varGlue("QMAKE_PKGCONFIG_CFLAGS", "", " ", " ")
        //      << varGlue("DEFINES","-D"," -D"," ")
         ;
    if (!project->values("QMAKE_DEFAULT_INCDIRS").contains(includeDir))
        t << "-I${includedir}";
    if (target_mode == TARG_MAC_MODE && project->isActiveConfig("lib_bundle")
        && libDir != QLatin1String("/Library/Frameworks")) {
            t << " -F${libdir}";
    }
    t << Qt::endl;

    // requires
    const QString requiresString = project->values("QMAKE_PKGCONFIG_REQUIRES").join(' ');
    if (!requiresString.isEmpty()) {
        t << "Requires: " << requiresString << Qt::endl;
    }

    t << Qt::endl;
}

static QString windowsifyPath(const QString &str)
{
    // The paths are escaped in prl files, so every slash needs to turn into two backslashes.
    // Then each backslash needs to be escaped for sed. And another level for C quoting here.
    return QString(str).replace('/', QLatin1String("\\\\\\\\"));
}

QString MakefileGenerator::installMetaFile(const ProKey &replace_rule, const QString &src, const QString &dst)
{
    QString ret;
    if (project->isEmpty(replace_rule)
        || project->isActiveConfig("no_sed_meta_install")) {
        ret += "$(INSTALL_FILE) " + escapeFilePath(src) + ' ' + escapeFilePath(dst);
    } else {
        QString sedargs;
        const ProStringList &replace_rules = project->values(replace_rule);
        for (int r = 0; r < replace_rules.size(); ++r) {
            const ProString match = project->first(ProKey(replace_rules.at(r) + ".match")),
                        replace = project->first(ProKey(replace_rules.at(r) + ".replace"));
            if (!match.isEmpty() /*&& match != replace*/) {
                sedargs += " -e " + shellQuote("s," + match + "," + replace + ",g");
                if (isWindowsShell() && project->first(ProKey(replace_rules.at(r) + ".CONFIG")).contains("path"))
                    sedargs += " -e " + shellQuote("s," + windowsifyPath(match.toQString())
                                               + "," + windowsifyPath(replace.toQString()) + ",gi");
            }
        }
        if (sedargs.isEmpty()) {
            ret += "$(INSTALL_FILE) " + escapeFilePath(src) + ' ' + escapeFilePath(dst);
        } else {
            ret += "$(SED) " + sedargs + ' ' + escapeFilePath(src) + " > " + escapeFilePath(dst);
        }
    }
    return ret;
}

QString MakefileGenerator::shellQuote(const QString &str)
{
    return isWindowsShell() ? IoUtils::shellQuoteWin(str) : IoUtils::shellQuoteUnix(str);
}

/*
 * Returns the name of the variable that contains the fully resolved target
 * (including DESTDIR) of this generator.
 */
ProKey MakefileGenerator::fullTargetVariable() const
{
    return "TARGET";
}

void MakefileGenerator::createResponseFile(const QString &fileName, const ProStringList &objList)
{
    QString filePath = Option::output_dir + QDir::separator() + fileName;
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream t(&file);
        for (ProStringList::ConstIterator it = objList.constBegin(); it != objList.constEnd(); ++it) {
            QString path = (*it).toQString();
            // In response files, whitespace and special characters are
            // escaped with a backslash; backslashes themselves can either
            // be escaped into double backslashes, or, as this is a list of
            // path names, converted to forward slashes.
            path.replace(QLatin1Char('\\'), QLatin1String("/"))
                .replace(QLatin1Char(' '), QLatin1String("\\ "))
                .replace(QLatin1Char('\t'), QLatin1String("\\\t"))
                .replace(QLatin1Char('"'), QLatin1String("\\\""))
                .replace(QLatin1Char('\''), QLatin1String("\\'"));
            t << path << Qt::endl;
        }
        t.flush();
        file.close();
    }
}

QT_END_NAMESPACE
