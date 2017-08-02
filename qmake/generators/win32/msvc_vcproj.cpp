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

#include "msvc_vcproj.h"
#include "option.h"
#include "xmloutput.h"

#include <ioutils.h>

#include <qdir.h>
#include <qdiriterator.h>
#include <qcryptographichash.h>
#include <qhash.h>
#include <quuid.h>

#include <stdlib.h>

//#define DEBUG_SOLUTION_GEN

using namespace QMakeInternal;

QT_BEGIN_NAMESPACE
// Filter GUIDs (Do NOT change these!) ------------------------------
const char _GUIDSourceFiles[]          = "{4FC737F1-C7A5-4376-A066-2A32D752A2FF}";
const char _GUIDHeaderFiles[]          = "{93995380-89BD-4b04-88EB-625FBE52EBFB}";
const char _GUIDGeneratedFiles[]       = "{71ED8ED8-ACB9-4CE9-BBE1-E00B30144E11}";
const char _GUIDResourceFiles[]        = "{D9D6E242-F8AF-46E4-B9FD-80ECBC20BA3E}";
const char _GUIDLexYaccFiles[]         = "{E12AE0D2-192F-4d59-BD23-7D3FA58D3183}";
const char _GUIDTranslationFiles[]     = "{639EADAA-A684-42e4-A9AD-28FC9BCB8F7C}";
const char _GUIDFormFiles[]            = "{99349809-55BA-4b9d-BF79-8FDBB0286EB3}";
const char _GUIDExtraCompilerFiles[]   = "{E0D8C965-CC5F-43d7-AD63-FAEF0BBC0F85}";
const char _GUIDDeploymentFiles[]      = "{D9D6E243-F8AF-46E4-B9FD-80ECBC20BA3E}";
const char _GUIDDistributionFiles[]    = "{B83CAF91-C7BF-462F-B76C-EA11631F866C}";

// Flatfile Tags ----------------------------------------------------
const char _slnHeader70[]       = "Microsoft Visual Studio Solution File, Format Version 7.00";
const char _slnHeader71[]       = "Microsoft Visual Studio Solution File, Format Version 8.00";
const char _slnHeader80[]       = "Microsoft Visual Studio Solution File, Format Version 9.00"
                                  "\n# Visual Studio 2005";
const char _slnHeader90[]       = "Microsoft Visual Studio Solution File, Format Version 10.00"
                                  "\n# Visual Studio 2008";
const char _slnHeader100[]      = "Microsoft Visual Studio Solution File, Format Version 11.00"
                                  "\n# Visual Studio 2010";
const char _slnHeader110[]      = "Microsoft Visual Studio Solution File, Format Version 12.00"
                                  "\n# Visual Studio 2012";
const char _slnHeader120[]      = "Microsoft Visual Studio Solution File, Format Version 12.00"
                                  "\n# Visual Studio 2013";
const char _slnHeader140[]      = "Microsoft Visual Studio Solution File, Format Version 12.00"
                                  "\n# Visual Studio 2015";
const char _slnHeader141[]      = "Microsoft Visual Studio Solution File, Format Version 12.00"
                                  "\n# Visual Studio 2017";
                                  // The following UUID _may_ change for later servicepacks...
                                  // If so we need to search through the registry at
                                  // HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\VisualStudio\7.0\Projects
                                  // to find the subkey that contains a "PossibleProjectExtension"
                                  // containing "vcproj"...
                                  // Use the hardcoded value for now so projects generated on other
                                  // platforms are actually usable.
const char _slnMSVCvcprojGUID[] = "{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}";
const char _slnProjectBeg[]     = "\nProject(\"";
const char _slnProjectMid[]     = "\") = ";
const char _slnProjectEnd[]     = "\nEndProject";
const char _slnGlobalBeg[]      = "\nGlobal";
const char _slnGlobalEnd[]      = "\nEndGlobal";
const char _slnSolutionConf[]   = "\n\tGlobalSection(SolutionConfigurationPlatforms) = preSolution"
                                  "\n\t\tDebug|Win32 = Debug|Win32"
                                  "\n\t\tRelease|Win32 = Release|Win32"
                                  "\n\tEndGlobalSection";

const char _slnProjDepBeg[]     = "\n\tProjectSection(ProjectDependencies) = postProject";
const char _slnProjDepEnd[]     = "\n\tEndProjectSection";
const char _slnProjConfBeg[]    = "\n\tGlobalSection(ProjectConfigurationPlatforms) = postSolution";
const char _slnProjRelConfTag1[]= ".Release|%1.ActiveCfg = Release|";
const char _slnProjRelConfTag2[]= ".Release|%1.Build.0 = Release|";
const char _slnProjRelConfTag3[]= ".Release|%1.Deploy.0 = Release|";
const char _slnProjDbgConfTag1[]= ".Debug|%1.ActiveCfg = Debug|";
const char _slnProjDbgConfTag2[]= ".Debug|%1.Build.0 = Debug|";
const char _slnProjDbgConfTag3[]= ".Debug|%1.Deploy.0 = Debug|";
const char _slnProjConfEnd[]    = "\n\tEndGlobalSection";
const char _slnExtSections[]    = "\n\tGlobalSection(ExtensibilityGlobals) = postSolution"
                                  "\n\tEndGlobalSection"
                                  "\n\tGlobalSection(ExtensibilityAddIns) = postSolution"
                                  "\n\tEndGlobalSection";
// ------------------------------------------------------------------

VcprojGenerator::VcprojGenerator()
    : Win32MakefileGenerator(),
      is64Bit(false),
      customBuildToolFilterFileSuffix(QStringLiteral(".cbt")),
      projectWriter(0)
{
}

VcprojGenerator::~VcprojGenerator()
{
    delete projectWriter;
}

bool VcprojGenerator::writeMakefile(QTextStream &t)
{
    initProject(); // Fills the whole project with proper data

    // Generate solution file
    if(project->first("TEMPLATE") == "vcsubdirs") {
        if (!project->isActiveConfig("build_pass")) {
            debug_msg(1, "Generator: MSVC.NET: Writing solution file");
            writeSubDirs(t);
        } else {
            debug_msg(1, "Generator: MSVC.NET: Not writing solution file for build_pass configs");
        }
        return true;
    } else
    // Generate single configuration project file
    if (project->first("TEMPLATE") == "vcapp" ||
        project->first("TEMPLATE") == "vclib") {
        if(!project->isActiveConfig("build_pass")) {
            debug_msg(1, "Generator: MSVC.NET: Writing single configuration project file");
            XmlOutput xmlOut(t);
            projectWriter->write(xmlOut, vcProject);
        }
        return true;
    }
    return project->isActiveConfig("build_pass");
}

bool VcprojGenerator::writeProjectMakefile()
{
    QTextStream t(&Option::output);

    // Check if all requirements are fulfilled
    if(!project->values("QMAKE_FAILED_REQUIREMENTS").isEmpty()) {
        fprintf(stderr, "Project file not generated because all requirements not met:\n\t%s\n",
                var("QMAKE_FAILED_REQUIREMENTS").toLatin1().constData());
        return true;
    }

    // Generate project file
    if(project->first("TEMPLATE") == "vcapp" ||
       project->first("TEMPLATE") == "vclib") {
        if (!mergedProjects.count()) {
            warn_msg(WarnLogic, "Generator: MSVC.NET: no single configuration created, cannot output project!");
            return false;
        }

        debug_msg(1, "Generator: MSVC.NET: Writing project file");
        VCProject mergedProject;
        for (int i = 0; i < mergedProjects.count(); ++i) {
            VCProjectSingleConfig *singleProject = &(mergedProjects.at(i)->vcProject);
            mergedProject.SingleProjects += *singleProject;
            for (int j = 0; j < singleProject->ExtraCompilersFiles.count(); ++j) {
                const QString &compilerName = singleProject->ExtraCompilersFiles.at(j).Name;
                if (!mergedProject.ExtraCompilers.contains(compilerName))
                    mergedProject.ExtraCompilers += compilerName;
            }
        }

        if(mergedProjects.count() > 1 &&
           mergedProjects.at(0)->vcProject.Name ==
           mergedProjects.at(1)->vcProject.Name)
            mergedProjects.at(0)->writePrlFile();
        mergedProject.Name = project->first("QMAKE_PROJECT_NAME").toQString();
        mergedProject.Version = mergedProjects.at(0)->vcProject.Version;
        mergedProject.SdkVersion = mergedProjects.at(0)->vcProject.SdkVersion;
        mergedProject.ProjectGUID = project->isEmpty("QMAKE_UUID") ? getProjectUUID().toString().toUpper() : project->first("QMAKE_UUID").toQString();
        mergedProject.Keyword = project->first("VCPROJ_KEYWORD").toQString();
        mergedProject.SccProjectName = mergedProjects.at(0)->vcProject.SccProjectName;
        mergedProject.SccLocalPath = mergedProjects.at(0)->vcProject.SccLocalPath;
        mergedProject.PlatformName = mergedProjects.at(0)->vcProject.PlatformName;

        XmlOutput xmlOut(t);
        projectWriter->write(xmlOut, mergedProject);
        return true;
    } else if(project->first("TEMPLATE") == "vcsubdirs") {
        return writeMakefile(t);
    }
    return false;
}

