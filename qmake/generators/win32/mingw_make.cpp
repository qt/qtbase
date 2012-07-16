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

#include "mingw_make.h"
#include "option.h"
#include "meta.h"
#include <qregexp.h>
#include <qdir.h>
#include <stdlib.h>
#include <time.h>

QT_BEGIN_NAMESPACE

MingwMakefileGenerator::MingwMakefileGenerator() : Win32MakefileGenerator(), init_flag(false)
{
    if (isWindowsShell())
        quote = "\"";
    else
        quote = "'";
}

QString MingwMakefileGenerator::escapeDependencyPath(const QString &path) const
{
    QString ret = path;
    ret.remove('\"');
    ret.replace('\\', "/");
    ret.replace(' ', "\\ ");
    return ret;
}

QString MingwMakefileGenerator::getLibTarget()
{
    return QString("lib" + project->first("TARGET") + project->first("TARGET_VERSION_EXT") + ".a");
}

bool MingwMakefileGenerator::findLibraries()
{
    QList<QMakeLocalFileName> dirs;
    {
        const QStringList &libpaths = project->values("QMAKE_LIBDIR");
        for (QStringList::ConstIterator libpathit = libpaths.begin();
            libpathit != libpaths.end(); ++libpathit)
            dirs.append(QMakeLocalFileName((*libpathit)));
    }

  const QString lflags[] = { "QMAKE_LIBS", "QMAKE_LIBS_PRIVATE", QString() };
  for (int i = 0; !lflags[i].isNull(); i++) {
    QStringList &l = project->values(lflags[i]);
    QStringList::Iterator it = l.begin();
    while (it != l.end()) {
        if ((*it).startsWith("-l")) {
            QString steam = (*it).mid(2), out;
            QString suffix;
            if (!project->isEmpty("QMAKE_" + steam.toUpper() + "_SUFFIX"))
                suffix = project->first("QMAKE_" + steam.toUpper() + "_SUFFIX");
            for (QList<QMakeLocalFileName>::Iterator dir_it = dirs.begin(); dir_it != dirs.end(); ++dir_it) {
                QString extension;
                int ver = findHighestVersion((*dir_it).local(), steam, "dll.a|a");
                if (ver != -1)
                    extension += QString::number(ver);
                extension += suffix;
                if(QMakeMetaInfo::libExists((*dir_it).local() + Option::dir_sep + steam) ||
                    exists((*dir_it).local() + Option::dir_sep + steam + extension + ".a") ||
                    exists((*dir_it).local() + Option::dir_sep + steam + extension + ".dll.a")) {
                        out = (*it) + extension;
                        break;
                }
            }
            if (!out.isEmpty()) // We assume if it never finds it that its correct
                (*it) = out;
	    } else if((*it).startsWith("-L")) {
            dirs.append(QMakeLocalFileName((*it).mid(2)));
        }

        ++it;
    }
  }
    return true;
}

bool MingwMakefileGenerator::writeMakefile(QTextStream &t)
{
    writeHeader(t);
    if(!project->values("QMAKE_FAILED_REQUIREMENTS").isEmpty()) {
        t << "all clean:" << "\n\t"
          << "@echo \"Some of the required modules ("
          << var("QMAKE_FAILED_REQUIREMENTS") << ") are not available.\"" << "\n\t"
          << "@echo \"Skipped.\"" << endl << endl;
        writeMakeQmake(t);
        return true;
    }

    if(project->first("TEMPLATE") == "app" ||
       project->first("TEMPLATE") == "lib" ||
       project->first("TEMPLATE") == "aux") {
        if(project->isActiveConfig("create_pc") && project->first("TEMPLATE") == "lib")
            writePkgConfigFile();

        if(Option::mkfile::do_stub_makefile) {
            t << "QMAKE    = " << var("QMAKE_QMAKE") << endl;
            const QStringList &qut = project->values("QMAKE_EXTRA_TARGETS");
            for(QStringList::ConstIterator it = qut.begin(); it != qut.end(); ++it)
                t << *it << " ";
            t << "first all clean install distclean uninstall: qmake" << endl
              << "qmake_all:" << endl;
            writeMakeQmake(t);
            if(project->isEmpty("QMAKE_NOFORCE"))
                t << "FORCE:" << endl << endl;
            return true;
        }
        writeMingwParts(t);
        return MakefileGenerator::writeMakefile(t);
    }
    else if(project->first("TEMPLATE") == "subdirs") {
        writeSubDirs(t);
        return true;
    }
    return false;
 }

