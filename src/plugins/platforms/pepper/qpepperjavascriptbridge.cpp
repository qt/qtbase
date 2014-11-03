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

#include "qpepperjavascriptbridge.h"

#include <QtCore/QtCore>

QPepperJavascriptBridge::QPepperJavascriptBridge(pp::Instance *instance)
    :m_instance(instance)
{

}

QVariant QPepperJavascriptBridge::callJavascriptFunction(const QByteArray &tag, const QByteArray &code)
{
    evalSource("this.qCallFunction(\"" + tag  + ":" + code +"\")");
}

void QPepperJavascriptBridge::evalSource(const QByteArray &code)
{
    // Post message to the Qt NaCl loader, which will eval() the message content.
    // "this" will be set to the nacl <embed> element. See handleMessage() in
    // qttools/src/naclshared/qtnaclloader.js.
    m_instance->PostMessage(pp::Var(code.constData()));
}

void QPepperJavascriptBridge::evalFile(const QString &fileName)
{
    QFile f(fileName);
    if (!f.exists()) {
        qDebug() << "QPepperJavascriptBridge::evalFile: File not found" << fileName;
        return;
    }

    f.open(QIODevice::ReadOnly);
    evalSource(f.readAll());
}
