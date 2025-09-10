// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mingw_make.h"
#include "option.h"

#include <proitems.h>

#include <qregularexpression.h>
#include <qdir.h>
#include <stdlib.h>
#include <time.h>

QT_BEGIN_NAMESPACE

QString MingwMakefileGenerator::escapeDependencyPath(const QString &path) const
{
    QString ret = path;
    ret.replace('\\', "/");  // ### this shouldn't be here
    return MakefileGenerator::escapeDependencyPath(ret);
}

ProString MingwMakefileGenerator::fixLibFlag(const ProString &lib)
{
    if (lib.startsWith("-l"))  // Fallback for unresolved -l libs.
        return QLatin1String("-l") + escapeFilePath(lib.mid(2));
    if (lib.startsWith("-L"))  // Lib search path. Needed only by -l above.
        return QLatin1String("-L")
                + escapeFilePath(Option::fixPathToTargetOS(lib.mid(2).toQString(), false));
    if (lib.startsWith("lib"))  // Fallback for unresolved MSVC-style libs.
        return QLatin1String("-l") + escapeFilePath(lib.mid(3).toQString());
    return escapeFilePath(Option::fixPathToTargetOS(lib.toQString(), false));
}

MakefileGenerator::LibFlagType
MingwMakefileGenerator::parseLibFlag(const ProString &flag, ProString *arg)
{
    // Skip MSVC handling from Win32MakefileGenerator
    return MakefileGenerator::parseLibFlag(flag, arg);
}

bool MingwMakefileGenerator::processPrlFileBase(QString &origFile, QStringView origName,
                                                QStringView fixedBase, int slashOff)
{
    if (origName.startsWith(u"lib")) {
        QString newFixedBase = fixedBase.left(slashOff) + fixedBase.mid(slashOff + 3);
        if (Win32MakefileGenerator::processPrlFileBase(origFile, origName,
                                                       QStringView(newFixedBase), slashOff)) {
            return true;
        }
    }
    return Win32MakefileGenerator::processPrlFileBase(origFile, origName, fixedBase, slashOff);
}

bool MingwMakefileGenerator::writeMakefile(QTextStream &t)
{
    writeHeader(t);
    if (writeDummyMakefile(t))
        return true;

    if(project->first("TEMPLATE") == "app" ||
       project->first("TEMPLATE") == "lib" ||
       project->first("TEMPLATE") == "aux") {
        if(project->isActiveConfig("create_pc") && project->first("TEMPLATE") == "lib")
            writePkgConfigFile();
        writeMingwParts(t);
        return MakefileGenerator::writeMakefile(t);
    }
    else if(project->first("TEMPLATE") == "subdirs") {
        writeSubDirs(t);
        return true;
    }
    return false;
 }

QString MingwMakefileGenerator::installRoot() const
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

void MingwMakefileGenerator::writeMingwParts(QTextStream &t)
{
    writeStandardParts(t);

    if (!preCompHeaderOut.isEmpty()) {
        QString header = project->first("PRECOMPILED_HEADER").toQString();
        QString cHeader = preCompHeaderOut + Option::dir_sep + "c";
        t << escapeDependencyPath(cHeader) << ": " << escapeDependencyPath(header) << " "
          << finalizeDependencyPaths(findDependencies(header)).join(" \\\n\t\t")
          << "\n\t" << mkdir_p_asstring(preCompHeaderOut)
          << "\n\t$(CC) -x c-header -c $(CFLAGS) $(INCPATH) -o " << escapeFilePath(cHeader)
          << ' ' << escapeFilePath(header) << Qt::endl << Qt::endl;
        QString cppHeader = preCompHeaderOut + Option::dir_sep + "c++";
        t << escapeDependencyPath(cppHeader) << ": " << escapeDependencyPath(header) << " "
          << finalizeDependencyPaths(findDependencies(header)).join(" \\\n\t\t")
          << "\n\t" << mkdir_p_asstring(preCompHeaderOut)
          << "\n\t$(CXX) -x c++-header -c $(CXXFLAGS) $(INCPATH) -o " << escapeFilePath(cppHeader)
          << ' ' << escapeFilePath(header) << Qt::endl << Qt::endl;
    }
}