struct VcsolutionDepend {
    QString uuid;
    QString vcprojFile, orig_target, target;
    Target targetType;
    QStringList dependencies;
};

/* Disable optimization in getProjectUUID() due to a compiler
 * bug in MSVC 2010 that causes ASSERT: "&other != this" in the QString
 * copy constructor for non-empty file names at:
 * filename.isEmpty()?project->first("QMAKE_MAKEFILE"):filename */

#ifdef Q_CC_MSVC
#   pragma optimize( "g", off )
#   pragma warning ( disable : 4748 )
#endif

QUuid VcprojGenerator::getProjectUUID(const QString &filename)
{
    bool validUUID = true;

    // Read GUID from variable-space
    auto uuid = QUuid::fromString(project->first("GUID").toQStringView());

    // If none, create one based on the MD5 of absolute project path
    if(uuid.isNull() || !filename.isEmpty()) {
        QString abspath = Option::fixPathToTargetOS(
                    filename.isEmpty() ? project->first("QMAKE_MAKEFILE").toQString() : filename);
        QByteArray digest = QCryptographicHash::hash(abspath.toUtf8(), QCryptographicHash::Sha1);
        memcpy((unsigned char*)(&uuid), digest.constData(), sizeof(QUuid));
        validUUID = !uuid.isNull();
        uuid.data4[0] = (uuid.data4[0] & 0x3F) | 0x80; // UV_DCE variant
        uuid.data3 = (uuid.data3 & 0x0FFF) | (QUuid::Name<<12);
    }

    // If still not valid, generate new one, and suggest adding to .pro
    if(uuid.isNull() || !validUUID) {
        uuid = QUuid::createUuid();
        fprintf(stderr,
                "qmake couldn't create a GUID based on filepath, and we couldn't\nfind a valid GUID in the .pro file (Consider adding\n'GUID = %s'  to the .pro file)\n",
                uuid.toString().toUpper().toLatin1().constData());
    }

    // Store GUID in variable-space
    project->values("GUID") = ProStringList(uuid.toString().toUpper());
    return uuid;
}

#ifdef Q_CC_MSVC
#   pragma optimize( "g", on )
#endif

QUuid VcprojGenerator::increaseUUID(const QUuid &id)
{
    QUuid result(id);
    qint64 dataFirst = (result.data4[0] << 24) +
                       (result.data4[1] << 16) +
                       (result.data4[2] << 8) +
                        result.data4[3];
    qint64 dataLast =  (result.data4[4] << 24) +
                       (result.data4[5] << 16) +
                       (result.data4[6] <<  8) +
                        result.data4[7];

    if(!(dataLast++))
        dataFirst++;

    result.data4[0] = uchar((dataFirst >> 24) & 0xff);
    result.data4[1] = uchar((dataFirst >> 16) & 0xff);
    result.data4[2] = uchar((dataFirst >>  8) & 0xff);
    result.data4[3] = uchar(dataFirst         & 0xff);
    result.data4[4] = uchar((dataLast  >> 24) & 0xff);
    result.data4[5] = uchar((dataLast  >> 16) & 0xff);
    result.data4[6] = uchar((dataLast  >>  8) & 0xff);
    result.data4[7] = uchar(dataLast          & 0xff);
    return result;
}

QString VcprojGenerator::retrievePlatformToolSet() const
{
    // The PlatformToolset string corresponds to the name of a directory in
    // $(VCTargetsPath)\Platforms\{Win32,x64,...}\PlatformToolsets
    // e.g. v90, v100, v110, v110_xp, v120_CTP_Nov, v120, or WindowsSDK7.1

    // This environment variable may be set by a commandline build
    // environment such as the Windows SDK command prompt
    QByteArray envVar = qgetenv("PlatformToolset");
    if (!envVar.isEmpty())
        return envVar;

    switch (vcProject.Configuration.CompilerVersion)
    {
    case NET2012:
        return QStringLiteral("v110");
    case NET2013:
        return QStringLiteral("v120");
    case NET2015:
        return QStringLiteral("v140");
    case NET2017:
        return QStringLiteral("v141");
    default:
        return QString();
    }
}

bool VcprojGenerator::isStandardSuffix(const QString &suffix) const
{
    if (!project->values("QMAKE_APP_FLAG").isEmpty()) {
        if (suffix.compare("exe", Qt::CaseInsensitive) == 0)
            return true;
    } else if (project->isActiveConfig("shared")) {
        if (suffix.compare("dll", Qt::CaseInsensitive) == 0)
            return true;
    } else {
        if (suffix.compare("lib", Qt::CaseInsensitive) == 0)
            return true;
    }
    return false;
}

ProString VcprojGenerator::firstInputFileName(const ProString &extraCompilerName) const
{
    for (const ProString &var : project->values(ProKey(extraCompilerName + ".input"))) {
        const ProStringList &files = project->values(var.toKey());
        if (!files.isEmpty())
            return files.first();
    }
    return ProString();
}

QString VcprojGenerator::firstExpandedOutputFileName(const ProString &extraCompilerName)
{
    const ProString firstOutput = project->first(ProKey(extraCompilerName + ".output"));
    return replaceExtraCompilerVariables(firstOutput.toQString(),
                                         firstInputFileName(extraCompilerName).toQString(),
                                         QString(), NoShell);
}

