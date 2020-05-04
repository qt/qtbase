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

#include "winmakefile.h"
#include "option.h"
#include "project.h"
#include "meta.h"
#include <qtextstream.h>
#include <qstring.h>
#include <qhash.h>
#include <qregexp.h>
#include <qstringlist.h>
#include <qdir.h>
#include <stdlib.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

ProString Win32MakefileGenerator::fixLibFlag(const ProString &lib)
{
    if (lib.startsWith("-l"))  // Fallback for unresolved -l libs.
        return escapeFilePath(lib.mid(2) + QLatin1String(".lib"));
    if (lib.startsWith("-L"))  // Lib search path. Needed only by -l above.
        return QLatin1String("/LIBPATH:")
                + escapeFilePath(Option::fixPathToTargetOS(lib.mid(2).toQString(), false));
    return escapeFilePath(Option::fixPathToTargetOS(lib.toQString(), false));
}

MakefileGenerator::LibFlagType
Win32MakefileGenerator::parseLibFlag(const ProString &flag, ProString *arg)
{
    LibFlagType ret = MakefileGenerator::parseLibFlag(flag, arg);
    if (ret != LibFlagFile)
        return ret;
    // MSVC compatibility. This should be deprecated.
    if (flag.startsWith("/LIBPATH:")) {
        *arg = flag.mid(9);
        return LibFlagPath;
    }
    // These are pure qmake inventions. They *really* should be deprecated.
    if (flag.startsWith("/L")) {
        *arg = flag.mid(2);
        return LibFlagPath;
    }
    if (flag.startsWith("/l")) {
        *arg = flag.mid(2);
        return LibFlagLib;
    }
    return LibFlagFile;
}

class LibrarySearchPath : public QMakeLocalFileName
{
public:
    LibrarySearchPath() = default;

    LibrarySearchPath(const QString &s)
        : QMakeLocalFileName(s)
    {
    }

    LibrarySearchPath(QString &&s, bool isDefault = false)
        : QMakeLocalFileName(std::move(s)), _default(isDefault)
    {
    }

    bool isDefault() const { return _default; }

private:
    bool _default = false;
};

bool
Win32MakefileGenerator::findLibraries(bool linkPrl, bool mergeLflags)
{
    ProStringList impexts = project->values("QMAKE_LIB_EXTENSIONS");
    if (impexts.isEmpty())
        impexts = project->values("QMAKE_EXTENSION_STATICLIB");
    QVector<LibrarySearchPath> dirs;
    int libidx = 0;
    for (const ProString &dlib : project->values("QMAKE_DEFAULT_LIBDIRS"))
        dirs.append(LibrarySearchPath(dlib.toQString(), true));
  static const char * const lflags[] = { "LIBS", "LIBS_PRIVATE",
                                         "QMAKE_LIBS", "QMAKE_LIBS_PRIVATE", nullptr };
  for (int i = 0; lflags[i]; i++) {
    ProStringList &l = project->values(lflags[i]);
    for (ProStringList::Iterator it = l.begin(); it != l.end();) {
        const ProString &opt = *it;
        ProString arg;
        LibFlagType type = parseLibFlag(opt, &arg);
        if (type == LibFlagPath) {
            const QString argqstr = arg.toQString();
            auto dit = std::find_if(dirs.cbegin(), dirs.cend(),
                                 [&argqstr](const LibrarySearchPath &p)
                                 {
                                     return p.real() == argqstr;
                                 });
            int idx = dit == dirs.cend()
                    ? -1
                    : std::distance(dirs.cbegin(), dit);
            if (idx >= 0 && idx < libidx) {
                it = l.erase(it);
                continue;
            }
            const LibrarySearchPath lp(argqstr);
            dirs.insert(libidx++, lp);
            (*it) = "-L" + lp.real();
        } else if (type == LibFlagLib) {
            QString lib = arg.toQString();
            ProString verovr =
                    project->first(ProKey("QMAKE_" + lib.toUpper() + "_VERSION_OVERRIDE"));
            for (auto dir_it = dirs.begin(); dir_it != dirs.end(); ++dir_it) {
                QString cand = (*dir_it).real() + Option::dir_sep + lib;
                if (linkPrl && processPrlFile(cand, true)) {
                    (*it) = cand;
                    goto found;
                }
                QString libBase = (*dir_it).local() + '/' + lib + verovr;
                for (ProStringList::ConstIterator extit = impexts.cbegin();
                     extit != impexts.cend(); ++extit) {
                    if (exists(libBase + '.' + *extit)) {
                        *it = (dir_it->isDefault() ? lib : cand)
                                + verovr + '.' + *extit;
                        goto found;
                    }
                }
            }
            // We assume if it never finds it that it's correct
          found: ;
        } else if (linkPrl && type == LibFlagFile) {
            QString lib = opt.toQString();
            if (fileInfo(lib).isAbsolute()) {
                if (processPrlFile(lib, false))
                    (*it) = lib;
            } else {
                for (auto dir_it = dirs.begin(); dir_it != dirs.end(); ++dir_it) {
                    QString cand = (*dir_it).real() + Option::dir_sep + lib;
                    if (processPrlFile(cand, false)) {
                        (*it) = cand;
                        break;
                    }
                }
            }
        }

        ProStringList &prl_libs = project->values("QMAKE_CURRENT_PRL_LIBS");
        for (int prl = 0; prl < prl_libs.size(); ++prl)
            it = l.insert(++it, prl_libs.at(prl));
        prl_libs.clear();
        ++it;
    }
    if (mergeLflags) {
        ProStringList lopts;
        for (int lit = 0; lit < l.size(); ++lit) {
            ProString opt = l.at(lit);
            if (opt.startsWith(QLatin1String("-L"))) {
                if (!lopts.contains(opt))
                    lopts.append(opt);
            } else {
                // Make sure we keep the dependency order of libraries
                lopts.removeAll(opt);
                lopts.append(opt);
            }
        }
        l = lopts;
    }
  }
    return true;
}

