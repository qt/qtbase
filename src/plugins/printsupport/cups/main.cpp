// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#include "qcupsprintersupport_p.h"

#include <qpa/qplatformprintplugin.h>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class QCupsPrinterSupportPlugin : public QPlatformPrinterSupportPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformPrinterSupportFactoryInterface_iid FILE "cups.json")

public:
    QStringList keys() const;
    QPlatformPrinterSupport *create(const QString &) override;
};

QStringList QCupsPrinterSupportPlugin::keys() const
{
    return QStringList(QStringLiteral("cupsprintersupport"));
}

QPlatformPrinterSupport *QCupsPrinterSupportPlugin::create(const QString &key)
{
    if (key.compare(key, "cupsprintersupport"_L1, Qt::CaseInsensitive) == 0)
        return new QCupsPrinterSupport;
    return 0;
}

QT_END_NAMESPACE

#include "main.moc"