ProStringList VcprojGenerator::collectDependencies(QMakeProject *proj, QHash<QString, QString> &projLookup,
                                                   QHash<QString, QString> &projGuids,
                                                   QHash<VcsolutionDepend *, QStringList> &extraSubdirs,
                                                   QHash<QString, VcsolutionDepend*> &solution_depends,
                                                   QList<VcsolutionDepend*> &solution_cleanup,
                                                   QTextStream &t,
                                                   QHash<QString, ProStringList> &subdirProjectLookup,
                                                   const ProStringList &allDependencies)
{
    QVector<QPair<QString, ProStringList> > collectedSubdirs;
    ProStringList tmp_proj_subdirs = proj->values("SUBDIRS");
    ProStringList projectsInProject;
    const int numSubdirs = tmp_proj_subdirs.size();
    collectedSubdirs.reserve(numSubdirs);
    for (int x = 0; x < numSubdirs; ++x) {
        ProString tmpdir = tmp_proj_subdirs.at(x);
        const ProKey tmpdirConfig(tmpdir + ".CONFIG");
        if (!proj->isEmpty(tmpdirConfig)) {
            const ProStringList config = proj->values(tmpdirConfig);
            if (config.contains(QStringLiteral("no_default_target")))
                continue; // Ignore this sub-dir
        }
        const ProKey fkey(tmpdir + ".file");
        const ProKey skey(tmpdir + ".subdir");
        if (!proj->isEmpty(fkey)) {
            if (!proj->isEmpty(skey))
                warn_msg(WarnLogic, "Cannot assign both file and subdir for subdir %s",
                         tmpdir.toLatin1().constData());
            tmpdir = proj->first(fkey);
        } else if (!proj->isEmpty(skey)) {
            tmpdir = proj->first(skey);
        }
        projectsInProject.append(tmpdir);
        collectedSubdirs.append(qMakePair(tmpdir.toQString(), proj->values(ProKey(tmp_proj_subdirs.at(x) + ".depends"))));
        projLookup.insert(tmp_proj_subdirs.at(x).toQString(), tmpdir.toQString());
    }
    for (const auto &subdir : qAsConst(collectedSubdirs)) {
        QString profile = subdir.first;
        QFileInfo fi(fileInfo(Option::normalizePath(profile)));
        if (fi.exists()) {
            if (fi.isDir()) {
                if (!profile.endsWith(Option::dir_sep))
                    profile += Option::dir_sep;
                profile += fi.baseName() + Option::pro_ext;
                QString profileKey = fi.absoluteFilePath();
                fi = QFileInfo(fileInfo(Option::normalizePath(profile)));
                if (!fi.exists())
                    continue;
                projLookup.insert(profileKey, fi.absoluteFilePath());
            }
            QString oldpwd = qmake_getpwd();
            QString oldoutpwd = Option::output_dir;
            QMakeProject tmp_proj;
            QString dir = fi.absolutePath(), fn = fi.fileName();
            if (!dir.isEmpty()) {
                if (!qmake_setpwd(dir))
                    fprintf(stderr, "Cannot find directory: %s", dir.toLatin1().constData());
            }
            Option::output_dir = Option::globals->shadowedPath(QDir::cleanPath(dir));
            if (tmp_proj.read(fn)) {
                // Check if all requirements are fulfilled
                if (!tmp_proj.isEmpty("QMAKE_FAILED_REQUIREMENTS")) {
                    fprintf(stderr, "Project file(%s) not added to Solution because all requirements not met:\n\t%s\n",
                            fn.toLatin1().constData(),
                            tmp_proj.values("QMAKE_FAILED_REQUIREMENTS").join(' ').toLatin1().constData());
                    qmake_setpwd(oldpwd);
                    Option::output_dir = oldoutpwd;
                    continue;
                }
                if (tmp_proj.first("TEMPLATE") == "vcsubdirs") {
                    ProStringList tmpList = collectDependencies(&tmp_proj, projLookup, projGuids, extraSubdirs, solution_depends, solution_cleanup, t, subdirProjectLookup, subdir.second);
                    subdirProjectLookup.insert(subdir.first, tmpList);
                } else {
                    ProStringList tmpList;
                    tmpList += subdir.second;
                    tmpList += allDependencies;
                    // Initialize a 'fake' project to get the correct variables
                    // and to be able to extract all the dependencies
                    Option::QMAKE_MODE old_mode = Option::qmake_mode;
                    Option::qmake_mode = Option::QMAKE_GENERATE_NOTHING;
                    VcprojGenerator tmp_vcproj;
                    tmp_vcproj.setNoIO(true);
                    tmp_vcproj.setProjectFile(&tmp_proj);
                    Option::qmake_mode = old_mode;

                    // We assume project filename is [QMAKE_PROJECT_NAME].vcproj
                    QString vcproj = tmp_vcproj.project->first("QMAKE_PROJECT_NAME") + project->first("VCPROJ_EXTENSION");
                    QString vcprojDir = Option::output_dir;

                    // If file doesn't exsist, then maybe the users configuration
                    // doesn't allow it to be created. Skip to next...
                    if (!exists(vcprojDir + Option::dir_sep + vcproj)) {
                        warn_msg(WarnLogic, "Ignored (not found) '%s'", QString(vcprojDir + Option::dir_sep + vcproj).toLatin1().constData());
                        goto nextfile; // # Dirty!
                    }

                    VcsolutionDepend *newDep = new VcsolutionDepend;
                    newDep->vcprojFile = vcprojDir + Option::dir_sep + vcproj;
                    newDep->orig_target = tmp_proj.first("QMAKE_ORIG_TARGET").toQString();
                    newDep->target = tmp_proj.first("MSVCPROJ_TARGET").toQString().section(Option::dir_sep, -1);
                    newDep->targetType = tmp_vcproj.projectTarget;
                    newDep->uuid = tmp_proj.isEmpty("QMAKE_UUID") ? getProjectUUID(Option::fixPathToLocalOS(vcprojDir + QDir::separator() + vcproj)).toString().toUpper(): tmp_proj.first("QMAKE_UUID").toQString();
                    // We want to store it as the .lib name.
                    if (newDep->target.endsWith(".dll"))
                        newDep->target = newDep->target.left(newDep->target.length()-3) + "lib";
                    projGuids.insert(newDep->orig_target, newDep->target);

                    if (tmpList.size()) {
                        const ProStringList depends = tmpList;
                        for (const ProString &dep : depends) {
                            QString depend = dep.toQString();
                            if (!projGuids[depend].isEmpty()) {
                                newDep->dependencies << projGuids[depend];
                            } else if (subdirProjectLookup[projLookup[depend]].size() > 0) {
                                const ProStringList tmpLst = subdirProjectLookup[projLookup[depend]];
                                for (const ProString &tDep : tmpLst) {
                                    QString tmpDep = tDep.toQString();
                                    newDep->dependencies << projGuids[projLookup[tmpDep]];
                                }
                            } else {
                                extraSubdirs.insert(newDep, tmpList.toQStringList());
                                newDep->dependencies.clear();
                                break;
                            }
                        }
                    }

                    // All ActiveQt Server projects are dependent on idc.exe
                    if (tmp_proj.values("CONFIG").contains("qaxserver"))
                        newDep->dependencies << "idc.exe";

                    // Add all unknown libs to the deps
                    QStringList where = QStringList() << "QMAKE_LIBS" << "QMAKE_LIBS_PRIVATE";
                    if (!tmp_proj.isEmpty("QMAKE_INTERNAL_PRL_LIBS"))
                    where = tmp_proj.values("QMAKE_INTERNAL_PRL_LIBS").toQStringList();
                    for (QStringList::ConstIterator wit = where.begin();
                        wit != where.end(); ++wit) {
                            const ProStringList &l = tmp_proj.values(ProKey(*wit));
                            for (ProStringList::ConstIterator it = l.begin(); it != l.end(); ++it) {
                                const QString opt = fixLibFlag(*it).toQString();
                                if (!opt.startsWith("/") &&   // Not a switch
                                    opt != newDep->target && // Not self
                                    opt != "opengl32.lib" && // We don't care about these libs
                                    opt != "glu32.lib" &&    // to make depgen alittle faster
                                    opt != "kernel32.lib" &&
                                    opt != "user32.lib" &&
                                    opt != "gdi32.lib" &&
                                    opt != "comdlg32.lib" &&
                                    opt != "advapi32.lib" &&
                                    opt != "shell32.lib" &&
                                    opt != "ole32.lib" &&
                                    opt != "oleaut32.lib" &&
                                    opt != "uuid.lib" &&
                                    opt != "imm32.lib" &&
                                    opt != "winmm.lib" &&
                                    opt != "wsock32.lib" &&
                                    opt != "ws2_32.lib" &&
                                    opt != "winspool.lib" &&
                                    opt != "delayimp.lib")
                                {
                                    newDep->dependencies << opt.section(Option::dir_sep, -1);
                                }
                            }
                    }
#ifdef DEBUG_SOLUTION_GEN
                    qDebug("Deps for %20s: [%s]", newDep->target.toLatin1().constData(), newDep->dependencies.join(" :: ").toLatin1().constData());
#endif
                    solution_cleanup.append(newDep);
                    solution_depends.insert(newDep->target, newDep);
                }
nextfile:
                qmake_setpwd(oldpwd);
                Option::output_dir = oldoutpwd;
            }
        }
    }
    return projectsInProject;
}

