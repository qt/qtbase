// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef METAMAKEFILE_H
#define METAMAKEFILE_H

#include <qlist.h>
#include <qstring.h>

QT_BEGIN_NAMESPACE

class QMakeProject;
class MakefileGenerator;

class MetaMakefileGenerator
{
protected:
    MetaMakefileGenerator(QMakeProject *p, const QString &n, bool op=true) : project(p), own_project(op), name(n) { }
    QMakeProject *project;
    bool own_project;
    QString name;

public:

    virtual ~MetaMakefileGenerator();

    static MetaMakefileGenerator *createMetaGenerator(QMakeProject *proj, const QString &name, bool op=true, bool *success = nullptr);
    static MakefileGenerator *createMakefileGenerator(QMakeProject *proj, bool noIO = false);

    inline QMakeProject *projectFile() const { return project; }

    virtual bool init() = 0;
    virtual int type() const { return -1; }
    virtual bool write() = 0;
};

QT_END_NAMESPACE

#endif // METAMAKEFILE_H
