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


#include <qpa/qplatformprintplugin.h>
#include <QtCore/QStringList>

#include "qwindowsprintersupport.h"

QT_BEGIN_NAMESPACE

class QWindowsPrinterSupportPlugin : public QPlatformPrinterSupportPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformPrinterSupportFactoryInterface_iid FILE "windows.json")

public:
    QPlatformPrinterSupport *create(const QString &);
};

QPlatformPrinterSupport *QWindowsPrinterSupportPlugin::create(const QString &key)
{
    if (key.compare(key, QLatin1String("windowsprintsupport"), Qt::CaseInsensitive) == 0)
        return new QWindowsPrinterSupport;
    return 0;
}

QT_END_NAMESPACE

#include "main.moc"