void VcprojGenerator::writeSubDirs(QTextStream &t)
{
    // Check if all requirements are fulfilled
    if(!project->values("QMAKE_FAILED_REQUIREMENTS").isEmpty()) {
        fprintf(stderr, "Project file not generated because all requirements not met:\n\t%s\n",
                var("QMAKE_FAILED_REQUIREMENTS").toLatin1().constData());
        return;
    }

    switch (vcProject.Configuration.CompilerVersion) {
    case NET2017:
        t << _slnHeader141;
        break;
    case NET2015:
        t << _slnHeader140;
        break;
    case NET2013:
        t << _slnHeader120;
        break;
    case NET2012:
        t << _slnHeader110;
        break;
    case NET2010:
        t << _slnHeader100;
        break;
    case NET2008:
        t << _slnHeader90;
        break;
    case NET2005:
        t << _slnHeader80;
        break;
    case NET2003:
        t << _slnHeader71;
        break;
    case NET2002:
        t << _slnHeader70;
        break;
    default:
        t << _slnHeader70;
        warn_msg(WarnLogic, "Generator: MSVC.NET: Unknown version (%d) of MSVC detected for .sln",
                 vcProject.Configuration.CompilerVersion);
        break;
    }

    QHash<QString, VcsolutionDepend*> solution_depends;
    QList<VcsolutionDepend*> solution_cleanup;

    // Make sure that all temp projects are configured
    // for release so that the depends are created
    // without the debug <lib>dxxx.lib name mangling
    QString old_after_vars = Option::globals->extra_cmds[QMakeEvalAfter];
    Option::globals->extra_cmds[QMakeEvalAfter].append("\nCONFIG+=release");

    QHash<QString, QString> profileLookup;
    QHash<QString, QString> projGuids;
    QHash<VcsolutionDepend *, QStringList> extraSubdirs;
    QHash<QString, ProStringList> subdirProjectLookup;
    collectDependencies(project, profileLookup, projGuids, extraSubdirs, solution_depends, solution_cleanup, t, subdirProjectLookup);

    // write out projects
    for (QList<VcsolutionDepend*>::Iterator it = solution_cleanup.begin(); it != solution_cleanup.end(); ++it) {
        // ### quoting rules?
        t << _slnProjectBeg << _slnMSVCvcprojGUID << _slnProjectMid
            << "\"" << (*it)->orig_target << "\", \"" << (*it)->vcprojFile
            << "\", \"" << (*it)->uuid << "\"";

        debug_msg(1, "Project %s has dependencies: %s", (*it)->target.toLatin1().constData(), (*it)->dependencies.join(" ").toLatin1().constData());

        bool hasDependency = false;
        for (QStringList::iterator dit = (*it)->dependencies.begin();  dit != (*it)->dependencies.end(); ++dit) {
            if (VcsolutionDepend *vc = solution_depends[*dit]) {
                if (!hasDependency) {
                    hasDependency = true;
                    t << _slnProjDepBeg;
                }
                t << "\n\t\t" << vc->uuid << " = " << vc->uuid;
            }
        }
        if (hasDependency)
            t << _slnProjDepEnd;

        t << _slnProjectEnd;
    }

    t << _slnGlobalBeg;

    for (auto extraIt = extraSubdirs.cbegin(), end = extraSubdirs.cend(); extraIt != end; ++extraIt) {
        for (const QString &depend : extraIt.value()) {
            if (!projGuids[depend].isEmpty()) {
                extraIt.key()->dependencies << projGuids[depend];
            } else if (!profileLookup[depend].isEmpty()) {
                if (!projGuids[profileLookup[depend]].isEmpty())
                    extraIt.key()->dependencies << projGuids[profileLookup[depend]];
            }
        }
    }
    QString slnConf = _slnSolutionConf;
    if (!project->isEmpty("VCPROJ_ARCH")) {
        slnConf.replace(QLatin1String("|Win32"), "|" + project->first("VCPROJ_ARCH"));
    } else if (is64Bit) {
        slnConf.replace(QLatin1String("|Win32"), QLatin1String("|x64"));
    }
    t << slnConf;

    // Restore previous after_user_var options
    Option::globals->extra_cmds[QMakeEvalAfter] = old_after_vars;

    t << _slnProjConfBeg;
    for(QList<VcsolutionDepend*>::Iterator it = solution_cleanup.begin(); it != solution_cleanup.end(); ++it) {
        QString platform = is64Bit ? "x64" : "Win32";
        QString xplatform = platform;
        const bool isWinRT = project->isActiveConfig("winrt");
        if (!project->isEmpty("VCPROJ_ARCH")) {
            xplatform = project->first("VCPROJ_ARCH").toQString();
        }
        if (!project->isHostBuild())
            platform = xplatform;
        t << "\n\t\t" << (*it)->uuid << QString(_slnProjDbgConfTag1).arg(xplatform) << platform;
        t << "\n\t\t" << (*it)->uuid << QString(_slnProjDbgConfTag2).arg(xplatform) << platform;
        if (isWinRT)
            t << "\n\t\t" << (*it)->uuid << QString(_slnProjDbgConfTag3).arg(xplatform) << platform;
        t << "\n\t\t" << (*it)->uuid << QString(_slnProjRelConfTag1).arg(xplatform) << platform;
        t << "\n\t\t" << (*it)->uuid << QString(_slnProjRelConfTag2).arg(xplatform) << platform;
        if (isWinRT)
            t << "\n\t\t" << (*it)->uuid << QString(_slnProjRelConfTag3).arg(xplatform) << platform;
    }
    t << _slnProjConfEnd;
    t << _slnExtSections;
    t << _slnGlobalEnd;


    while (!solution_cleanup.isEmpty())
        delete solution_cleanup.takeFirst();
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------

bool VcprojGenerator::hasBuiltinCompiler(const QString &file)
{
    // Source files
    for (int i = 0; i < Option::cpp_ext.count(); ++i)
        if (file.endsWith(Option::cpp_ext.at(i)))
            return true;
    for (int i = 0; i < Option::c_ext.count(); ++i)
        if (file.endsWith(Option::c_ext.at(i)))
            return true;
    if (file.endsWith(".rc")
        || file.endsWith(".idl"))
        return true;
    return false;
}

void VcprojGenerator::createCustomBuildToolFakeFile(const QString &cbtFilePath,
                                                    const QString &realOutFilePath)
{
    QFile file(fileFixify(cbtFilePath, FileFixifyFromOutdir | FileFixifyAbsolute));
    if (file.exists())
        return;
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        warn_msg(WarnLogic, "Cannot create '%s'.", qPrintable(file.fileName()));
        return;
    }
    file.write("This is a dummy file needed to create ");
    file.write(qPrintable(realOutFilePath));
    file.write("\n");
}

void VcprojGenerator::init()
{
    is64Bit = (project->first("QMAKE_TARGET.arch") == "x86_64");
    projectWriter = createProjectWriter();

    if(project->first("TEMPLATE") == "vcsubdirs") //too much work for subdirs
        return;

    debug_msg(1, "Generator: MSVC.NET: Initializing variables");

    // this should probably not be here, but I'm using it to wrap the .t files
    if (project->first("TEMPLATE") == "vcapp")
        project->values("QMAKE_APP_FLAG").append("1");
    else if (project->first("TEMPLATE") == "vclib")
        project->values("QMAKE_LIB_FLAG").append("1");

    processVars();

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

    MakefileGenerator::init();

    // $$QMAKE.. -> $$MSVCPROJ.. -------------------------------------
    const ProStringList &incs = project->values("INCLUDEPATH");
    for (ProStringList::ConstIterator incit = incs.begin(); incit != incs.end(); ++incit) {
        QString inc = (*incit).toQString();
        project->values("MSVCPROJ_INCPATH").append("-I" + escapeFilePath(inc));
    }

    QString dest = Option::fixPathToTargetOS(project->first("TARGET").toQString()) + project->first("TARGET_EXT");
    project->values("MSVCPROJ_TARGET") = ProStringList(dest);

    // DLL COPY ------------------------------------------------------
    if (project->isActiveConfig("dll") && !project->values("DLLDESTDIR").isEmpty()) {
        const ProStringList &dlldirs = project->values("DLLDESTDIR");
        QString copydll("");
        ProStringList::ConstIterator dlldir;
        for (dlldir = dlldirs.begin(); dlldir != dlldirs.end(); ++dlldir) {
            if (!copydll.isEmpty())
                copydll += " && ";
            copydll += "copy \"$(TargetPath)\" " + escapeFilePath(*dlldir);
        }

        QString deststr("Copy " + dest + " to ");
        for (dlldir = dlldirs.begin(); dlldir != dlldirs.end();) {
            deststr += *dlldir;
            ++dlldir;
            if (dlldir != dlldirs.end())
                deststr += ", ";
        }

        project->values("MSVCPROJ_COPY_DLL").append(copydll);
        project->values("MSVCPROJ_COPY_DLL_DESC").append(deststr);
    }

#if 0
    // Verbose output if "-d -d"...
    qDebug("Generator: MSVC.NET: List of current variables:");
    for (ProValueMap::ConstIterator it = project->variables().begin(); it != project->variables().end(); ++it)
        qDebug("Generator: MSVC.NET: %s => %s", qPrintable(it.key().toQString()), qPrintable(it.value().join(" | ")));
#endif

    // Figure out what we're trying to build
    if(project->first("TEMPLATE") == "vcapp") {
        projectTarget = Application;
    } else if(project->first("TEMPLATE") == "vclib") {
        if(project->isActiveConfig("staticlib")) {
            project->values("QMAKE_LIBS") += project->values("RES_FILE");
            projectTarget = StaticLib;
        } else
            projectTarget = SharedLib;
    }

    // Setup PCH variables
    precompH = project->first("PRECOMPILED_HEADER").toQString();
    precompCPP = project->first("PRECOMPILED_SOURCE").toQString();
    usePCH = !precompH.isEmpty() && project->isActiveConfig("precompile_header");
    if (usePCH) {
        precompHFilename = fileInfo(precompH).fileName();
        // Created files
        QString origTarget = project->first("QMAKE_ORIG_TARGET").toQString();
        precompObj = origTarget + Option::obj_ext;
        precompPch = origTarget + ".pch";
        // Add PRECOMPILED_HEADER to HEADERS
        if (!project->values("HEADERS").contains(precompH))
            project->values("HEADERS") += precompH;
        // Return to variable pool
        project->values("PRECOMPILED_OBJECT") = ProStringList(precompObj);
        project->values("PRECOMPILED_PCH")    = ProStringList(precompPch);

        autogenPrecompCPP = precompCPP.isEmpty() && project->isActiveConfig("autogen_precompile_source");
        if (autogenPrecompCPP) {
            precompCPP = precompH
                + (Option::cpp_ext.count() ? Option::cpp_ext.at(0) : QLatin1String(".cpp"));
            project->values("GENERATED_SOURCES") += precompCPP;
        } else if (!precompCPP.isEmpty()) {
            project->values("SOURCES") += precompCPP;
        }
    }

    // Add all input files for a custom compiler into a map for uniqueness,
    // unless the compiler is configure as a combined stage, then use the first one
    for (const ProString &quc : project->values("QMAKE_EXTRA_COMPILERS")) {
        const ProStringList &invar = project->values(ProKey(quc + ".input"));
        const QString compiler_out = project->first(ProKey(quc + ".output")).toQString();
        for (ProStringList::ConstIterator iit = invar.constBegin(); iit != invar.constEnd(); ++iit) {
            ProStringList fileList = project->values((*iit).toKey());
            if (!fileList.isEmpty()) {
                if (project->values(ProKey(quc + ".CONFIG")).indexOf("combine") != -1)
                    fileList.erase(fileList.begin() + 1, fileList.end());
                for (ProStringList::ConstIterator fit = fileList.constBegin(); fit != fileList.constEnd(); ++fit) {
                    QString file = (*fit).toQString();
                    if (verifyExtraCompiler(quc, file)) {
                        if (!hasBuiltinCompiler(file)) {
                            extraCompilerSources[file] += quc.toQString();
                        } else {
                            // Create a fake file foo.moc.cbt for the project view.
                            // This prevents VS from complaining about a circular
                            // dependency from foo.moc -> foo.moc.
                            QString realOut = replaceExtraCompilerVariables(
                                compiler_out, file, QString(), NoShell);
                            QString out = realOut + customBuildToolFilterFileSuffix;
                            createCustomBuildToolFakeFile(out, realOut);
                            out = Option::fixPathToTargetOS(out, false);
                            extraCompilerSources[out] += quc.toQString();
                            extraCompilerOutputs[out] = file;
                        }
                    }
                }
            }
        }
    }

#if 0 // Debugging
    for (auto it = extraCompilerSources.cbegin(), end = extraCompilerSources.cend(); it != end; ++it)
        qDebug("Extracompilers for %s are (%s)", it.key().toLatin1().constData(), it.value().join(", ").toLatin1().constData());
    for (auto it = extraCompilerOutputs.cbegin(), end = extraCompilerOutputs.cend(); it != end; ++it)
        qDebug("Object mapping for %s is (%s)", it.key().toLatin1().constData(), it.value().join(", ").toLatin1().constData());
    qDebug("");
#endif
}