void MingwMakefileGenerator::init()
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

    if (project->isActiveConfig("dll")) {
        QString destDir = "";
        if(!project->first("DESTDIR").isEmpty())
            destDir = Option::fixPathToTargetOS(project->first("DESTDIR") + Option::dir_sep, false, false);
        project->values("MINGW_IMPORT_LIB").prepend(destDir + project->first("LIB_TARGET"));
        project->values("QMAKE_LFLAGS").append(QString("-Wl,--out-implib,") + fileVar("MINGW_IMPORT_LIB"));
    }

    if (!project->values("DEF_FILE").isEmpty()) {
        QString defFileName = fileFixify(project->first("DEF_FILE").toQString());
        project->values("QMAKE_LFLAGS").append(QString("-Wl,") + escapeFilePath(defFileName));
    }

    if (project->isActiveConfig("staticlib") && project->first("TEMPLATE") == "lib")
        project->values("QMAKE_LFLAGS").append("-static");

    MakefileGenerator::init();

    // precomp
    if (!project->first("PRECOMPILED_HEADER").isEmpty()
        && project->isActiveConfig("precompile_header")) {
        QString preCompHeader = var("PRECOMPILED_DIR")
                    + QFileInfo(project->first("PRECOMPILED_HEADER").toQString()).fileName();
        preCompHeaderOut = preCompHeader + ".gch";
        project->values("QMAKE_CLEAN").append(preCompHeaderOut + Option::dir_sep + "c");
        project->values("QMAKE_CLEAN").append(preCompHeaderOut + Option::dir_sep + "c++");

        preCompHeader = escapeFilePath(preCompHeader);
        project->values("QMAKE_RUN_CC").clear();
        project->values("QMAKE_RUN_CC").append("$(CC) -c -include " + preCompHeader +
                                                    " $(CFLAGS) $(INCPATH) " + var("QMAKE_CC_O_FLAG") + "$obj $src");
        project->values("QMAKE_RUN_CC_IMP").clear();
        project->values("QMAKE_RUN_CC_IMP").append("$(CC)  -c -include " + preCompHeader +
                                                        " $(CFLAGS) $(INCPATH) " + var("QMAKE_CC_O_FLAG") + "$@ $<");
        project->values("QMAKE_RUN_CXX").clear();
        project->values("QMAKE_RUN_CXX").append("$(CXX) -c -include " + preCompHeader +
                                                     " $(CXXFLAGS) $(INCPATH) " + var("QMAKE_CC_O_FLAG") + "$obj $src");
        project->values("QMAKE_RUN_CXX_IMP").clear();
        project->values("QMAKE_RUN_CXX_IMP").append("$(CXX) -c -include " + preCompHeader +
                                                         " $(CXXFLAGS) $(INCPATH) " + var("QMAKE_CC_O_FLAG") + "$@ $<");
    }

    if(project->isActiveConfig("dll")) {
        project->values("QMAKE_DISTCLEAN").append(project->first("MINGW_IMPORT_LIB"));
    }
}

void MingwMakefileGenerator::writeIncPart(QTextStream &t)
{
    t << "INCPATH       = ";

    const ProStringList &incs = project->values("INCLUDEPATH");
    QFile responseFile;
    QTextStream responseStream;
    QChar sep(' ');
    int totalLength = std::accumulate(incs.constBegin(), incs.constEnd(), 0,
                                      [](int total, const ProString &inc) {
        return total + inc.size() + 2;
    });
    if (totalLength > project->intValue("QMAKE_RESPONSEFILE_THRESHOLD", 8000)) {
        const QString fileName = createResponseFile("incpath", incs, "-I");
        if (!fileName.isEmpty()) {
            t << '@' + fileName;
            t << Qt::endl;
            return;
        }
    }
    for (const ProString &incit: std::as_const(incs)) {
        QString inc = incit.toQString();
        inc.replace(QRegularExpression("\\\\$"), "");
        inc.replace('\\', '/');
        t << "-I" << escapeFilePath(inc) << sep;
    }
    t << Qt::endl;
}

void MingwMakefileGenerator::writeLibsPart(QTextStream &t)
{
    if(project->isActiveConfig("staticlib") && project->first("TEMPLATE") == "lib") {
        t << "LIB        =        " << var("QMAKE_LIB") << Qt::endl;
    } else {
        t << "LINKER      =        " << var("QMAKE_LINK") << Qt::endl;
        t << "LFLAGS        =        " << var("QMAKE_LFLAGS") << Qt::endl;
        t << "LIBS        =        "
          << fixLibFlags("LIBS").join(' ') << ' '
          << fixLibFlags("LIBS_PRIVATE").join(' ') << ' '
          << fixLibFlags("QMAKE_LIBS").join(' ') << ' '
          << fixLibFlags("QMAKE_LIBS_PRIVATE").join(' ') << Qt::endl;
    }
}

