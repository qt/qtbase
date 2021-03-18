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

#include "msvc_nmake.h"
#include "option.h"

#include <qregexp.h>
#include <qdir.h>
#include <qdiriterator.h>
#include <qset.h>

#include <time.h>

QT_BEGIN_NAMESPACE

bool
NmakeMakefileGenerator::writeMakefile(QTextStream &t)
{
    writeHeader(t);
    if (writeDummyMakefile(t))
        return true;

    if(project->first("TEMPLATE") == "app" ||
       project->first("TEMPLATE") == "lib" ||
       project->first("TEMPLATE") == "aux") {
        writeNmakeParts(t);
        return MakefileGenerator::writeMakefile(t);
    }
    else if(project->first("TEMPLATE") == "subdirs") {
        writeSubDirs(t);
        return true;
    }
    return false;
}

void NmakeMakefileGenerator::writeSubMakeCall(QTextStream &t, const QString &callPrefix,
                                              const QString &makeArguments)
{
    // Pass MAKEFLAGS as environment variable to sub-make calls.
    // Unlike other make tools nmake doesn't do this automatically.
    t << "\n\t@set MAKEFLAGS=$(MAKEFLAGS)";
    Win32MakefileGenerator::writeSubMakeCall(t, callPrefix, makeArguments);
}

ProStringList NmakeMakefileGenerator::extraSubTargetDependencies()
{
    return { "$(MAKEFILE)" };
}

QString NmakeMakefileGenerator::defaultInstall(const QString &t)
{
    QString ret = Win32MakefileGenerator::defaultInstall(t);
    if (ret.isEmpty())
        return ret;

    const QString root = installRoot();
    ProStringList &uninst = project->values(ProKey(t + ".uninstall"));
    QString targetdir = fileFixify(project->first(ProKey(t + ".path")).toQString(), FileFixifyAbsolute);
    if(targetdir.right(1) != Option::dir_sep)
        targetdir += Option::dir_sep;

    if (project->isActiveConfig("debug_info")) {
        if (t == "dlltarget" || project->values(ProKey(t + ".CONFIG")).indexOf("no_dll") == -1) {
            const QFileInfo targetFileInfo = project->first("DESTDIR") + project->first("TARGET")
                    + project->first("TARGET_EXT");
            const QString pdb_target = targetFileInfo.completeBaseName() + ".pdb";
            QString src_targ = (project->isEmpty("DESTDIR") ? QString("$(DESTDIR)") : project->first("DESTDIR")) + pdb_target;
            QString dst_targ = filePrefixRoot(root, fileFixify(targetdir + pdb_target, FileFixifyAbsolute));
            if(!ret.isEmpty())
                ret += "\n\t";
            ret += QString("-$(INSTALL_FILE) ") + escapeFilePath(src_targ) + ' ' + escapeFilePath(dst_targ);
            if(!uninst.isEmpty())
                uninst.append("\n\t");
            uninst.append("-$(DEL_FILE) " + escapeFilePath(dst_targ));
        }
    }

    return ret;
}

QStringList &NmakeMakefileGenerator::findDependencies(const QString &file)
{
    QStringList &aList = MakefileGenerator::findDependencies(file);
    for(QStringList::Iterator it = Option::cpp_ext.begin(); it != Option::cpp_ext.end(); ++it) {
        if(file.endsWith(*it)) {
            if(!precompObj.isEmpty() && !aList.contains(precompObj))
                aList += precompObj;
            break;
        }
    }
    for (QStringList::Iterator it = Option::c_ext.begin(); it != Option::c_ext.end(); ++it) {
        if (file.endsWith(*it)) {
            if (!precompObjC.isEmpty() && !aList.contains(precompObjC))
                aList += precompObjC;
            break;
        }
    }
    return aList;
}

