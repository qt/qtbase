// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2022 Mimer Information Technology
// SPDX-License-Identifier: LicenseRef-Qt-Commercial
#include "qsql_mimer.h"

#include <qsqldriverplugin.h>
#include <qstringlist.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class QMimerSQLDriverPlugin : public QSqlDriverPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QSqlDriverFactoryInterface" FILE "mimer.json")
public:
    QMimerSQLDriverPlugin();
    QSqlDriver *create(const QString &) override;
};

QMimerSQLDriverPlugin::QMimerSQLDriverPlugin() : QSqlDriverPlugin() { }

QSqlDriver *QMimerSQLDriverPlugin::create(const QString &name)
{
    if (name == "QMIMER"_L1)
        return new QMimerSQLDriver;
    return nullptr;
}

QT_END_NAMESPACE

#include "main.moc"