bool Win32MakefileGenerator::processPrlFileBase(QString &origFile, const QStringRef &origName,
                                                const QStringRef &fixedBase, int slashOff)
{
    if (MakefileGenerator::processPrlFileBase(origFile, origName, fixedBase, slashOff))
        return true;
    for (int off = fixedBase.length(); off > slashOff; off--) {
        if (!fixedBase.at(off - 1).isDigit()) {
            if (off != fixedBase.length()) {
                return MakefileGenerator::processPrlFileBase(
                            origFile, origName, fixedBase.left(off), slashOff);
            }
            break;
        }
    }
    return false;
}

void Win32MakefileGenerator::processVars()
{
    if (project->first("TEMPLATE").endsWith("aux"))
        return;

    project->values("PRL_TARGET") =
            project->values("QMAKE_ORIG_TARGET") = project->values("TARGET");
    if (project->isEmpty("QMAKE_PROJECT_NAME"))
        project->values("QMAKE_PROJECT_NAME") = project->values("QMAKE_ORIG_TARGET");
    else if (project->first("TEMPLATE").startsWith("vc"))
        project->values("MAKEFILE") = project->values("QMAKE_PROJECT_NAME");

    project->values("QMAKE_INCDIR") += project->values("QMAKE_INCDIR_POST");
    project->values("QMAKE_LIBDIR") += project->values("QMAKE_LIBDIR_POST");

    if (!project->values("QMAKE_INCDIR").isEmpty())
        project->values("INCLUDEPATH") += project->values("QMAKE_INCDIR");

    if (!project->values("VERSION").isEmpty()) {
        QStringList l = project->first("VERSION").toQString().split('.');
        if (l.size() > 0)
            project->values("VER_MAJ").append(l[0]);
        if (l.size() > 1)
            project->values("VER_MIN").append(l[1]);
    }

    // TARGET_VERSION_EXT will be used to add a version number onto the target name
    if (!project->isActiveConfig("skip_target_version_ext")
        && project->values("TARGET_VERSION_EXT").isEmpty()
        && !project->values("VER_MAJ").isEmpty())
        project->values("TARGET_VERSION_EXT").append(project->first("VER_MAJ"));

    fixTargetExt();
    processRcFileVar();

    ProStringList libs;
    ProStringList &libDir = project->values("QMAKE_LIBDIR");
    for (ProStringList::Iterator libDir_it = libDir.begin(); libDir_it != libDir.end(); ++libDir_it) {
        QString lib = (*libDir_it).toQString();
        if (!lib.isEmpty()) {
            if (lib.endsWith('\\'))
                lib.chop(1);
            libs << QLatin1String("-L") + lib;
        }
    }
    ProStringList &qmklibs = project->values("LIBS");
    qmklibs = libs + qmklibs;

    if (project->values("TEMPLATE").contains("app")) {
        project->values("QMAKE_CFLAGS") += project->values("QMAKE_CFLAGS_APP");
        project->values("QMAKE_CXXFLAGS") += project->values("QMAKE_CXXFLAGS_APP");
        project->values("QMAKE_LFLAGS") += project->values("QMAKE_LFLAGS_APP");
    } else if (project->values("TEMPLATE").contains("lib") && project->isActiveConfig("dll")) {
        if(!project->isActiveConfig("plugin") || !project->isActiveConfig("plugin_no_share_shlib_cflags")) {
            project->values("QMAKE_CFLAGS") += project->values("QMAKE_CFLAGS_SHLIB");
            project->values("QMAKE_CXXFLAGS") += project->values("QMAKE_CXXFLAGS_SHLIB");
        }
        if (project->isActiveConfig("plugin")) {
            project->values("QMAKE_CFLAGS") += project->values("QMAKE_CFLAGS_PLUGIN");
            project->values("QMAKE_CXXFLAGS") += project->values("QMAKE_CXXFLAGS_PLUGIN");
            project->values("QMAKE_LFLAGS") += project->values("QMAKE_LFLAGS_PLUGIN");
        } else {
            project->values("QMAKE_LFLAGS") += project->values("QMAKE_LFLAGS_SHLIB");
        }
    }
}