void NmakeMakefileGenerator::writeNmakeParts(QTextStream &t)
{
    writeStandardParts(t);

    // precompiled header
    if(usePCH) {
        QString precompRule = QString("-c -Yc -Fp%1 -Fo%2")
                .arg(escapeFilePath(precompPch), escapeFilePath(precompObj));
        t << escapeDependencyPath(precompObj) << ": " << escapeDependencyPath(precompH) << ' '
          << finalizeDependencyPaths(findDependencies(precompH)).join(" \\\n\t\t")
          << "\n\t$(CXX) " + precompRule +" $(CXXFLAGS) $(INCPATH) -TP "
          << escapeFilePath(precompH) << Qt::endl << Qt::endl;
    }
    if (usePCHC) {
        QString precompRuleC = QString("-c -Yc -Fp%1 -Fo%2")
                .arg(escapeFilePath(precompPchC), escapeFilePath(precompObjC));
        t << escapeDependencyPath(precompObjC) << ": " << escapeDependencyPath(precompH) << ' '
          << finalizeDependencyPaths(findDependencies(precompH)).join(" \\\n\t\t")
          << "\n\t$(CC) " + precompRuleC +" $(CFLAGS) $(INCPATH) -TC "
          << escapeFilePath(precompH) << Qt::endl << Qt::endl;
    }
}

QString NmakeMakefileGenerator::var(const ProKey &value) const
{
    if (usePCH || usePCHC) {
        const bool isRunC = (value == "QMAKE_RUN_CC_IMP_BATCH"
                             || value == "QMAKE_RUN_CC_IMP"
                             || value == "QMAKE_RUN_CC");
        const bool isRunCpp = (value == "QMAKE_RUN_CXX_IMP_BATCH"
                               || value == "QMAKE_RUN_CXX_IMP"
                               || value == "QMAKE_RUN_CXX");
        if ((isRunCpp && usePCH) || (isRunC && usePCHC)) {
            QString precompH_f = escapeFilePath(fileFixify(precompH, FileFixifyBackwards));
            QString precompRule = QString("-c -FI%1 -Yu%2 -Fp%3")
                    .arg(precompH_f, precompH_f, escapeFilePath(isRunC ? precompPchC : precompPch));
            // ### For clang_cl 8 we force inline methods to be compiled here instead
            // linking them from a pch.o file. We do this by pretending we are also doing
            // the pch.o generation step.
            if (project->isActiveConfig("clang_cl"))
                precompRule += QString(" -Xclang -building-pch-with-obj");
            QString p = MakefileGenerator::var(value);
            p.replace(QLatin1String("-c"), precompRule);
            return p;
        }
    }

    // Normal val
    return MakefileGenerator::var(value);
}

