// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include <qsqldriverplugin.h>
#include <qstringlist.h>
#include "qsql_oci_p.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class QOCIDriverPlugin : public QSqlDriverPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QSqlDriverFactoryInterface" FILE "oci.json")

public:
    QOCIDriverPlugin();

    QSqlDriver* create(const QString &) override;
};

QOCIDriverPlugin::QOCIDriverPlugin()
    : QSqlDriverPlugin()
{
}

QSqlDriver* QOCIDriverPlugin::create(const QString &name)
{
    if (name == "QOCI"_L1) {
        QOCIDriver* driver = new QOCIDriver();
        return driver;
    }
    return 0;
}

QT_END_NAMESPACE

#include "main.moc"
