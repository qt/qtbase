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

#ifndef MSVC_NMAKE_H
#define MSVC_NMAKE_H

#include "winmakefile.h"

QT_BEGIN_NAMESPACE

class NmakeMakefileGenerator : public Win32MakefileGenerator
{
    void writeNmakeParts(QTextStream &);
    bool writeMakefile(QTextStream &);
    void writeImplicitRulesPart(QTextStream &t);
    void writeBuildRulesPart(QTextStream &t);
    void writeLinkCommand(QTextStream &t, const QString &extraFlags = QString(), const QString &extraInlineFileContent = QString());
    void writeResponseFileFiles(QTextStream &t, const ProStringList &files);
    int msvcVersion() const;
    void init();
    static QStringList sourceFilesForImplicitRulesFilter();

protected:
    virtual void writeSubMakeCall(QTextStream &t, const QString &callPrefix,
                                  const QString &makeArguments);
    virtual QString defaultInstall(const QString &t);
    virtual QStringList &findDependencies(const QString &file);
    QString var(const ProKey &value) const;
    QString precompH, precompObj, precompPch;
    QString precompObjC, precompPchC;
    bool usePCH, usePCHC;

public:
    NmakeMakefileGenerator();
    ~NmakeMakefileGenerator();

};

inline NmakeMakefileGenerator::~NmakeMakefileGenerator()
{ }

QT_END_NAMESPACE

#endif // MSVC_NMAKE_H
