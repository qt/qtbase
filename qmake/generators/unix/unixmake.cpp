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

#include "unixmake.h"
#include "option.h"
#include <qregexp.h>
#include <qfile.h>
#include <qhash.h>
#include <qdir.h>
#include <time.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

void
UnixMakefileGenerator::init()
{
    ProStringList &configs = project->values("CONFIG");
    if(project->isEmpty("ICON") && !project->isEmpty("RC_FILE"))
        project->values("ICON") = project->values("RC_FILE");
    if(project->isEmpty("QMAKE_EXTENSION_PLUGIN"))
        project->values("QMAKE_EXTENSION_PLUGIN").append(project->first("QMAKE_EXTENSION_SHLIB"));

    project->values("QMAKE_ORIG_TARGET") = project->values("TARGET");

    //version handling
    if (project->isEmpty("VERSION")) {
        project->values("VERSION").append(
            "1.0." + (project->isEmpty("VER_PAT") ? QString("0") : project->first("VER_PAT")));
    }
    QStringList l = project->first("VERSION").toQString().split('.');
    l << "0" << "0"; //make sure there are three
    project->values("VER_MAJ").append(l[0]);
    project->values("VER_MIN").append(l[1]);
    project->values("VER_PAT").append(l[2]);

    QString sroot = project->sourceRoot();
    for (const ProString &iif : project->values("QMAKE_INTERNAL_INCLUDED_FILES")) {
        if (iif == project->cacheFile())
            continue;
        if (iif.startsWith(sroot) && iif.at(sroot.length()) == QLatin1Char('/'))
            project->values("DISTFILES") += fileFixify(iif.toQString(), FileFixifyRelative);
    }

    /* this should probably not be here, but I'm using it to wrap the .t files */
    if(project->first("TEMPLATE") == "app")
        project->values("QMAKE_APP_FLAG").append("1");
    else if(project->first("TEMPLATE") == "lib")
        project->values("QMAKE_LIB_FLAG").append("1");
    else if(project->first("TEMPLATE") == "subdirs") {
        MakefileGenerator::init();
        if(project->isEmpty("MAKEFILE"))
            project->values("MAKEFILE").append("Makefile");
        return; /* subdirs is done */
    }

    project->values("QMAKE_ORIG_DESTDIR") = project->values("DESTDIR");
    project->values("QMAKE_LIBS") += project->values("LIBS");
    project->values("QMAKE_LIBS_PRIVATE") += project->values("LIBS_PRIVATE");
    if((!project->isEmpty("QMAKE_LIB_FLAG") && !project->isActiveConfig("staticlib")) ||
       (project->isActiveConfig("qt") &&  project->isActiveConfig("plugin"))) {
        if(configs.indexOf("dll") == -1) configs.append("dll");
    } else if(!project->isEmpty("QMAKE_APP_FLAG") || project->isActiveConfig("dll")) {
        configs.removeAll("staticlib");
    }
    if(!project->isEmpty("QMAKE_INCREMENTAL"))
        project->values("QMAKE_LFLAGS") += project->values("QMAKE_LFLAGS_INCREMENTAL");
    else if(!project->isEmpty("QMAKE_LFLAGS_PREBIND") &&
            !project->values("QMAKE_LIB_FLAG").isEmpty() &&
            project->isActiveConfig("dll"))
        project->values("QMAKE_LFLAGS") += project->values("QMAKE_LFLAGS_PREBIND");
    if(!project->isEmpty("QMAKE_INCDIR"))
        project->values("INCLUDEPATH") += project->values("QMAKE_INCDIR");
    ProStringList ldadd;
    if(!project->isEmpty("QMAKE_LIBDIR")) {
        const ProStringList &libdirs = project->values("QMAKE_LIBDIR");
        for(int i = 0; i < libdirs.size(); ++i) {
            if(!project->isEmpty("QMAKE_LFLAGS_RPATH") && project->isActiveConfig("rpath_libdirs"))
                project->values("QMAKE_LFLAGS") += var("QMAKE_LFLAGS_RPATH") + libdirs[i];
            project->values("QMAKE_LIBDIR_FLAGS") += "-L" + escapeFilePath(libdirs[i]);
        }
    }
    ldadd += project->values("QMAKE_LIBDIR_FLAGS");
    if (project->isActiveConfig("mac")) {
        if (!project->isEmpty("QMAKE_FRAMEWORKPATH")) {
            const ProStringList &fwdirs = project->values("QMAKE_FRAMEWORKPATH");
            for (int i = 0; i < fwdirs.size(); ++i)
                project->values("QMAKE_FRAMEWORKPATH_FLAGS") += "-F" + escapeFilePath(fwdirs[i]);
        }
        ldadd += project->values("QMAKE_FRAMEWORKPATH_FLAGS");
    }
    ProStringList &qmklibs = project->values("QMAKE_LIBS");
    qmklibs = ldadd + qmklibs;
    if (!project->isEmpty("QMAKE_RPATHDIR") && !project->isEmpty("QMAKE_LFLAGS_RPATH")) {
        const ProStringList &rpathdirs = project->values("QMAKE_RPATHDIR");
        for (int i = 0; i < rpathdirs.size(); ++i) {
            QString rpathdir = rpathdirs[i].toQString();
            if (rpathdir.length() > 1 && rpathdir.at(0) == '$' && rpathdir.at(1) != '(') {
                rpathdir.replace(0, 1, "\\$$");  // Escape from make and the shell
            } else if (!rpathdir.startsWith('@') && fileInfo(rpathdir).isRelative()) {
                QString rpathbase = project->first("QMAKE_REL_RPATH_BASE").toQString();
                if (rpathbase.isEmpty()) {
                    fprintf(stderr, "Error: This platform does not support relative paths in QMAKE_RPATHDIR (%s)\n",
                                    rpathdir.toLatin1().constData());
                    continue;
                }
                if (rpathbase.startsWith('$'))
                    rpathbase.replace(0, 1, "\\$$");  // Escape from make and the shell
                if (rpathdir == ".")
                    rpathdir = rpathbase;
                else
                    rpathdir.prepend(rpathbase + '/');
                project->values("QMAKE_LFLAGS").insertUnique(project->values("QMAKE_LFLAGS_REL_RPATH"));
            }
            project->values("QMAKE_LFLAGS") += var("QMAKE_LFLAGS_RPATH") + escapeFilePath(rpathdir);
        }
    }
    if (!project->isEmpty("QMAKE_RPATHLINKDIR")) {
        const ProStringList &rpathdirs = project->values("QMAKE_RPATHLINKDIR");
        for (int i = 0; i < rpathdirs.size(); ++i) {
            if (!project->isEmpty("QMAKE_LFLAGS_RPATHLINK"))
                project->values("QMAKE_LFLAGS") += var("QMAKE_LFLAGS_RPATHLINK") + escapeFilePath(QFileInfo(rpathdirs[i].toQString()).absoluteFilePath());
        }
    }

    if(project->isActiveConfig("GNUmake") && !project->isEmpty("QMAKE_CFLAGS_DEPS"))
        include_deps = true; //do not generate deps

    MakefileGenerator::init();

    if (project->isActiveConfig("objective_c"))
        project->values("QMAKE_BUILTIN_COMPILERS") << "OBJC" << "OBJCXX";

    for (const ProString &compiler : project->values("QMAKE_BUILTIN_COMPILERS")) {
        QString compile_flag = var("QMAKE_COMPILE_FLAG");
        if(compile_flag.isEmpty())
            compile_flag = "-c";

        if(doPrecompiledHeaders() && !project->isEmpty("PRECOMPILED_HEADER")) {
            QString pchFlags = var(ProKey("QMAKE_" + compiler + "FLAGS_USE_PRECOMPILE"));

            QString pchBaseName;
            if(!project->isEmpty("PRECOMPILED_DIR")) {
                pchBaseName = Option::fixPathToTargetOS(project->first("PRECOMPILED_DIR").toQString());
                if(!pchBaseName.endsWith(Option::dir_sep))
                    pchBaseName += Option::dir_sep;
            }
            pchBaseName += project->first("QMAKE_ORIG_TARGET").toQString();

            // replace place holders
            pchFlags.replace(QLatin1String("${QMAKE_PCH_INPUT}"),
                             escapeFilePath(project->first("PRECOMPILED_HEADER").toQString()));
            pchFlags.replace(QLatin1String("${QMAKE_PCH_OUTPUT_BASE}"), escapeFilePath(pchBaseName));
            if (project->isActiveConfig("icc_pch_style")) {
                // icc style
                pchFlags.replace(QLatin1String("${QMAKE_PCH_OUTPUT}"),
                                 escapeFilePath(pchBaseName + project->first("QMAKE_PCH_OUTPUT_EXT")));
            } else {
                // gcc style (including clang_pch_style)
                QString headerSuffix;
                if (project->isActiveConfig("clang_pch_style"))
                    headerSuffix = project->first("QMAKE_PCH_OUTPUT_EXT").toQString();
                else
                    pchBaseName += project->first("QMAKE_PCH_OUTPUT_EXT").toQString();

                pchBaseName += Option::dir_sep;

                ProString language = project->first(ProKey("QMAKE_LANGUAGE_" + compiler));
                if (!language.isEmpty()) {
                    pchFlags.replace(QLatin1String("${QMAKE_PCH_OUTPUT}"),
                                     escapeFilePath(pchBaseName + language + headerSuffix));
                }
            }

            if (!pchFlags.isEmpty())
                compile_flag += " " + pchFlags;
        }

        QString compilerExecutable;
        if (compiler == "C" || compiler == "OBJC") {
            compilerExecutable = "$(CC)";
            compile_flag += " $(CFLAGS)";
        } else {
            compilerExecutable = "$(CXX)";
            compile_flag += " $(CXXFLAGS)";
        }

        compile_flag += " $(INCPATH)";

        ProString compilerVariable = compiler;
        if (compilerVariable == "C")
            compilerVariable = ProString("CC");

        const ProKey runComp("QMAKE_RUN_" + compilerVariable);
        if(project->isEmpty(runComp))
            project->values(runComp).append(compilerExecutable + " " + compile_flag + " " + var("QMAKE_CC_O_FLAG") + "$obj $src");
        const ProKey runCompImp("QMAKE_RUN_" + compilerVariable + "_IMP");
        if(project->isEmpty(runCompImp))
            project->values(runCompImp).append(compilerExecutable + " " + compile_flag + " " + var("QMAKE_CC_O_FLAG") + "\"$@\" \"$<\"");
    }

    if (project->isActiveConfig("mac") && !project->isEmpty("TARGET") &&
       ((project->isActiveConfig("build_pass") || project->isEmpty("BUILDS")))) {
        ProString bundle;
        if(project->isActiveConfig("bundle") && !project->isEmpty("QMAKE_BUNDLE_EXTENSION")) {
            bundle = project->first("TARGET");
            if(!project->isEmpty("QMAKE_BUNDLE_NAME"))
                bundle = project->first("QMAKE_BUNDLE_NAME");
            if(!bundle.endsWith(project->first("QMAKE_BUNDLE_EXTENSION")))
                bundle += project->first("QMAKE_BUNDLE_EXTENSION");
        } else if(project->first("TEMPLATE") == "app" && project->isActiveConfig("app_bundle")) {
            bundle = project->first("TARGET");
            if(!project->isEmpty("QMAKE_APPLICATION_BUNDLE_NAME"))
                bundle = project->first("QMAKE_APPLICATION_BUNDLE_NAME");
            if(!bundle.endsWith(".app"))
                bundle += ".app";
            if(project->isEmpty("QMAKE_BUNDLE_LOCATION"))
                project->values("QMAKE_BUNDLE_LOCATION").append("Contents/MacOS");
            project->values("QMAKE_PKGINFO").append(project->first("DESTDIR") + bundle + "/Contents/PkgInfo");
            project->values("QMAKE_BUNDLE_RESOURCE_FILE").append(project->first("DESTDIR") + bundle + "/Contents/Resources/empty.lproj");
        } else if(project->first("TEMPLATE") == "lib" && !project->isActiveConfig("staticlib") &&
                  ((!project->isActiveConfig("plugin") && project->isActiveConfig("lib_bundle")) ||
                   (project->isActiveConfig("plugin") && project->isActiveConfig("plugin_bundle")))) {
            bundle = project->first("TARGET");
            if(project->isActiveConfig("plugin")) {
                if(!project->isEmpty("QMAKE_PLUGIN_BUNDLE_NAME"))
                    bundle = project->first("QMAKE_PLUGIN_BUNDLE_NAME");
                if (project->isEmpty("QMAKE_BUNDLE_EXTENSION"))
                    project->values("QMAKE_BUNDLE_EXTENSION").append(".plugin");
                if (!bundle.endsWith(project->first("QMAKE_BUNDLE_EXTENSION")))
                    bundle += project->first("QMAKE_BUNDLE_EXTENSION");
                if(project->isEmpty("QMAKE_BUNDLE_LOCATION"))
                    project->values("QMAKE_BUNDLE_LOCATION").append("Contents/MacOS");
            } else {
                if(!project->isEmpty("QMAKE_FRAMEWORK_BUNDLE_NAME"))
                    bundle = project->first("QMAKE_FRAMEWORK_BUNDLE_NAME");
                if (project->isEmpty("QMAKE_BUNDLE_EXTENSION"))
                    project->values("QMAKE_BUNDLE_EXTENSION").append(".framework");
                if (!bundle.endsWith(project->first("QMAKE_BUNDLE_EXTENSION")))
                    bundle += project->first("QMAKE_BUNDLE_EXTENSION");
            }
        }
        if(!bundle.isEmpty()) {
            project->values("QMAKE_BUNDLE") = ProStringList(bundle);
        } else {
            project->values("QMAKE_BUNDLE").clear();
            project->values("QMAKE_BUNDLE_LOCATION").clear();
        }
    } else { //no bundling here
        project->values("QMAKE_BUNDLE").clear();
        project->values("QMAKE_BUNDLE_LOCATION").clear();
    }

    init2();
    project->values("QMAKE_INTERNAL_PRL_LIBS") << "QMAKE_LIBS";
    ProString target = project->first("TARGET");
    int slsh = target.lastIndexOf(Option::dir_sep);
    if (slsh != -1)
        target.chopFront(slsh + 1);
    project->values("LIB_TARGET").prepend(target);
    if(!project->isEmpty("QMAKE_MAX_FILES_PER_AR")) {
        bool ok;
        int max_files = project->first("QMAKE_MAX_FILES_PER_AR").toInt(&ok);
        ProStringList ar_sublibs, objs = project->values("OBJECTS");
        if(ok && max_files > 5 && max_files < (int)objs.count()) {
            QString lib;
            for(int i = 0, obj_cnt = 0, lib_cnt = 0; i != objs.size(); ++i) {
                if((++obj_cnt) >= max_files) {
                    if(lib_cnt) {
                        lib.sprintf("lib%s-tmp%d.a",
                                    project->first("QMAKE_ORIG_TARGET").toLatin1().constData(), lib_cnt);
                        ar_sublibs << lib;
                        obj_cnt = 0;
                    }
                    lib_cnt++;
                }
            }
        }
        if(!ar_sublibs.isEmpty()) {
            project->values("QMAKE_AR_SUBLIBS") = ar_sublibs;
            project->values("QMAKE_INTERNAL_PRL_LIBS") << "QMAKE_AR_SUBLIBS";
        }
    }
}