void NmakeMakefileGenerator::init()
{
    /* this should probably not be here, but I'm using it to wrap the .t files */
    if(project->first("TEMPLATE") == "app")
        project->values("QMAKE_APP_FLAG").append("1");
    else if(project->first("TEMPLATE") == "lib")
        project->values("QMAKE_LIB_FLAG").append("1");
    else if(project->first("TEMPLATE") == "subdirs") {
        MakefileGenerator::init();
        if(project->values("MAKEFILE").isEmpty())
            project->values("MAKEFILE").append("Makefile");
        return;
    }

    processVars();

    project->values("LIBS") += project->values("RES_FILE");

    if (!project->values("DEF_FILE").isEmpty()) {
        QString defFileName = fileFixify(project->first("DEF_FILE").toQString());
        project->values("QMAKE_LFLAGS").append(QString("/DEF:") + escapeFilePath(defFileName));
    }

    // set /VERSION for EXE/DLL header
    ProString major_minor = project->first("VERSION_PE_HEADER");
    if (major_minor.isEmpty()) {
        ProString version = project->first("VERSION");
        if (!version.isEmpty()) {
            int firstDot = version.indexOf(".");
            int secondDot = version.indexOf(".", firstDot + 1);
            major_minor = version.left(secondDot);
        }
    }
    if (!major_minor.isEmpty())
        project->values("QMAKE_LFLAGS").append("/VERSION:" + major_minor);

    if (project->isEmpty("QMAKE_LINK_O_FLAG"))
        project->values("QMAKE_LINK_O_FLAG").append("/OUT:");

    // Base class init!
    MakefileGenerator::init();

    // Setup PCH variables
    precompH = project->first("PRECOMPILED_HEADER").toQString();
    usePCH = !precompH.isEmpty() && project->isActiveConfig("precompile_header");
    usePCHC = !precompH.isEmpty() && project->isActiveConfig("precompile_header_c");
    if (usePCH) {
        // Created files
        precompObj = var("PRECOMPILED_DIR") + project->first("TARGET") + "_pch" + Option::obj_ext;
        precompPch = var("PRECOMPILED_DIR") + project->first("TARGET") + "_pch.pch";
        // Add linking of precompObj (required for whole precompiled classes)
        // ### For clang_cl we currently let inline methods be generated in the normal objects,
        // since the PCH object is buggy (as of clang 8.0.0)
        if (!project->isActiveConfig("clang_cl"))
            project->values("OBJECTS") += precompObj;
        // Add pch file to cleanup
        project->values("QMAKE_CLEAN") += precompPch;
        // Return to variable pool
        project->values("PRECOMPILED_OBJECT") = ProStringList(precompObj);
        project->values("PRECOMPILED_PCH")    = ProStringList(precompPch);
    }
    if (usePCHC) {
        precompObjC = var("PRECOMPILED_DIR") + project->first("TARGET") + "_pch_c" + Option::obj_ext;
        precompPchC = var("PRECOMPILED_DIR") + project->first("TARGET") + "_pch_c.pch";
        if (!project->isActiveConfig("clang_cl"))
            project->values("OBJECTS") += precompObjC;
        project->values("QMAKE_CLEAN") += precompPchC;
        project->values("PRECOMPILED_OBJECT_C") = ProStringList(precompObjC);
        project->values("PRECOMPILED_PCH_C")    = ProStringList(precompPchC);
    }

    const QFileInfo targetFileInfo = project->first("DESTDIR") + project->first("TARGET")
            + project->first("TARGET_EXT");
    const ProString targetBase = targetFileInfo.path() + '/' + targetFileInfo.completeBaseName();
    if (project->first("TEMPLATE") == "lib" && project->isActiveConfig("shared")) {
        project->values("QMAKE_CLEAN").append(targetBase + ".exp");
        project->values("QMAKE_DISTCLEAN").append(targetBase + ".lib");
    }
    if (project->isActiveConfig("debug_info")) {
        QString pdbfile;
        QString distPdbFile = targetBase + ".pdb";
        if (project->isActiveConfig("staticlib")) {
            // For static libraries, the compiler's pdb file and the dist pdb file are the same.
            pdbfile = distPdbFile;
        } else {
            // Use $${TARGET}.vc.pdb in the OBJECTS_DIR for the compiler and
            // $${TARGET}.pdb (the default) for the linker.
            pdbfile = var("OBJECTS_DIR") + project->first("TARGET") + ".vc.pdb";
        }
        QString escapedPdbFile = escapeFilePath(pdbfile);
        project->values("QMAKE_CFLAGS").append("/Fd" + escapedPdbFile);
        project->values("QMAKE_CXXFLAGS").append("/Fd" + escapedPdbFile);
        project->values("QMAKE_CLEAN").append(pdbfile);
        project->values("QMAKE_DISTCLEAN").append(distPdbFile);
    }
    if (project->isActiveConfig("debug")) {
        project->values("QMAKE_CLEAN").append(targetBase + ".ilk");
        project->values("QMAKE_CLEAN").append(targetBase + ".idb");
    }

    if (project->values("QMAKE_APP_FLAG").isEmpty() && project->isActiveConfig("dll")) {
        ProStringList &defines = project->values("DEFINES");
        if (!defines.contains("_WINDLL"))
            defines.append("_WINDLL");
    }
}

QStringList NmakeMakefileGenerator::sourceFilesForImplicitRulesFilter()
{
    QStringList filter;
    const QChar wildcard = QLatin1Char('*');
    for (const QString &ext : qAsConst(Option::c_ext))
        filter << wildcard + ext;
    for (const QString &ext : qAsConst(Option::cpp_ext))
        filter << wildcard + ext;
    return filter;
}

