// Copyright (C) 2022-2023 Gaurav Guleria <tinytrebuchet@protonmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qcpdbprintersupport_p.h"

#include <qpa/qplatformprintplugin.h>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class QCpdbPrinterSupportPlugin : public QPlatformPrinterSupportPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformPrinterSupportFactoryInterface_iid FILE "cpdb.json")

public:
    QStringList keys() const;
    QPlatformPrinterSupport *create(const QString &) override;
};

QStringList QCpdbPrinterSupportPlugin::keys() const
{
    return QStringList(QStringLiteral("cpdbprintersupport"));
}

QPlatformPrinterSupport *QCpdbPrinterSupportPlugin::create(const QString &key)
{
    if (key.compare(key, "cpdbprintersupport"_L1, Qt::CaseInsensitive) == 0)
        return new QCpdbPrinterSupport;
    return 0;
}

QT_END_NAMESPACE

#include "main.moc"