void createLdObjectScriptFile(const QString &fileName, const QStringList &objList)
{
    QString filePath = Option::output_dir + QDir::separator() + fileName;
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream t(&file);
        t << "INPUT(" << endl;
        for (QStringList::ConstIterator it = objList.constBegin(); it != objList.constEnd(); ++it) {
            if (QDir::isRelativePath(*it))
		t << "./" << *it << endl;
	    else
		t << *it << endl;
        }
        t << ");" << endl;
	t.flush();
        file.close();
    }
}

void createArObjectScriptFile(const QString &fileName, const QString &target, const QStringList &objList)
{
    QString filePath = Option::output_dir + QDir::separator() + fileName;
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream t(&file);
        t << "CREATE " << target << endl;
        for (QStringList::ConstIterator it = objList.constBegin(); it != objList.constEnd(); ++it) {
            t << "ADDMOD " << *it << endl;
        }
        t << "SAVE" << endl;
	t.flush();
        file.close();
    }
}

void createRvctObjectScriptFile(const QString &fileName, const QStringList &objList)
{
    QString filePath = Option::output_dir + QDir::separator() + fileName;
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream t(&file);
        for (QStringList::ConstIterator it = objList.constBegin(); it != objList.constEnd(); ++it) {
            if (QDir::isRelativePath(*it))
                t << "./" << *it << endl;
            else
                t << *it << endl;
        }
        t.flush();
        file.close();
    }
}

void MingwMakefileGenerator::writeMingwParts(QTextStream &t)
{
    writeStandardParts(t);

    if (!preCompHeaderOut.isEmpty()) {
	QString header = project->first("PRECOMPILED_HEADER");
	QString cHeader = preCompHeaderOut + Option::dir_sep + "c";
	t << escapeDependencyPath(cHeader) << ": " << escapeDependencyPath(header) << " "
          << escapeDependencyPaths(findDependencies(header)).join(" \\\n\t\t")
	  << "\n\t" << mkdir_p_asstring(preCompHeaderOut)
	  << "\n\t" << "$(CC) -x c-header -c $(CFLAGS) $(INCPATH) -o " << cHeader << " " << header
          << endl << endl;
	QString cppHeader = preCompHeaderOut + Option::dir_sep + "c++";
	t << escapeDependencyPath(cppHeader) << ": " << escapeDependencyPath(header) << " "
          << escapeDependencyPaths(findDependencies(header)).join(" \\\n\t\t")
	  << "\n\t" << mkdir_p_asstring(preCompHeaderOut)
	  << "\n\t" << "$(CXX) -x c++-header -c $(CXXFLAGS) $(INCPATH) -o " << cppHeader << " " << header
          << endl << endl;
    }
}