void NmakeMakefileGenerator::writeImplicitRulesPart(QTextStream &t)
{
    t << "####### Implicit rules\n\n";

    t << ".SUFFIXES:";
    for(QStringList::Iterator cit = Option::c_ext.begin(); cit != Option::c_ext.end(); ++cit)
        t << " " << (*cit);
    for(QStringList::Iterator cppit = Option::cpp_ext.begin(); cppit != Option::cpp_ext.end(); ++cppit)
        t << " " << (*cppit);
    t << Qt::endl << Qt::endl;

    bool useInferenceRules = !project->isActiveConfig("no_batch");
    QSet<QString> source_directories;
    if (useInferenceRules) {
        source_directories.insert(".");
        static const char * const directories[] = { "UI_SOURCES_DIR", "UI_DIR", nullptr };
        for (int y = 0; directories[y]; y++) {
            QString dirTemp = project->first(directories[y]).toQString();
            if (dirTemp.endsWith("\\"))
                dirTemp.truncate(dirTemp.length()-1);
            if(!dirTemp.isEmpty())
                source_directories.insert(dirTemp);
        }
        static const char * const srcs[] = { "SOURCES", "GENERATED_SOURCES", nullptr };
        for (int x = 0; srcs[x]; x++) {
            const ProStringList &l = project->values(srcs[x]);
            for (ProStringList::ConstIterator sit = l.begin(); sit != l.end(); ++sit) {
                QString sep = "\\";
                if((*sit).indexOf(sep) == -1)
                    sep = "/";
                QString dir = (*sit).toQString().section(sep, 0, -2);
                if (!dir.isEmpty())
                    source_directories.insert(dir);
            }
        }

        // nmake's inference rules might pick up the wrong files when encountering source files with
        // the same name in different directories. In this situation, turn inference rules off.
        QHash<QString, QString> fileNames;
        bool duplicatesFound = false;
        const QStringList sourceFilesFilter = sourceFilesForImplicitRulesFilter();
        QStringList fixifiedSourceDirs = fileFixify(QList<QString>(source_directories.constBegin(), source_directories.constEnd()), FileFixifyAbsolute);
        fixifiedSourceDirs.removeDuplicates();
        for (const QString &sourceDir : qAsConst(fixifiedSourceDirs)) {
            QDirIterator dit(sourceDir, sourceFilesFilter, QDir::Files | QDir::NoDotAndDotDot);
            while (dit.hasNext()) {
                dit.next();
                const QFileInfo fi = dit.fileInfo();
                QString &duplicate = fileNames[fi.completeBaseName()];
                if (duplicate.isNull()) {
                    duplicate = fi.filePath();
                } else {
                    warn_msg(WarnLogic, "%s conflicts with %s", qPrintable(duplicate),
                             qPrintable(fi.filePath()));
                    duplicatesFound = true;
                }
            }
        }
        if (duplicatesFound) {
            useInferenceRules = false;
            warn_msg(WarnLogic, "Automatically turning off nmake's inference rules. (CONFIG += no_batch)");
        }
    }

    if (useInferenceRules) {
        // Batchmode doesn't use the non implicit rules QMAKE_RUN_CXX & QMAKE_RUN_CC
        project->variables().remove("QMAKE_RUN_CXX");
        project->variables().remove("QMAKE_RUN_CC");

        for (const QString &sourceDir : qAsConst(source_directories)) {
            if (sourceDir.isEmpty())
                continue;
            QString objDir = var("OBJECTS_DIR");
            if (objDir == ".\\")
                objDir = "";
            for(QStringList::Iterator cppit = Option::cpp_ext.begin(); cppit != Option::cpp_ext.end(); ++cppit)
                t << '{' << escapeDependencyPath(sourceDir) << '}' << (*cppit)
                  << '{' << escapeDependencyPath(objDir) << '}' << Option::obj_ext << "::\n\t"
                  << var("QMAKE_RUN_CXX_IMP_BATCH").replace(QRegExp("\\$@"), fileVar("OBJECTS_DIR"))
                  << "\n\t$<\n<<\n\n";
            for(QStringList::Iterator cit = Option::c_ext.begin(); cit != Option::c_ext.end(); ++cit)
                t << '{' << escapeDependencyPath(sourceDir) << '}' << (*cit)
                  << '{' << escapeDependencyPath(objDir) << '}' << Option::obj_ext << "::\n\t"
                  << var("QMAKE_RUN_CC_IMP_BATCH").replace(QRegExp("\\$@"), fileVar("OBJECTS_DIR"))
                  << "\n\t$<\n<<\n\n";
        }
    } else {
        for(QStringList::Iterator cppit = Option::cpp_ext.begin(); cppit != Option::cpp_ext.end(); ++cppit)
            t << (*cppit) << Option::obj_ext << ":\n\t" << var("QMAKE_RUN_CXX_IMP") << Qt::endl << Qt::endl;
        for(QStringList::Iterator cit = Option::c_ext.begin(); cit != Option::c_ext.end(); ++cit)
            t << (*cit) << Option::obj_ext << ":\n\t" << var("QMAKE_RUN_CC_IMP") << Qt::endl << Qt::endl;
    }

}

