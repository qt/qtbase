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

#include "meta.h"
#include "project.h"
#include "option.h"
#include <qdir.h>

QT_BEGIN_NAMESPACE

QHash<QString, ProValueMap> QMakeMetaInfo::cache_vars;

bool
QMakeMetaInfo::readLib(const QString &meta_file)
{
    if(cache_vars.contains(meta_file)) {
        vars = cache_vars[meta_file];
        return true;
    }

    QMakeProject proj;
    if (!proj.read(Option::normalizePath(meta_file), QMakeEvaluator::LoadProOnly))
        return false;
    vars = proj.variables();
    cache_vars.insert(meta_file, vars);
    return true;
}


QString
QMakeMetaInfo::checkLib(const QString &lib)
{
    QString ret = QFile::exists(lib) ? lib : QString();
    if(ret.isNull()) {
        debug_msg(2, "QMakeMetaInfo: Cannot find info file for %s", lib.toLatin1().constData());
    } else {
        debug_msg(2, "QMakeMetaInfo: Found info file %s for %s", ret.toLatin1().constData(), lib.toLatin1().constData());
    }
    return ret;
}

QT_END_NAMESPACE