void Win32MakefileGenerator::fixTargetExt()
{
    if (!project->values("QMAKE_APP_FLAG").isEmpty()) {
        project->values("TARGET_EXT").append(".exe");
    } else if (project->isActiveConfig("shared")) {
        project->values("LIB_TARGET").prepend(project->first("QMAKE_PREFIX_STATICLIB")
                                              + project->first("TARGET") + project->first("TARGET_VERSION_EXT")
                                              + '.' + project->first("QMAKE_EXTENSION_STATICLIB"));
        project->values("TARGET_EXT").append(project->first("TARGET_VERSION_EXT") + "."
                + project->first("QMAKE_EXTENSION_SHLIB"));
        project->values("TARGET").first() = project->first("QMAKE_PREFIX_SHLIB") + project->first("TARGET");
    } else {
        project->values("TARGET_EXT").append("." + project->first("QMAKE_EXTENSION_STATICLIB"));
        project->values("TARGET").first() = project->first("QMAKE_PREFIX_STATICLIB") + project->first("TARGET");
        project->values("LIB_TARGET").prepend(project->first("TARGET") + project->first("TARGET_EXT"));  // for the .prl only
    }
}

void Win32MakefileGenerator::processRcFileVar()
{
    if (Option::qmake_mode == Option::QMAKE_GENERATE_NOTHING)
        return;

    const QString manifestFile = project->first("QMAKE_MANIFEST").toQString();
    if (((!project->values("VERSION").isEmpty() || !project->values("RC_ICONS").isEmpty() || !manifestFile.isEmpty())
        && project->values("RC_FILE").isEmpty()
        && project->values("RES_FILE").isEmpty()
        && !project->isActiveConfig("no_generated_target_info")
        && (project->isActiveConfig("shared") || !project->values("QMAKE_APP_FLAG").isEmpty()))
        || !project->values("QMAKE_WRITE_DEFAULT_RC").isEmpty()){

        QByteArray rcString;
        QTextStream ts(&rcString, QFile::WriteOnly);

        QStringList vers = project->first("VERSION").toQString().split(".", Qt::SkipEmptyParts);
        for (int i = vers.size(); i < 4; i++)
            vers += "0";
        QString versionString = vers.join('.');

        QStringList rcIcons;
        const auto icons = project->values("RC_ICONS");
        rcIcons.reserve(icons.size());
        for (const ProString &icon : icons)
            rcIcons.append(fileFixify(icon.toQString(), FileFixifyAbsolute));

        QString companyName;
        if (!project->values("QMAKE_TARGET_COMPANY").isEmpty())
            companyName = project->values("QMAKE_TARGET_COMPANY").join(' ');

        QString description;
        if (!project->values("QMAKE_TARGET_DESCRIPTION").isEmpty())
            description = project->values("QMAKE_TARGET_DESCRIPTION").join(' ');

        QString copyright;
        if (!project->values("QMAKE_TARGET_COPYRIGHT").isEmpty())
            copyright = project->values("QMAKE_TARGET_COPYRIGHT").join(' ');

        QString productName;
        if (!project->values("QMAKE_TARGET_PRODUCT").isEmpty())
            productName = project->values("QMAKE_TARGET_PRODUCT").join(' ');
        else
            productName = project->first("TARGET").toQString();

        QString originalName = project->first("TARGET") + project->first("TARGET_EXT");
        int rcLang = project->intValue("RC_LANG", 1033);            // default: English(USA)
        int rcCodePage = project->intValue("RC_CODEPAGE", 1200);    // default: Unicode

        ts << "#include <windows.h>\n";
        ts << Qt::endl;
        if (!rcIcons.isEmpty()) {
            for (int i = 0; i < rcIcons.size(); ++i)
                ts << QString("IDI_ICON%1\tICON\tDISCARDABLE\t%2").arg(i + 1).arg(cQuoted(rcIcons[i])) << Qt::endl;
            ts << Qt::endl;
        }
        if (!manifestFile.isEmpty()) {
            QString manifestResourceId;
            if (project->first("TEMPLATE") == "lib")
                manifestResourceId = QStringLiteral("ISOLATIONAWARE_MANIFEST_RESOURCE_ID");
            else
                manifestResourceId = QStringLiteral("CREATEPROCESS_MANIFEST_RESOURCE_ID");
            ts << manifestResourceId << " RT_MANIFEST \"" << manifestFile << "\"\n";
        }
        ts << "VS_VERSION_INFO VERSIONINFO\n";
        ts << "\tFILEVERSION " << QString(versionString).replace(".", ",") << Qt::endl;
        ts << "\tPRODUCTVERSION " << QString(versionString).replace(".", ",") << Qt::endl;
        ts << "\tFILEFLAGSMASK 0x3fL\n";
        ts << "#ifdef _DEBUG\n";
        ts << "\tFILEFLAGS VS_FF_DEBUG\n";
        ts << "#else\n";
        ts << "\tFILEFLAGS 0x0L\n";
        ts << "#endif\n";
        ts << "\tFILEOS VOS__WINDOWS32\n";
        if (project->isActiveConfig("shared"))
            ts << "\tFILETYPE VFT_DLL\n";
        else
            ts << "\tFILETYPE VFT_APP\n";
        ts << "\tFILESUBTYPE 0x0L\n";
        ts << "\tBEGIN\n";
        ts << "\t\tBLOCK \"StringFileInfo\"\n";
        ts << "\t\tBEGIN\n";
        ts << "\t\t\tBLOCK \""
           << QString("%1%2").arg(rcLang, 4, 16, QLatin1Char('0')).arg(rcCodePage, 4, 16, QLatin1Char('0'))
           << "\"\n";
        ts << "\t\t\tBEGIN\n";
        ts << "\t\t\t\tVALUE \"CompanyName\", \"" << companyName << "\\0\"\n";
        ts << "\t\t\t\tVALUE \"FileDescription\", \"" <<  description << "\\0\"\n";
        ts << "\t\t\t\tVALUE \"FileVersion\", \"" << versionString << "\\0\"\n";
        ts << "\t\t\t\tVALUE \"LegalCopyright\", \"" << copyright << "\\0\"\n";
        ts << "\t\t\t\tVALUE \"OriginalFilename\", \"" << originalName << "\\0\"\n";
        ts << "\t\t\t\tVALUE \"ProductName\", \"" << productName << "\\0\"\n";
        ts << "\t\t\t\tVALUE \"ProductVersion\", \"" << versionString << "\\0\"\n";
        ts << "\t\t\tEND\n";
        ts << "\t\tEND\n";
        ts << "\t\tBLOCK \"VarFileInfo\"\n";
        ts << "\t\tBEGIN\n";
        ts << "\t\t\tVALUE \"Translation\", "
           << QString("0x%1").arg(rcLang, 4, 16, QLatin1Char('0'))
           << ", " << QString("%1").arg(rcCodePage, 4) << Qt::endl;
        ts << "\t\tEND\n";
        ts << "\tEND\n";
        ts << "/* End of Version info */\n";
        ts << Qt::endl;

        ts.flush();


        QString rcFilename = project->first("OUT_PWD")
                           + "/"
                           + project->first("TARGET")
                           + "_resource"
                           + ".rc";
        QFile rcFile(QDir::cleanPath(rcFilename));

        bool writeRcFile = true;
        if (rcFile.exists() && rcFile.open(QFile::ReadOnly)) {
            writeRcFile = rcFile.readAll() != rcString;
            rcFile.close();
        }
        if (writeRcFile) {
            bool ok;
            ok = rcFile.open(QFile::WriteOnly);
            if (!ok) {
                // The file can't be opened... try creating the containing
                // directory first (needed for clean shadow builds)
                QDir().mkpath(QFileInfo(rcFile).path());
                ok = rcFile.open(QFile::WriteOnly);
            }
            if (!ok) {
                ::fprintf(stderr, "Cannot open for writing: %s", rcFile.fileName().toLatin1().constData());
                ::exit(1);
            }
            rcFile.write(rcString);
            rcFile.close();
        }
        if (project->values("QMAKE_WRITE_DEFAULT_RC").isEmpty())
            project->values("RC_FILE").insert(0, rcFile.fileName());
    }
    if (!project->values("RC_FILE").isEmpty()) {
        if (!project->values("RES_FILE").isEmpty()) {
            fprintf(stderr, "Both rc and res file specified.\n");
            fprintf(stderr, "Please specify one of them, not both.");
            exit(1);
        }
        QString resFile = project->first("RC_FILE").toQString();

        // if this is a shadow build then use the absolute path of the rc file
        if (Option::output_dir != qmake_getpwd()) {
            QFileInfo fi(resFile);
            project->values("RC_FILE").first() = fi.absoluteFilePath();
        }

        resFile.replace(QLatin1String(".rc"), Option::res_ext);
        project->values("RES_FILE").prepend(fileInfo(resFile).fileName());
        QString resDestDir;
        if (project->isActiveConfig("staticlib"))
            resDestDir = project->first("DESTDIR").toQString();
        else
            resDestDir = project->first("OBJECTS_DIR").toQString();
        if (!resDestDir.isEmpty()) {
            resDestDir.append(Option::dir_sep);
            project->values("RES_FILE").first().prepend(resDestDir);
        }
        project->values("RES_FILE").first() = Option::fixPathToTargetOS(
                    project->first("RES_FILE").toQString(), false);
        project->values("POST_TARGETDEPS") += project->values("RES_FILE");
        project->values("CLEAN_FILES") += project->values("RES_FILE");
    }
}