void NmakeMakefileGenerator::writeBuildRulesPart(QTextStream &t)
{
    const ProString templateName = project->first("TEMPLATE");

    t << "first: all\n";
    t << "all: " << escapeDependencyPath(fileFixify(Option::output.fileName()))
      << ' ' << depVar("ALL_DEPS") << ' ' << depVar("DEST_TARGET") << "\n\n";
    t << depVar("DEST_TARGET") << ": "
      << depVar("PRE_TARGETDEPS") << " $(OBJECTS) " << depVar("POST_TARGETDEPS");
    if (templateName == "aux") {
        t << "\n\n";
        return;
    }

    if(!project->isEmpty("QMAKE_PRE_LINK"))
        t << "\n\t" <<var("QMAKE_PRE_LINK");
    if(project->isActiveConfig("staticlib")) {
        t << "\n\t$(LIBAPP) $(LIBFLAGS) " << var("QMAKE_LINK_O_FLAG") << "$(DESTDIR_TARGET) @<<\n\t  ";
        writeResponseFileFiles(t, project->values("OBJECTS"));
        t << "<<";
    } else {
        const bool embedManifest = ((templateName == "app" && project->isActiveConfig("embed_manifest_exe"))
                                    || (templateName == "lib" && project->isActiveConfig("embed_manifest_dll")
                                        && !(project->isActiveConfig("plugin") && project->isActiveConfig("no_plugin_manifest"))
                                        ));
        if (embedManifest) {
            bool generateManifest = false;
            const QString target = var("DEST_TARGET");
            QString manifest = project->first("QMAKE_MANIFEST").toQString();
            QString extraLFlags;
            const bool linkerSupportsEmbedding = (msvcVersion() >= 1200);
            if (manifest.isEmpty()) {
                generateManifest = true;
                if (linkerSupportsEmbedding) {
                    extraLFlags = "/MANIFEST:embed";
                } else {
                    manifest = target + ".embed.manifest";
                    extraLFlags += "/MANIFEST /MANIFESTFILE:" + escapeFilePath(manifest);
                    project->values("QMAKE_CLEAN") << manifest;
                }
            } else {
                manifest = fileFixify(manifest);
                if (linkerSupportsEmbedding)
                    extraLFlags = "/MANIFEST:embed /MANIFESTINPUT:" + escapeFilePath(manifest);
            }

            const QString resourceId = (templateName == "app") ? "1" : "2";
            const bool incrementalLinking = project->values("QMAKE_LFLAGS").toQStringList().filter(QRegExp("(/|-)INCREMENTAL:NO")).isEmpty();
            if (incrementalLinking && !linkerSupportsEmbedding) {
                // Link a resource that contains the manifest without modifying the exe/dll after linking.

                QString manifest_rc = target +  "_manifest.rc";
                QString manifest_res = target +  "_manifest.res";
                project->values("QMAKE_CLEAN") << manifest_rc << manifest_res;
                manifest_rc = escapeFilePath(manifest_rc);
                manifest_res = escapeFilePath(manifest_res);

                t << "\n\techo " << resourceId
                  << " /* CREATEPROCESS_MANIFEST_RESOURCE_ID */ 24 /* RT_MANIFEST */ "
                  << cQuoted(manifest) << '>' << manifest_rc;

                if (generateManifest) {
                    manifest = escapeFilePath(manifest);
                    QString manifest_bak = escapeFilePath(target +  "_manifest.bak");
                    project->values("QMAKE_CLEAN") << manifest_bak;
                    t << "\n\tif not exist $(DESTDIR_TARGET) if exist " << manifest
                      << " del " << manifest;
                    t << "\n\tif exist " << manifest << " copy /Y " << manifest << ' ' << manifest_bak;
                    const QString extraInlineFileContent = "\n!IF EXIST(" + manifest_res + ")\n" + manifest_res + "\n!ENDIF";
                    t << "\n\t";
                    writeLinkCommand(t, extraLFlags, extraInlineFileContent);
                    t << "\n\tif exist " << manifest_bak << " fc /b " << manifest << ' ' << manifest_bak << " >NUL || del " << manifest_bak;
                    t << "\n\tif not exist " << manifest_bak << " rc.exe /fo" << manifest_res << ' ' << manifest_rc;
                    t << "\n\tif not exist " << manifest_bak << ' ';
                    writeLinkCommand(t, extraLFlags, manifest_res);
                    t << "\n\tif exist " << manifest_bak << " del " << manifest_bak;
                } else {
                    t << "\n\trc.exe /fo" << manifest_res << " " << manifest_rc;
                    t << "\n\t";
                    writeLinkCommand(t, extraLFlags, manifest_res);
                }
            } else {
                // directly embed the manifest in the executable after linking
                t << "\n\t";
                writeLinkCommand(t, extraLFlags);
                if (!linkerSupportsEmbedding) {
                    t << "\n\tmt.exe /nologo /manifest " << escapeFilePath(manifest)
                      << " /outputresource:$(DESTDIR_TARGET);" << resourceId;
                }
            }
        }  else {
            t << "\n\t";
            writeLinkCommand(t);
        }
    }
    if(!project->isEmpty("QMAKE_POST_LINK")) {
        t << "\n\t" << var("QMAKE_POST_LINK");
    }
    t << Qt::endl;
}

