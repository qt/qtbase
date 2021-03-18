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
    bool processPrlFileBase(QString &origFile, const QStringRef &origName,
                            const QStringRef &fixedBase, int slashOff) override;
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
};

QT_END_NAMESPACE

#endif // MINGW_MAKE_H