void MingwMakefileGenerator::writeObjectsPart(QTextStream &t)
{
    linkerResponseFile = maybeCreateLinkerResponseFile();
    if (!linkerResponseFile.isValid()) {
        objectsLinkLine = "$(OBJECTS)";
    } else if (project->isActiveConfig("staticlib") && project->first("TEMPLATE") == "lib") {
        // QMAKE_LIB is used for win32, including mingw, whereas QMAKE_AR is used on Unix.
        QString ar_cmd = var("QMAKE_LIB");
        if (ar_cmd.isEmpty())
            ar_cmd = "ar -rc";
        objectsLinkLine = ar_cmd + ' ' + var("DEST_TARGET") + " @"
            + escapeFilePath(linkerResponseFile.filePath);
    } else {
        objectsLinkLine = "@" + escapeFilePath(linkerResponseFile.filePath);
    }
    Win32MakefileGenerator::writeObjectsPart(t);
}

void MingwMakefileGenerator::writeBuildRulesPart(QTextStream &t)
{
    t << "first: all\n";
    t << "all: " << escapeDependencyPath(fileFixify(Option::output.fileName()))
      << ' ' << depVar("ALL_DEPS") << ' ' << depVar("DEST_TARGET") << "\n\n";
    t << depVar("DEST_TARGET") << ": "
      << depVar("PRE_TARGETDEPS") << " $(OBJECTS) " << depVar("POST_TARGETDEPS");
    if (project->first("TEMPLATE") == "aux") {
        t << "\n\n";
        return;
    }

    if(!project->isEmpty("QMAKE_PRE_LINK"))
        t << "\n\t" <<var("QMAKE_PRE_LINK");
    if(project->isActiveConfig("staticlib") && project->first("TEMPLATE") == "lib") {
        t << "\n\t-$(DEL_FILE) $(DESTDIR_TARGET) 2>" << var("QMAKE_SHELL_NULL_DEVICE");
        const ProString &objmax = project->first("QMAKE_LINK_OBJECT_MAX");
        if (objmax.isEmpty() || project->values("OBJECTS").size() < objmax.toInt()) {
            t << "\n\t$(LIB) $(DESTDIR_TARGET) " << objectsLinkLine << " " ;
        } else {
            t << "\n\t" << objectsLinkLine << " " ;
        }
    } else {
        t << "\n\t$(LINKER) $(LFLAGS) " << var("QMAKE_LINK_O_FLAG") << "$(DESTDIR_TARGET) "
          << objectsLinkLine;
        if (!linkerResponseFile.isValid() || linkerResponseFile.onlyObjects)
            t << " $(LIBS)";
    }
    if(!project->isEmpty("QMAKE_POST_LINK"))
        t << "\n\t" <<var("QMAKE_POST_LINK");
    t << Qt::endl;
}

void MingwMakefileGenerator::writeRcFilePart(QTextStream &t)
{
    const QString rc_file = fileFixify(project->first("RC_FILE").toQString());

    ProStringList rcIncPaths = project->values("RC_INCLUDEPATH");
    rcIncPaths.prepend(fileInfo(rc_file).path());
    QString incPathStr;
    for (int i = 0; i < rcIncPaths.size(); ++i) {
        const ProString &path = rcIncPaths.at(i);
        if (path.isEmpty())
            continue;
        incPathStr += QStringLiteral(" --include-dir=");
        if (path != "." && QDir::isRelativePath(path.toQString()))
            incPathStr += "./";
        incPathStr += escapeFilePath(path);
    }

    if (!rc_file.isEmpty()) {

        ProString defines = varGlue("RC_DEFINES", " -D", " -D", "");
        if (defines.isEmpty())
            defines = ProString(" $(DEFINES)");

        addSourceFile(rc_file, QMakeSourceFileInfo::SEEK_DEPS);
        const QStringList rcDeps = QStringList(rc_file) << dependencies(rc_file);

        t << escapeDependencyPath(var("RES_FILE")) << ": "
          << escapeDependencyPaths(rcDeps).join(' ') << "\n\t"
          << var("QMAKE_RC") << " -i " << escapeFilePath(rc_file) << " -o " << fileVar("RES_FILE")
          << incPathStr << defines << "\n\n";
    }
}

QStringList &MingwMakefileGenerator::findDependencies(const QString &file)
{
    QStringList &aList = MakefileGenerator::findDependencies(file);
    if (preCompHeaderOut.isEmpty())
        return aList;
    for (QStringList::Iterator it = Option::c_ext.begin(); it != Option::c_ext.end(); ++it) {
        if (file.endsWith(*it)) {
            QString cHeader = preCompHeaderOut + Option::dir_sep + "c";
            if (!aList.contains(cHeader))
                aList += cHeader;
            break;
        }
    }
    for (QStringList::Iterator it = Option::cpp_ext.begin(); it != Option::cpp_ext.end(); ++it) {
        if (file.endsWith(*it)) {
            QString cppHeader = preCompHeaderOut + Option::dir_sep + "c++";
            if (!aList.contains(cppHeader))
                aList += cppHeader;
            break;
        }
    }
    return aList;
}

QT_END_NAMESPACE