void MingwMakefileGenerator::init()
{
    if(init_flag)
        return;
    init_flag = true;

    /* this should probably not be here, but I'm using it to wrap the .t files */
    if(project->first("TEMPLATE") == "app")
        project->values("QMAKE_APP_FLAG").append("1");
    else if(project->first("TEMPLATE") == "lib")
        project->values("QMAKE_LIB_FLAG").append("1");
    else if(project->first("TEMPLATE") == "subdirs") {
        MakefileGenerator::init();
        if(project->isEmpty("QMAKE_COPY_FILE"))
            project->values("QMAKE_COPY_FILE").append("$(COPY)");
        if(project->isEmpty("QMAKE_COPY_DIR"))
            project->values("QMAKE_COPY_DIR").append("xcopy /s /q /y /i");
        if(project->isEmpty("QMAKE_INSTALL_FILE"))
            project->values("QMAKE_INSTALL_FILE").append("$(COPY_FILE)");
        if(project->isEmpty("QMAKE_INSTALL_PROGRAM"))
            project->values("QMAKE_INSTALL_PROGRAM").append("$(COPY_FILE)");
        if(project->isEmpty("QMAKE_INSTALL_DIR"))
            project->values("QMAKE_INSTALL_DIR").append("$(COPY_DIR)");
        if(project->values("MAKEFILE").isEmpty())
            project->values("MAKEFILE").append("Makefile");
        return;
    }

    project->values("TARGET_PRL").append(project->first("TARGET"));

    processVars();

    if (!project->values("RES_FILE").isEmpty()) {
        project->values("QMAKE_LIBS") += escapeFilePaths(project->values("RES_FILE"));
    }

    // LIBS defined in Profile comes first for gcc
    project->values("QMAKE_LIBS") += escapeFilePaths(project->values("LIBS"));
    project->values("QMAKE_LIBS_PRIVATE") += escapeFilePaths(project->values("LIBS_PRIVATE"));

    QString targetfilename = project->values("TARGET").first();
    QStringList &configs = project->values("CONFIG");

    if(project->isActiveConfig("qt_dll"))
        if(configs.indexOf("qt") == -1)
            configs.append("qt");

    if (project->isActiveConfig("dll")) {
        QString destDir = "";
        if(!project->first("DESTDIR").isEmpty())
            destDir = Option::fixPathToTargetOS(project->first("DESTDIR") + Option::dir_sep, false, false);
        project->values("MINGW_IMPORT_LIB").prepend(destDir + "lib" + project->first("TARGET")
                                                         + project->first("TARGET_VERSION_EXT") + ".a");
	project->values("QMAKE_LFLAGS").append(QString("-Wl,--out-implib,") + project->first("MINGW_IMPORT_LIB"));
    }

    if (!project->values("DEF_FILE").isEmpty()) {
        QString defFileName = fileFixify(project->values("DEF_FILE")).first();
        project->values("QMAKE_LFLAGS").append(QString("-Wl,") + escapeFilePath(defFileName));
    }

    if (project->isActiveConfig("staticlib") && project->first("TEMPLATE") == "lib")
        project->values("QMAKE_LFLAGS").append("-static");

    MakefileGenerator::init();

    // precomp
    if (!project->first("PRECOMPILED_HEADER").isEmpty()
        && project->isActiveConfig("precompile_header")) {
        QString preCompHeader = var("PRECOMPILED_DIR")
		         + QFileInfo(project->first("PRECOMPILED_HEADER")).fileName();
	preCompHeaderOut = preCompHeader + ".gch";
	project->values("QMAKE_CLEAN").append(preCompHeaderOut + Option::dir_sep + "c");
	project->values("QMAKE_CLEAN").append(preCompHeaderOut + Option::dir_sep + "c++");

	project->values("QMAKE_RUN_CC").clear();
	project->values("QMAKE_RUN_CC").append("$(CC) -c -include " + preCompHeader +
                                                    " $(CFLAGS) $(INCPATH) -o $obj $src");
        project->values("QMAKE_RUN_CC_IMP").clear();
	project->values("QMAKE_RUN_CC_IMP").append("$(CC)  -c -include " + preCompHeader +
                                                        " $(CFLAGS) $(INCPATH) -o $@ $<");
        project->values("QMAKE_RUN_CXX").clear();
	project->values("QMAKE_RUN_CXX").append("$(CXX) -c -include " + preCompHeader +
                                                     " $(CXXFLAGS) $(INCPATH) -o $obj $src");
        project->values("QMAKE_RUN_CXX_IMP").clear();
	project->values("QMAKE_RUN_CXX_IMP").append("$(CXX) -c -include " + preCompHeader +
                                                         " $(CXXFLAGS) $(INCPATH) -o $@ $<");
    }

    if(project->isActiveConfig("dll")) {
        project->values("QMAKE_CLEAN").append(project->first("MINGW_IMPORT_LIB"));
    }
}

void MingwMakefileGenerator::writeIncPart(QTextStream &t)
{
    t << "INCPATH       = ";

    const QStringList &incs = project->values("INCLUDEPATH");
    for (QStringList::ConstIterator incit = incs.begin(); incit != incs.end(); ++incit) {
        QString inc = (*incit);
        inc.replace(QRegExp("\\\\$"), "");
        inc.replace(QRegExp("\""), "");
        t << "-I" << quote << inc << quote << " ";
    }
    t << "-I" << quote << specdir() << quote
      << endl;
}

void MingwMakefileGenerator::writeLibsPart(QTextStream &t)
{
    if(project->isActiveConfig("staticlib") && project->first("TEMPLATE") == "lib") {
        t << "LIB        =        " << var("QMAKE_LIB") << endl;
    } else {
        t << "LINK        =        " << var("QMAKE_LINK") << endl;
        t << "LFLAGS        =        " << var("QMAKE_LFLAGS") << endl;
        t << "LIBS        =        ";
        if(!project->values("QMAKE_LIBDIR").isEmpty())
            writeLibDirPart(t);
        t << var("QMAKE_LIBS").replace(QRegExp("(\\slib|^lib)")," -l") << ' '
          << var("QMAKE_LIBS_PRIVATE").replace(QRegExp("(\\slib|^lib)")," -l") << endl;
    }
}

void MingwMakefileGenerator::writeLibDirPart(QTextStream &t)
{
    QStringList libDirs = project->values("QMAKE_LIBDIR");
    for (int i = 0; i < libDirs.size(); ++i) {
        libDirs[i].remove("\"");
        if (libDirs[i].endsWith("\\"))
            libDirs[i].chop(1);
    }
    t << valGlue(libDirs, "-L" + quote, quote + " -L" + quote, quote) << " ";
}

