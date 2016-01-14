/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the qmake application of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
public:
    MingwMakefileGenerator();
    ~MingwMakefileGenerator();
protected:
    QString escapeDependencyPath(const QString &path) const;
    ProString escapeDependencyPath(const ProString &path) const { return MakefileGenerator::escapeDependencyPath(path); }
    virtual ProString fixLibFlag(const ProString &lib);
    virtual QString getManifestFileForRcFile() const;
    bool writeMakefile(QTextStream &);
    void init();
    virtual QString installRoot() const;
private:
    void writeMingwParts(QTextStream &);
    void writeIncPart(QTextStream &t);
    void writeLibsPart(QTextStream &t);
    void writeObjectsPart(QTextStream &t);
    void writeBuildRulesPart(QTextStream &t);
    void writeRcFilePart(QTextStream &t);

    QStringList &findDependencies(const QString &file);

    QString preCompHeaderOut;

    virtual LibFlagType parseLibFlag(const ProString &flag, ProString *arg);

    QString objectsLinkLine;
};

inline MingwMakefileGenerator::~MingwMakefileGenerator()
{ }

QT_END_NAMESPACE

#endif // MINGW_MAKE_H