void Win32MakefileGenerator::writeCleanParts(QTextStream &t)
{
    t << "clean: compiler_clean " << depVar("CLEAN_DEPS");
    {
        const char *clean_targets[] = { "OBJECTS", "QMAKE_CLEAN", "CLEAN_FILES", nullptr };
        for(int i = 0; clean_targets[i]; ++i) {
            const ProStringList &list = project->values(clean_targets[i]);
            const QString del_statement("-$(DEL_FILE)");
            if(project->isActiveConfig("no_delete_multiple_files")) {
                for (ProStringList::ConstIterator it = list.begin(); it != list.end(); ++it)
                    t << "\n\t" << del_statement
                      << ' ' << escapeFilePath(Option::fixPathToTargetOS((*it).toQString()));
            } else {
                QString files, file;
                const int commandlineLimit = 2047; // NT limit, expanded
                for (ProStringList::ConstIterator it = list.begin(); it != list.end(); ++it) {
                    file = ' ' + escapeFilePath(Option::fixPathToTargetOS((*it).toQString()));
                    if(del_statement.length() + files.length() +
                       qMax(fixEnvVariables(file).length(), file.length()) > commandlineLimit) {
                        t << "\n\t" << del_statement << files;
                        files.clear();
                    }
                    files += file;
                }
                if(!files.isEmpty())
                    t << "\n\t" << del_statement << files;
            }
        }
    }
    t << Qt::endl << Qt::endl;

    t << "distclean: clean " << depVar("DISTCLEAN_DEPS");
    {
        const char *clean_targets[] = { "QMAKE_DISTCLEAN", nullptr };
        for(int i = 0; clean_targets[i]; ++i) {
            const ProStringList &list = project->values(clean_targets[i]);
            const QString del_statement("-$(DEL_FILE)");
            if(project->isActiveConfig("no_delete_multiple_files")) {
                for (ProStringList::ConstIterator it = list.begin(); it != list.end(); ++it)
                    t << "\n\t" << del_statement << " "
                      << escapeFilePath(Option::fixPathToTargetOS((*it).toQString()));
            } else {
                QString files, file;
                const int commandlineLimit = 2047; // NT limit, expanded
                for (ProStringList::ConstIterator it = list.begin(); it != list.end(); ++it) {
                    file = " " + escapeFilePath(Option::fixPathToTargetOS((*it).toQString()));
                    if(del_statement.length() + files.length() +
                       qMax(fixEnvVariables(file).length(), file.length()) > commandlineLimit) {
                        t << "\n\t" << del_statement << files;
                        files.clear();
                    }
                    files += file;
                }
                if(!files.isEmpty())
                    t << "\n\t" << del_statement << files;
            }
        }
    }
    t << "\n\t-$(DEL_FILE) $(DESTDIR_TARGET)\n";
    {
        QString ofile = fileFixify(Option::output.fileName());
        if(!ofile.isEmpty())
            t << "\t-$(DEL_FILE) " << escapeFilePath(ofile) << Qt::endl;
    }
    t << Qt::endl;
}

