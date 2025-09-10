// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtWidgets/qstyleplugin.h>
#include "qandroidstyle_p.h"

QT_BEGIN_NAMESPACE

class QAndroidStylePlugin : public QStylePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QStyleFactoryInterface" FILE "androidstyle.json")
public:
    QStyle *create(const QString &key);
};

QStyle *QAndroidStylePlugin::create(const QString &key)
{
    if (key.compare(QLatin1String("android"), Qt::CaseInsensitive) == 0)
        return new QAndroidStyle();

    return 0;
}

QT_END_NAMESPACE

#include "main.moc"

