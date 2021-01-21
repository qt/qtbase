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

#include <QtWidgets/qstyleplugin.h>
#include "qmacstyle_mac_p.h"

QT_BEGIN_NAMESPACE

class QMacStylePlugin : public QStylePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QStyleFactoryInterface" FILE "macstyle.json")
public:
    QStyle *create(const QString &key);
};

QStyle *QMacStylePlugin::create(const QString &key)
{
    QMacAutoReleasePool pool;
    if (key.compare(QLatin1String("macintosh"), Qt::CaseInsensitive) == 0)
        return new QMacStyle();

    return 0;
}

QT_END_NAMESPACE

#include "main.moc"