void NmakeMakefileGenerator::writeLinkCommand(QTextStream &t, const QString &extraFlags, const QString &extraInlineFileContent)
{
    t << "$(LINKER) $(LFLAGS)";
    if (!extraFlags.isEmpty())
        t << ' ' << extraFlags;
    t << " " << var("QMAKE_LINK_O_FLAG") << "$(DESTDIR_TARGET) @<<\n";
    writeResponseFileFiles(t, project->values("OBJECTS"));
    t << "$(LIBS)\n";
    if (!extraInlineFileContent.isEmpty())
        t << extraInlineFileContent << '\n';
    t << "<<";
}

void NmakeMakefileGenerator::writeResponseFileFiles(QTextStream &t, const ProStringList &files)
{
    // Add line breaks in file lists in reponse files to work around LNK1170.
    // The actual line length limit is 131070, but let's use a smaller limit
    // in case other tools are similarly hampered.
    const int maxLineLength = 1000;
    int len = 0;
    for (const ProString &file : files) {
        const ProString escapedFilePath = escapeFilePath(file);
        if (len) {
            if (len + escapedFilePath.length() > maxLineLength) {
                t << '\n';
                len = 0;
            } else {
                t << ' ';
                len++;
            }
        }
        t << escapedFilePath;
        len += escapedFilePath.length();
    }
    t << '\n';
}

int NmakeMakefileGenerator::msvcVersion() const
{
    const int fallbackVersion = 800;    // Visual Studio 2005
    const QString ver = project->first(ProKey("MSVC_VER")).toQString();
    bool ok;
    float f = ver.toFloat(&ok);
    return ok ? int(f * 100) : fallbackVersion;
}

QT_END_NAMESPACE
