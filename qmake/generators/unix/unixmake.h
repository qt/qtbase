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

#ifndef UNIXMAKE_H
#define UNIXMAKE_H

#include "makefile.h"

QT_BEGIN_NAMESPACE

class UnixMakefileGenerator : public MakefileGenerator
{
    bool include_deps = false;
    QString libtoolFileName(bool fixify=true);
    void writeLibtoolFile();     // for libtool
    void writePrlFile(QTextStream &) override;

protected:
    virtual bool doPrecompiledHeaders() const { return project->isActiveConfig("precompile_header"); }
#ifdef Q_OS_WIN // MinGW x-compiling for QNX
    QString installRoot() const override;
#endif
    QString defaultInstall(const QString &) override;
    ProString fixLibFlag(const ProString &lib) override;

    bool findLibraries(bool linkPrl, bool mergeLflags) override;
    QString escapeFilePath(const QString &path) const override;
    ProString escapeFilePath(const ProString &path) const { return MakefileGenerator::escapeFilePath(path); }
    QStringList &findDependencies(const QString &) override;
    void init() override;

    void writeDefaultVariables(QTextStream &t) override;
    void writeSubTargets(QTextStream &t, QList<SubTarget*> subtargets, int flags) override;
    void writeMakeParts(QTextStream &);
    bool writeMakefile(QTextStream &) override;
    std::pair<bool, QString> writeObjectsPart(QTextStream &, bool do_incremental);
private:
    void init2();
    ProStringList libdirToFlags(const ProKey &key);
};

QT_END_NAMESPACE

#endif // UNIXMAKE_H
