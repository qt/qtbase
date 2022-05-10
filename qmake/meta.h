// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef META_H
#define META_H

#include "project.h"

#include <qhash.h>
#include <qstringlist.h>
#include <qstring.h>

QT_BEGIN_NAMESPACE

class QMakeProject;

class QMakeMetaInfo
{
    ProValueMap vars;
    static QHash<QString, ProValueMap> cache_vars;
public:

    // These functions expect the path to be normalized
    static QString checkLib(const QString &lib);
    bool readLib(const QString &meta_file);

    bool isEmpty(const ProKey &v);
    ProStringList &values(const ProKey &v);
    ProString first(const ProKey &v);
    ProValueMap &variables();
};

inline bool QMakeMetaInfo::isEmpty(const ProKey &v)
{ return !vars.contains(v) || vars[v].isEmpty(); }

inline ProStringList &QMakeMetaInfo::values(const ProKey &v)
{ return vars[v]; }

inline ProString QMakeMetaInfo::first(const ProKey &v)
{
#if defined(Q_CC_SUN) && (__SUNPRO_CC == 0x500) || defined(Q_CC_HP)
    // workaround for Sun WorkShop 5.0 bug fixed in Forte 6
    if (isEmpty(v))
        return ProString("");
    else
        return vars[v].first();
#else
    return isEmpty(v) ? ProString("") : vars[v].first();
#endif
}

inline ProValueMap &QMakeMetaInfo::variables()
{ return vars; }

QT_END_NAMESPACE

#endif // META_H
