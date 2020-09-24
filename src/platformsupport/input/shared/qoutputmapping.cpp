/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
