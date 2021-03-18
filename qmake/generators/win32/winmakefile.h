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
    bool processPrlFileBase(QString &origFile, const QStringRef &origName,
                            const QStringRef &fixedBase, int slashOff) override;

    void processVars();
    void fixTargetExt();
    void processRcFileVar();
    static QString cQuoted(const QString &str);

public:
    ProKey fullTargetVariable() const override;
};

QT_END_NAMESPACE

#endif // WINMAKEFILE_H
