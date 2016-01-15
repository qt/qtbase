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

#ifndef PBUILDER_PBX_H
#define PBUILDER_PBX_H

#include "unixmake.h"

QT_BEGIN_NAMESPACE

class ProjectBuilderMakefileGenerator : public UnixMakefileGenerator
{
    bool writingUnixMakefileGenerator;
    QString pbx_dir;
    int pbuilderVersion() const;
    bool writeSubDirs(QTextStream &);
    bool writeMakeParts(QTextStream &);
    bool writeMakefile(QTextStream &);

    QString pbxbuild();
    QHash<QString, QString> keys;
    QString keyFor(const QString &file);
    QString findProgram(const ProString &prog);
    QString fixForOutput(const QString &file);
    ProStringList fixListForOutput(const char *where);
    ProStringList fixListForOutput(const ProStringList &list);
    int     reftypeForFile(const QString &where);
    QString projectSuffix() const;
    enum { SettingsAsList=0x01, SettingsNoQuote=0x02 };
    inline QString writeSettings(const QString &var, const char *val, int flags=0, int indent_level=0)
        { return writeSettings(var, ProString(val), flags, indent_level); }
    inline QString writeSettings(const QString &var, const QString &val, int flags=0, int indent_level=0)
        { return writeSettings(var, ProString(val), flags, indent_level); }
    inline QString writeSettings(const QString &var, const ProString &val, int flags=0, int indent_level=0)
        { return writeSettings(var, ProStringList(val), flags, indent_level); }
    QString writeSettings(const QString &var, const ProStringList &vals, int flags=0, int indent_level=0);

public:
    ProjectBuilderMakefileGenerator();
    ~ProjectBuilderMakefileGenerator();

    virtual bool supportsMetaBuild() { return false; }
    virtual bool openOutput(QFile &, const QString &) const;
protected:
    bool doPrecompiledHeaders() const { return false; }
    virtual bool doDepends() const { return writingUnixMakefileGenerator && UnixMakefileGenerator::doDepends(); }
};

inline ProjectBuilderMakefileGenerator::~ProjectBuilderMakefileGenerator()
{ }

QT_END_NAMESPACE

#endif // PBUILDER_PBX_H
