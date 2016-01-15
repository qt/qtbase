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
    bool readLibtoolFile(const QString &f);
    bool readPkgCfgFile(const QString &f);
    QMakeProject *conf;
    ProValueMap vars;
    QString meta_type;
    static QHash<QString, ProValueMap> cache_vars;
public:
    QMakeMetaInfo(QMakeProject *_conf);

    // These functions expect the path to be normalized
    static QString findLib(const QString &lib);
    bool readLib(const QString &meta_file);

    QString type() const;
    bool isEmpty(const ProKey &v);
    ProStringList &values(const ProKey &v);
    ProString first(const ProKey &v);
    ProValueMap &variables();
};

inline bool QMakeMetaInfo::isEmpty(const ProKey &v)
{ return !vars.contains(v) || vars[v].isEmpty(); }

inline QString QMakeMetaInfo::type() const
{ return meta_type; }

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
