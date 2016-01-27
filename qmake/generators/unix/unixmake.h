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

#ifndef UNIXMAKE_H
#define UNIXMAKE_H

#include "makefile.h"

QT_BEGIN_NAMESPACE

class UnixMakefileGenerator : public MakefileGenerator
{
    bool include_deps;
    QString libtoolFileName(bool fixify=true);
    void writeLibtoolFile();     // for libtool
    void writePrlFile(QTextStream &);

public:
    UnixMakefileGenerator();
    ~UnixMakefileGenerator();

protected:
    virtual bool doPrecompiledHeaders() const { return project->isActiveConfig("precompile_header"); }
    virtual bool doDepends() const { return !Option::mkfile::do_stub_makefile && MakefileGenerator::doDepends(); }
#ifdef Q_OS_WIN // MinGW x-compiling for QNX
    virtual QString installRoot() const;
#endif
    virtual QString defaultInstall(const QString &);
    virtual ProString fixLibFlag(const ProString &lib);

    virtual bool findLibraries(bool linkPrl, bool mergeLflags);
    virtual QString escapeFilePath(const QString &path) const;
    ProString escapeFilePath(const ProString &path) const { return MakefileGenerator::escapeFilePath(path); }
    virtual QStringList &findDependencies(const QString &);
    virtual void init();

    virtual void writeDefaultVariables(QTextStream &t);
    virtual void writeSubTargets(QTextStream &t, QList<SubTarget*> subtargets, int flags);
    void writeMakeParts(QTextStream &);
    bool writeMakefile(QTextStream &);

private:
    void init2();
};

inline UnixMakefileGenerator::~UnixMakefileGenerator()
{ }

QT_END_NAMESPACE

#endif // UNIXMAKE_H