QStringList
&UnixMakefileGenerator::findDependencies(const QString &f)
{
    QStringList &ret = MakefileGenerator::findDependencies(f);
    if (doPrecompiledHeaders() && !project->isEmpty("PRECOMPILED_HEADER")) {
        ProString file = f;
        QString header_prefix;
        if(!project->isEmpty("PRECOMPILED_DIR"))
            header_prefix = project->first("PRECOMPILED_DIR").toQString();
        header_prefix += project->first("QMAKE_ORIG_TARGET").toQString();
        if (!project->isActiveConfig("clang_pch_style"))
            header_prefix += project->first("QMAKE_PCH_OUTPUT_EXT").toQString();
        if (project->isActiveConfig("icc_pch_style")) {
            // icc style
            for(QStringList::Iterator it = Option::cpp_ext.begin(); it != Option::cpp_ext.end(); ++it) {
                if(file.endsWith(*it)) {
                    ret += header_prefix;
                    break;
                }
            }
        } else {
            // gcc style (including clang_pch_style)
            QString header_suffix = project->isActiveConfig("clang_pch_style")
                    ? project->first("QMAKE_PCH_OUTPUT_EXT").toQString() : "";
            header_prefix += Option::dir_sep + project->first("QMAKE_PRECOMP_PREFIX");

            for (const ProString &compiler : project->values("QMAKE_BUILTIN_COMPILERS")) {
                if (project->isEmpty(ProKey("QMAKE_" + compiler + "FLAGS_PRECOMPILE")))
                    continue;

                ProString language = project->first(ProKey("QMAKE_LANGUAGE_" + compiler));
                if (language.isEmpty())
                    continue;

                // Unfortunately we were not consistent about the C++ naming
                ProString extensionSuffix = compiler;
                if (extensionSuffix == "CXX")
                    extensionSuffix = ProString("CPP");

                for (const ProString &extension : project->values(ProKey("QMAKE_EXT_" + extensionSuffix))) {
                    if (!file.endsWith(extension.toQString()))
                        continue;

                    QString precompiledHeader = header_prefix + language + header_suffix;
                    if (!ret.contains(precompiledHeader))
                        ret += precompiledHeader;

                    goto foundPrecompiledDependency;
                }
            }
          foundPrecompiledDependency:
            ; // Hurray!!
        }
    }
    return ret;
}

