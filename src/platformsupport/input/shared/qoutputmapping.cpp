/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins module of the Qt Toolkit.
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
******************************************************************************/

#include "qoutputmapping_p.h"
#include <QFile>
#include <QFileInfo>
#include <QVariantMap>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

QT_BEGIN_NAMESPACE

static QOutputMapping *s_outputMapping = nullptr;

QOutputMapping *QOutputMapping::get()
{
    if (!s_outputMapping)
        s_outputMapping = new QDefaultOutputMapping;

    return s_outputMapping;
}

bool QOutputMapping::load()
{
   return false;
}

QString QOutputMapping::screenNameForDeviceNode(const QString &deviceNode)
{
    Q_UNUSED(deviceNode);
    return QString();
}

#ifdef Q_OS_WEBOS
QWindow *QOutputMapping::windowForDeviceNode(const QString &deviceNode)
{
    Q_UNUSED(deviceNode);
    return nullptr;
}

void QOutputMapping::set(QOutputMapping *mapping)
{
    if (s_outputMapping)
        delete s_outputMapping;

    s_outputMapping = mapping;
}
#endif // Q_OS_WEBOS

bool QDefaultOutputMapping::load()
{
    static QByteArray configFile = qgetenv("QT_QPA_EGLFS_KMS_CONFIG");
    if (configFile.isEmpty())
        return false;

    QFile file(QString::fromUtf8(configFile));
    if (!file.open(QFile::ReadOnly)) {
        qWarning("touch input support: Failed to open %s", configFile.constData());
        return false;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        qWarning("touch input support: Failed to parse %s", configFile.constData());
        return false;
    }

    // What we are interested is the virtualIndex and touchDevice properties for
    // each element in the outputs array.
    const QJsonArray outputs = doc.object().value(QLatin1String("outputs")).toArray();
    for (int i = 0; i < outputs.size(); ++i) {
        const QVariantMap output = outputs.at(i).toObject().toVariantMap();
        if (!output.contains(QStringLiteral("touchDevice")))
            continue;
        if (!output.contains(QStringLiteral("name"))) {
            qWarning("evdevtouch: Output %d specifies touchDevice but not name, this is wrong", i);
            continue;
        }
        QFileInfo deviceNode(output.value(QStringLiteral("touchDevice")).toString());
        const QString &screenName = output.value(QStringLiteral("name")).toString();
        m_screenTable.insert(deviceNode.canonicalFilePath(), screenName);
    }

    return true;
}

QString QDefaultOutputMapping::screenNameForDeviceNode(const QString &deviceNode)
{
    return m_screenTable.value(deviceNode);
}

QT_END_NAMESPACE
