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

#include "databaseinfo.h"
#include "driver.h"
#include "ui4.h"
#include "utils.h"

QT_BEGIN_NAMESPACE

DatabaseInfo::DatabaseInfo() = default;

void DatabaseInfo::acceptUI(DomUI *node)
{
    m_connections.clear();
    m_cursors.clear();
    m_fields.clear();

    TreeWalker::acceptUI(node);

    m_connections.removeDuplicates();
}

void DatabaseInfo::acceptWidget(DomWidget *node)
{
    QHash<QString, DomProperty*> properties = propertyMap(node->elementProperty());

    DomProperty *frameworkCode = properties.value(QLatin1String("frameworkCode"));
    if (frameworkCode && toBool(frameworkCode->elementBool()) == false)
        return;

    DomProperty *db = properties.value(QLatin1String("database"));
    if (db && db->elementStringList()) {
        QStringList info = db->elementStringList()->elementString();
        if (info.isEmpty() || info.constFirst().isEmpty())
            return;
        const QString &connection = info.constFirst();
        m_connections.append(connection);

        QString table = info.size() > 1 ? info.at(1) : QString();
        if (table.isEmpty())
            return;
        m_cursors[connection].append(table);

        QString field = info.size() > 2 ? info.at(2) : QString();
        if (field.isEmpty())
            return;
        m_fields[connection].append(field);
    }

    TreeWalker::acceptWidget(node);
}

QT_END_NAMESPACE
