// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "databaseinfo.h"
#include "driver.h"
#include "ui4.h"
#include "utils.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

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

    DomProperty *frameworkCode = properties.value("frameworkCode"_L1);
    if (frameworkCode && toBool(frameworkCode->elementBool()) == false)
        return;

    DomProperty *db = properties.value("database"_L1);
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