void Win32MakefileGenerator::writeIncPart(QTextStream &t)
{
    t << "INCPATH       = ";

    const ProStringList &incs = project->values("INCLUDEPATH");
    for(int i = 0; i < incs.size(); ++i) {
        QString inc = incs.at(i).toQString();
        inc.replace(QRegExp("\\\\$"), "");
        if(!inc.isEmpty())
            t << "-I" << escapeFilePath(inc) << ' ';
    }
    t << Qt::endl;
}

void Win32MakefileGenerator::writeStandardParts(QTextStream &t)
{
    writeExportedVariables(t);

    t << "####### Compiler, tools and options\n\n";
    t << "CC            = " << var("QMAKE_CC") << Qt::endl;
    t << "CXX           = " << var("QMAKE_CXX") << Qt::endl;
    t << "DEFINES       = "
      << varGlue("PRL_EXPORT_DEFINES","-D"," -D"," ")
      << varGlue("DEFINES","-D"," -D","") << Qt::endl;
    t << "CFLAGS        = " << var("QMAKE_CFLAGS") << " $(DEFINES)\n";
    t << "CXXFLAGS      = " << var("QMAKE_CXXFLAGS") << " $(DEFINES)\n";

    writeIncPart(t);
    writeLibsPart(t);
    writeDefaultVariables(t);
    t << Qt::endl;

    t << "####### Output directory\n\n";
    if(!project->values("OBJECTS_DIR").isEmpty())
        t << "OBJECTS_DIR   = " << escapeFilePath(var("OBJECTS_DIR").remove(QRegExp("\\\\$"))) << Qt::endl;
    else
        t << "OBJECTS_DIR   = . \n";
    t << Qt::endl;

    t << "####### Files\n\n";
    t << "SOURCES       = " << valList(escapeFilePaths(project->values("SOURCES")))
      << " " << valList(escapeFilePaths(project->values("GENERATED_SOURCES"))) << Qt::endl;

    // do this here so we can set DEST_TARGET to be the complete path to the final target if it is needed.
    QString orgDestDir = var("DESTDIR");
    QString destDir = Option::fixPathToTargetOS(orgDestDir, false);
    if (!destDir.isEmpty() && (orgDestDir.endsWith('/') || orgDestDir.endsWith(Option::dir_sep)))
        destDir += Option::dir_sep;
    QString target = QString(project->first("TARGET")+project->first("TARGET_EXT"));
    project->values("DEST_TARGET").prepend(destDir + target);

    writeObjectsPart(t);

    writeExtraCompilerVariables(t);
    writeExtraVariables(t);

    t << "DIST          = " << fileVarList("DISTFILES") << ' '
      << fileVarList("HEADERS") << ' ' << fileVarList("SOURCES") << Qt::endl;
    t << "QMAKE_TARGET  = " << fileVar("QMAKE_ORIG_TARGET") << Qt::endl;  // unused
    // The comment is important to maintain variable compatibility with Unix
    // Makefiles, while not interpreting a trailing-slash as a linebreak
    t << "DESTDIR        = " << escapeFilePath(destDir) << " #avoid trailing-slash linebreak\n";
    t << "TARGET         = " << escapeFilePath(target) << Qt::endl;
    t << "DESTDIR_TARGET = " << fileVar("DEST_TARGET") << Qt::endl;
    t << Qt::endl;

    writeImplicitRulesPart(t);

    t << "####### Build rules\n\n";
    writeBuildRulesPart(t);

    if (project->first("TEMPLATE") != "aux") {
        if (project->isActiveConfig("shared") && !project->values("DLLDESTDIR").isEmpty()) {
            const ProStringList &dlldirs = project->values("DLLDESTDIR");
            for (ProStringList::ConstIterator dlldir = dlldirs.begin(); dlldir != dlldirs.end(); ++dlldir) {
                t << "\t-$(COPY_FILE) $(DESTDIR_TARGET) "
                  << escapeFilePath(Option::fixPathToTargetOS((*dlldir).toQString(), false)) << Qt::endl;
            }
        }
        t << Qt::endl;

        writeRcFilePart(t);
    }

    writeMakeQmake(t);

    QStringList dist_files = fileFixify(Option::mkfile::project_files);
    if(!project->isEmpty("QMAKE_INTERNAL_INCLUDED_FILES"))
        dist_files += project->values("QMAKE_INTERNAL_INCLUDED_FILES").toQStringList();
    if(!project->isEmpty("TRANSLATIONS"))
        dist_files << var("TRANSLATIONS");
    if(!project->isEmpty("FORMS")) {
        const ProStringList &forms = project->values("FORMS");
        for (ProStringList::ConstIterator formit = forms.begin(); formit != forms.end(); ++formit) {
            QString ui_h = fileFixify((*formit) + Option::h_ext.first());
            if(exists(ui_h))
                dist_files << ui_h;
        }
    }
    t << "dist:\n\t"
      << "$(ZIP) " << var("QMAKE_ORIG_TARGET") << ".zip $(SOURCES) $(DIST) "
      << escapeFilePaths(dist_files).join(' ') << ' ' << fileVar("TRANSLATIONS") << ' ';
    if(!project->isEmpty("QMAKE_EXTRA_COMPILERS")) {
        const ProStringList &quc = project->values("QMAKE_EXTRA_COMPILERS");
        for (ProStringList::ConstIterator it = quc.begin(); it != quc.end(); ++it) {
            const ProStringList &inputs = project->values(ProKey(*it + ".input"));
            for (ProStringList::ConstIterator input = inputs.begin(); input != inputs.end(); ++input) {
                const ProStringList &val = project->values((*input).toKey());
                t << escapeFilePaths(val).join(' ') << ' ';
            }
        }
    }
    t << Qt::endl << Qt::endl;

    writeCleanParts(t);
    writeExtraTargets(t);
    writeExtraCompilerTargets(t);
    t << Qt::endl << Qt::endl;
}

