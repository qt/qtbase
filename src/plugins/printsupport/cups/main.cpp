/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/


#include "qcupsprintersupport_p.h"

#include <qpa/qplatformprintplugin.h>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE

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
    if (key.compare(key, QLatin1String("cupsprintersupport"), Qt::CaseInsensitive) == 0)
        return new QCupsPrinterSupport;
    return 0;
}

QT_END_NAMESPACE

#include "main.moc"