void MingwMakefileGenerator::writeObjectsPart(QTextStream &t)
{
    if (project->values("OBJECTS").count() < var("QMAKE_LINK_OBJECT_MAX").toInt()) {
        objectsLinkLine = "$(OBJECTS)";
    } else if (project->isActiveConfig("staticlib") && project->first("TEMPLATE") == "lib") {
	QString ar_script_file = var("QMAKE_LINK_OBJECT_SCRIPT") + "." + var("TARGET");
	if (!var("BUILD_NAME").isEmpty()) {
	    ar_script_file += "." + var("BUILD_NAME");
	}
        // QMAKE_LIB is used for win32, including mingw, whereas QMAKE_AR is used on Unix.
        if (project->isActiveConfig("rvct_linker")) {
            createRvctObjectScriptFile(ar_script_file, project->values("OBJECTS"));
            QString ar_cmd = project->values("QMAKE_LIB").join(" ");
            if (ar_cmd.isEmpty())
                ar_cmd = "armar --create";
            objectsLinkLine = ar_cmd + " " + var("DEST_TARGET") + " --via " + escapeFilePath(ar_script_file);
        } else {
            // Strip off any options since the ar commands will be read from file.
            QString ar_cmd = var("QMAKE_LIB").section(" ", 0, 0);;
            if (ar_cmd.isEmpty())
                ar_cmd = "ar";
            createArObjectScriptFile(ar_script_file, var("DEST_TARGET"), project->values("OBJECTS"));
            objectsLinkLine = ar_cmd + " -M < " + escapeFilePath(ar_script_file);
        }
    } else {
        QString ld_script_file = var("QMAKE_LINK_OBJECT_SCRIPT") + "." + var("TARGET");
	if (!var("BUILD_NAME").isEmpty()) {
	    ld_script_file += "." + var("BUILD_NAME");
	}
        if (project->isActiveConfig("rvct_linker")) {
            createRvctObjectScriptFile(ld_script_file, project->values("OBJECTS"));
            objectsLinkLine = QString::fromLatin1("--via ") + escapeFilePath(ld_script_file);
        } else {
            createLdObjectScriptFile(ld_script_file, project->values("OBJECTS"));
            objectsLinkLine = escapeFilePath(ld_script_file);
        }
    }
    Win32MakefileGenerator::writeObjectsPart(t);
}

void MingwMakefileGenerator::writeBuildRulesPart(QTextStream &t)
{
    if (project->first("TEMPLATE") == "aux") {
        t << "first:" << endl;
        t << "all:" << endl;
        return;
    }

    t << "first: all" << endl;
    t << "all: " << escapeDependencyPath(fileFixify(Option::output.fileName())) << " " << valGlue(escapeDependencyPaths(project->values("ALL_DEPS"))," "," "," ") << " $(DESTDIR_TARGET)" << endl << endl;
    t << "$(DESTDIR_TARGET): " << var("PRE_TARGETDEPS") << " $(OBJECTS) " << var("POST_TARGETDEPS");
    if(!project->isEmpty("QMAKE_PRE_LINK"))
        t << "\n\t" <<var("QMAKE_PRE_LINK");
    if(project->isActiveConfig("staticlib") && project->first("TEMPLATE") == "lib") {
	if (project->values("OBJECTS").count() < var("QMAKE_LINK_OBJECT_MAX").toInt()) {
            t << "\n\t" << "$(LIB) $(DESTDIR_TARGET) " << objectsLinkLine << " " ;
        } else {
            t << "\n\t" << objectsLinkLine << " " ;
        }
    } else {
        t << "\n\t" << "$(LINK) $(LFLAGS) -o $(DESTDIR_TARGET) " << objectsLinkLine << " " << " $(LIBS)";
    }
    if(!project->isEmpty("QMAKE_POST_LINK"))
        t << "\n\t" <<var("QMAKE_POST_LINK");
    t << endl;
}

void MingwMakefileGenerator::writeRcFilePart(QTextStream &t)
{
    const QString rc_file = fileFixify(project->first("RC_FILE"));

    QString incPathStr = fileInfo(rc_file).path();
    if (incPathStr != "." && QDir::isRelativePath(incPathStr))
        incPathStr.prepend("./");

    if (!rc_file.isEmpty()) {
        t << escapeDependencyPath(var("RES_FILE")) << ": " << rc_file << "\n\t"
          << var("QMAKE_RC") << " -i " << rc_file << " -o " << var("RES_FILE") 
          << " --include-dir=" << incPathStr << " $(DEFINES)" << endl << endl;
    }
}

QStringList &MingwMakefileGenerator::findDependencies(const QString &file)
{
    QStringList &aList = MakefileGenerator::findDependencies(file);
    // Note: The QMAKE_IMAGE_COLLECTION file have all images
    // as dependency, so don't add precompiled header then
    if (file == project->first("QMAKE_IMAGE_COLLECTION")
        || preCompHeaderOut.isEmpty())
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
