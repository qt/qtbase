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

#ifndef MSVC_VCPROJ_H
#define MSVC_VCPROJ_H

#include "winmakefile.h"
#include "msvc_objectmodel.h"

QT_BEGIN_NAMESPACE

enum Target {
    Application,
    SharedLib,
    StaticLib
};

class QUuid;
struct VcsolutionDepend;
class VcprojGenerator : public Win32MakefileGenerator
{
    bool is64Bit;
    bool writeVcprojParts(QTextStream &);

    bool writeMakefile(QTextStream &) override;
    bool writeProjectMakefile() override;

    void init() override;

public:
    VcprojGenerator();
    ~VcprojGenerator();

    QString defaultMakefile() const;
    QString precompH, precompHFilename, precompSource,
            precompObj, precompPch;
    bool autogenPrecompSource;
    static bool hasBuiltinCompiler(const QString &file);

    QHash<QString, QStringList> extraCompilerSources;
    QHash<QString, QString> extraCompilerOutputs;
    const QString customBuildToolFilterFileSuffix;
    bool usePCH;
    bool pchIsCFile = false;
    VCProjectWriter *projectWriter;

    using Win32MakefileGenerator::callExtraCompilerDependCommand;

protected:
    virtual VCProjectWriter *createProjectWriter();
    bool doDepends() const override { return false; } // Never necessary
    using Win32MakefileGenerator::replaceExtraCompilerVariables;
    QString replaceExtraCompilerVariables(const QString &, const QStringList &, const QStringList &, ReplaceFor) override;
    QString extraCompilerName(const ProString &extraCompiler, const QStringList &inputs,
                              const QStringList &outputs);
    bool supportsMetaBuild() override { return true; }
    bool supportsMergedBuilds() override { return true; }
    bool mergeBuildProject(MakefileGenerator *other) override;

    bool openOutput(QFile &file, const QString &build) const override;

    virtual void initProject();
    void initConfiguration();
    void initCompilerTool();
    void initLinkerTool();
    void initLibrarianTool();
    void initManifestTool();
    void initResourceTool();
    void initIDLTool();
    void initCustomBuildTool();
    void initPreBuildEventTools();
    void initPostBuildEventTools();
    void initDeploymentTool();
    void initWinDeployQtTool();
    void initPreLinkEventTools();
    void initRootFiles();
    void initSourceFiles();
    void initHeaderFiles();
    void initGeneratedFiles();
    void initTranslationFiles();
    void initFormFiles();
    void initResourceFiles();
    void initDeploymentFiles();
    void initDistributionFiles();
    void initLexYaccFiles();
    void initExtraCompilerOutputs();

    void writeSubDirs(QTextStream &t); // Called from VCXProj backend
    QUuid getProjectUUID(const QString &filename=QString()); // Called from VCXProj backend

    Target projectTarget;

    // Used for single project
    VCProjectSingleConfig vcProject;

    // Holds all configurations for glue (merged) project
    QList<VcprojGenerator*> mergedProjects;

private:
    ProStringList collectDependencies(QMakeProject *proj, QHash<QString, QString> &projLookup,
                                      QHash<QString, QString> &projGuids,
                                      QHash<VcsolutionDepend *, QStringList> &extraSubdirs,
                                      QHash<QString, VcsolutionDepend*> &solution_depends,
                                      QList<VcsolutionDepend*> &solution_cleanup,
                                      QTextStream &t,
                                      QHash<QString, ProStringList> &subdirProjectLookup,
                                      const ProStringList &allDependencies = ProStringList());
    QUuid increaseUUID(const QUuid &id);
    QString retrievePlatformToolSet() const;
    bool isStandardSuffix(const QString &suffix) const;
    ProString firstInputFileName(const ProString &extraCompilerName) const;
    QString firstExpandedOutputFileName(const ProString &extraCompilerName);
    void createCustomBuildToolFakeFile(const QString &cbtFilePath, const QString &realOutFilePath);
    bool otherFiltersContain(const QString &fileName) const;
    friend class VCFilter;
};

inline QString VcprojGenerator::defaultMakefile() const
{
    return project->first("TARGET") + project->first("VCPROJ_EXTENSION");
}

QT_END_NAMESPACE

#endif // MSVC_VCPROJ_H