bool VcprojGenerator::mergeBuildProject(MakefileGenerator *other)
{
    if (!other || !other->projectFile()) {
        warn_msg(WarnLogic, "VcprojGenerator: Cannot merge null project.");
        return false;
    }
    if (other->projectFile()->first("MAKEFILE_GENERATOR") != project->first("MAKEFILE_GENERATOR")) {
        warn_msg(WarnLogic, "VcprojGenerator: Cannot merge other types of projects! (ignored)");
        return false;
    }

    VcprojGenerator *otherVC = static_cast<VcprojGenerator*>(other);
    mergedProjects += otherVC;
    return true;
}

void VcprojGenerator::initProject()
{
    // Initialize XML sub elements
    // - Do this first since project elements may need
    // - to know of certain configuration options
    initConfiguration();
    initRootFiles();
    initSourceFiles();
    initHeaderFiles();
    initGeneratedFiles();
    initLexYaccFiles();
    initTranslationFiles();
    initFormFiles();
    initResourceFiles();
    initDistributionFiles();
    initExtraCompilerOutputs();

    // Own elements -----------------------------
    vcProject.Name = project->first("QMAKE_ORIG_TARGET").toQString();
    switch (vcProject.Configuration.CompilerVersion) {
    case NET2017:
        vcProject.Version = "15.00";
        break;
    case NET2015:
        vcProject.Version = "14.00";
        break;
    case NET2013:
        vcProject.Version = "12.00";
        break;
    case NET2012:
        vcProject.Version = "11.00";
        break;
    case NET2010:
        vcProject.Version = "10.00";
        break;
    case NET2008:
        vcProject.Version = "9,00";
        break;
    case NET2005:
                //### using ',' because of a bug in 2005 B2
                //### VS uses '.' or ',' depending on the regional settings! Using ',' always works.
        vcProject.Version = "8,00";
        break;
    case NET2003:
        vcProject.Version = "7.10";
        break;
    case NET2002:
        vcProject.Version = "7.00";
        break;
    default:
        vcProject.Version = "7.00";
        warn_msg(WarnLogic, "Generator: MSVC.NET: Unknown version (%d) of MSVC detected for .vcproj", vcProject.Configuration.CompilerVersion);
        break;
    }

    vcProject.Keyword = project->first("VCPROJ_KEYWORD").toQString();
    if (!project->isEmpty("VCPROJ_ARCH")) {
        vcProject.PlatformName = project->first("VCPROJ_ARCH").toQString();
    } else {
        vcProject.PlatformName = (is64Bit ? "x64" : "Win32");
    }
    vcProject.SdkVersion = project->first("WINSDK_VER").toQString();
    // These are not used by Qt, but may be used by customers
    vcProject.SccProjectName = project->first("SCCPROJECTNAME").toQString();
    vcProject.SccLocalPath = project->first("SCCLOCALPATH").toQString();
    vcProject.flat_files = project->isActiveConfig("flat");
}

void VcprojGenerator::initConfiguration()
{
    // Initialize XML sub elements
    // - Do this first since main configuration elements may need
    // - to know of certain compiler/linker options
    VCConfiguration &conf = vcProject.Configuration;
    conf.suppressUnknownOptionWarnings = project->isActiveConfig("suppress_vcproj_warnings");
    conf.CompilerVersion = vsVersionFromString(project->first("MSVC_VER"));

    if (conf.CompilerVersion >= NET2012) {
        conf.WinRT = project->isActiveConfig("winrt");
        if (conf.WinRT) {
            // Saner defaults
            conf.compiler.UsePrecompiledHeader = pchNone;
            conf.compiler.CompileAsWinRT = _False;
            conf.linker.GenerateWindowsMetadata = _False;
        }
    }

    initCompilerTool();

    // Only on configuration per build
    bool isDebug = project->isActiveConfig("debug");

    if(projectTarget == StaticLib)
        initLibrarianTool();
    else {
        conf.linker.GenerateDebugInformation = project->isActiveConfig("debug_info") ? _True : _False;
        initLinkerTool();
    }
    initManifestTool();
    initResourceTool();
    initIDLTool();

    // Own elements -----------------------------
    ProString temp = project->first("BuildBrowserInformation");
    switch (projectTarget) {
    case SharedLib:
        conf.ConfigurationType = typeDynamicLibrary;
        break;
    case StaticLib:
        conf.ConfigurationType = typeStaticLibrary;
        break;
    case Application:
    default:
        conf.ConfigurationType = typeApplication;
        break;
    }

    conf.OutputDirectory = project->first("DESTDIR").toQString();
    if (conf.OutputDirectory.isEmpty())
        conf.OutputDirectory = ".\\";
    if (!conf.OutputDirectory.endsWith("\\"))
        conf.OutputDirectory += '\\';
    if (conf.CompilerVersion >= NET2010) {
        conf.PlatformToolSet = retrievePlatformToolSet();

        const QFileInfo targetInfo = fileInfo(project->first("MSVCPROJ_TARGET").toQString());
        conf.PrimaryOutput = targetInfo.completeBaseName();

        const QString targetSuffix = targetInfo.suffix();
        if (!isStandardSuffix(targetSuffix))
            conf.PrimaryOutputExtension = '.' + targetSuffix;
    }

    conf.Name = project->values("BUILD_NAME").join(' ');
    if (conf.Name.isEmpty())
        conf.Name = isDebug ? "Debug" : "Release";
    conf.ConfigurationName = conf.Name;
    if (!project->isEmpty("VCPROJ_ARCH")) {
        conf.Name += "|" + project->first("VCPROJ_ARCH");
    } else {
        conf.Name += (is64Bit ? "|x64" : "|Win32");
    }
    conf.ATLMinimizesCRunTimeLibraryUsage = (project->first("ATLMinimizesCRunTimeLibraryUsage").isEmpty() ? _False : _True);
    conf.BuildBrowserInformation = triState(temp.isEmpty() ? (short)unset : temp.toShort());
    temp = project->first("CharacterSet");
    conf.CharacterSet = charSet(temp.isEmpty() ? short(conf.WinRT ? charSetUnicode : charSetNotSet) : temp.toShort());
    conf.DeleteExtensionsOnClean = project->first("DeleteExtensionsOnClean").toQString();
    conf.ImportLibrary = conf.linker.ImportLibrary;
    conf.IntermediateDirectory = project->first("OBJECTS_DIR").toQString();
    conf.WholeProgramOptimization = conf.compiler.WholeProgramOptimization;
    temp = project->first("UseOfATL");
    if(!temp.isEmpty())
        conf.UseOfATL = useOfATL(temp.toShort());
    temp = project->first("UseOfMfc");
    if(!temp.isEmpty())
        conf.UseOfMfc = useOfMfc(temp.toShort());

    // Configuration does not need parameters from
    // these sub XML items;
    initCustomBuildTool();
    initPreBuildEventTools();
    initPostBuildEventTools();
    // Only deploy for crosscompiled projects
    if (!project->isHostBuild() || conf.WinRT)
        initDeploymentTool();
    initWinDeployQtTool();
    initPreLinkEventTools();

    if (!isDebug)
        conf.compiler.PreprocessorDefinitions += "NDEBUG";
}

