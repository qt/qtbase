// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    void init() override;
    bool writeMakefile(QTextStream &) override;

    QString escapeFilePath(const QString &) const override { Q_ASSERT(false); return QString(); }

public:
    bool supportsMetaBuild() override { return false; }
    bool openOutput(QFile &, const QString &) const override;
};

QT_END_NAMESPACE

#endif // PROJECTGENERATOR_H
