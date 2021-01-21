/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
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
