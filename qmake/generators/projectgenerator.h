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