ProString
UnixMakefileGenerator::fixLibFlag(const ProString &lib)
{
    return escapeFilePath(lib);
}

bool
UnixMakefileGenerator::findLibraries(bool linkPrl, bool mergeLflags)
{
    QList<QMakeLocalFileName> libdirs, frameworkdirs;
    int libidx = 0, fwidx = 0;
    for (const ProString &dlib : project->values("QMAKE_DEFAULT_LIBDIRS"))
        libdirs.append(QMakeLocalFileName(dlib.toQString()));
    frameworkdirs.append(QMakeLocalFileName("/System/Library/Frameworks"));
    frameworkdirs.append(QMakeLocalFileName("/Library/Frameworks"));
    static const char * const lflags[] = { "QMAKE_LIBS", "QMAKE_LIBS_PRIVATE", 0 };
    for (int i = 0; lflags[i]; i++) {
        ProStringList &l = project->values(lflags[i]);
        for (ProStringList::Iterator it = l.begin(); it != l.end(); ) {
            QString opt = (*it).toQString();
            if(opt.startsWith("-")) {
                if(opt.startsWith("-L")) {
                    QString lib = opt.mid(2);
                    QMakeLocalFileName f(lib);
                    int idx = libdirs.indexOf(f);
                    if (idx >= 0 && idx < libidx) {
                        it = l.erase(it);
                        continue;
                    }
                    libdirs.insert(libidx++, f);
                } else if(opt.startsWith("-l")) {
                    QString lib = opt.mid(2);
                    ProStringList extens;
                    extens << project->first("QMAKE_EXTENSION_SHLIB") << "a";
                    for (QList<QMakeLocalFileName>::Iterator dep_it = libdirs.begin();
                         dep_it != libdirs.end(); ++dep_it) {
                        QString libBase = (*dep_it).local() + '/'
                                + project->first("QMAKE_PREFIX_SHLIB") + lib;
                        if (linkPrl && processPrlFile(libBase))
                            goto found;
                        for (ProStringList::Iterator extit = extens.begin(); extit != extens.end(); ++extit) {
                            if (exists(libBase + '.' + (*extit)))
                                goto found;
                        }
                    }
                  found: ;
                } else if (target_mode == TARG_MAC_MODE && opt.startsWith("-F")) {
                    QMakeLocalFileName f(opt.mid(2));
                    if (!frameworkdirs.contains(f))
                        frameworkdirs.insert(fwidx++, f);
                } else if (target_mode == TARG_MAC_MODE && opt.startsWith("-framework")) {
                    if (linkPrl) {
                        if (opt.length() == 10)
                            opt = (*++it).toQString();
                        else
                            opt = opt.mid(10).trimmed();
                        for (const QMakeLocalFileName &dir : qAsConst(frameworkdirs)) {
                            QString prl = dir.local() + "/" + opt + ".framework/" + opt + Option::prl_ext;
                            if (processPrlFile(prl))
                                break;
                        }
                    } else {
                        if (opt.length() == 10)
                            ++it;
                        // Skip
                    }
                }
            } else if (linkPrl) {
                processPrlFile(opt);
            }

            ProStringList &prl_libs = project->values("QMAKE_CURRENT_PRL_LIBS");
            for (int prl = 0; prl < prl_libs.size(); ++prl)
                it = l.insert(++it, prl_libs.at(prl));
            prl_libs.clear();
            ++it;
        }

        if (mergeLflags) {
            QHash<ProKey, ProStringList> lflags;
            for(int lit = 0; lit < l.size(); ++lit) {
                ProKey arch("default");
                ProString opt = l.at(lit);
                if (opt.startsWith('-')) {
                    if (target_mode == TARG_MAC_MODE && opt.startsWith("-Xarch")) {
                        if (opt.length() > 7) {
                            arch = opt.mid(7).toKey();
                            opt = l.at(++lit);
                        }
                    }

                    if (opt.startsWith("-L")
                        || (target_mode == TARG_MAC_MODE && opt.startsWith("-F"))) {
                        if (!lflags[arch].contains(opt))
                            lflags[arch].append(opt);
                    } else if (opt.startsWith("-l") || opt == "-pthread") {
                        // Make sure we keep the dependency order of libraries
                        lflags[arch].removeAll(opt);
                        lflags[arch].append(opt);
                    } else if (target_mode == TARG_MAC_MODE && opt.startsWith("-framework")) {
                        if (opt.length() > 10) {
                            opt = opt.mid(10).trimmed();
                        } else {
                            opt = l.at(++lit);
                            if (opt.startsWith("-Xarch"))
                                opt = l.at(++lit); // The user has done the right thing and prefixed each part
                        }
                        bool found = false;
                        for(int x = 0; x < lflags[arch].size(); ++x) {
                            if (lflags[arch].at(x) == "-framework" && lflags[arch].at(++x) == opt) {
                                found = true;
                                break;
                            }
                        }
                        if(!found) {
                            lflags[arch].append("-framework");
                            lflags[arch].append(opt);
                        }
                    } else {
                        lflags[arch].append(opt);
                    }
                } else if(!opt.isNull()) {
                    if(!lflags[arch].contains(opt))
                        lflags[arch].append(opt);
                }
            }

            l =  lflags.take("default");

            // Process architecture specific options (Xarch)
            QHash<ProKey, ProStringList>::const_iterator archIterator = lflags.constBegin();
            while (archIterator != lflags.constEnd()) {
                const ProStringList &archOptions = archIterator.value();
                for (int i = 0; i < archOptions.size(); ++i) {
                    l.append(QLatin1String("-Xarch_") + archIterator.key());
                    l.append(archOptions.at(i));
                }
                ++archIterator;
            }
        }
    }
    return false;
}

