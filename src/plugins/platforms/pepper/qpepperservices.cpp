/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Native Client platform plugin.
**
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
**
****************************************************************************/

#include "qpepperservices.h"
#include "qpepperinstance.h"

#include <qpa/qplatformservices.h>

QT_BEGIN_NAMESPACE

QPepperServices::QPepperServices()
    :QPlatformServices()
{
    const char *addUrlMessageHandler = \
        "this.qtMessageHandlers[\"qtOpenUrl\"] = function(url) { "
        "    window.open(url);"
        "}";
    QPepperInstance::get()->runJavascript(addUrlMessageHandler);
}

bool QPepperServices::openUrl(const QUrl &url)
{
    QByteArray message = "qtOpenUrl:" + url.toString().toUtf8();
    QPepperInstance::get()->postMessage(message);
    return true;
}

QT_END_NAMESPACE

