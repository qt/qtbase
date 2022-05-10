// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef WINMAKEFILE_H
#define WINMAKEFILE_H

#include "makefile.h"

QT_BEGIN_NAMESPACE

class Win32MakefileGenerator : public MakefileGenerator
{
protected:
    QString defaultInstall(const QString &) override;
    void writeDefaultVariables(QTextStream &t) override;
    virtual void writeCleanParts(QTextStream &t);
    virtual void writeStandardParts(QTextStream &t);
    virtual void writeIncPart(QTextStream &t);
    virtual void writeLibsPart(QTextStream &t);
    virtual void writeObjectsPart(QTextStream &t);
    virtual void writeImplicitRulesPart(QTextStream &t);
    virtual void writeBuildRulesPart(QTextStream &);
    using MakefileGenerator::escapeFilePath;
    QString escapeFilePath(const QString &path) const override;
    using MakefileGenerator::escapeDependencyPath;
    QString escapeDependencyPath(const QString &path) const override;

    virtual void writeRcFilePart(QTextStream &t);

    bool findLibraries(bool linkPrl, bool mergeLflags) override;

    LibFlagType parseLibFlag(const ProString &flag, ProString *arg) override;
    ProString fixLibFlag(const ProString &lib) override;
    bool processPrlFileBase(QString &origFile, QStringView origName,
                            QStringView fixedBase, int slashOff) override;

    void processVars();
    void fixTargetExt();
    void processRcFileVar();
    static QString cQuoted(const QString &str);

public:
    ProKey fullTargetVariable() const override;
};

QT_END_NAMESPACE

#endif // WINMAKEFILE_H