void Win32MakefileGenerator::writeLibsPart(QTextStream &t)
{
    if(project->isActiveConfig("staticlib") && project->first("TEMPLATE") == "lib") {
        t << "LIBAPP        = " << var("QMAKE_LIB") << Qt::endl;
        t << "LIBFLAGS      = " << var("QMAKE_LIBFLAGS") << Qt::endl;
    } else {
        t << "LINKER        = " << var("QMAKE_LINK") << Qt::endl;
        t << "LFLAGS        = " << var("QMAKE_LFLAGS") << Qt::endl;
        t << "LIBS          = " << fixLibFlags("LIBS").join(' ') << ' '
                                << fixLibFlags("LIBS_PRIVATE").join(' ') << ' '
                                << fixLibFlags("QMAKE_LIBS").join(' ') << ' '
                                << fixLibFlags("QMAKE_LIBS_PRIVATE").join(' ') << Qt::endl;
    }
}

void Win32MakefileGenerator::writeObjectsPart(QTextStream &t)
{
    // Used in both deps and commands.
    t << "OBJECTS       = " << valList(escapeDependencyPaths(project->values("OBJECTS"))) << Qt::endl;
}

void Win32MakefileGenerator::writeImplicitRulesPart(QTextStream &)
{
}

void Win32MakefileGenerator::writeBuildRulesPart(QTextStream &)
{
}

