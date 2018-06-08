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

#ifndef PROJECTGENERATOR_H
#define PROJECTGENERATOR_H

#include "makefile.h"

QT_BEGIN_NAMESPACE

class ProjectGenerator : public MakefileGenerator
{
    bool addFile(QString);
    bool addConfig(const QString &, bool add=true);
    QString getWritableVar(const char *, bool fixPath=true);
    QString fixPathToQmake(const QString &file);
protected:
    virtual void init();
    virtual bool writeMakefile(QTextStream &);

    virtual QString escapeFilePath(const QString &path) const { Q_ASSERT(false); return QString(); }

public:
    ProjectGenerator();
    ~ProjectGenerator();
    virtual bool supportsMetaBuild() { return false; }
    virtual bool openOutput(QFile &, const QString &) const;
};

inline ProjectGenerator::~ProjectGenerator()
{ }

QT_END_NAMESPACE

#endif // PROJECTGENERATOR_H
