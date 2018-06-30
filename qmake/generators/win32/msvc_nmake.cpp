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

#include <registry_p.h>

#include <time.h>

QT_BEGIN_NAMESPACE

static QString nmakePathList(const QStringList &list)
{
    QStringList pathList;
    pathList.reserve(list.size());
    for (const QString &path : list)
        pathList.append(QDir::cleanPath(path));

    return QDir::toNativeSeparators(pathList.join(QLatin1Char(';')))
            .replace('#', QLatin1String("^#")).replace('$', QLatin1String("$$"));
}

NmakeMakefileGenerator::NmakeMakefileGenerator() : usePCH(false), usePCHC(false)
{

}

bool
NmakeMakefileGenerator::writeMakefile(QTextStream &t)
{
    writeHeader(t);
    if (writeDummyMakefile(t))
        return true;

    if(project->first("TEMPLATE") == "app" ||
       project->first("TEMPLATE") == "lib" ||
       project->first("TEMPLATE") == "aux") {
#if 0
        if(Option::mkfile::do_stub_makefile)
            return MakefileGenerator::writeStubMakefile(t);
#endif
        if (!project->isHostBuild()) {
            if (project->isActiveConfig(QStringLiteral("winrt"))) {
                QString arch = project->first("VCPROJ_ARCH").toQString().toLower();
                QString compiler;
                QString compilerArch;
                const QString msvcVer = project->first("MSVC_VER").toQString();
                if (msvcVer.isEmpty()) {
                    fprintf(stderr, "Mkspec does not specify MSVC_VER. Cannot continue.\n");
                    return false;
                }

                if (msvcVer == QStringLiteral("15.0")) {
                    const ProStringList hostArch = project->values("QMAKE_TARGET.arch");
                    if (hostArch.contains("x86_64"))
                        compiler = QStringLiteral("HostX64/");
                    else
                        compiler = QStringLiteral("HostX86/");
                    if (arch == QLatin1String("arm")) {
                        compiler += QStringLiteral("arm");
                        compilerArch = QStringLiteral("arm");
                    } else if (arch == QLatin1String("x64")) {
                        compiler += QStringLiteral("x64");
                        compilerArch = QStringLiteral("amd64");
                    } else {
                        arch = QStringLiteral("x86");
                        compiler += QStringLiteral("x86");
                    }
                } else {
                    if (arch == QLatin1String("arm")) {
                        compiler = QStringLiteral("x86_arm");
                        compilerArch = QStringLiteral("arm");
                    } else if (arch == QLatin1String("x64")) {
                        const ProStringList hostArch = project->values("QMAKE_TARGET.arch");
                        if (hostArch.contains("x86_64"))
                            compiler = QStringLiteral("amd64");
                        else
                            compiler = QStringLiteral("x86_amd64");
                        compilerArch = QStringLiteral("amd64");
                    } else {
                        arch = QStringLiteral("x86");
                    }
                }

                const QString winsdkVer = project->first("WINSDK_VER").toQString();
                if (winsdkVer.isEmpty()) {
                    fprintf(stderr, "Mkspec does not specify WINSDK_VER. Cannot continue.\n");
                    return false;
                }
                const QString targetVer = project->first("WINTARGET_VER").toQString();
                if (targetVer.isEmpty()) {
                    fprintf(stderr, "Mkspec does not specify WINTARGET_VER. Cannot continue.\n");
                    return false;
                }

#ifdef Q_OS_WIN
                QString regKey;
                if (msvcVer == QStringLiteral("15.0"))
                    regKey = QStringLiteral("Software\\Microsoft\\VisualStudio\\SxS\\VS7\\") + msvcVer;
                else
                    regKey = QStringLiteral("Software\\Microsoft\\VisualStudio\\") + msvcVer + ("\\Setup\\VC\\ProductDir");
                const QString vcInstallDir = qt_readRegistryKey(HKEY_LOCAL_MACHINE, regKey, KEY_WOW64_32KEY);
                if (vcInstallDir.isEmpty()) {
                    fprintf(stderr, "Failed to find the Visual Studio installation directory.\n");
                    return false;
                }

                const QString windowsPath = "Software\\Microsoft\\Microsoft SDKs\\Windows\\v";

                regKey = windowsPath + winsdkVer + QStringLiteral("\\InstallationFolder");
                const QString kitDir = qt_readRegistryKey(HKEY_LOCAL_MACHINE, regKey, KEY_WOW64_32KEY);
                if (kitDir.isEmpty()) {
                    fprintf(stderr, "Failed to find the Windows Kit installation directory.\n");
                    return false;
                }
#else
                const QString vcInstallDir = "/fake/vc_install_dir";
                const QString kitDir = "/fake/sdk_install_dir";
#endif // Q_OS_WIN
                QStringList incDirs;
                QStringList libDirs;
                QStringList binDirs;
                if (msvcVer == QStringLiteral("15.0")) {
                    const QString toolsInstallDir = qgetenv("VCToolsInstallDir");
                    if (toolsInstallDir.isEmpty()) {
                        fprintf(stderr, "Failed to access tools installation dir.\n");
                        return false;
                    }

                    binDirs << toolsInstallDir + QStringLiteral("bin/") + compiler;
                    if (arch == QStringLiteral("x64"))
                        binDirs << toolsInstallDir + QStringLiteral("bin/HostX86/X86");
                    binDirs << kitDir + QStringLiteral("bin/x86");
                    binDirs << vcInstallDir + QStringLiteral("Common7/Tools");
                    binDirs << vcInstallDir + QStringLiteral("Common7/ide");
                    binDirs << vcInstallDir + QStringLiteral("MSBuild/15.0/bin");

                    incDirs << toolsInstallDir + QStringLiteral("include");
                    incDirs << vcInstallDir + QStringLiteral("VC/Auxiliary/VS/include");

                    const QString crtVersion = qgetenv("UCRTVersion");
                    if (crtVersion.isEmpty()) {
                        fprintf(stderr, "Failed to access CRT version.\n");
                        return false;
                    }
                    const QString crtInclude = kitDir + QStringLiteral("Include/") + crtVersion;
                    const QString crtLib = kitDir + QStringLiteral("Lib/") + crtVersion;
                    incDirs << crtInclude + QStringLiteral("/ucrt");
                    incDirs << crtInclude + QStringLiteral("/um");
                    incDirs << crtInclude + QStringLiteral("/shared");
                    incDirs << crtInclude + QStringLiteral("/winrt");

                    incDirs << kitDir + QStringLiteral("Extension SDKs/WindowsMobile/")
                                      + crtVersion + QStringLiteral("/Include/WinRT");

                    libDirs << toolsInstallDir + QStringLiteral("lib/") + arch + QStringLiteral("/store");

                    libDirs << vcInstallDir + QStringLiteral("VC/Auxiliary/VS/lib/") + arch;

                    libDirs << crtLib + QStringLiteral("/ucrt/") + arch;
                    libDirs << crtLib + QStringLiteral("/um/") + arch;
                } else if (msvcVer == QStringLiteral("14.0")) {
                    binDirs << vcInstallDir + QStringLiteral("bin/") + compiler;
                    binDirs << vcInstallDir + QStringLiteral("bin/"); // Maybe remove for x86 again?
                    binDirs << kitDir + QStringLiteral("bin/") + (arch == QStringLiteral("arm") ? QStringLiteral("x86") : arch);
                    binDirs << vcInstallDir + QStringLiteral("../Common7/Tools/bin");
                    binDirs << vcInstallDir + QStringLiteral("../Common7/Tools");
                    binDirs << vcInstallDir + QStringLiteral("../Common7/ide");
                    binDirs << kitDir + QStringLiteral("Windows Performance Toolkit/");

                    incDirs << vcInstallDir + QStringLiteral("include");
                    incDirs << vcInstallDir + QStringLiteral("atlmfc/include");

                    const QString crtVersion = qgetenv("UCRTVersion");
                    if (crtVersion.isEmpty()) {
                        fprintf(stderr, "Failed to access CRT version.\n");
                        return false;
                    }
                    const QString crtInclude = kitDir + QStringLiteral("Include/") + crtVersion;
                    const QString crtLib = kitDir + QStringLiteral("Lib/") + crtVersion;
                    incDirs << crtInclude + QStringLiteral("/ucrt");
                    incDirs << crtInclude + QStringLiteral("/um");
                    incDirs << crtInclude + QStringLiteral("/shared");
                    incDirs << crtInclude + QStringLiteral("/winrt");

                    incDirs << kitDir + QStringLiteral("Extension SDKs/WindowsMobile/")
                                      + crtVersion + QStringLiteral("/Include/WinRT");

                    libDirs << vcInstallDir + QStringLiteral("lib/store/") + compilerArch;
                    libDirs << vcInstallDir + QStringLiteral("atlmfc/lib") + compilerArch;

                    libDirs << crtLib + QStringLiteral("/ucrt/") + arch;
                    libDirs << crtLib + QStringLiteral("/um/") + arch;
                } else {
                    incDirs << vcInstallDir + QStringLiteral("/include");
                    libDirs << vcInstallDir + QStringLiteral("/lib/store/") + compilerArch
                            << vcInstallDir + QStringLiteral("/lib/") + compilerArch;
                    binDirs << vcInstallDir + QStringLiteral("/bin/") + compiler
                            << vcInstallDir + QStringLiteral("/../Common7/IDE");
                    libDirs << kitDir + QStringLiteral("/Lib/") + targetVer + ("/um/") + arch;
                    incDirs << kitDir + QStringLiteral("/include/um")
                            << kitDir + QStringLiteral("/include/shared")
                            << kitDir + QStringLiteral("/include/winrt");
                }

                binDirs << vcInstallDir + QStringLiteral("/bin");

                // Inherit PATH
                binDirs << QString::fromLocal8Bit(qgetenv("PATH")).split(QLatin1Char(';'));

                t << "\nINCLUDE = " << nmakePathList(incDirs);
                t << "\nLIB = " << nmakePathList(libDirs);
                t << "\nPATH = " << nmakePathList(binDirs) << '\n';
            }
        }
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
            QString pdb_target = project->first("TARGET") + project->first("TARGET_VERSION_EXT") + ".pdb";
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
          << escapeFilePath(precompH) << endl << endl;
    }
    if (usePCHC) {
        QString precompRuleC = QString("-c -Yc -Fp%1 -Fo%2")
                .arg(escapeFilePath(precompPchC), escapeFilePath(precompObjC));
        t << escapeDependencyPath(precompObjC) << ": " << escapeDependencyPath(precompH) << ' '
          << finalizeDependencyPaths(findDependencies(precompH)).join(" \\\n\t\t")
          << "\n\t$(CC) " + precompRuleC +" $(CFLAGS) $(INCPATH) -TC "
          << escapeFilePath(precompH) << endl << endl;
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
            QFileInfo precompHInfo(fileInfo(precompH));
            QString precompH_f = escapeFilePath(precompHInfo.fileName());
            QString precompRule = QString("-c -FI%1 -Yu%2 -Fp%3")
                    .arg(precompH_f, precompH_f, escapeFilePath(isRunC ? precompPchC : precompPch));
            QString p = MakefileGenerator::var(value);
            p.replace(QLatin1String("-c"), precompRule);
            // Cannot use -Gm with -FI & -Yu, as this gives an
            // internal compiler error, on the newer compilers
            // ### work-around for a VS 2003 bug. Move to some prf file or remove completely.
            p.remove("-Gm");
            return p;
        } else if (value == "QMAKE_CXXFLAGS") {
            // Remove internal compiler error option
            // ### work-around for a VS 2003 bug. Move to some prf file or remove completely.
            return MakefileGenerator::var(value).remove("-Gm");
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

    project->values("QMAKE_LIBS") += project->values("RES_FILE");

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
        project->values("OBJECTS") += precompObjC;
        project->values("QMAKE_CLEAN") += precompPchC;
        project->values("PRECOMPILED_OBJECT_C") = ProStringList(precompObjC);
        project->values("PRECOMPILED_PCH_C")    = ProStringList(precompPchC);
    }

    ProString tgt = project->first("DESTDIR")
                    + project->first("TARGET") + project->first("TARGET_VERSION_EXT");
    if(project->isActiveConfig("shared")) {
        project->values("QMAKE_CLEAN").append(tgt + ".exp");
        project->values("QMAKE_DISTCLEAN").append(tgt + ".lib");
    }
    if (project->isActiveConfig("debug_info")) {
        QString pdbfile;
        QString distPdbFile = tgt + ".pdb";
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
        project->values("QMAKE_CLEAN").append(tgt + ".ilk");
        project->values("QMAKE_CLEAN").append(tgt + ".idb");
    } else {
        ProStringList &defines = project->values("DEFINES");
        if (!defines.contains("NDEBUG"))
            defines.append("NDEBUG");
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
    t << endl << endl;

    bool useInferenceRules = !project->isActiveConfig("no_batch");
    QSet<QString> source_directories;
    if (useInferenceRules) {
        source_directories.insert(".");
        static const char * const directories[] = { "UI_SOURCES_DIR", "UI_DIR", 0 };
        for (int y = 0; directories[y]; y++) {
            QString dirTemp = project->first(directories[y]).toQString();
            if (dirTemp.endsWith("\\"))
                dirTemp.truncate(dirTemp.length()-1);
            if(!dirTemp.isEmpty())
                source_directories.insert(dirTemp);
        }
        static const char * const srcs[] = { "SOURCES", "GENERATED_SOURCES", 0 };
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
        QStringList fixifiedSourceDirs = fileFixify(source_directories.toList(), FileFixifyAbsolute);
        fixifiedSourceDirs.removeDuplicates();
        for (const QString &sourceDir : qAsConst(fixifiedSourceDirs)) {
            QDirIterator dit(sourceDir, sourceFilesFilter, QDir::Files | QDir::NoDotAndDotDot);
            while (dit.hasNext()) {
                dit.next();
                QString &duplicate = fileNames[dit.fileName()];
                if (duplicate.isNull()) {
                    duplicate = dit.filePath();
                } else {
                    warn_msg(WarnLogic, "%s conflicts with %s", qPrintable(duplicate),
                             qPrintable(dit.filePath()));
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
            t << (*cppit) << Option::obj_ext << ":\n\t" << var("QMAKE_RUN_CXX_IMP") << endl << endl;
        for(QStringList::Iterator cit = Option::c_ext.begin(); cit != Option::c_ext.end(); ++cit)
            t << (*cit) << Option::obj_ext << ":\n\t" << var("QMAKE_RUN_CC_IMP") << endl << endl;
    }

}

void NmakeMakefileGenerator::writeBuildRulesPart(QTextStream &t)
{
    const ProString templateName = project->first("TEMPLATE");

    t << "first: all\n";
    t << "all: " << escapeDependencyPath(fileFixify(Option::output.fileName()))
      << ' ' << depVar("ALL_DEPS") << " $(DESTDIR_TARGET)\n\n";
    t << "$(DESTDIR_TARGET): " << depVar("PRE_TARGETDEPS") << " $(OBJECTS) " << depVar("POST_TARGETDEPS");
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
    t << endl;
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
