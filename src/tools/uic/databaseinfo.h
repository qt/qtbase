// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef DATABASEINFO_H
#define DATABASEINFO_H

#include "treewalker.h"
#include <qstringlist.h>
#include <qmap.h>

QT_BEGIN_NAMESPACE

class Driver;

class DatabaseInfo : public TreeWalker
{
public:
    DatabaseInfo();

    void acceptUI(DomUI *node) override;
    void acceptWidget(DomWidget *node) override;

    inline QStringList connections() const
    { return m_connections; }

private:
    QStringList m_connections;
    QMap<QString, QStringList> m_cursors;
    QMap<QString, QStringList> m_fields;
};

QT_END_NAMESPACE

#endif // DATABASEINFO_H