void VcprojGenerator::initCompilerTool()
{
    QString placement = project->first("OBJECTS_DIR").toQString();
    if(placement.isEmpty())
        placement = ".\\";

    VCConfiguration &conf = vcProject.Configuration;
    if (conf.CompilerVersion >= NET2010) {
        // adjust compiler tool defaults for VS 2010 and above
        conf.compiler.Optimization = optimizeDisabled;
    }
    conf.compiler.AssemblerListingLocation = placement ;
    conf.compiler.ObjectFile = placement ;
    conf.compiler.ExceptionHandling = ehNone;
    // PCH
    if (usePCH) {
        conf.compiler.UsePrecompiledHeader     = pchUseUsingSpecific;
        conf.compiler.PrecompiledHeaderFile    = "$(IntDir)\\" + precompPch;
        conf.compiler.PrecompiledHeaderThrough = project->first("PRECOMPILED_HEADER").toQString();
        conf.compiler.ForcedIncludeFiles       = project->values("PRECOMPILED_HEADER").toQStringList();

        if (conf.CompilerVersion <= NET2003) {
            // Minimal build option triggers an Internal Compiler Error
            // when used in conjunction with /FI and /Yu, so remove it
            // ### work-around for a VS 2003 bug. Move to some prf file or remove completely.
            project->values("QMAKE_CFLAGS_DEBUG").removeAll("-Gm");
            project->values("QMAKE_CFLAGS_DEBUG").removeAll("/Gm");
            project->values("QMAKE_CXXFLAGS_DEBUG").removeAll("-Gm");
            project->values("QMAKE_CXXFLAGS_DEBUG").removeAll("/Gm");
        }
    }

    conf.compiler.parseOptions(project->values("QMAKE_CXXFLAGS"));

    if (project->isActiveConfig("windows"))
        conf.compiler.PreprocessorDefinitions += "_WINDOWS";
    else if (project->isActiveConfig("console"))
        conf.compiler.PreprocessorDefinitions += "_CONSOLE";

    conf.compiler.PreprocessorDefinitions += project->values("DEFINES").toQStringList();
    conf.compiler.PreprocessorDefinitions += project->values("PRL_EXPORT_DEFINES").toQStringList();
    conf.compiler.parseOptions(project->values("MSVCPROJ_INCPATH"));
}

void VcprojGenerator::initLibrarianTool()
{
    VCConfiguration &conf = vcProject.Configuration;
    conf.librarian.OutputFile = "$(OutDir)\\";
    conf.librarian.OutputFile += project->first("MSVCPROJ_TARGET").toQString();
    conf.librarian.AdditionalOptions += project->values("QMAKE_LIBFLAGS").toQStringList();
}

void VcprojGenerator::initManifestTool()
{
    VCManifestTool &tool = vcProject.Configuration.manifestTool;
    const ProString tmplt = project->first("TEMPLATE");
    if ((tmplt == "vclib"
         && !project->isActiveConfig("embed_manifest_dll")
         && !project->isActiveConfig("static"))
        || (tmplt == "vcapp"
            && !project->isActiveConfig("embed_manifest_exe"))) {
        tool.EmbedManifest = _False;
    }
}

void VcprojGenerator::initLinkerTool()
{
    VCConfiguration &conf = vcProject.Configuration;
    conf.linker.parseOptions(project->values("QMAKE_LFLAGS"));

    if (!project->values("DEF_FILE").isEmpty())
        conf.linker.ModuleDefinitionFile = project->first("DEF_FILE").toQString();

    static const char * const lflags[] = { "QMAKE_LIBS", "QMAKE_LIBS_PRIVATE", 0 };
    for (int i = 0; lflags[i]; i++) {
        const auto libs = fixLibFlags(lflags[i]);
        for (const ProString &lib : libs) {
            if (lib.startsWith("/LIBPATH:"))
                conf.linker.AdditionalLibraryDirectories << lib.mid(9).toQString();
            else
                conf.linker.AdditionalDependencies << lib.toQString();
        }
    }

    conf.linker.OutputFile = "$(OutDir)\\";
    conf.linker.OutputFile += project->first("MSVCPROJ_TARGET").toQString();
}

void VcprojGenerator::initResourceTool()
{
    VCConfiguration &conf = vcProject.Configuration;

    ProStringList rcDefines = project->values("RC_DEFINES");
    if (rcDefines.size() > 0)
        conf.resource.PreprocessorDefinitions = rcDefines.toQStringList();
    else
        conf.resource.PreprocessorDefinitions = conf.compiler.PreprocessorDefinitions;

    for (const ProString &path : project->values("RC_INCLUDEPATH")) {
        QString fixedPath = fileFixify(path.toQString());
        if (fileInfo(fixedPath).isRelative()) {
            if (fixedPath == QLatin1String("."))
                fixedPath = QStringLiteral("$(ProjectDir)");
            else
                fixedPath.prepend(QStringLiteral("$(ProjectDir)\\"));
        }
        conf.resource.AdditionalIncludeDirectories << escapeFilePath(fixedPath);
    }

    // We need to add _DEBUG for the debug version of the project, since the normal compiler defines
    // do not contain it. (The compiler defines this symbol automatically, which is wy we don't need
    // to add it for the compiler) However, the resource tool does not do this.
    if(project->isActiveConfig("debug"))
        conf.resource.PreprocessorDefinitions += "_DEBUG";
    if (conf.CompilerVersion < NET2010 && project->isActiveConfig("staticlib"))
        conf.resource.ResourceOutputFileName = "$(OutDir)\\$(InputName).res";
}

void VcprojGenerator::initIDLTool()
{
}

void VcprojGenerator::initCustomBuildTool()
{
}

void VcprojGenerator::initPreBuildEventTools()
{
}

void VcprojGenerator::initPostBuildEventTools()
{
    VCConfiguration &conf = vcProject.Configuration;
    if (!project->values("QMAKE_POST_LINK").isEmpty()) {
        QStringList cmdline = VCToolBase::fixCommandLine(var("QMAKE_POST_LINK"));
        conf.postBuild.CommandLine = cmdline;
        conf.postBuild.Description = cmdline.join(QLatin1String("\r\n"));
        conf.postBuild.ExcludedFromBuild = _False;
    }
    if (!project->values("MSVCPROJ_COPY_DLL").isEmpty()) {
        conf.postBuild.Description += var("MSVCPROJ_COPY_DLL_DESC");
        conf.postBuild.CommandLine += var("MSVCPROJ_COPY_DLL");
        conf.postBuild.ExcludedFromBuild = _False;
    }
}

