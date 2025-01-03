// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef MINGW_MAKE_H
#define MINGW_MAKE_H

#include "winmakefile.h"

QT_BEGIN_NAMESPACE

class MingwMakefileGenerator : public Win32MakefileGenerator
{
protected:
    using MakefileGenerator::escapeDependencyPath;
    QString escapeDependencyPath(const QString &path) const override;
    ProString fixLibFlag(const ProString &lib) override;
    bool processPrlFileBase(QString &origFile, QStringView origName,
                            QStringView fixedBase, int slashOff) override;
    bool writeMakefile(QTextStream &) override;
    void init() override;
    QString installRoot() const override;
private:
    void writeMingwParts(QTextStream &);
    void writeIncPart(QTextStream &t) override;
    void writeLibsPart(QTextStream &t) override;
    void writeObjectsPart(QTextStream &t) override;
    void writeBuildRulesPart(QTextStream &t) override;
    void writeRcFilePart(QTextStream &t) override;

    QStringList &findDependencies(const QString &file) override;

    QString preCompHeaderOut;

    LibFlagType parseLibFlag(const ProString &flag, ProString *arg) override;

    QString objectsLinkLine;
    LinkerResponseFileInfo linkerResponseFile;
};

QT_END_NAMESPACE

#endif // MINGW_MAKE_H
