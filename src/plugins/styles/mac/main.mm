// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtWidgets/qstyleplugin.h>
#include "qmacstyle_mac_p.h"

#include <QtCore/private/qcore_mac_p.h>

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
    if (key.compare(QLatin1String("macos"), Qt::CaseInsensitive) == 0)
        return new QMacStyle();

    return 0;
}

QT_END_NAMESPACE

#include "main.moc"