void VcprojGenerator::initDeploymentTool()
{
    VCConfiguration &conf = vcProject.Configuration;
    QString targetPath;
    if (conf.WinRT) {
        vcProject.DeploymentFiles.Name = "Deployment Files";
        vcProject.DeploymentFiles.ParseFiles = _False;
        vcProject.DeploymentFiles.Filter = "deploy";
        vcProject.DeploymentFiles.Guid = _GUIDDeploymentFiles;
    } else {
        targetPath = project->values("deploy.path").join(' ');
        if (targetPath.isEmpty())
            targetPath = QString("%CSIDL_PROGRAM_FILES%\\") + project->first("TARGET");
        if (targetPath.endsWith("/") || targetPath.endsWith("\\"))
            targetPath.chop(1);
        conf.deployment.RemoteDirectory = targetPath;
    }
    const ProStringList dllPaths = project->values("QMAKE_DLL_PATHS");
    // Only deploy Qt libs for shared build
    if (!dllPaths.isEmpty() &&
        !(conf.WinRT && project->first("MSVC_VER").toQString() == "14.0")) {
        // FIXME: This code should actually resolve the libraries from all Qt modules.
        ProStringList arg = project->values("QMAKE_LIBS") + project->values("QMAKE_LIBS_PRIVATE");
        bool qpaPluginDeployed = false;
        for (ProStringList::ConstIterator it = arg.constBegin(); it != arg.constEnd(); ++it) {
            QString dllName = (*it).toQString();
            dllName.replace(QLatin1Char('\\'), QLatin1Char('/'));
            // LIBPATH isn't relevant for deployment
            if (dllName.startsWith(QLatin1String("/LIBPATH:")))
                continue;
            // We want to deploy .dlls not .libs
            if (dllName.endsWith(QLatin1String(".lib")))
                dllName.replace(dllName.length() - 3, 3, QLatin1String("dll"));
            // Use only the file name and check in Qt's install path and LIBPATHs to check for existence
            dllName.remove(0, dllName.lastIndexOf(QLatin1Char('/')) + 1);
            QFileInfo info;
            for (const ProString &dllPath : dllPaths) {
                QString absoluteDllFilePath = dllPath.toQString();
                if (!absoluteDllFilePath.endsWith(QLatin1Char('/')))
                    absoluteDllFilePath += QLatin1Char('/');
                absoluteDllFilePath += dllName;
                info = QFileInfo(absoluteDllFilePath);
                if (info.exists())
                    break;
            }

            if (!info.exists())
                continue;
            if (conf.WinRT) {
                QString absoluteFilePath(QDir::toNativeSeparators(info.absoluteFilePath()));
                vcProject.DeploymentFiles.addFile(absoluteFilePath);
            } else {
                conf.deployment.AdditionalFiles += info.fileName()
                        + "|" + QDir::toNativeSeparators(info.absolutePath())
                        + "|" + targetPath
                        + "|0;";
                if (!qpaPluginDeployed) {
                    QChar debugInfixChar;
                    bool foundGuid = dllName.contains(QLatin1String("Guid"));
                    if (foundGuid)
                        debugInfixChar = QLatin1Char('d');

                    if (foundGuid || dllName.contains(QLatin1String("Gui"))) {
                        QFileInfo info2;
                        for (const ProString &dllPath : dllPaths) {
                            QString absoluteDllFilePath = dllPath.toQString();
                            if (!absoluteDllFilePath.endsWith(QLatin1Char('/')))
                                absoluteDllFilePath += QLatin1Char('/');
                            absoluteDllFilePath += QLatin1String("../plugins/platforms/qwindows") + debugInfixChar + QLatin1String(".dll");
                            info2 = QFileInfo(absoluteDllFilePath);
                            if (info2.exists())
                                break;
                        }
                        if (info2.exists()) {
                            conf.deployment.AdditionalFiles += QLatin1String("qwindows") + debugInfixChar + QLatin1String(".dll")
                                                        + QLatin1Char('|') + QDir::toNativeSeparators(info2.absolutePath())
                                                        + QLatin1Char('|') + targetPath + QLatin1String("\\platforms")
                                                        + QLatin1String("|0;");
                            qpaPluginDeployed = true;
                        }
                    }
                }
            }
        }
    }

    for (const ProString &item : project->values("INSTALLS")) {
        // get item.path
        QString devicePath = project->first(ProKey(item + ".path")).toQString();
        if (!conf.WinRT) {
            if (devicePath.isEmpty())
                devicePath = targetPath;
            // check if item.path is relative (! either /,\ or %)
            if (!(devicePath.at(0) == QLatin1Char('/')
                || devicePath.at(0) == QLatin1Char('\\')
                || devicePath.at(0) == QLatin1Char('%'))) {
                // create output path
                devicePath = Option::fixPathToTargetOS(targetPath + QLatin1Char('\\') + devicePath);
            }
        }
        // foreach d in item.files
        for (const ProString &src : project->values(ProKey(item + ".files"))) {
            QString itemDevicePath = devicePath;
            QString source = Option::normalizePath(src.toQString());
            QString nameFilter;
            QFileInfo info(source);
            QString searchPath;
            if (info.isDir()) {
                nameFilter = QLatin1String("*");
                itemDevicePath += "\\" + info.fileName();
                searchPath = info.absoluteFilePath();
            } else {
                nameFilter = info.fileName();
                searchPath = info.absolutePath();
            }

            int pathSize = searchPath.size();
            QDirIterator iterator(searchPath, QStringList() << nameFilter
                                  , QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks
                                  , QDirIterator::Subdirectories);
            // foreach dirIterator-entry in d
            while(iterator.hasNext()) {
                iterator.next();
                if (conf.WinRT) {
                    QString absoluteItemFilePath = Option::fixPathToTargetOS(QFileInfo(iterator.filePath()).absoluteFilePath());
                    vcProject.DeploymentFiles.addFile(absoluteItemFilePath);
                } else {
                    QString absoluteItemPath = Option::fixPathToTargetOS(QFileInfo(iterator.filePath()).absolutePath());
                    // Identify if it is just another subdir
                    int diffSize = absoluteItemPath.size() - pathSize;
                    // write out rules
                    conf.deployment.AdditionalFiles += iterator.fileName()
                            + "|" + absoluteItemPath
                            + "|" + itemDevicePath + (diffSize ? (absoluteItemPath.right(diffSize)) : QLatin1String(""))
                            + "|0;";
                }
            }
        }

        if (conf.WinRT) {
            vcProject.DeploymentFiles.Project = this;
            vcProject.DeploymentFiles.Config = &(vcProject.Configuration);
        }
    }
}

void VcprojGenerator::initWinDeployQtTool()
{
    VCConfiguration &conf = vcProject.Configuration;
    conf.windeployqt.ExcludedFromBuild = true;
    if (project->isActiveConfig("windeployqt")) {
        conf.windeployqt.Record = QStringLiteral("$(TargetName).windeployqt.$(Platform).$(Configuration)");
        const QString commandLine = MakefileGenerator::shellQuote(QDir::toNativeSeparators(project->first("QMAKE_WINDEPLOYQT").toQString()))
                + QLatin1Char(' ') + project->values("WINDEPLOYQT_OPTIONS").join(QLatin1Char(' '));

        //  Visual Studio copies all files to be deployed into the MSIL directory
        //  and then invokes MDILXapCompile on it, which checks for managed code and
        //  translates it into native code. The problem is that all entries of the
        //  package will be copied into the MSIL directly, losing the subdirectory
        //  structure (for instance for plugins). However, the MDILXapCompile call
        //  itself contains the original subdirectories as parameters and hence the
        //  call fails.
        //  Hence the only way to get a build done is to recreate the directory
        //  structure manually by invoking windeployqt a second time, so that
        //  the MDILXapCompile call succeeds and deployment continues.
        conf.windeployqt.CommandLine += commandLine
                + QStringLiteral(" -list relative -dir \"$(MSBuildProjectDirectory)\" \"$(OutDir)\\$(TargetName).exe\" > ")
                + MakefileGenerator::shellQuote(conf.windeployqt.Record);
        conf.windeployqt.config = &vcProject.Configuration;
        conf.windeployqt.ExcludedFromBuild = false;
    }
}

void VcprojGenerator::initPreLinkEventTools()
{
    VCConfiguration &conf = vcProject.Configuration;
    if(!project->values("QMAKE_PRE_LINK").isEmpty()) {
        QStringList cmdline = VCToolBase::fixCommandLine(var("QMAKE_PRE_LINK"));
        conf.preLink.CommandLine = cmdline;
        conf.preLink.Description = cmdline.join(QLatin1String("\r\n"));
        conf.preLink.ExcludedFromBuild = _False;
    }
}

void VcprojGenerator::initRootFiles()
{
    // Note: Root files do _not_ have any filter name, filter nor GUID!
    vcProject.RootFiles.addFiles(project->values("RC_FILE"));

    vcProject.RootFiles.Project = this;
    vcProject.RootFiles.Config = &(vcProject.Configuration);
}

void VcprojGenerator::initSourceFiles()
{
    vcProject.SourceFiles.Name = "Source Files";
    vcProject.SourceFiles.Filter = "cpp;c;cxx;def;odl;idl;hpj;bat;asm;asmx";
    vcProject.SourceFiles.Guid = _GUIDSourceFiles;

    vcProject.SourceFiles.addFiles(project->values("SOURCES"));

    vcProject.SourceFiles.Project = this;
    vcProject.SourceFiles.Config = &(vcProject.Configuration);
}

void VcprojGenerator::initHeaderFiles()
{
    vcProject.HeaderFiles.Name = "Header Files";
    vcProject.HeaderFiles.Filter = "h;hpp;hxx;hm;inl;inc;xsd";
    vcProject.HeaderFiles.Guid = _GUIDHeaderFiles;

    vcProject.HeaderFiles.addFiles(project->values("HEADERS"));
    if (usePCH) // Generated PCH cpp file
        vcProject.HeaderFiles.addFile(precompH);

    vcProject.HeaderFiles.Project = this;
    vcProject.HeaderFiles.Config = &(vcProject.Configuration);
//    vcProject.HeaderFiles.CustomBuild = mocHdr;
//    addMocArguments(vcProject.HeaderFiles);
}

void VcprojGenerator::initGeneratedFiles()
{
    vcProject.GeneratedFiles.Name = "Generated Files";
    vcProject.GeneratedFiles.Filter = "cpp;c;cxx;moc;h;def;odl;idl;res;";
    vcProject.GeneratedFiles.Guid = _GUIDGeneratedFiles;

    // ### These cannot have CustomBuild (mocSrc)!!
    vcProject.GeneratedFiles.addFiles(project->values("GENERATED_SOURCES"));
    vcProject.GeneratedFiles.addFiles(project->values("GENERATED_FILES"));
    vcProject.GeneratedFiles.addFiles(project->values("IDLSOURCES"));
    if (project->values("RC_FILE").isEmpty())
        vcProject.GeneratedFiles.addFiles(project->values("RES_FILE"));
    if(!extraCompilerOutputs.isEmpty())
        vcProject.GeneratedFiles.addFiles(extraCompilerOutputs.keys());

    vcProject.GeneratedFiles.Project = this;
    vcProject.GeneratedFiles.Config = &(vcProject.Configuration);
//    vcProject.GeneratedFiles.CustomBuild = mocSrc;
}

void VcprojGenerator::initLexYaccFiles()
{
    vcProject.LexYaccFiles.Name = "Lex / Yacc Files";
    vcProject.LexYaccFiles.ParseFiles = _False;
    vcProject.LexYaccFiles.Filter = "l;y";
    vcProject.LexYaccFiles.Guid = _GUIDLexYaccFiles;

    vcProject.LexYaccFiles.addFiles(project->values("LEXSOURCES"));
    vcProject.LexYaccFiles.addFiles(project->values("YACCSOURCES"));

    vcProject.LexYaccFiles.Project = this;
    vcProject.LexYaccFiles.Config = &(vcProject.Configuration);
}