void Win32MakefileGenerator::writeRcFilePart(QTextStream &t)
{
    if(!project->values("RC_FILE").isEmpty()) {
        const ProString res_file = project->first("RES_FILE");
        const QString rc_file = fileFixify(project->first("RC_FILE").toQString());

        const ProStringList rcIncPaths = project->values("RC_INCLUDEPATH");
        QString incPathStr;
        for (int i = 0; i < rcIncPaths.count(); ++i) {
            const ProString &path = rcIncPaths.at(i);
            if (path.isEmpty())
                continue;
            incPathStr += QStringLiteral(" /i ");
            incPathStr += escapeFilePath(path);
        }

        addSourceFile(rc_file, QMakeSourceFileInfo::SEEK_DEPS);
        const QStringList rcDeps = QStringList(rc_file) << dependencies(rc_file);

        // The resource tool may use defines. This might be the same defines passed in as the
        // compiler, since you may use these defines in the .rc file itself.
        // As the escape syntax for the command line defines for RC is different from that for CL,
        // we might have to set specific defines for RC.
        ProString defines = varGlue("RC_DEFINES", " -D", " -D", "");
        if (defines.isEmpty())
            defines = ProString(" $(DEFINES)");

        // Also, we need to add the _DEBUG define manually since the compiler defines this symbol
        // by itself, and we use it in the automatically created rc file when VERSION is defined
        // in the .pro file.
        t << escapeDependencyPath(res_file) << ": "
          << escapeDependencyPaths(rcDeps).join(' ') << "\n\t"
          << var("QMAKE_RC") << (project->isActiveConfig("debug") ? " -D_DEBUG" : "")
          << defines << incPathStr << " -fo " << escapeFilePath(res_file)
          << ' ' << escapeFilePath(rc_file);
        t << Qt::endl << Qt::endl;
    }
}

