// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