void VcprojGenerator::initTranslationFiles()
{
    vcProject.TranslationFiles.Name = "Translation Files";
    vcProject.TranslationFiles.ParseFiles = _False;
    vcProject.TranslationFiles.Filter = "ts;xlf";
    vcProject.TranslationFiles.Guid = _GUIDTranslationFiles;

    vcProject.TranslationFiles.addFiles(project->values("TRANSLATIONS"));

    vcProject.TranslationFiles.Project = this;
    vcProject.TranslationFiles.Config = &(vcProject.Configuration);
}

void VcprojGenerator::initFormFiles()
{
    vcProject.FormFiles.Name = "Form Files";
    vcProject.FormFiles.ParseFiles = _False;
    vcProject.FormFiles.Filter = "ui";
    vcProject.FormFiles.Guid = _GUIDFormFiles;
    vcProject.FormFiles.addFiles(project->values("FORMS"));
    vcProject.FormFiles.Project = this;
    vcProject.FormFiles.Config = &(vcProject.Configuration);
}

void VcprojGenerator::initResourceFiles()
{
    vcProject.ResourceFiles.Name = "Resource Files";
    vcProject.ResourceFiles.ParseFiles = _False;
    vcProject.ResourceFiles.Filter = "qrc;*"; //"rc;ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe;resx;ts;xlf;qrc";
    vcProject.ResourceFiles.Guid = _GUIDResourceFiles;

    // Bad hack, please look away -------------------------------------
    QString rcc_dep_cmd = project->values("rcc.depend_command").join(' ');
    if(!rcc_dep_cmd.isEmpty()) {
        ProStringList qrc_files = project->values("RESOURCES");
        QStringList deps;
        if(!qrc_files.isEmpty()) {
            for (int i = 0; i < qrc_files.count(); ++i) {
                char buff[256];
                QString dep_cmd = replaceExtraCompilerVariables(
                        rcc_dep_cmd, qrc_files.at(i).toQString(), QString(), LocalShell);

                dep_cmd = Option::fixPathToLocalOS(dep_cmd, true, false);
                if(canExecute(dep_cmd)) {
                    dep_cmd.prepend(QLatin1String("cd ")
                                    + IoUtils::shellQuote(Option::fixPathToLocalOS(Option::output_dir, false))
                                    + QLatin1String(" && "));
                    if (FILE *proc = QT_POPEN(dep_cmd.toLatin1().constData(), QT_POPEN_READ)) {
                        QString indeps;
                        while(!feof(proc)) {
                            int read_in = (int)fread(buff, 1, 255, proc);
                            if(!read_in)
                                break;
                            indeps += QByteArray(buff, read_in);
                        }
                        QT_PCLOSE(proc);
                        if(!indeps.isEmpty())
                            deps += fileFixify(indeps.replace('\n', ' ').simplified().split(' '),
                                               FileFixifyFromOutdir);
                    }
                }
            }
            vcProject.ResourceFiles.addFiles(deps);
        }
    }
    // You may look again --------------------------------------------

    vcProject.ResourceFiles.addFiles(project->values("RESOURCES"));

    vcProject.ResourceFiles.Project = this;
    vcProject.ResourceFiles.Config = &(vcProject.Configuration);
}

void VcprojGenerator::initDistributionFiles()
{
    vcProject.DistributionFiles.Name = "Distribution Files";
    vcProject.DistributionFiles.ParseFiles = _False;
    vcProject.DistributionFiles.Filter = "*";
    vcProject.DistributionFiles.Guid = _GUIDDistributionFiles;
    vcProject.DistributionFiles.addFiles(project->values("DISTFILES"));
    vcProject.DistributionFiles.Project = this;
    vcProject.DistributionFiles.Config = &(vcProject.Configuration);
}

void VcprojGenerator::initExtraCompilerOutputs()
{
    ProStringList otherFilters;
    otherFilters << "FORMS"
                 << "GENERATED_FILES"
                 << "GENERATED_SOURCES"
                 << "HEADERS"
                 << "IDLSOURCES"
                 << "LEXSOURCES"
                 << "RC_FILE"
                 << "RESOURCES"
                 << "RES_FILE"
                 << "SOURCES"
                 << "TRANSLATIONS"
                 << "YACCSOURCES";
    const ProStringList &quc = project->values("QMAKE_EXTRA_COMPILERS");
    for (ProStringList::ConstIterator it = quc.begin(); it != quc.end(); ++it) {
        ProString extracompilerName = project->first(ProKey(*it + ".name"));
        if (extracompilerName.isEmpty())
            extracompilerName = (*it);

        // Create an extra compiler filter and add the files
        VCFilter extraCompile;
        extraCompile.Name = extracompilerName.toQString();
        extraCompile.ParseFiles = _False;
        extraCompile.Filter = "";
        extraCompile.Guid = QString(_GUIDExtraCompilerFiles) + "-" + (*it);

        // If the extra compiler has a variable_out set the output file
        // is added to an other file list, and does not need its own..
        bool addOnInput = hasBuiltinCompiler(firstExpandedOutputFileName(*it));
        const ProString &tmp_other_out = project->first(ProKey(*it + ".variable_out"));
        if (!tmp_other_out.isEmpty() && !addOnInput)
            continue;

        if (!addOnInput) {
            QString tmp_out = project->first(ProKey(*it + ".output")).toQString();
            if (project->values(ProKey(*it + ".CONFIG")).indexOf("combine") != -1) {
                // Combined output, only one file result
                extraCompile.addFile(Option::fixPathToTargetOS(
                        replaceExtraCompilerVariables(tmp_out, QString(), QString(), NoShell), false));
            } else {
                // One output file per input
                const ProStringList &tmp_in = project->values(project->first(ProKey(*it + ".input")).toKey());
                for (int i = 0; i < tmp_in.count(); ++i) {
                    const QString &filename = tmp_in.at(i).toQString();
                    if (extraCompilerSources.contains(filename))
                        extraCompile.addFile(Option::fixPathToTargetOS(
                                replaceExtraCompilerVariables(filename, tmp_out, QString(), NoShell), false));
                }
            }
        } else {
            // In this case we the outputs have a built-in compiler, so we cannot add the custom
            // build steps there. So, we turn it around and add it to the input files instead,
            // provided that the input file variable is not handled already (those in otherFilters
            // are handled, so we avoid them).
            const ProStringList &inputVars = project->values(ProKey(*it + ".input"));
            for (const ProString &inputVar : inputVars) {
                if (!otherFilters.contains(inputVar)) {
                    const ProStringList &tmp_in = project->values(inputVar.toKey());
                    for (int i = 0; i < tmp_in.count(); ++i) {
                        const QString &filename = tmp_in.at(i).toQString();
                        if (extraCompilerSources.contains(filename))
                            extraCompile.addFile(Option::fixPathToTargetOS(
                                    replaceExtraCompilerVariables(filename, QString(), QString(), NoShell), false));
                    }
                }
            }
        }
        extraCompile.Project = this;
        extraCompile.Config = &(vcProject.Configuration);

        vcProject.ExtraCompilersFiles.append(extraCompile);
    }
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------

VCProjectWriter *VcprojGenerator::createProjectWriter()
{
    return new VCProjectWriter;
}

QString VcprojGenerator::replaceExtraCompilerVariables(
        const QString &var, const QStringList &in, const QStringList &out, ReplaceFor forShell)
{
    QString ret = MakefileGenerator::replaceExtraCompilerVariables(var, in, out, forShell);

    ProStringList &defines = project->values("VCPROJ_MAKEFILE_DEFINES");
    if(defines.isEmpty())
        defines.append(varGlue("PRL_EXPORT_DEFINES"," -D"," -D","") +
                       varGlue("DEFINES"," -D"," -D",""));
    ret.replace(QLatin1String("$(DEFINES)"), defines.first().toQString());

    ProStringList &incpath = project->values("VCPROJ_MAKEFILE_INCPATH");
    if(incpath.isEmpty() && !this->var("MSVCPROJ_INCPATH").isEmpty())
        incpath.append(this->var("MSVCPROJ_INCPATH"));
    ret.replace(QLatin1String("$(INCPATH)"), incpath.join(' '));

    return ret;
}

bool VcprojGenerator::openOutput(QFile &file, const QString &/*build*/) const
{
    QString outdir;
    if(!file.fileName().isEmpty()) {
        QFileInfo fi(fileInfo(file.fileName()));
        if(fi.isDir())
            outdir = file.fileName() + QDir::separator();
    }
    if(!outdir.isEmpty() || file.fileName().isEmpty()) {
        ProString ext = project->first("VCPROJ_EXTENSION");
        if(project->first("TEMPLATE") == "vcsubdirs")
            ext = project->first("VCSOLUTION_EXTENSION");
        ProString outputName = project->first("TARGET");
        if (!project->first("MAKEFILE").isEmpty())
            outputName = project->first("MAKEFILE");
        file.setFileName(outdir + outputName + ext);
    }
    return Win32MakefileGenerator::openOutput(file, QString());
}

QT_END_NAMESPACE
