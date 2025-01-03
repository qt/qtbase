// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef MSVC_NMAKE_H
#define MSVC_NMAKE_H

#include "winmakefile.h"

QT_BEGIN_NAMESPACE

class NmakeMakefileGenerator : public Win32MakefileGenerator
{
    void writeNmakeParts(QTextStream &);
    bool writeMakefile(QTextStream &) override;
    void writeImplicitRulesPart(QTextStream &t) override;
    void writeBuildRulesPart(QTextStream &t) override;
    void writeLinkCommand(QTextStream &t, const QString &extraFlags = QString(), const QString &extraInlineFileContent = QString());
    void writeResponseFileFiles(QTextStream &t, const ProStringList &files);
    int msvcVersion() const;
    void init() override;
    static QStringList sourceFilesForImplicitRulesFilter();

protected:
    void writeSubMakeCall(QTextStream &t, const QString &callPrefix,
                          const QString &makeArguments) override;
    ProStringList extraSubTargetDependencies() override;
    QString defaultInstall(const QString &t) override;
    QStringList &findDependencies(const QString &file) override;
    QString var(const ProKey &value) const override;
    void suppressBuiltinRules(QTextStream &t) const override;
    QString precompH, precompObj, precompPch;
    QString precompObjC, precompPchC;
    bool usePCH = false;
    bool usePCHC = false;
};

QT_END_NAMESPACE

#endif // MSVC_NMAKE_H
