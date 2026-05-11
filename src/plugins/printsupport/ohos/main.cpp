// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosprintersupport.h"
#include <qpa/qplatformprintplugin.h>

QT_BEGIN_NAMESPACE

class QOhosPrinterSupportPlugin : public QPlatformPrinterSupportPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformPrinterSupportFactoryInterface_iid FILE "ohos.json")

public:
    QPlatformPrinterSupport *create(const QString &);
};

QPlatformPrinterSupport *QOhosPrinterSupportPlugin::create(const QString &key)
{
    if (key.compare(key, QLatin1String("ohosprintersupport"), Qt::CaseInsensitive) == 0)
        return new QOhosPrinterSupport;
    return nullptr;
}

QT_END_NAMESPACE

#include "main.moc"