#ifdef Q_OS_WIN // MinGW x-compiling for QNX
QString UnixMakefileGenerator::installRoot() const
{
    /*
      We include a magic prefix on the path to bypass mingw-make's "helpful"
      intervention in the environment, recognising variables that look like
      paths and adding the msys system root as prefix, which we don't want.
      Once this hack has smuggled INSTALL_ROOT into make's variable space, we
      can trivially strip the magic prefix back off to get the path we meant.
     */
    return QStringLiteral("$(INSTALL_ROOT:@msyshack@%=%)");
}
#endif

QString
UnixMakefileGenerator::defaultInstall(const QString &t)
{
    if(t != "target" || project->first("TEMPLATE") == "subdirs")
        return QString();

    enum { NoBundle, SolidBundle, SlicedBundle } bundle = NoBundle;
    bool isAux = (project->first("TEMPLATE") == "aux");
    const QString root = installRoot();
    ProStringList &uninst = project->values(ProKey(t + ".uninstall"));
    QString ret, destdir = project->first("DESTDIR").toQString();
    if(!destdir.isEmpty() && destdir.right(1) != Option::dir_sep)
        destdir += Option::dir_sep;
    QString targetdir = fileFixify(project->first("target.path").toQString(), FileFixifyAbsolute);
    if(targetdir.right(1) != Option::dir_sep)
        targetdir += Option::dir_sep;

    ProStringList links;
    QString target="$(TARGET)";
    const ProStringList &targets = project->values(ProKey(t + ".targets"));
    if(!project->isEmpty("QMAKE_BUNDLE")) {
        target = project->first("QMAKE_BUNDLE").toQString();
        bundle = project->isActiveConfig("sliced_bundle") ? SlicedBundle : SolidBundle;
    } else if(project->first("TEMPLATE") == "app") {
        target = "$(QMAKE_TARGET)";
    } else if(project->first("TEMPLATE") == "lib") {
            if (!project->isActiveConfig("staticlib")
                    && !project->isActiveConfig("plugin")
                    && !project->isActiveConfig("unversioned_libname")) {
                if(project->isEmpty("QMAKE_HPUX_SHLIB")) {
                    links << "$(TARGET0)" << "$(TARGET1)" << "$(TARGET2)";
                } else {
                    links << "$(TARGET0)";
                }
            }
    }
    for(int i = 0; i < targets.size(); ++i) {
        QString src = targets.at(i).toQString(),
                dst = escapeFilePath(filePrefixRoot(root, targetdir + src.section('/', -1)));
        if(!ret.isEmpty())
            ret += "\n\t";
        ret += "-$(INSTALL_FILE) " + escapeFilePath(Option::fixPathToTargetOS(src, false)) + ' ' + dst;
        if(!uninst.isEmpty())
            uninst.append("\n\t");
        uninst.append("-$(DEL_FILE) " + dst);
    }

    {
        QString src_targ = target;
        if(!destdir.isEmpty())
            src_targ = Option::fixPathToTargetOS(destdir + target, false);
        QString plain_targ = filePrefixRoot(root, fileFixify(targetdir + target, FileFixifyAbsolute));
        QString dst_targ = plain_targ;
        plain_targ = escapeFilePath(plain_targ);
        if (bundle != NoBundle) {
            QString suffix;
            if (project->first("TEMPLATE") == "lib")
                suffix = "/Versions/" + project->first("QMAKE_FRAMEWORK_VERSION") + "/$(TARGET)";
            else
                suffix = "/" + project->first("QMAKE_BUNDLE_LOCATION") + "/$(QMAKE_TARGET)";
            dst_targ += suffix;
            if (bundle == SolidBundle) {
                if (!ret.isEmpty())
                    ret += "\n\t";
                ret += "$(DEL_FILE) -r " + plain_targ + "\n\t";
            } else {
                src_targ += suffix;
            }
        }
        src_targ = escapeFilePath(src_targ);
        dst_targ = escapeFilePath(dst_targ);

        QString copy_cmd;
        if (bundle == SolidBundle) {
            copy_cmd += "-$(INSTALL_DIR) " + src_targ + ' ' + plain_targ;
        } else if (project->first("TEMPLATE") == "lib" && project->isActiveConfig("staticlib")) {
            copy_cmd += "-$(INSTALL_FILE) " + src_targ + ' ' + dst_targ;
        } else if (!isAux) {
            if (bundle == SlicedBundle)
                ret += mkdir_p_asstring("\"`dirname " + dst_targ + "`\"", false) + "\n\t";
            copy_cmd += "-$(INSTALL_PROGRAM) " + src_targ + ' ' + dst_targ;
        }
        if(project->first("TEMPLATE") == "lib" && !project->isActiveConfig("staticlib")
           && project->values(ProKey(t + ".CONFIG")).indexOf("fix_rpath") != -1) {
            if (!ret.isEmpty())
                ret += "\n\t";
            if(!project->isEmpty("QMAKE_FIX_RPATH")) {
                ret += copy_cmd;
                ret += "\n\t-" + var("QMAKE_FIX_RPATH") + ' ' + dst_targ + ' ' + dst_targ;
            } else if(!project->isEmpty("QMAKE_LFLAGS_RPATH")) {
                ret += "-$(LINK) $(LFLAGS) " + var("QMAKE_LFLAGS_RPATH") + targetdir + " -o " +
                       dst_targ + " $(OBJECTS) $(LIBS) $(OBJCOMP)";
            } else {
                ret += copy_cmd;
            }
        } else if (!copy_cmd.isEmpty()) {
            if (!ret.isEmpty())
                ret += "\n\t";
            ret += copy_cmd;
        }

        if (isAux) {
        } else if (project->first("TEMPLATE") == "lib" && project->isActiveConfig("staticlib")) {
            if(!project->isEmpty("QMAKE_RANLIB"))
                ret += QString("\n\t$(RANLIB) ") + dst_targ;
        } else if (!project->isActiveConfig("debug_info") && !project->isActiveConfig("nostrip")
                   && !project->isEmpty("QMAKE_STRIP")) {
            ret += "\n\t-$(STRIP)";
            if (project->first("TEMPLATE") == "lib") {
                if (!project->isEmpty("QMAKE_STRIPFLAGS_LIB"))
                    ret += " " + var("QMAKE_STRIPFLAGS_LIB");
            } else if (project->first("TEMPLATE") == "app") {
                if (!project->isEmpty("QMAKE_STRIPFLAGS_APP"))
                    ret += " " + var("QMAKE_STRIPFLAGS_APP");
            }
            ret += ' ' + dst_targ;
        }
        if(!uninst.isEmpty())
            uninst.append("\n\t");
        if (bundle == SolidBundle)
            uninst.append("-$(DEL_FILE) -r " + plain_targ);
        else if (!isAux)
            uninst.append("-$(DEL_FILE) " + dst_targ);
        if (bundle == SlicedBundle) {
            int dstlen = project->first("DESTDIR").length();
            for (const ProString &src : project->values("QMAKE_BUNDLED_FILES")) {
                ProString file = src.mid(dstlen);
                QString dst = escapeFilePath(
                            filePrefixRoot(root, fileFixify(targetdir + file, FileFixifyAbsolute)));
                if (!ret.isEmpty())
                    ret += "\n\t";
                ret += mkdir_p_asstring("\"`dirname " + dst + "`\"", false) + "\n\t";
                ret += "-$(DEL_FILE) " + dst + "\n\t"; // Can't overwrite symlinks to directories
                ret += "-$(INSTALL_DIR) " + escapeFilePath(src) + " " + dst; // Use cp -R to copy symlinks
                if (!uninst.isEmpty())
                    uninst.append("\n\t");
                uninst.append("-$(DEL_FILE) " + dst);
            }
        }
        if(!links.isEmpty()) {
            for(int i = 0; i < links.size(); ++i) {
                if (target_mode == TARG_UNIX_MODE || target_mode == TARG_MAC_MODE) {
                    QString link = Option::fixPathToTargetOS(destdir + links[i], false);
                    int lslash = link.lastIndexOf(Option::dir_sep);
                    if(lslash != -1)
                        link = link.right(link.length() - (lslash + 1));
                    QString dst_link = escapeFilePath(
                                filePrefixRoot(root, fileFixify(targetdir + link, FileFixifyAbsolute)));
                    ret += "\n\t-$(SYMLINK) $(TARGET) " + dst_link;
                    if(!uninst.isEmpty())
                        uninst.append("\n\t");
                    uninst.append("-$(DEL_FILE) " + dst_link);
                }
            }
        }
    }
    if(project->first("TEMPLATE") == "lib") {
        QStringList types;
        types << "prl" << "libtool" << "pkgconfig";
        for(int i = 0; i < types.size(); ++i) {
            const QString type = types.at(i);
            QString meta;
            if(type == "prl" && project->isActiveConfig("create_prl") && !project->isActiveConfig("no_install_prl") &&
               !project->isEmpty("QMAKE_INTERNAL_PRL_FILE"))
                meta = prlFileName(false);
            if (type == "libtool" && project->isActiveConfig("create_libtool"))
                meta = libtoolFileName(false);
            if(type == "pkgconfig" && project->isActiveConfig("create_pc"))
                meta = pkgConfigFileName(false);
            if(!meta.isEmpty()) {
                QString src_meta = meta;
                if(!destdir.isEmpty())
                    src_meta = Option::fixPathToTargetOS(destdir + meta, false);
                QString dst_meta = filePrefixRoot(root, fileFixify(targetdir + meta, FileFixifyAbsolute));
                if(!uninst.isEmpty())
                    uninst.append("\n\t");
                uninst.append("-$(DEL_FILE) " + escapeFilePath(dst_meta));
                const QString dst_meta_dir = fileInfo(dst_meta).path();
                if(!dst_meta_dir.isEmpty()) {
                    if(!ret.isEmpty())
                        ret += "\n\t";
                    ret += mkdir_p_asstring(dst_meta_dir, true);
                }
                if (!ret.isEmpty())
                    ret += "\n\t";
                ret += installMetaFile(ProKey("QMAKE_" + type.toUpper() + "_INSTALL_REPLACE"), src_meta, dst_meta);
            }
        }
    }
    return ret;
}

QString
UnixMakefileGenerator::escapeFilePath(const QString &path) const
{
    QString ret = path;
    if(!ret.isEmpty()) {
        ret.replace(QLatin1Char(' '), QLatin1String("\\ "))
           .replace(QLatin1Char('\t'), QLatin1String("\\\t"));
        debug_msg(2, "EscapeFilePath: %s -> %s", path.toLatin1().constData(), ret.toLatin1().constData());
    }
    return ret;
}

QT_END_NAMESPACE
