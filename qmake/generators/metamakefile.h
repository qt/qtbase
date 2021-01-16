/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the qmake application of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
****************************************************************************/

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