QString Win32MakefileGenerator::defaultInstall(const QString &t)
{
    if((t != "target" && t != "dlltarget") ||
       (t == "dlltarget" && (project->first("TEMPLATE") != "lib" || !project->isActiveConfig("shared"))) ||
        project->first("TEMPLATE") == "subdirs" || project->first("TEMPLATE") == "aux")
       return QString();

    const QString root = installRoot();
    ProStringList &uninst = project->values(ProKey(t + ".uninstall"));
    QString ret;
    QString targetdir = fileFixify(project->first(ProKey(t + ".path")).toQString(), FileFixifyAbsolute);
    if(targetdir.right(1) != Option::dir_sep)
        targetdir += Option::dir_sep;

    const ProStringList &targets = project->values(ProKey(t + ".targets"));
    for (int i = 0; i < targets.size(); ++i) {
        QString src = targets.at(i).toQString(),
                dst = escapeFilePath(filePrefixRoot(root, targetdir + src.section('/', -1)));
        if (!ret.isEmpty())
            ret += "\n\t";
        ret += "$(QINSTALL) " + escapeFilePath(Option::fixPathToTargetOS(src, false)) + ' ' + dst;
        if (!uninst.isEmpty())
            uninst.append("\n\t");
        uninst.append("-$(DEL_FILE) " + dst);
    }

    if(t == "target" && project->first("TEMPLATE") == "lib") {
        if(project->isActiveConfig("create_prl") && !project->isActiveConfig("no_install_prl") &&
           !project->isEmpty("QMAKE_INTERNAL_PRL_FILE")) {
            QString dst_prl = Option::fixPathToTargetOS(project->first("QMAKE_INTERNAL_PRL_FILE").toQString());
            int slsh = dst_prl.lastIndexOf(Option::dir_sep);
            if(slsh != -1)
                dst_prl = dst_prl.right(dst_prl.length() - slsh - 1);
            dst_prl = filePrefixRoot(root, targetdir + dst_prl);
            if (!ret.isEmpty())
                ret += "\n\t";
            ret += installMetaFile(ProKey("QMAKE_PRL_INSTALL_REPLACE"), project->first("QMAKE_INTERNAL_PRL_FILE").toQString(), dst_prl);
            if(!uninst.isEmpty())
                uninst.append("\n\t");
            uninst.append("-$(DEL_FILE) " + escapeFilePath(dst_prl));
        }
        if(project->isActiveConfig("create_pc")) {
            QString dst_pc = pkgConfigFileName(false);
            if (!dst_pc.isEmpty()) {
                dst_pc = filePrefixRoot(root, targetdir + dst_pc);
                const QString dst_pc_dir = Option::fixPathToTargetOS(fileInfo(dst_pc).path(), false);
                if (!dst_pc_dir.isEmpty()) {
                    if (!ret.isEmpty())
                        ret += "\n\t";
                    ret += mkdir_p_asstring(dst_pc_dir, true);
                }
                if(!ret.isEmpty())
                    ret += "\n\t";
                ret += installMetaFile(ProKey("QMAKE_PKGCONFIG_INSTALL_REPLACE"), pkgConfigFileName(true), dst_pc);
                if(!uninst.isEmpty())
                    uninst.append("\n\t");
                uninst.append("-$(DEL_FILE) " + escapeFilePath(dst_pc));
            }
        }
        if(project->isActiveConfig("shared") && !project->isActiveConfig("plugin")) {
            ProString lib_target = project->first("LIB_TARGET");
            QString src_targ = escapeFilePath(
                    (project->isEmpty("DESTDIR") ? QString("$(DESTDIR)") : project->first("DESTDIR"))
                    + lib_target);
            QString dst_targ = escapeFilePath(
                    filePrefixRoot(root, fileFixify(targetdir + lib_target, FileFixifyAbsolute)));
            if(!ret.isEmpty())
                ret += "\n\t";
            ret += QString("-$(INSTALL_FILE) ") + src_targ + ' ' + dst_targ;
            if(!uninst.isEmpty())
                uninst.append("\n\t");
            uninst.append("-$(DEL_FILE) " + dst_targ);
        }
    }

    if (t == "dlltarget" || project->values(ProKey(t + ".CONFIG")).indexOf("no_dll") == -1) {
        QString src_targ = "$(DESTDIR_TARGET)";
        QString dst_targ = escapeFilePath(
                filePrefixRoot(root, fileFixify(targetdir + "$(TARGET)", FileFixifyAbsolute)));
        if(!ret.isEmpty())
            ret += "\n\t";
        ret += QString("-$(INSTALL_FILE) ") + src_targ + ' ' + dst_targ;
        if(!uninst.isEmpty())
            uninst.append("\n\t");
        uninst.append("-$(DEL_FILE) " + dst_targ);
    }
    return ret;
}

void Win32MakefileGenerator::writeDefaultVariables(QTextStream &t)
{
    MakefileGenerator::writeDefaultVariables(t);
    t << "IDC           = " << (project->isEmpty("QMAKE_IDC") ? QString("idc") : var("QMAKE_IDC"))
                            << Qt::endl;
    t << "IDL           = " << (project->isEmpty("QMAKE_IDL") ? QString("midl") : var("QMAKE_IDL"))
                            << Qt::endl;
    t << "ZIP           = " << var("QMAKE_ZIP") << Qt::endl;
    t << "DEF_FILE      = " << fileVar("DEF_FILE") << Qt::endl;
    t << "RES_FILE      = " << fileVar("RES_FILE") << Qt::endl; // Not on mingw, can't see why not though...
    t << "SED           = " << var("QMAKE_STREAM_EDITOR") << Qt::endl;
    t << "MOVE          = " << var("QMAKE_MOVE") << Qt::endl;
}

QString Win32MakefileGenerator::escapeFilePath(const QString &path) const
{
    QString ret = path;
    if(!ret.isEmpty()) {
        if (ret.contains(' ') || ret.contains('\t'))
            ret = "\"" + ret + "\"";
        debug_msg(2, "EscapeFilePath: %s -> %s", path.toLatin1().constData(), ret.toLatin1().constData());
    }
    return ret;
}

QString Win32MakefileGenerator::escapeDependencyPath(const QString &path) const
{
    QString ret = path;
    if (!ret.isEmpty()) {
        static const QRegExp criticalChars(QStringLiteral("([\t #])"));
        if (ret.contains(criticalChars))
            ret = "\"" + ret + "\"";
        debug_msg(2, "EscapeDependencyPath: %s -> %s", path.toLatin1().constData(), ret.toLatin1().constData());
    }
    return ret;
}

QString Win32MakefileGenerator::cQuoted(const QString &str)
{
    QString ret = str;
    ret.replace(QLatin1Char('\\'), QLatin1String("\\\\"));
    ret.replace(QLatin1Char('"'), QLatin1String("\\\""));
    ret.prepend(QLatin1Char('"'));
    ret.append(QLatin1Char('"'));
    return ret;
}

ProKey Win32MakefileGenerator::fullTargetVariable() const
{
    return "DEST_TARGET";
}

QT_END_NAMESPACE
